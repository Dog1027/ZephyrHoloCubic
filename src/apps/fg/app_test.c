#include "fg_app.h"

APP_FG_REGISTER(Test, app_test_init, app_test_run, CONFIG_APP_FG_PRIORITY_DEFAULT);

lv_obj_t *label;
char buf[16];

static void key_event_handler(lv_event_t * e)
{
    lv_indev_t * indev = lv_indev_active();
    char key = lv_indev_get_key(indev);
    if (key == '\0') {
        return;
    }
    buf[0] = key;
    lv_label_set_text(label, buf);
}

static void app_test_init(void *scr)
{
    // Initialization code for foreground test app
    label = lv_label_create(scr);
    lv_label_set_text(label, "Foreground App Initialized");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(label, key_event_handler, LV_EVENT_KEY, NULL);

    lv_obj_t * ta = lv_textarea_create(scr);
    lv_textarea_set_one_line(ta, true);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 10);

    lv_group_t *g = lv_group_get_default();
    lv_group_add_obj(g, ta);
    lv_group_add_obj(g, label);
}

static void app_test_run(void *scr)
{
    // Main loop code for foreground test app
}

