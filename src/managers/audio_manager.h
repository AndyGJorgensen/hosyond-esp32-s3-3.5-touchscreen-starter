#pragma once
#include <Arduino.h>
#include <vector>
#include "app_config.h"

// Speaker output: ES8311 codec + SC8002B amp, MP3/WAV files from /audio on the SD card.
// A background task runs the decoder (ESP32-audioI2S); play()/stop() queue commands to it. loop()
// drives the amp enable, the codec's sample rate and the volume (with an optional ramp).
// A missing card or file plays a beep kept in internal flash (LittleFS) instead, so play() always makes a sound.
class AudioManager {
 public:
  void begin();  // after Wire.begin() (codec on I2C) and storage_manager.begin()
  void loop();

  // name: a file in /audio, e.g. "chime.mp3" ("" = the built-in beep). volume 0..100.
  // ramp_sec: rise from AUDIO_RAMP_START_PCT to volume over this many seconds (0 = full volume at once).
  void play(const char *name, bool repeat = false, uint8_t volume_pct = AUDIO_DEFAULT_VOLUME_PCT,
            uint16_t ramp_sec = 0);
  void stop();
  void set_volume(uint8_t pct);                  // volume of what is playing now
  bool playing() const;
  bool list_sounds(std::vector<String> &names);  // .wav / .mp3 files in /audio

 private:
  void update_output();

  uint8_t target_pct_ = 0;
  uint16_t ramp_sec_ = 0;
  uint32_t start_ms_ = 0;
};

extern AudioManager audio_manager;
