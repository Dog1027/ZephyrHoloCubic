#include "ui.h"

static lv_style_t style_item;
static lv_style_t style_item_focused;
static bool styles_inited;

static void styles_init(void)
{
    if (styles_inited) {
        return;
    }
    styles_inited = true;

    lv_style_init(&style_item);
    lv_style_set_width(&style_item, LV_PCT(100));
    lv_style_set_height(&style_item, 32);
    lv_style_set_radius(&style_item, 10);
    lv_style_set_pad_hor(&style_item, 10);
    lv_style_set_pad_column(&style_item, 8);
    lv_style_set_bg_opa(&style_item, LV_OPA_TRANSP);
    lv_style_set_text_color(&style_item, lv_color_white());
    lv_style_set_text_font(&style_item, &lv_font_montserrat_14);

    lv_style_init(&style_item_focused);
    lv_style_set_bg_opa(&style_item_focused, LV_OPA_COVER);
    lv_style_set_text_color(&style_item_focused, lv_color_hex(UI_BG_COLOR));
}

void ui_screen_init(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_BG_COLOR), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *ui_title(lv_obj_t *scr, const char *text)
{
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, text);
    lv_obj_set_width(title, 220);
    lv_label_set_long_mode(title, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(UI_DIM_COLOR), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    return title;
}

lv_obj_t *ui_list_create(lv_obj_t *scr)
{
    styles_init();

    lv_obj_t *list = lv_obj_create(scr);
    lv_obj_remove_style_all(list);
    lv_obj_set_size(list, LV_PCT(100), UI_SCREEN_SIZE - 34);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_hor(list, 8, 0);
    lv_obj_set_style_pad_bottom(list, 8, 0);
    lv_obj_set_style_pad_row(list, 2, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    return list;
}

lv_obj_t *ui_list_item(lv_obj_t *list, const char *icon, const char *text, const char *detail,
                       lv_color_t accent)
{
    lv_obj_t *item = lv_obj_create(list);
    lv_obj_remove_style_all(item);
    lv_obj_add_style(item, &style_item, 0);
    lv_obj_add_style(item, &style_item_focused, LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(item, accent, 0);
    lv_obj_add_flag(item, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *label = lv_label_create(item);
    lv_label_set_text(label, icon);

    label = lv_label_create(item);
    lv_label_set_text(label, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_flex_grow(label, 1);

    if (detail != NULL) {
        label = lv_label_create(item);
        lv_label_set_text(label, detail);
    }

    lv_group_add_obj(lv_group_get_default(), item);
    return item;
}

static void strdup_free_cb(lv_event_t *e)
{
    lv_free(lv_event_get_user_data(e));
}

const char *ui_obj_strdup(lv_obj_t *obj, const char *text)
{
    char *copy = lv_strdup(text);
    if (copy != NULL) {
        lv_obj_add_event_cb(obj, strdup_free_cb, LV_EVENT_DELETE, copy);
    }
    return copy;
}

lv_obj_t *ui_list_message(lv_obj_t *list, const char *text)
{
    lv_obj_t *label = lv_label_create(list);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(UI_DIM_COLOR), 0);
    lv_obj_set_style_pad_top(label, 40, 0);
    lv_obj_set_width(label, LV_PCT(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    return label;
}

lv_obj_t *ui_tap_area(lv_obj_t *scr, lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *area = lv_obj_create(scr);
    lv_obj_remove_style_all(area);
    lv_obj_set_size(area, LV_PCT(100), LV_PCT(100));
    lv_obj_remove_flag(area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(area, cb, LV_EVENT_CLICKED, user_data);
    lv_group_add_obj(lv_group_get_default(), area);
    return area;
}

void ui_group_solo(lv_obj_t *obj)
{
    lv_group_t *g = lv_group_get_default();
    lv_group_remove_all_objs(g);
    lv_group_add_obj(g, obj);
}

void ui_group_add_children(lv_obj_t *parent)
{
    lv_group_t *g = lv_group_get_default();
    uint32_t count = lv_obj_get_child_count(parent);
    for (uint32_t i = 0; i < count; i++) {
        lv_obj_t *child = lv_obj_get_child(parent, i);
        if (lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) {
            lv_group_add_obj(g, child);
        }
    }
}
