#pragma once
#include <stdint.h>

// Everest ES8311 audio codec on the shared I2C bus (Wire, started in main.cpp), I2S slave.
// Playback (DAC -> SC8002B amp -> speaker) and the analog mic (MIC1 -> ADC, see es8311_mic_begin). MCLK comes from the MCLK pin at 256 x sample rate,
// which is what the ESP32 legacy I2S driver outputs. Register setup follows Espressif's es8311 driver
// (as used in the vendor's Example_16_music).

bool es8311_begin(uint32_t sample_rate);         // reset, clocks, 16-bit I2S, DAC powered up
bool es8311_set_sample_rate(uint32_t sample_rate);  // 8k, 11.025k, 12k, 16k, 22.05k, 24k, 32k, 44.1k, 48k
bool es8311_set_volume_db(float db);             // DAC digital volume: -95.5 .. +32 dB in 0.5 dB steps (0 dB = unity)
bool es8311_mute(bool mute);
bool es8311_mic_begin(int pga_db, float adc_db);  // analog mic: PGA 0-30 dB (3 dB steps), ADC volume like the DAC's
