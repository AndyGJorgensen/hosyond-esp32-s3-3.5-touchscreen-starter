// Native actions and variables for the Main screen dashboard: one card per board feature
// (WiFi, battery, SD card, speaker, microphone, backlight + RGB LED). WiFi values come from gui/wifi_screens.cpp.
#include "gui/main_screen.h"
#include <Arduino.h>
#include "app_config.h"
#include "drivers/backlight.h"
#include "drivers/rgb_led.h"
#include "managers/audio_manager.h"
#include "managers/battery_manager.h"
#include "managers/mic_manager.h"
#include "managers/storage_manager.h"
#include "ui/screens.h"
#include "ui/actions.h"
#include "ui/vars.h"

// Buffers returned by the native variable getters (EEZ flow reads them every tick). Every text was measured
// to fit the cards' 132 px text width at Montserrat 14 (the battery voltage uses Montserrat 24).
static char s_sd_status[24] = "No card";
static char s_sd_space[24] = "-";
static char s_battery_volts[12] = "-";

static void refresh_sd() {
  if (!storage_manager.mounted()) {
    strlcpy(s_sd_status, "No card", sizeof(s_sd_status));
    strlcpy(s_sd_space, "-", sizeof(s_sd_space));
    return;
  }
  // exists() touches the card, so this runs once a second rather than every tick
  strlcpy(s_sd_status, storage_manager.exists("/audio/" AUDIO_TEST_SOUND) ? "Ready" : "No " AUDIO_TEST_SOUND,
          sizeof(s_sd_status));
  snprintf(s_sd_space, sizeof(s_sd_space), "%.1f / %.1f GB", storage_manager.free_gb(), storage_manager.total_gb());
}

static void refresh_battery() {
  snprintf(s_battery_volts, sizeof(s_battery_volts), "%.2f V", battery_manager.voltage());
}

void main_screen_init() {
  rgb_led_off();
  // The slider's range is 5..100 in the EEZ project; start it at the configured brightness
  if (objects.backlight_slider) lv_slider_set_value(objects.backlight_slider, BACKLIGHT_DEFAULT_PCT, LV_ANIM_OFF);
  refresh_battery();
}

void main_screen_loop() {
  static uint32_t last;
  if (millis() - last < 1000) return;
  last = millis();
  refresh_sd();
  refresh_battery();
}

// Plays AUDIO_TEST_SOUND from the card (the built-in beep if it's missing); a second press stops it
void action_play_sd_sound(lv_event_t *e) {
  if (audio_manager.playing()) audio_manager.stop();
  else audio_manager.play(AUDIO_TEST_SOUND);
}

void action_backlight_changed(lv_event_t *e) {
  const int pct = lv_slider_get_value((lv_obj_t *)lv_event_get_target(e));
  backlight_set(max(pct, BACKLIGHT_MIN_PCT));
}

// Cycles the RGB LED: off -> red -> green -> blue -> white -> off
void action_led_next(lv_event_t *e) {
  static const uint8_t COLORS[][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 1, 1}};
  static int i;
  i = (i + 1) % 5;
  rgb_led_set(COLORS[i][0] * RGB_LED_LEVEL, COLORS[i][1] * RGB_LED_LEVEL, COLORS[i][2] * RGB_LED_LEVEL);
}

const char *get_var_sd_status() { return s_sd_status; }
void set_var_sd_status(const char *) {}
const char *get_var_sd_space() { return s_sd_space; }
void set_var_sd_space(const char *) {}
const char *get_var_battery_volts() { return s_battery_volts; }
void set_var_battery_volts(const char *) {}
int32_t get_var_mic_level() { return mic_manager.level_pct(); }
void set_var_mic_level(int32_t) {}
