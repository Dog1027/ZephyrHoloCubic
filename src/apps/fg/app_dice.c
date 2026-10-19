#include "fg_app.h"
#include "ui.h"
#include "led.h"
#include "nav.h"
#include "file.h"
#include <zephyr/fs/fs.h>
#include <zephyr/random/random.h>
#include <string.h>

APP_FG_REGISTER(Dice, LV_SYMBOL_SHUFFLE, app_dice_init, app_dice_run, app_dice_exit, 10);

// 抽籤清單：每行一個選項，只能用英數字 (字型沒有中文)
#define LOTS_FILE           "/0:/lots.txt"
#define LOTS_MAX            32
#define LOTS_LEN            40
#define DICE_SIZE           110
#define DICE_PIP_SIZE       18
#define DICE_PIP_OFFSET     30
#define ROLL_MS             700   // 停止搖晃後再滾這麼久才停
#define ROLL_STEP_MS        60
#define LED_HOLD_MS         800
#define DICE_ACCENT_COLOR   0xf25f5c

enum { MODE_DICE, MODE_LOTS };

// 3x3 格子上的 7 個點：左上、右上、左中、中、右中、左下、右下
static const int8_t pip_pos[7][2] = {
    {-1, -1}, {1, -1}, {-1, 0}, {0, 0}, {1, 0}, {-1, 1}, {1, 1},
};
static const uint8_t pip_mask[7] = {
    0,
    BIT(3),
    BIT(0) | BIT(6),
    BIT(0) | BIT(3) | BIT(6),
    BIT(0) | BIT(1) | BIT(5) | BIT(6),
    BIT(0) | BIT(1) | BIT(3) | BIT(5) | BIT(6),
    BIT(0) | BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(6),
};

static lv_style_t style_tab;
static lv_style_t style_tab_focused;
static bool styles_inited;

static lv_obj_t *dice;
static lv_obj_t *pips[7];
static lv_obj_t *lot_label;
static char lots[LOTS_MAX][LOTS_LEN + 1];
static int lots_count;
static int mode;
static bool rolling;
static int64_t roll_end_ms;
static int64_t next_step_ms;
static int64_t led_off_ms;

static void styles_init(void)
{
    if (styles_inited) {
        return;
    }
    styles_inited = true;

    lv_style_init(&style_tab);
    lv_style_set_size(&style_tab, 90, 28);
    lv_style_set_radius(&style_tab, LV_RADIUS_CIRCLE);
    lv_style_set_border_color(&style_tab, lv_color_hex(UI_DIM_COLOR));
    lv_style_set_border_width(&style_tab, 1);
    lv_style_set_bg_color(&style_tab, lv_color_hex(DICE_ACCENT_COLOR));
    lv_style_set_bg_opa(&style_tab, LV_OPA_TRANSP);
    lv_style_set_text_color(&style_tab, lv_color_hex(UI_DIM_COLOR));
    lv_style_set_text_font(&style_tab, &lv_font_montserrat_14);

    lv_style_init(&style_tab_focused);
    lv_style_set_bg_opa(&style_tab_focused, LV_OPA_COVER);
    lv_style_set_border_width(&style_tab_focused, 0);
    lv_style_set_text_color(&style_tab_focused, lv_color_hex(UI_BG_COLOR));
}

static void lots_load(void)
{
    static char buf[LOTS_MAX * (LOTS_LEN + 2)];
    struct fs_file_t file;

    lots_count = 0;
    fs_file_t_init(&file);
    if (!file_exists(LOTS_FILE) || fs_open(&file, LOTS_FILE, FS_O_READ) < 0) {
        return;
    }
    ssize_t len = fs_read(&file, buf, sizeof(buf) - 1);
    fs_close(&file);
    if (len <= 0) {
        return;
    }
    buf[len] = '\0';

    for (char *line = strtok(buf, "\r\n"); line != NULL && lots_count < LOTS_MAX; line = strtok(NULL, "\r\n")) {
        if (line[0] != '\0') {
            strncpy(lots[lots_count], line, LOTS_LEN);
            lots[lots_count][LOTS_LEN] = '\0';
            lots_count++;
        }
    }
}

