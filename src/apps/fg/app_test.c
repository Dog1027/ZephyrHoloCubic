#include "fg_app.h"
#include "ui.h"

// Scratch app for trying things out, kept last in the menu
APP_FG_REGISTER(Test, LV_SYMBOL_SETTINGS, app_test_init, app_test_run, app_test_exit, 99);

static void app_test_init(void *scr)
{
    // Initialization code for foreground test app
    ui_screen_init(scr);
    ui_title(scr, "Test");
}

static void app_test_run(void *scr)
{
    // Main loop code for foreground test app
}

static void app_test_exit(void *scr)
{
    // Cleanup code for foreground test app
}
