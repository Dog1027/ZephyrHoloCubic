#pragma once

#include <stdint.h>

// Brightness cap applied by led_set(), full 255 on a WS2812 is blinding
#define LED_MAX_BRIGHTNESS 64

// Set the RGB LED, each channel 0-255 is scaled down to LED_MAX_BRIGHTNESS
int led_set(uint8_t r, uint8_t g, uint8_t b);

// Same as led_set() but takes 0xRRGGBB
int led_set_hex(uint32_t rgb);

// Set the color the LED returns to after temporary effects (the Light app's choice), saved across reboots
int led_set_base(uint32_t rgb);

// The current base color (0xRRGGBB, 0 means off)
uint32_t led_get_base(void);

// Go back to the base color, use this instead of turning the LED off
int led_restore(void);

// Write the raw values without scaling
int led_set_raw(uint8_t r, uint8_t g, uint8_t b);
