#include "fg_app.h"
#include "ui.h"
#include "led.h"
#include <math.h>

APP_FG_REGISTER(Light, LV_SYMBOL_CHARGE, app_light_init, app_light_run, app_light_exit, 11);

#define LIGHT_RING_RADIUS   98
#define LIGHT_DOT_SIZE      22
#define LIGHT_PREVIEW_SIZE  110
#define LIGHT_OFF           (ARRAY_SIZE(colors) - 1)

typedef struct {
    const char *name;
    uint32_t rgb;
} light_color_t;

static const light_color_t colors[] = {
    {"Red", 0xff3b30},    {"Orange", 0xff9500}, {"Yellow", 0xffcc00}, {"Lime", 0xa8e10c},
    {"Green", 0x34c759},  {"Mint", 0x00c7be},   {"Cyan", 0x32d6ff},   {"Sky", 0x0a84ff},
    {"Blue", 0x3a3aff},   {"Purple", 0x8e44ff}, {"Magenta", 0xff2dd1}, {"Pink", 0xff6b9a},
    {"White", 0xffffff},  {"Off", 0x000000},
};

static lv_style_t style_dot;
static lv_style_t style_dot_focused;
static lv_style_t style_dot_checked;
static bool styles_inited;

static lv_obj_t *dots[ARRAY_SIZE(colors)];
static lv_obj_t *preview;
static lv_obj_t *name_label;
static lv_obj_t *state_label;
// 確認過的顏色離開 app 後還會繼續亮著，存在 led.c 的基礎顏色裡 (重開機也會保留)
static int saved = LIGHT_OFF;

static void styles_init(void)
{
    if (styles_inited) {
        return;
    }
    styles_inited = true;

    lv_style_init(&style_dot);
    lv_style_set_size(&style_dot, LIGHT_DOT_SIZE, LIGHT_DOT_SIZE);
    lv_style_set_radius(&style_dot, LV_RADIUS_CIRCLE);
    lv_style_set_bg_opa(&style_dot, LV_OPA_COVER);
    lv_style_set_border_color(&style_dot, lv_color_hex(UI_DIM_COLOR));

    lv_style_init(&style_dot_focused);
    lv_style_set_transform_width(&style_dot_focused, 5);
    lv_style_set_transform_height(&style_dot_focused, 5);
    lv_style_set_border_color(&style_dot_focused, lv_color_white());
    lv_style_set_border_width(&style_dot_focused, 2);

    lv_style_init(&style_dot_checked);
    lv_style_set_outline_color(&style_dot_checked, lv_color_white());
    lv_style_set_outline_width(&style_dot_checked, 2);
    lv_style_set_outline_pad(&style_dot_checked, 3);
}

static void preview_show(int index)
{
    lv_obj_set_style_bg_color(preview, lv_color_hex(colors[index].rgb), 0);
    lv_obj_set_style_border_width(preview, index == LIGHT_OFF ? 2 : 0, 0);
    lv_label_set_text(name_label, colors[index].name);
    // 文字顏色設在 preview 上，兩個 label 一起繼承
    lv_obj_set_style_text_color(preview, lv_color_hex(index == LIGHT_OFF ? UI_DIM_COLOR : UI_BG_COLOR), 0);
    lv_label_set_text(state_label, index == saved ? "Current" : "OK to set");
}

static void dot_focused_cb(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    preview_show(index);
    // 預覽：highlight 到哪個顏色 LED 就先亮哪個
    led_set_hex(colors[index].rgb);
}

static void dot_clicked_cb(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);

    lv_obj_remove_state(dots[saved], LV_STATE_CHECKED);
    saved = index;
    lv_obj_add_state(dots[saved], LV_STATE_CHECKED);
    led_set_base(colors[saved].rgb);
    preview_show(index);
}

static void app_light_init(void *scr)
{
    styles_init();
    ui_screen_init(scr);

    saved = LIGHT_OFF;
    for (int i = 0; i < (int)ARRAY_SIZE(colors); i++) {
        if (colors[i].rgb == led_get_base()) {
            saved = i;
            break;
        }
    }

    preview = lv_obj_create(scr);
    lv_obj_remove_style_all(preview);
    lv_obj_set_size(preview, LIGHT_PREVIEW_SIZE, LIGHT_PREVIEW_SIZE);
    lv_obj_set_style_radius(preview, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(preview, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(preview, lv_color_hex(UI_DIM_COLOR), 0);
    lv_obj_center(preview);
    lv_obj_remove_flag(preview, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    name_label = lv_label_create(preview);
    lv_obj_align(name_label, LV_ALIGN_CENTER, 0, -8);

    state_label = lv_label_create(preview);
    lv_obj_set_style_text_font(state_label, &lv_font_montserrat_14, 0);
    lv_obj_align(state_label, LV_ALIGN_CENTER, 0, 14);

    // 從正上方開始順時針排一圈
    for (int i = 0; i < (int)ARRAY_SIZE(colors); i++) {
        float a = 2.0f * 3.14159265f * i / ARRAY_SIZE(colors);
        lv_obj_t *dot = lv_obj_create(scr);
        lv_obj_remove_style_all(dot);
        lv_obj_add_style(dot, &style_dot, 0);
        lv_obj_add_style(dot, &style_dot_focused, LV_STATE_FOCUSED);
        lv_obj_add_style(dot, &style_dot_checked, LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(dot, lv_color_hex(colors[i].rgb), 0);
        // 可捲動的物件被 encoder 按下時會進入編輯模式而不是點選
        lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        if (i == LIGHT_OFF) {
            lv_obj_set_style_border_width(dot, 1, 0);
        }
        lv_obj_align(dot, LV_ALIGN_CENTER, (int32_t)(LIGHT_RING_RADIUS * sinf(a)),
                     (int32_t)(-LIGHT_RING_RADIUS * cosf(a)));
        lv_obj_add_event_cb(dot, dot_focused_cb, LV_EVENT_FOCUSED, (void *)(intptr_t)i);
        lv_obj_add_event_cb(dot, dot_clicked_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        dots[i] = dot;
    }

    lv_obj_add_state(dots[saved], LV_STATE_CHECKED);
    // 全部建好才加入 group，第一個加入的會被 focus 並觸發 preview_show
    for (int i = 0; i < (int)ARRAY_SIZE(colors); i++) {
        lv_group_add_obj(lv_group_get_default(), dots[i]);
    }
    lv_group_focus_obj(dots[saved]);
}

static void app_light_run(void *scr)
{
}

static void app_light_exit(void *scr)
{
    // 只是預覽沒按確認的話，回到確認過的顏色
    led_restore();
}
