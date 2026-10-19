#include "bg_app.h"
#include "app_list.h"
#include "ui.h"

APP_BG_REGISTER(Menu, app_menu_init, app_menu_run);

#define MENU_BG_GRAD_COLOR  0x18202e
#define MENU_ICON_SIZE      96
#define MENU_ICON_GAP       20
#define MENU_ICON_GROW      12  // 被選到時每邊放大的像素
#define MENU_ANIM_MS        180

static const uint32_t accent_colors[] = {
    0x4f8ef7, 0xf25f5c, 0x43c59e, 0xf5a623, 0x9b6cf2, 0x2ec4d6,
};

static lv_style_t style_icon;
static lv_style_t style_icon_focused;
static lv_style_t style_dot;
static lv_style_t style_dot_active;

static lv_obj_t *name_label;
static lv_obj_t *dots;

static lv_color_t app_accent(int index)
{
    return lv_color_hex(accent_colors[(index - 1) % ARRAY_SIZE(accent_colors)]);
}

static void styles_init(void)
{
    static const lv_style_prop_t tr_props[] = {
        LV_STYLE_TRANSFORM_WIDTH, LV_STYLE_TRANSFORM_HEIGHT,
        LV_STYLE_BG_OPA, LV_STYLE_TEXT_OPA, LV_STYLE_WIDTH, 0,
    };
    static lv_style_transition_dsc_t tr;
    lv_style_transition_dsc_init(&tr, tr_props, lv_anim_path_ease_out, MENU_ANIM_MS, 0, NULL);

    lv_style_init(&style_icon);
    lv_style_set_size(&style_icon, MENU_ICON_SIZE, MENU_ICON_SIZE);
    lv_style_set_radius(&style_icon, 28);
    lv_style_set_bg_opa(&style_icon, LV_OPA_30);
    lv_style_set_text_color(&style_icon, lv_color_white());
    lv_style_set_text_opa(&style_icon, LV_OPA_50);
    lv_style_set_text_font(&style_icon, &lv_font_montserrat_48);
    lv_style_set_transition(&style_icon, &tr);

    lv_style_init(&style_icon_focused);
    lv_style_set_bg_opa(&style_icon_focused, LV_OPA_COVER);
    lv_style_set_text_opa(&style_icon_focused, LV_OPA_COVER);
    lv_style_set_transform_width(&style_icon_focused, MENU_ICON_GROW);
    lv_style_set_transform_height(&style_icon_focused, MENU_ICON_GROW);

    lv_style_init(&style_dot);
    lv_style_set_size(&style_dot, 6, 6);
    lv_style_set_radius(&style_dot, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&style_dot, lv_color_hex(UI_DIM_COLOR));
    lv_style_set_bg_opa(&style_dot, LV_OPA_COVER);
    lv_style_set_transition(&style_dot, &tr);

    lv_style_init(&style_dot_active);
    lv_style_set_width(&style_dot_active, 18);
}

static void icon_focused_cb(lv_event_t *e)
{
    lv_obj_t *icon = lv_event_get_target(e);
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    const app_t *app = app_list_get(index);

    lv_label_set_text(name_label, app->name);

    uint32_t count = lv_obj_get_child_count(dots);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *dot = lv_obj_get_child(dots, i);
        if (i == lv_obj_get_index(icon)) {
            lv_obj_add_style(dot, &style_dot_active, 0);
            lv_obj_set_style_bg_color(dot, app_accent(index), 0);
        }
        else {
            lv_obj_remove_style(dot, &style_dot_active, 0);
            lv_obj_set_style_bg_color(dot, lv_color_hex(UI_DIM_COLOR), 0);
        }
    }
}

static void icon_clicked_cb(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    LOG_INF("Open app index %d", index);
    app_mgr_load_app(index);
}

static lv_obj_t *icon_create(lv_obj_t *parent, int index, const app_t *app)
{
    lv_obj_t *icon = lv_obj_create(parent);
    lv_obj_remove_style_all(icon);
    lv_obj_add_style(icon, &style_icon, 0);
    lv_obj_add_style(icon, &style_icon_focused, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(icon, app_accent(index), 0);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_remove_flag(icon, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *symbol = lv_label_create(icon);
    lv_label_set_text(symbol, app->icon ? app->icon : LV_SYMBOL_FILE);
    lv_obj_center(symbol);

    lv_obj_add_event_cb(icon, icon_focused_cb, LV_EVENT_FOCUSED, (void *)(intptr_t)index);
    lv_obj_add_event_cb(icon, icon_clicked_cb, LV_EVENT_CLICKED, (void *)(intptr_t)index);
    lv_group_add_obj(lv_group_get_default(), icon);
    return icon;
}

static void app_menu_init(void *scr)
{
    styles_init();

    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_BG_COLOR), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(MENU_BG_GRAD_COLOR), 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "CUBE");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(UI_DIM_COLOR), 0);
    lv_obj_set_style_text_letter_space(title, 4, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    // 橫向輪播：被選到的圖示會自動捲到正中間
    lv_obj_t *carousel = lv_obj_create(scr);
    lv_obj_remove_style_all(carousel);
    lv_obj_set_size(carousel, LV_PCT(100), MENU_ICON_SIZE + 2 * MENU_ICON_GROW + 8);
    lv_obj_align(carousel, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_flex_flow(carousel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(carousel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(carousel, MENU_ICON_GAP, 0);
    lv_obj_set_style_pad_hor(carousel, (UI_SCREEN_SIZE - MENU_ICON_SIZE) / 2, 0);
    lv_obj_set_scroll_dir(carousel, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(carousel, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(carousel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(carousel, LV_OBJ_FLAG_SCROLL_ELASTIC);

    name_label = lv_label_create(scr);
    lv_obj_set_style_text_color(name_label, lv_color_white(), 0);
    lv_obj_align(name_label, LV_ALIGN_BOTTOM_MID, 0, -42);

    dots = lv_obj_create(scr);
    lv_obj_remove_style_all(dots);
    lv_obj_set_size(dots, LV_PCT(100), 10);
    lv_obj_align(dots, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_flex_flow(dots, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dots, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dots, 6, 0);

    // index 0 是 Menu 自己，前景 app 從 1 開始
    const app_t *app;
    // 先建圓點再建圖示：第一個圖示加入 group 時就會被 focus，更新時需要對應的圓點已存在
    for (int i = 1; (app = app_list_get(i)) != NULL; i++) {
        lv_obj_t *dot = lv_obj_create(dots);
        lv_obj_remove_style_all(dot);
        lv_obj_add_style(dot, &style_dot, 0);

        icon_create(carousel, i, app);
    }

    if (lv_obj_get_child_count(carousel) == 0) {
        lv_label_set_text(name_label, "No apps");
        lv_obj_set_style_text_color(name_label, lv_color_hex(UI_DIM_COLOR), 0);
    }
}

static void app_menu_run(void *scr)
{
}
