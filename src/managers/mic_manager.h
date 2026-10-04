#pragma once
#include <stddef.h>
#include <stdint.h>

// Microphone: LMA2718B381 analog mic -> ES8311 ADC -> I2S1 (receive-only slave) -> ESP32.
// I2S1 takes its bit/word clocks from the pins that playback's I2S0 drives (the audio library), so the mic
// only works after audio_manager.begin(). Its sample rate follows whatever playback last set (22.05 kHz at
// boot, the file's rate after a sound). A background task reads it and keeps a meter level.
class MicManager {
 public:
  bool begin();  // after audio_manager.begin()

  float level_dbfs() const;  // peak-hold meter, -120 .. 0 dBFS
  int level_pct() const;     // the same, MIC_METER_FLOOR_DBFS .. 0 dBFS -> 0..100

  // Optional: receive raw mono 16-bit samples (from the mic task) to record or analyse them.
  // sample_rate is the current I2S clock rate.
  using SampleHandler = void (*)(const int16_t *samples, size_t count, uint32_t sample_rate);
  void set_sample_handler(SampleHandler handler);
};

extern MicManager mic_manager;
