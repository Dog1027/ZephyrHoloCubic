#include "fg_app.h"
#include "ui.h"
#include "input.h"
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/version.h>
#include <lvgl_mem.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

APP_FG_REGISTER(SysInfo, LV_SYMBOL_LIST, app_sysinfo_init, app_sysinfo_run, app_sysinfo_exit, 13);

#define SYSINFO_ACCENT_COLOR    0x9b6cf2
#define SYSINFO_REFRESH_MS      500
#define SYSINFO_SD_REFRESH_MS   5000
#define SYSINFO_SD_PATH         "/0:"
#define SYSINFO_MAX_THREADS     12  // 每列都佔 LVGL 記憶體，總列數控制在 ~20
#define SYSINFO_TEXT_MAX        24

extern struct k_heap _system_heap;

enum {
    ROW_UPTIME,
    ROW_SYS_HEAP,
    ROW_LVGL_HEAP,
    ROW_SD,
    ROW_ROLL,
    ROW_PITCH,
    ROW_FIXED_COUNT,
};

typedef struct {
    lv_obj_t *detail;
    char text[SYSINFO_TEXT_MAX];
} sysinfo_row_t;

static sysinfo_row_t rows[ROW_FIXED_COUNT];
static sysinfo_row_t thread_rows[SYSINFO_MAX_THREADS];
static k_tid_t threads[SYSINFO_MAX_THREADS];
static int thread_count;
static int64_t last_refresh;
static int64_t last_sd_refresh;

// 只有文字變了才更新 label，避免每次都重繪
static void row_set(sysinfo_row_t *row, const char *fmt, ...)
{
    char buf[SYSINFO_TEXT_MAX];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (row->detail == NULL || strcmp(buf, row->text) == 0) {
        return;
    }
    strcpy(row->text, buf);
    lv_label_set_text(row->detail, buf);
}

static lv_obj_t *row_create(lv_obj_t *list, const char *icon, const char *name, sysinfo_row_t *row)
{
    lv_obj_t *item = ui_list_item(list, icon, name, "-", lv_color_hex(SYSINFO_ACCENT_COLOR));

    row->detail = lv_obj_get_child(item, 2);
    strcpy(row->text, "-");
    return item;
}

// 角度轉成一位小數的字串，不用 %f (picolibc 可能沒有浮點 printf)
static void row_set_angle(sysinfo_row_t *row, float deg)
{
    int tenths = (int)(deg * 10.0f + (deg < 0 ? -0.5f : 0.5f));
    const char *sign = tenths < 0 ? "-" : "";

    if (tenths < 0) {
        tenths = -tenths;
    }
    row_set(row, "%s%d.%d deg", sign, tenths / 10, tenths % 10);
}

static void thread_collect_cb(const struct k_thread *thread, void *user_data)
{
    if (thread_count < SYSINFO_MAX_THREADS) {
        threads[thread_count++] = (k_tid_t)thread;
    }
}

static void update_uptime(void)
{
    uint32_t sec = (uint32_t)(k_uptime_get() / 1000);
    uint32_t days = sec / 86400;

    sec %= 86400;
    if (days > 0) {
        row_set(&rows[ROW_UPTIME], "%ud %02u:%02u:%02u", (unsigned int)days, (unsigned int)(sec / 3600),
                (unsigned int)(sec / 60 % 60), (unsigned int)(sec % 60));
    }
    else {
        row_set(&rows[ROW_UPTIME], "%u:%02u:%02u", (unsigned int)(sec / 3600), (unsigned int)(sec / 60 % 60),
                (unsigned int)(sec % 60));
    }
}

static void update_heap(sysinfo_row_t *row, const struct sys_memory_stats *stats)
{
    size_t total = stats->allocated_bytes + stats->free_bytes;

    row_set(row, "%u / %u KB", (unsigned int)((stats->allocated_bytes + 1023) / 1024),
            (unsigned int)(total / 1024));
}

