#include "managers/battery_manager.h"
#include <Arduino.h>
#include "app_config.h"
#include "managers/audio_manager.h"

BatteryManager battery_manager;

void BatteryManager::begin() { voltage_ = read_voltage(); }

float BatteryManager::read_voltage() {
  analogReadResolution(12);
  analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);  // pin sees up to ~2.1 V (4.2 V / 2)
  uint32_t sum = 0;
  for (int i = 0; i < 64; i++) sum += analogReadMilliVolts(BAT_ADC_PIN);
  return sum / 64.0f / 1000.0f * BAT_ADC_RATIO + BAT_CAL_OFFSET_V;
}

void BatteryManager::loop() {
  static uint32_t last_sample;
  if (millis() - last_sample < BAT_SAMPLE_MS) return;
  last_sample = millis();
  // The speaker's current dips the reading: skip while a sound plays and BAT_AUDIO_SETTLE_MS after
  static uint32_t last_audio_ms;
  if (audio_manager.playing()) last_audio_ms = millis();
  if (last_audio_ms && millis() - last_audio_ms < BAT_AUDIO_SETTLE_MS) return;
  voltage_ += (read_voltage() - voltage_) * 0.3f;  // light smoothing
}
