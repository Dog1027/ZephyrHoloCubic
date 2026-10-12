#include "bg_app.h"

APP_BG_REGISTER(Menu, app_menu_init, app_menu_run);

static lv_obj_t *img1;

static void app_menu_init(void *scr)
{
    // Initialization code for app menu
    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "App Menu Initialized");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    img1 = lv_image_create(scr);
    lv_image_set_src(img1, "/0:/image/img1.bin");
    lv_obj_align(img1, LV_ALIGN_CENTER, 0, 0);
    // lv_image_set_rotation(img1, 450);
}

static void app_menu_run(void *scr)
{
    // Main loop code for app menu
    // lv_image_set_rotation(img1, 450);
}