static void update_sd(void)
{
    struct fs_statvfs st;

    if (fs_statvfs(SYSINFO_SD_PATH, &st) < 0 || st.f_blocks == 0) {
        row_set(&rows[ROW_SD], "No card");
        return;
    }

    uint64_t total = (uint64_t)st.f_frsize * st.f_blocks;
    uint64_t used = (uint64_t)st.f_frsize * (st.f_blocks - st.f_bfree);
    row_set(&rows[ROW_SD], "%u / %u MB", (unsigned int)(used >> 20), (unsigned int)(total >> 20));
}

static void update_threads(void)
{
    for (int i = 0; i < thread_count; i++) {
        size_t unused;

        // 執行緒結束後 stack 可能已無效，查詢失敗就顯示 "-"
        if (k_thread_stack_space_get(threads[i], &unused) != 0) {
            row_set(&thread_rows[i], "-");
            continue;
        }

        size_t size = threads[i]->stack_info.size;
        row_set(&thread_rows[i], "%u / %u", (unsigned int)(size - unused), (unsigned int)size);
    }
}

static void refresh(bool sd)
{
    struct sys_memory_stats stats;
    input_angle_t angle;

    update_uptime();

    sys_heap_runtime_stats_get(&_system_heap.heap, &stats);
    update_heap(&rows[ROW_SYS_HEAP], &stats);

    lvgl_heap_stats(&stats);
    update_heap(&rows[ROW_LVGL_HEAP], &stats);

    if (sd) {
        update_sd();
    }

    input_get(&angle);
    row_set_angle(&rows[ROW_ROLL], angle.roll);
    row_set_angle(&rows[ROW_PITCH], angle.pitch);

    update_threads();
}

static void app_sysinfo_init(void *scr)
{
    ui_screen_init(scr);
    ui_title(scr, "System");
    lv_obj_t *list = ui_list_create(scr);

    row_create(list, LV_SYMBOL_REFRESH, "Uptime", &rows[ROW_UPTIME]);
    row_create(list, LV_SYMBOL_DRIVE, "Sys heap", &rows[ROW_SYS_HEAP]);
    row_create(list, LV_SYMBOL_IMAGE, "LVGL heap", &rows[ROW_LVGL_HEAP]);
    row_create(list, LV_SYMBOL_SD_CARD, "SD card", &rows[ROW_SD]);
    row_create(list, LV_SYMBOL_GPS, "Roll", &rows[ROW_ROLL]);
    row_create(list, LV_SYMBOL_GPS, "Pitch", &rows[ROW_PITCH]);

    // 先收集執行緒再建立列，不能在 foreach callback 裡呼叫 LVGL
    thread_count = 0;
    k_thread_foreach_unlocked(thread_collect_cb, NULL);
    for (int i = 0; i < thread_count; i++) {
        const char *name = k_thread_name_get(threads[i]);
        char fallback[16];

        if (name == NULL || name[0] == '\0') {
            snprintf(fallback, sizeof(fallback), "%p", (void *)threads[i]);
            name = fallback;
        }
        row_create(list, LV_SYMBOL_SETTINGS, name, &thread_rows[i]);
    }

    ui_list_item(list, LV_SYMBOL_HOME, "Board", CONFIG_BOARD, lv_color_hex(SYSINFO_ACCENT_COLOR));
    ui_list_item(list, LV_SYMBOL_EYE_OPEN, "Zephyr", KERNEL_VERSION_STRING, lv_color_hex(SYSINFO_ACCENT_COLOR));

    refresh(true);
    last_refresh = k_uptime_get();
    last_sd_refresh = last_refresh;
}

static void app_sysinfo_run(void *scr)
{
    int64_t now = k_uptime_get();

    if (now - last_refresh < SYSINFO_REFRESH_MS) {
        return;
    }
    last_refresh = now;

    bool sd = now - last_sd_refresh >= SYSINFO_SD_REFRESH_MS;
    if (sd) {
        last_sd_refresh = now;
    }
    refresh(sd);
}

static void app_sysinfo_exit(void *scr)
{
    // LVGL 物件由框架刪除，這裡只清掉指標
    memset(rows, 0, sizeof(rows));
    memset(thread_rows, 0, sizeof(thread_rows));
    thread_count = 0;
}