static void dice_show(int value)
{
    for (int i = 0; i < 7; i++) {
        if (pip_mask[value] & BIT(i)) {
            lv_obj_remove_flag(pips[i], LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(pips[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void roll_step(void)
{
    if (mode == MODE_DICE) {
        dice_show(1 + sys_rand32_get() % 6);
    }
    else if (lots_count > 0) {
        lv_label_set_text(lot_label, lots[sys_rand32_get() % lots_count]);
    }
}

static void mode_set(int new_mode)
{
    mode = new_mode;
    if (rolling) {
        // 擲到一半切換分頁，LED 不能停在滾動中的隨機顏色
        rolling = false;
        led_off_ms = 0;
        led_restore();
    }
    if (mode == MODE_DICE) {
        lv_obj_remove_flag(dice, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(lot_label, LV_OBJ_FLAG_HIDDEN);
    }
    else {
        lv_obj_add_flag(dice, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(lot_label, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(lot_label, lots_count > 0 ? "?" : "Put one item per line\nin /lots.txt");
    }
}

static void tab_focused_cb(lv_event_t *e)
{
    mode_set((int)(intptr_t)lv_event_get_user_data(e));
}

// 搖晃會連續觸發敲擊，每次都重新計時，停手後才停下來
static void tab_clicked_cb(lv_event_t *e)
{
    if (mode == MODE_LOTS && lots_count == 0) {
        return;
    }
    int64_t now = k_uptime_get();
    rolling = true;
    roll_end_ms = now + ROLL_MS;
    next_step_ms = now;
}

// 敲擊 / 搖晃是擲骰，不是切換分頁；next / prev 切分頁，ok 擲
static bool nav_cb(nav_cmd_t cmd)
{
    if (cmd != NAV_TAP) {
        return false;
    }
    tab_clicked_cb(NULL);
    return true;
}

static lv_obj_t *tab_create(lv_obj_t *parent, const char *text, int tab_mode)
{
    lv_obj_t *tab = lv_obj_create(parent);
    lv_obj_remove_style_all(tab);
    lv_obj_add_style(tab, &style_tab, 0);
    lv_obj_add_style(tab, &style_tab_focused, LV_STATE_FOCUSED);
    lv_obj_remove_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(tab);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    lv_obj_add_event_cb(tab, tab_focused_cb, LV_EVENT_FOCUSED, (void *)(intptr_t)tab_mode);
    lv_obj_add_event_cb(tab, tab_clicked_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(lv_group_get_default(), tab);
    return tab;
}

static void app_dice_init(void *scr)
{
    styles_init();
    ui_screen_init(scr);
    lots_load();

    dice = lv_obj_create(scr);
    lv_obj_remove_style_all(dice);
    lv_obj_set_size(dice, DICE_SIZE, DICE_SIZE);
    lv_obj_set_style_radius(dice, 22, 0);
    lv_obj_set_style_bg_color(dice, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(dice, LV_OPA_COVER, 0);
    lv_obj_align(dice, LV_ALIGN_CENTER, 0, 6);
    lv_obj_remove_flag(dice, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    for (int i = 0; i < 7; i++) {
        pips[i] = lv_obj_create(dice);
        lv_obj_remove_style_all(pips[i]);
        lv_obj_set_size(pips[i], DICE_PIP_SIZE, DICE_PIP_SIZE);
        lv_obj_set_style_radius(pips[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(pips[i], lv_color_hex(UI_BG_COLOR), 0);
        lv_obj_set_style_bg_opa(pips[i], LV_OPA_COVER, 0);
        lv_obj_align(pips[i], LV_ALIGN_CENTER, pip_pos[i][0] * DICE_PIP_OFFSET, pip_pos[i][1] * DICE_PIP_OFFSET);
        lv_obj_remove_flag(pips[i], LV_OBJ_FLAG_CLICKABLE);
    }
    dice_show(6);

    lot_label = lv_label_create(scr);
    lv_obj_set_width(lot_label, 200);
    lv_label_set_long_mode(lot_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(lot_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lot_label, lv_color_white(), 0);
    lv_obj_align(lot_label, LV_ALIGN_CENTER, 0, 6);

    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "Tap or shake");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(UI_DIM_COLOR), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -12);

    lv_obj_t *tabs = lv_obj_create(scr);
    lv_obj_remove_style_all(tabs);
    lv_obj_set_size(tabs, LV_PCT(100), 28);
    lv_obj_align(tabs, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tabs, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tabs, 12, 0);

    rolling = false;
    led_off_ms = 0;
    // 第一個 tab 加入 group 時會被 focus，觸發 mode_set，所以內容要先建好
    tab_create(tabs, "DICE", MODE_DICE);
    tab_create(tabs, "LOTS", MODE_LOTS);
    nav_set_handler(nav_cb);
}

static void app_dice_run(void *scr)
{
    int64_t now = k_uptime_get();

    if (led_off_ms != 0 && now >= led_off_ms) {
        led_off_ms = 0;
        led_restore();
    }

    if (!rolling || now < next_step_ms) {
        return;
    }

    roll_step();
    if (now < roll_end_ms) {
        // 滾動中 LED 隨機變色
        led_set_hex(sys_rand32_get() & 0xffffff);
        next_step_ms = now + ROLL_STEP_MS;
        return;
    }

    rolling = false;
    led_set_hex(DICE_ACCENT_COLOR);
    led_off_ms = now + LED_HOLD_MS;
}

static void app_dice_exit(void *scr)
{
    led_restore();
}
