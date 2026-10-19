#include "fg_app.h"
#include "ui.h"
#include "input.h"
#include "led.h"
#include "nav.h"
#include <math.h>

APP_FG_REGISTER(Timer, LV_SYMBOL_BELL, app_timer_init, app_timer_run, app_timer_exit, 12);

#define TIMER_ACCENT_COLOR  0xf25f5c
#define TIMER_ARC_SIZE      196
#define TIMER_FACE_DOWN_DEG 150.0f  // |roll| 超過這個角度表示螢幕朝下
#define TIMER_BLINK_MS      250
#define TIMER_BLINK_COUNT   12

typedef enum { STATE_SET, STATE_RUNNING, STATE_PAUSED, STATE_DONE } timer_state_t;

static const uint8_t presets[] = {5, 10, 15, 25, 45, 60};

static lv_style_t style_chip;
static lv_style_t style_chip_focused;
static bool styles_inited;

static lv_obj_t *arc;
static lv_obj_t *time_label;
static lv_obj_t *state_label;
static lv_obj_t *chips;
static lv_obj_t *tap_area;

// 計時狀態放在 app 外面，離開 app 也會繼續跑，回來時接著顯示
static timer_state_t state = STATE_SET;
static uint32_t total_ms = 25 * 60 * 1000;
static uint32_t remain_ms = 25 * 60 * 1000;
static int64_t end_ms;
static int32_t shown_sec;
static bool face_down;

