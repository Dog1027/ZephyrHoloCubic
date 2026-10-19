#pragma once

#include <lvgl.h>

#define UI_BG_COLOR     0x0b0f17
#define UI_DIM_COLOR    0x5a6478
#define UI_SCREEN_SIZE  240

// Black background, no scrolling
void ui_screen_init(lv_obj_t *scr);

// Small grey title at the top of the screen
lv_obj_t *ui_title(lv_obj_t *scr, const char *text);

// Vertical list below the title, items scroll into view when highlighted
lv_obj_t *ui_list_create(lv_obj_t *scr);

// List row: child 0 is the icon, child 1 the text, child 2 the optional detail.
// The row is added to the default group and gets the accent color when highlighted.
lv_obj_t *ui_list_item(lv_obj_t *list, const char *icon, const char *text, const char *detail,
                       lv_color_t accent);

// Copy of text that is freed together with obj. Use it instead of reading a label back:
// LV_LABEL_LONG_MODE_DOTS overwrites the end of the label's own text with "..."
const char *ui_obj_strdup(lv_obj_t *obj, const char *text);

// Grey centered text inside a list, e.g. "Empty folder"
lv_obj_t *ui_list_message(lv_obj_t *list, const char *text);

// Invisible full screen object in the default group, receives OK (ui ok) when nothing else is selectable
lv_obj_t *ui_tap_area(lv_obj_t *scr, lv_event_cb_t cb, void *user_data);

// Keep only obj in the default group, so next / prev cannot move the highlight away
void ui_group_solo(lv_obj_t *obj);

// Put the clickable children of parent back into the default group
void ui_group_add_children(lv_obj_t *parent);
