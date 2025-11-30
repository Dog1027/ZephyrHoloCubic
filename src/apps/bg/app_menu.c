#include "bg_app.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_menu, CONFIG_APP_MENU_LOG_LEVEL);

static void app_menu_init(lv_obj_t *scr)
{
    // Initialization code for app menu
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "App Menu Initialized");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

}

static void app_menu_run(lv_obj_t *scr)
{
    // Main loop code for app menu
}

APP_BG_REGISTER("Menu", app_menu_init, app_menu_run);
