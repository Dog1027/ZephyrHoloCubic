#include "fg_app.h"
#include "ui.h"
#include "jpeg.h"
#include "file.h"
#include <zephyr/fs/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

APP_FG_REGISTER(Anim, LV_SYMBOL_PLAY, app_anim_init, app_anim_run, app_anim_exit, 7);

// 每個動畫是 ANIM_ROOT 底下的一個資料夾：000.jpg、001.jpg……，可選的 delay.txt 寫每格毫秒數
#define ANIM_ROOT             "/0:/anim"
#define ANIM_PATH_MAX         128
#define ANIM_MAX_ENTRIES      16
#define ANIM_DEFAULT_DELAY_MS 100
#define ANIM_ACCENT_COLOR     0x2ec4d6

static lv_obj_t *title;
static lv_obj_t *list;
static lv_obj_t *player;

static char anim_path[ANIM_PATH_MAX];
static char pending_path[ANIM_PATH_MAX];
static bool pending_start;
static bool pending_stop;
static bool playing;
static int frame;
static uint32_t delay_ms;
static int64_t next_frame_ms;

static int frame_show(int index)
{
    char path[ANIM_PATH_MAX + 16];

    snprintf(path, sizeof(path), "%s/%03d.jpg", anim_path, index);
    return jpeg_show(path);
}

static uint32_t delay_read(void)
{
    struct fs_file_t file;
    char path[ANIM_PATH_MAX + 16];
    char buf[12] = {0};

    snprintf(path, sizeof(path), "%s/delay.txt", anim_path);
    fs_file_t_init(&file);
    if (!file_exists(path) || fs_open(&file, path, FS_O_READ) < 0) {
        return ANIM_DEFAULT_DELAY_MS;
    }
    fs_read(&file, buf, sizeof(buf) - 1);
    fs_close(&file);

    int ms = atoi(buf);
    return ms > 0 ? ms : ANIM_DEFAULT_DELAY_MS;
}

static void item_clicked_cb(lv_event_t *e)
{
    const char *name = lv_event_get_user_data(e);

    int len = snprintf(pending_path, sizeof(pending_path), "%s/%s", ANIM_ROOT, name);
    if (len < 0 || len >= (int)sizeof(pending_path)) {
        LOG_WRN("Path too long: %s", name);
        return;
    }
    // 不在事件裡切換畫面，交給 run 處理
    pending_start = true;
}

static void player_clicked_cb(lv_event_t *e)
{
    pending_stop = true;
}

static void list_load(void)
{
    static struct fs_dirent entry;
    struct fs_dir_t dir;
    int count = 0;

    fs_dir_t_init(&dir);
    if (!file_exists(ANIM_ROOT) || fs_opendir(&dir, ANIM_ROOT) < 0) {
        ui_list_message(list, "Put JPEG folders\nin /anim on the SD card");
        return;
    }

    while (count < ANIM_MAX_ENTRIES && fs_readdir(&dir, &entry) == 0 && entry.name[0] != '\0') {
        if (entry.type == FS_DIR_ENTRY_DIR) {
            lv_obj_t *item = ui_list_item(list, LV_SYMBOL_PLAY, entry.name, NULL,
                                          lv_color_hex(ANIM_ACCENT_COLOR));
            // 另存一份完整名稱，label 的文字太長時會被改成 "..."
            const char *name = ui_obj_strdup(item, entry.name);
            if (name != NULL) {
                lv_obj_add_event_cb(item, item_clicked_cb, LV_EVENT_CLICKED, (void *)name);
            }
            count++;
        }
    }
    fs_closedir(&dir);

    if (count == 0) {
        ui_list_message(list, "No animations in /anim");
    }
}

// 播放時畫面由解碼直接寫到螢幕，LVGL 這邊只留一個不畫任何東西的 player 接收 OK
static void play_start(void)
{
    strcpy(anim_path, pending_path);
    delay_ms = delay_read();
    frame = 0;
    next_frame_ms = k_uptime_get();
    LOG_INF("Play %s, %u ms per frame", anim_path, delay_ms);

    lv_obj_add_flag(title, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(player, LV_OBJ_FLAG_HIDDEN);
    ui_group_solo(player);

    jpeg_begin();
    playing = true;
}

static void play_stop(void)
{
    playing = false;
    lv_group_remove_obj(player);
    lv_obj_add_flag(player, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(title, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(list, LV_OBJ_FLAG_HIDDEN);
    ui_group_add_children(list);

    // LVGL 只重畫有變動的區域，整個畫面標記重畫才能蓋掉最後一格動畫
    lv_obj_invalidate(lv_screen_active());
}

static void app_anim_init(void *scr)
{
    ui_screen_init(scr);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    title = ui_title(scr, "Animations");
    list = ui_list_create(scr);

    player = ui_tap_area(scr, player_clicked_cb, NULL);
    lv_group_remove_obj(player);
    lv_obj_add_flag(player, LV_OBJ_FLAG_HIDDEN);

    pending_start = false;
    pending_stop = false;
    playing = false;
    list_load();
}

static void app_anim_run(void *scr)
{
    if (pending_start) {
        pending_start = false;
        play_start();
    }

    if (pending_stop) {
        pending_stop = false;
        play_stop();
    }

    if (!playing || k_uptime_get() < next_frame_ms) {
        return;
    }

    int ret = frame_show(frame);
    if (ret == -ENOENT && frame > 0) {
        frame = 0;
        ret = frame_show(frame);
    }
    if (ret < 0) {
        LOG_ERR("Stop playing %s (%d)", anim_path, ret);
        play_stop();
        return;
    }

    frame++;
    next_frame_ms += delay_ms;
    // 解碼跟不上時不要累積延遲，直接從現在重新計時
    if (next_frame_ms < k_uptime_get()) {
        next_frame_ms = k_uptime_get();
    }
}

static void app_anim_exit(void *scr)
{
    playing = false;
    pending_start = false;
    pending_stop = false;
}
