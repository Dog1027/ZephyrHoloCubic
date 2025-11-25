#include <zephyr/drivers/led.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <lvgl.h>

static void gui_thread_entry(void *arg1, void *arg2, void *arg3);

// 參數順序：(名稱, 堆疊大小, 函式指標, p1, p2, p3, 優先級, 選項, 啟動延遲)
K_THREAD_DEFINE(
    gui_thread,                 // 執行緒名稱 (也是變數名)
    1024,                       // 堆疊大小
    gui_thread_entry,        // 執行緒函式
    NULL, NULL, NULL,           // 參數 p1, p2, p3
    7,                          // 優先級 (0 是最高優先級)
    0,                          // 選項旗標 (通常為 0)
    0                           // 啟動延遲 (0 表示立即啟動)
);

void lv_example_get_started_1(void)
{

    /* Configure screen and background for transparency */
    // lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_TRANSP, LV_PART_MAIN);
    // lv_obj_set_style_bg_opa(lv_layer_bottom(), LV_OPA_TRANSP, LV_PART_MAIN);
    
    lv_obj_t *scr = lv_scr_act();
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
}

void lv_example_get_started_2(void)
{
    /*Create an array for the points of the line*/
    static lv_point_precise_t line_points[] = { {0, 0}, {238, 0}, {238, 238}, {0, 238}, {0, 0} };

    /*Create style*/
    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 1);
    lv_style_set_line_color(&style_line, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_line_rounded(&style_line, true);

    /*Create a line and apply the new style*/
    lv_obj_t * line1;
    line1 = lv_line_create(lv_screen_active());
    lv_line_set_points(line1, line_points, 5);     /*Set the points*/
    lv_obj_add_style(line1, &style_line, 0);
    lv_obj_center(line1);
}

int backlight_init(void)
{
    const struct device *led_dev;

    led_dev = DEVICE_DT_GET(DT_PATH(leds));
    if (!device_is_ready(led_dev)) {
        printk("LED device not ready\n");
        return -1;
    }

    int ret;
    ret = led_on(led_dev, 0);
    if (ret != 0) {
        printk("Failed to turn on led 0\n");
        return ret;
    }

    return 0;
}


// 執行緒的進入點函式
static void gui_thread_entry(void *arg1, void *arg2, void *arg3)
{
    printk("GUI thread started.\n");
    while (1) {
        uint32_t sleep_ms = lv_timer_handler();
        k_msleep(MIN(sleep_ms, INT32_MAX));
    }
}


int gui_init(void)
{
    const struct device *display_dev;

    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        printk("Device not ready, aborting test");
        return 0;
    }

    int ret;

    ret = backlight_init();
    if (ret != 0) {
        printk("Failed to initialize backlight\n");
        return ret;
    }

    lv_example_get_started_1();
    lv_example_get_started_2();
    
    display_blanking_off(display_dev);

    printk("GUI initialized successfully.\n");
    return 0;
}

SYS_INIT(gui_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

