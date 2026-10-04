#pragma once

// Battery voltage: 1-cell LiPo on JP1 (TP4054 charger), read on BAT_ADC_PIN through a 1:2 divider.
// Only the voltage reaches the ESP32 (no charge-status or USB-sense pin). Readings are averaged, calibrated
// (BAT_CAL_OFFSET_V) and smoothed; samples are skipped while a sound plays (the speaker current dips them).
// With no battery fitted the reading is the charger's output (about 4.2 V and up).
class BatteryManager {
 public:
  void begin();
  void loop();

  float voltage() const { return voltage_; }  // smoothed, calibrated cell voltage
  static float read_voltage();                // one averaged reading (ADC, divider, calibration)

 private:
  float voltage_ = 0;
};

extern BatteryManager battery_manager;
