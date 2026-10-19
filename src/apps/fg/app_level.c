#include "fg_app.h"
#include "ui.h"
#include "input.h"
#include "led.h"
#include "prefs.h"
#include "nav.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

APP_FG_REGISTER(Level, LV_SYMBOL_GPS, app_level_init, app_level_run, app_level_exit, 9);

#define LEVEL_DIAL_SIZE     200
#define LEVEL_BUBBLE_SIZE   28
#define LEVEL_FULL_DEG      20.0f  // 傾斜這麼多度時氣泡碰到外圈
#define LEVEL_OK_DEG        1.0f   // 兩軸都在這個角度內算水平
#define LEVEL_OK_COLOR      0x43c59e
#define LEVEL_TILT_COLOR    0xf5a623

static lv_obj_t *mode_label;
static lv_obj_t *angle_label;
static lv_obj_t *bubble;
static input_angle_t zero;
static input_angle_t shown;
static bool zeroed;
static int is_level;

static lv_obj_t *circle_create(lv_obj_t *parent, int32_t size)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, size, size);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_center(obj);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

static void line_create(lv_obj_t *parent, int32_t w, int32_t h)
{
    lv_obj_t *obj = circle_create(parent, 0);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(UI_DIM_COLOR), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_50, 0);
    lv_obj_center(obj);
}

static void fmt_deg(char *buf, size_t len, float deg)
{
    int tenths = (int)lroundf(deg * 10.0f);
    snprintf(buf, len, "%s%d.%d°", tenths < 0 ? "-" : "", abs(tenths) / 10, abs(tenths) % 10);
}

static void tap_cb(lv_event_t *e)
{
    // OK 或敲一下以目前角度為零點，再一次回到絕對角度
    zeroed = !zeroed;
    if (zeroed) {
        input_get(&zero);
    }
    else {
        zero.roll = 0.0f;
        zero.pitch = 0.0f;
    }
    lv_label_set_text(mode_label, zeroed ? "ZEROED" : "LEVEL");
    prefs_save("level/zero", &zero, sizeof(zero));
    // 強制下一次 run 更新
    shown.roll = NAN;
}

// 敲一下也能設零點 (不接電腦時用)
static bool nav_cb(nav_cmd_t cmd)
{
    if (cmd != NAV_TAP) {
        return false;
    }
    tap_cb(NULL);
    return true;
}

static void app_level_init(void *scr)
{
    ui_screen_init(scr);
    mode_label = ui_title(scr, "LEVEL");

    lv_obj_t *dial = circle_create(scr, LEVEL_DIAL_SIZE);
    lv_obj_set_style_border_color(dial, lv_color_hex(UI_DIM_COLOR), 0);
    lv_obj_set_style_border_width(dial, 2, 0);

    line_create(scr, LEVEL_DIAL_SIZE, 1);
    line_create(scr, 1, LEVEL_DIAL_SIZE);

    lv_obj_t *target = circle_create(scr, LEVEL_BUBBLE_SIZE + 8);
    lv_obj_set_style_border_color(target, lv_color_white(), 0);
    lv_obj_set_style_border_width(target, 1, 0);

    bubble = circle_create(scr, LEVEL_BUBBLE_SIZE);
    lv_obj_set_style_bg_opa(bubble, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bubble, lv_color_hex(LEVEL_TILT_COLOR), 0);

    angle_label = lv_label_create(scr);
    lv_obj_set_style_text_font(angle_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(angle_label, lv_color_white(), 0);
    lv_obj_align(angle_label, LV_ALIGN_BOTTOM_MID, 0, -4);

    ui_tap_area(scr, tap_cb, NULL);
    nav_set_handler(nav_cb);

    // 零點存在 flash，重開機後還是同一個基準
    if (prefs_load("level/zero", &zero, sizeof(zero)) != sizeof(zero)) {
        zero.roll = 0.0f;
        zero.pitch = 0.0f;
    }
    zeroed = zero.roll != 0.0f || zero.pitch != 0.0f;
    lv_label_set_text(mode_label, zeroed ? "ZEROED" : "LEVEL");
    shown.roll = NAN;
    is_level = -1;
}

static void app_level_run(void *scr)
{
    input_angle_t angle;
    char roll[16];
    char pitch[16];

    input_get(&angle);
    angle.roll -= zero.roll;
    angle.pitch -= zero.pitch;

    if (fabsf(angle.roll - shown.roll) < 0.05f && fabsf(angle.pitch - shown.pitch) < 0.05f) {
        return;
    }
    shown = angle;

    // 氣泡往高的那一側跑，所以跟傾斜方向相反
    const float max_r = (LEVEL_DIAL_SIZE - LEVEL_BUBBLE_SIZE) / 2.0f;
    float x = -angle.roll / LEVEL_FULL_DEG * max_r;
    float y = -angle.pitch / LEVEL_FULL_DEG * max_r;
    float r = sqrtf(x * x + y * y);
    if (r > max_r) {
        x = x * max_r / r;
        y = y * max_r / r;
    }
    lv_obj_align(bubble, LV_ALIGN_CENTER, (int32_t)x, (int32_t)y);

    fmt_deg(roll, sizeof(roll), angle.roll);
    fmt_deg(pitch, sizeof(pitch), angle.pitch);
    lv_label_set_text_fmt(angle_label, "R %s   P %s", roll, pitch);

    int level = fabsf(angle.roll) < LEVEL_OK_DEG && fabsf(angle.pitch) < LEVEL_OK_DEG;
    if (level != is_level) {
        is_level = level;
        lv_obj_set_style_bg_color(bubble, lv_color_hex(level ? LEVEL_OK_COLOR : LEVEL_TILT_COLOR), 0);
        if (level) {
            led_set_hex(LEVEL_OK_COLOR);
        }
        else {
            led_restore();
        }
    }
}

static void app_level_exit(void *scr)
{
    led_restore();
}
