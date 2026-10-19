#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include "led.h"
#include "prefs.h"

static const struct device *led_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(rgb_led));
// app thread、shell 和 Timer 的 work queue 都會寫 LED
static K_MUTEX_DEFINE(led_mutex);
static uint32_t base_rgb;

int led_set_raw(uint8_t r, uint8_t g, uint8_t b)
{
    if (led_dev == NULL || !device_is_ready(led_dev)) {
        return -ENODEV;
    }

    struct led_rgb pixel = { .r = r, .g = g, .b = b };
    k_mutex_lock(&led_mutex, K_FOREVER);
    int ret = led_strip_update_rgb(led_dev, &pixel, 1);
    k_mutex_unlock(&led_mutex);
    return ret;
}

int led_set(uint8_t r, uint8_t g, uint8_t b)
{
    return led_set_raw(r * LED_MAX_BRIGHTNESS / 255, g * LED_MAX_BRIGHTNESS / 255,
                       b * LED_MAX_BRIGHTNESS / 255);
}

int led_set_hex(uint32_t rgb)
{
    return led_set((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
}

int led_set_base(uint32_t rgb)
{
    if (rgb != base_rgb) {
        prefs_save("led/base", &rgb, sizeof(rgb));
    }
    base_rgb = rgb;
    return led_set_hex(rgb);
}

uint32_t led_get_base(void)
{
    return base_rgb;
}

int led_restore(void)
{
    return led_set_hex(base_rgb);
}

// 開機時恢復上次 Light app 設定的顏色
static int led_init(void)
{
    if (prefs_load("led/base", &base_rgb, sizeof(base_rgb)) == sizeof(base_rgb)) {
        led_restore();
    }
    else {
        base_rgb = 0;
    }
    return 0;
}

SYS_INIT(led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
