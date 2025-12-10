#include "bg_app.h"
#include <zephyr/logging/log.h>

APP_BG_REGISTER(Menu, app_menu_init, app_menu_run);

static void app_menu_init(void *scr)
{
    // Initialization code for app menu
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "App Menu Initialized");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

}

static void app_menu_run(void *scr)
{
    // Main loop code for app menu
}
