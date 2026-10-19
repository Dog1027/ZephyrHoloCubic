#include <zephyr/drivers/led.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include "gui.h"

LOG_MODULE_REGISTER(gui, CONFIG_GUI_LOG_LEVEL);

static const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

static void gui_clear_screen(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
}

// 不能在 SYS_INIT 裡畫：LVGL flush thread 要等所有 SYS_INIT 跑完才會啟動，
// 在那之前畫面送不出去，LVGL 會一直等它而卡死開機
void gui_start(void)
{
    lv_refr_now(NULL);

    int ret = display_blanking_off(display_dev);
    if (ret != 0) {
        LOG_ERR("Failed to turn on display (%d)", ret);
    }
}

static int gui_backlight_init(void)
{
    const struct device *led_dev;

    led_dev = DEVICE_DT_GET(DT_PATH(leds));
    if (!device_is_ready(led_dev)) {
        LOG_ERR("LED device not ready");
        return -1;
    }

    int ret;
    ret = led_on(led_dev, 0);
    if (ret != 0) {
        LOG_ERR("Failed to turn on led 0");
        return ret;
    }

    return 0;
}

static int gui_init(void)
{
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Device not ready");
        return 0;
    }

    int ret;

    // video
    ret = gui_backlight_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize backlight");
        return ret;
    }

    // 只設定背景，第一次繪圖和開啟螢幕由 app_thread 呼叫 gui_start()
    gui_clear_screen();

    LOG_INF("GUI initialized");
    return 0;
}

SYS_INIT(gui_init, APPLICATION, CONFIG_GUI_INIT_PRIORITY);
