#include "fg_app.h"

static void app_test_init(lv_obj_t *scr)
{
    // Initialization code for foreground test app
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Foreground App Initialized");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

static void app_test_run(lv_obj_t *scr)
{
    // Main loop code for foreground test app
}

APP_FG_REGISTER("Test", app_test_init, app_test_run, CONFIG_APP_FG_PRIORITY_DEFAULT);
