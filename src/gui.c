#include <zephyr/drivers/led.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include "input.h"

LOG_MODULE_REGISTER(gui, CONFIG_GUI_LOG_LEVEL);

static void _test_1(void)
{

    /* Configure screen and background for transparency */
    // lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, LV_PART_MAIN);
    // lv_obj_set_style_bg_opa(lv_layer_bottom(), LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t *scr = lv_scr_act();
    if (!scr) {
        LOG_ERR("Failed to get active screen");
        return;
    }
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *label;
    /* Create a label, set its text and align it to the center */
    label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello, world!");
    lv_obj_set_style_text_color(label, lv_color_hex(0xff00ff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);
    label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "RED");
    lv_obj_set_style_text_color(label, lv_color_hex(0xff0000), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, -70, 20);
    label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "GREEN");
    lv_obj_set_style_text_color(label, lv_color_hex(0x00ff00), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 20);
    label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "BLUE");
    lv_obj_set_style_text_color(label, lv_color_hex(0x0000ff), LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 70, 20);

    /*Create an array for the points of the line*/
    static lv_point_precise_t line_points[] = { { 0, 0 }, { 238, 0 }, { 238, 238 }, { 0, 238 }, { 0, 0 } };

    /*Create style*/
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 1);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_line_rounded(&style_line, true);

    /*Create a line and apply the new style*/
    lv_obj_t *line1;
    line1 = lv_line_create(scr);
    lv_line_set_points(line1, line_points, 5); /*Set the points*/
    lv_obj_add_style(line1, &style_line, 0);
    lv_obj_center(line1);
}

static int gui_hello_word(void)
{
    _test_1();
    // _test_2();
    lv_timer_handler();
    return 0;
}

static void _input_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    char key = input_get();
    if (key) {
        data->key = key;
        data->state = LV_INDEV_STATE_PRESSED;
    } 
    else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static int gui_input_init(void)
{
    lv_group_t * g = lv_group_create();
    lv_group_set_default(g);

    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, _input_read);
    lv_indev_set_group(indev, g);
    return 0;
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

    const struct device *display_dev;

    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
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

    ret = gui_hello_word();
    if (ret != 0) {
        LOG_ERR("Failed to initialize GUI library");
        return ret;
    }

    ret = display_blanking_off(display_dev);
    if (ret != 0) {
        LOG_ERR("Failed to turn on display");
        return ret;
    }

    // input
    ret = gui_input_init();
    if (ret != 0) {
        LOG_ERR("Failed to initialize input device");
        return ret;
    }

    LOG_INF("GUI initialized");
    return 0;
}

SYS_INIT(gui_init, APPLICATION, CONFIG_GUI_INIT_PRIORITY);
