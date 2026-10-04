#pragma once
#include <stdint.h>

// The board's single WS2812B RGB LED (RGB_LED_PIN), using the Arduino core's built-in neopixelWrite().
void rgb_led_set(uint8_t r, uint8_t g, uint8_t b);  // 0-255 each
void rgb_led_off();
