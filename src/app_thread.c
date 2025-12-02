#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "app_mgr.h"
#include <lvgl.h>

LOG_MODULE_REGISTER(app_thread, CONFIG_APP_THREAD_LOG_LEVEL);

static void app_thread_entry(void *arg1, void *arg2, void *arg3);

K_THREAD_DEFINE(
    app_thread,
    0x2000,
    app_thread_entry,
    NULL, NULL, NULL,
    7,
    0,
    0
);

#if IS_ENABLED(CONFIG_APP_THREAD_SHOW_FPS)
static int _draw_fps(uint32_t sleep_ms)
{
    static lv_obj_t *label = NULL;
    if (!label) {
        lv_obj_t *scr = lv_layer_sys();
        label = lv_label_create(scr);
        lv_obj_set_style_text_color(label, lv_color_hex(0xff00ff), LV_PART_MAIN);
    }
    int frame_rate = 1000 / (sleep_ms ? sleep_ms : 1);
    char fps[5] = {0};
    snprintf(fps, sizeof(fps), "%d", frame_rate);
    lv_label_set_text(label, fps);
    return 0;
}
#endif

static void app_thread_entry(void *arg1, void *arg2, void *arg3)
{
    int ret;
    uint32_t sleep_ms = 0;
    const app_t *fg_app = NULL;
    const app_t *bg_app = NULL;
    lv_obj_t *fg_scr = lv_obj_create(NULL);
    lv_obj_t *bg_scr = lv_obj_create(NULL);
    app_mgr_event_t event;
    const void *data;
    do {
        ret = app_mgr_check(&event, &data);
        k_sleep(K_MSEC(100));
    }
    while (ret != 0);

    if (event != APP_MGR_EVENT_APP_LOAD || data == NULL) {
        LOG_ERR("No initial application to load, Exiting app thread");
        return;
    }

    bg_app = (app_t *)data;
    if (bg_app == NULL) {
        LOG_ERR("No background application set, Exiting app thread");
        return;
    }

    lv_screen_load_anim(bg_scr, LV_SCR_LOAD_ANIM_FADE_IN, CONFIG_APP_THREAD_ANIM_DURATION_MS, 0, true);
    bg_app->init(bg_scr);
    LOG_INF("Starting background application: %s", bg_app->name);
    while (1) {
        if (fg_app == NULL) {
            bg_app->run(bg_scr);
        }
        else {
            fg_app->run(fg_scr);
        }

        ret = app_mgr_check(&event, &data);
        if (ret == 0) {
            switch (event) {
                case APP_MGR_EVENT_NONE:
                    break;
                case APP_MGR_EVENT_APP_LOAD:
                    fg_app = (app_t *)data;
                    LOG_INF("Starting foreground application: %s", fg_app->name);
                    lv_obj_clean(fg_scr);
                    fg_app->init(fg_scr);
                    lv_screen_load_anim(fg_scr, LV_SCR_LOAD_ANIM_FADE_IN, CONFIG_APP_THREAD_ANIM_DURATION_MS, 0, false);
                    break;
                case APP_MGR_EVENT_APP_END:
                    if (fg_app == NULL) {
                        LOG_WRN("No foreground application to end");
                        break;
                    }
                    LOG_INF("Ending foreground application: %s", fg_app->name);
                    fg_app = NULL;
                    lv_screen_load_anim(bg_scr, LV_SCR_LOAD_ANIM_FADE_OUT, CONFIG_APP_THREAD_ANIM_DURATION_MS, 0, false);
                    break;
                default:
                    LOG_ERR("Unknown app manager event: %d, Exiting app thread", event);
                    return;
            }
        }

    #if IS_ENABLED(CONFIG_APP_THREAD_SHOW_FPS)
        _draw_fps(sleep_ms);
    #endif

        sleep_ms = lv_timer_handler();
        if (sleep_ms == LV_NO_TIMER_READY) {
            sleep_ms = CONFIG_APP_THREAD_MAX_SLEEP_MS;
        }
        k_msleep(sleep_ms);
    }
}