static void alarm_blink(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(alarm_work, alarm_blink);
static int blink_step;

static void alarm_blink(struct k_work *work)
{
    if (blink_step >= TIMER_BLINK_COUNT) {
        led_restore();
        return;
    }
    if (blink_step % 2 == 0) {
        led_set_hex(TIMER_ACCENT_COLOR);
    }
    else {
        led_set(0, 0, 0);
    }
    blink_step++;
    k_work_reschedule(&alarm_work, K_MSEC(TIMER_BLINK_MS));
}

// 不在 app 裡也要能響，所以用 kernel timer 而不是在 run 裡檢查
static void alarm_expiry(struct k_timer *timer)
{
    blink_step = 0;
    k_work_reschedule(&alarm_work, K_NO_WAIT);
}
static K_TIMER_DEFINE(alarm_timer, alarm_expiry, NULL);

static void styles_init(void)
{
    if (styles_inited) {
        return;
    }
    styles_inited = true;

    lv_style_init(&style_chip);
    lv_style_set_size(&style_chip, 32, 24);
    lv_style_set_radius(&style_chip, 8);
    lv_style_set_bg_color(&style_chip, lv_color_hex(TIMER_ACCENT_COLOR));
    lv_style_set_bg_opa(&style_chip, LV_OPA_TRANSP);
    lv_style_set_text_color(&style_chip, lv_color_hex(UI_DIM_COLOR));
    lv_style_set_text_font(&style_chip, &lv_font_montserrat_14);

    lv_style_init(&style_chip_focused);
    lv_style_set_bg_opa(&style_chip_focused, LV_OPA_COVER);
    lv_style_set_text_color(&style_chip_focused, lv_color_hex(UI_BG_COLOR));
}

static uint32_t remaining(void)
{
    if (state != STATE_RUNNING) {
        return state == STATE_DONE ? 0 : remain_ms;
    }
    int64_t left = end_ms - k_uptime_get();
    return left > 0 ? (uint32_t)left : 0;
}

static void time_show(uint32_t ms, bool force)
{
    // 無條件進位，剩 0.5 秒時還是顯示 00:01
    int32_t sec = (ms + 999) / 1000;
    if (sec == shown_sec && !force) {
        return;
    }
    shown_sec = sec;
    lv_label_set_text_fmt(time_label, "%02d:%02d", sec / 60, sec % 60);
    lv_arc_set_value(arc, total_ms ? (int32_t)((uint64_t)ms * 1000 / total_ms) : 0);
}

static void state_set(timer_state_t new_state)
{
    static const char *const text[] = {
        [STATE_SET] = "Next / prev to pick, OK to start",
        [STATE_RUNNING] = "OK, tap or face down to pause",
        [STATE_PAUSED] = "Paused, OK or tap to resume",
        [STATE_DONE] = "Done! OK or tap to reset",
    };

    // 狀態相同時只更新畫面 (例如重新開啟 app)，不要重新計時
    switch (new_state == state ? -1 : (int)new_state) {
    case STATE_RUNNING:
        end_ms = k_uptime_get() + remain_ms;
        k_timer_start(&alarm_timer, K_MSEC(remain_ms), K_NO_WAIT);
        break;
    case STATE_PAUSED:
        remain_ms = remaining();
        k_timer_stop(&alarm_timer);
        break;
    case STATE_SET:
        k_work_cancel_delayable(&alarm_work);
        led_restore();
        remain_ms = total_ms;
        break;
    case STATE_DONE:
        remain_ms = 0;
        break;
    default:
        break;
    }
    state = new_state;

    lv_label_set_text(state_label, text[state]);
    if (state == STATE_SET) {
        lv_obj_remove_flag(chips, LV_OBJ_FLAG_HIDDEN);
        lv_group_remove_obj(tap_area);
        // 第一個加入 group 的選項會被 focus 並改掉 total_ms，先記住上次選的
        uint32_t last_ms = total_ms;
        ui_group_add_children(chips);
        // highlight 上次選的分鐘數，不要每次都跳回第一個
        for (int i = 0; i < (int)ARRAY_SIZE(presets); i++) {
            if (presets[i] * 60 * 1000 == last_ms) {
                lv_group_focus_obj(lv_obj_get_child(chips, i));
                break;
            }
        }
    }
    else {
        lv_obj_add_flag(chips, LV_OBJ_FLAG_HIDDEN);
        ui_group_solo(tap_area);
    }
    time_show(remaining(), true);
}

static void chip_focused_cb(lv_event_t *e)
{
    total_ms = (uint32_t)(intptr_t)lv_event_get_user_data(e) * 60 * 1000;
    remain_ms = total_ms;
    time_show(remain_ms, true);
}

static void chip_clicked_cb(lv_event_t *e)
{
    state_set(STATE_RUNNING);
}

static void tap_cb(lv_event_t *e)
{
    switch (state) {
    case STATE_RUNNING:
        state_set(STATE_PAUSED);
        break;
    case STATE_PAUSED:
        state_set(STATE_RUNNING);
        break;
    case STATE_DONE:
        state_set(STATE_SET);
        break;
    default:
        break;
    }
}

// 計時中敲一下等於 OK (暫停 / 繼續 / 重設)；選分鐘時敲一下照預設換下一個
static bool nav_cb(nav_cmd_t cmd)
{
    if (cmd != NAV_TAP || state == STATE_SET) {
        return false;
    }
    tap_cb(NULL);
    return true;
}

static void app_timer_init(void *scr)
{
    styles_init();
    ui_screen_init(scr);

    arc = lv_arc_create(scr);
    lv_obj_set_size(arc, TIMER_ARC_SIZE, TIMER_ARC_SIZE);
    lv_obj_center(arc);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, 1000);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x1c2333), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(TIMER_ACCENT_COLOR), LV_PART_INDICATOR);

    time_label = lv_label_create(scr);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -10);

    state_label = lv_label_create(scr);
    lv_obj_set_width(state_label, 140);
    lv_label_set_long_mode(state_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_align(state_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(state_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(state_label, lv_color_hex(UI_DIM_COLOR), 0);
    lv_obj_align(state_label, LV_ALIGN_CENTER, 0, 34);

    tap_area = ui_tap_area(scr, tap_cb, NULL);
    lv_group_remove_obj(tap_area);

    chips = lv_obj_create(scr);
    lv_obj_remove_style_all(chips);
    lv_obj_set_size(chips, LV_PCT(100), 24);
    lv_obj_align(chips, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_flex_flow(chips, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(chips, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(chips, 4, 0);

    for (int i = 0; i < (int)ARRAY_SIZE(presets); i++) {
        lv_obj_t *chip = lv_obj_create(chips);
        lv_obj_remove_style_all(chip);
        lv_obj_add_style(chip, &style_chip, 0);
        lv_obj_add_style(chip, &style_chip_focused, LV_STATE_FOCUSED);
        lv_obj_remove_flag(chip, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *label = lv_label_create(chip);
        lv_label_set_text_fmt(label, "%d", presets[i]);
        lv_obj_center(label);

        lv_obj_add_event_cb(chip, chip_focused_cb, LV_EVENT_FOCUSED, (void *)(intptr_t)presets[i]);
        lv_obj_add_event_cb(chip, chip_clicked_cb, LV_EVENT_CLICKED, NULL);
    }

    face_down = false;
    nav_set_handler(nav_cb);
    if (state == STATE_RUNNING && remaining() == 0) {
        state = STATE_DONE;
    }
    state_set(state);
}

static void app_timer_run(void *scr)
{
    input_angle_t angle;

    if (state == STATE_RUNNING && remaining() == 0) {
        state_set(STATE_DONE);
        return;
    }

    // 翻面朝下時暫停，只在翻過去的那一下觸發
    input_get(&angle);
    bool down = fabsf(angle.roll) > TIMER_FACE_DOWN_DEG;
    if (down && !face_down && state == STATE_RUNNING) {
        state_set(STATE_PAUSED);
    }
    face_down = down;

    time_show(remaining(), false);
}

static void app_timer_exit(void *scr)
{
}
