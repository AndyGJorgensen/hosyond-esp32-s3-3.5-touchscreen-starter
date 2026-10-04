#include "drivers/rgb_led.h"
#include <Arduino.h>
#include "app_config.h"

void rgb_led_set(uint8_t r, uint8_t g, uint8_t b) { neopixelWrite(RGB_LED_PIN, r, g, b); }

void rgb_led_off() { rgb_led_set(0, 0, 0); }
