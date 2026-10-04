#include "managers/audio_manager.h"
#include <Audio.h>
#include <LittleFS.h>
#include <math.h>
#include "drivers/es8311.h"
#include "managers/storage_manager.h"

AudioManager audio_manager;

static const char *FALLBACK_PATH = "/beep.wav";  // on LittleFS, written at boot if missing

struct Command {
  enum Type : uint8_t { PLAY, STOP } type;
  bool repeat;
  bool from_sd;
  char path[80];
};

// Owned by the audio task
static Audio *s_audio;
static QueueHandle_t s_queue;
static Command s_current;               // what is playing (for repeats)
// Shared with loop() / the storage mount task
static volatile bool s_active;          // a sound is playing (or repeating)
static volatile bool s_from_sd;
static volatile bool s_sd_closing;      // storage is about to unmount the card
static volatile uint32_t s_sample_rate;

static bool start(const Command &c) {
  fs::FS &fs = c.from_sd ? storage_manager.fs() : (fs::FS &)LittleFS;
  return s_audio->connecttoFS(fs, c.path);
}

static void start_fallback(bool repeat) {
  s_current.type = Command::PLAY;
  s_current.repeat = repeat;
  s_current.from_sd = false;
  strlcpy(s_current.path, FALLBACK_PATH, sizeof(s_current.path));
  s_from_sd = false;
  s_active = start(s_current);
}

static void audio_task(void *) {
  for (;;) {
    Command c;
    while (xQueueReceive(s_queue, &c, 0) == pdTRUE) {
      s_audio->stopSong();
      if (c.type == Command::STOP) {
        s_active = false;
        continue;
      }
      s_current = c;
      s_from_sd = c.from_sd;
      s_active = start(c);
      if (!s_active && c.from_sd) start_fallback(c.repeat);  // unreadable file: never stay silent
    }

    // Card about to go: close the file now; a repeating sound carries on with the built-in beep
    if (s_sd_closing) {
      if (s_active && s_from_sd) {
        s_audio->stopSong();
        if (s_current.repeat) start_fallback(true);
        else s_active = false;
      }
      s_sd_closing = false;
    }

    if (s_active) {
      s_audio->loop();
      if (!s_audio->isRunning()) {
        if (!s_current.repeat) {
          s_active = false;
        } else {
          s_active = start(s_current);  // loop: start the file again
          if (!s_active && s_current.from_sd) start_fallback(true);
        }
      }
      s_sample_rate = s_audio->getSampleRate();
    }
    vTaskDelay(1);
  }
}

// Called from the storage mount task before it unmounts the card
static void before_unmount() {
  s_sd_closing = true;  // the audio task closes the file between reads (a read on a pulled card can take seconds)
  for (uint32_t t = 0; t < SD_UNMOUNT_WAIT_MS && s_sd_closing; t += 10) vTaskDelay(pdMS_TO_TICKS(10));
}

// 1 s of 4 short beeps (22.05 kHz, 16-bit mono WAV): the sound when there's no SD card or file
static void write_fallback_beep() {
  if (LittleFS.exists(FALLBACK_PATH)) return;
  const uint32_t rate = 22050, samples = rate;
  File f = LittleFS.open(FALLBACK_PATH, "w");
  if (!f) return;
  auto u32 = [&](uint32_t v) { f.write((const uint8_t *)&v, 4); };
  auto u16 = [&](uint16_t v) { f.write((const uint8_t *)&v, 2); };
  f.write((const uint8_t *)"RIFF", 4); u32(36 + samples * 2);
  f.write((const uint8_t *)"WAVEfmt ", 8); u32(16); u16(1); u16(1); u32(rate); u32(rate * 2); u16(2); u16(16);
  f.write((const uint8_t *)"data", 4); u32(samples * 2);
  int16_t buf[256];
  for (uint32_t i = 0; i < samples; i += 256) {
    for (uint32_t k = 0; k < 256; k++) {
      const uint32_t n = i + k;
      const float t = (float)n / rate;
      const float in_beep = fmodf(t, 0.13f);
      const bool on = t < 0.52f && in_beep < 0.07f;
      const float env = on ? fminf(1.0f, fminf(in_beep, 0.07f - in_beep) / 0.004f) : 0;
      buf[k] = (int16_t)(sinf(2 * PI * 2400 * t) * env * 26000);
    }
    f.write((const uint8_t *)buf, sizeof(buf));
  }
  f.close();
}

void AudioManager::begin() {
  pinMode(AUDIO_EN_PIN, OUTPUT);
  digitalWrite(AUDIO_EN_PIN, AUDIO_EN_ACTIVE_LOW ? HIGH : LOW);  // amp off until something plays

  if (!es8311_begin(22050)) Serial.println("ES8311 audio codec init failed");
  es8311_mute(true);
  if (LittleFS.begin(true)) write_fallback_beep();
  else Serial.println("LittleFS mount failed (no fallback beep)");

  s_audio = new Audio();
  // Playback owns I2S0 exactly as the library sets it up (output only). If you add the mic, record on I2S1 as a
  // slave on the same clock pins: running I2S0 in duplex broke playback and corrupted memory (see board/board.md).
  s_audio->setPinout(I2S_PIN_SCK, I2S_PIN_LRC, I2S_PIN_DO, I2S_PIN_NO_CHANGE, I2S_PIN_MCK);
  s_audio->setVolume(21);  // library volume at unity; the volume is set on the codec
  s_queue = xQueueCreate(4, sizeof(Command));
  xTaskCreatePinnedToCore(audio_task, "audio", 8192, nullptr, 3, nullptr, 0);
  storage_manager.add_before_unmount(before_unmount);
}

void AudioManager::play(const char *name, bool repeat, uint8_t volume_pct, uint16_t ramp_sec) {
  Command c = {Command::PLAY, repeat, false, ""};
  if (name && name[0]) {
    snprintf(c.path, sizeof(c.path), "/audio/%s", name);
    c.from_sd = storage_manager.exists(c.path);
  }
  if (!c.from_sd) strlcpy(c.path, FALLBACK_PATH, sizeof(c.path));
  target_pct_ = min<uint8_t>(volume_pct, 100);
  ramp_sec_ = ramp_sec;
  start_ms_ = millis();
  s_active = true;  // so loop() turns the amp on right away
  xQueueSend(s_queue, &c, 0);
}

void AudioManager::stop() {
  Command c = {Command::STOP, false, false, ""};
  xQueueSend(s_queue, &c, 0);
  s_active = false;
}

void AudioManager::set_volume(uint8_t pct) { target_pct_ = min<uint8_t>(pct, 100); }

bool AudioManager::playing() const { return s_active; }

bool AudioManager::list_sounds(std::vector<String> &names) {
  std::vector<String> all;
  names.clear();
  if (!storage_manager.list("/audio", all)) return false;
  for (const String &n : all) {
    String lower = n;
    lower.toLowerCase();
    if (lower.endsWith(".wav") || lower.endsWith(".mp3")) names.push_back(n);
  }
  return true;
}

// Amp, codec volume (with ramp) and codec sample rate
void AudioManager::update_output() {
  static bool amp_on = false;
  static int volume_set = -1;
  static uint32_t rate_set = 22050;

  const bool active = s_active;
  if (active != amp_on) {
    amp_on = active;
    es8311_mute(!active);
    digitalWrite(AUDIO_EN_PIN, active == AUDIO_EN_ACTIVE_LOW ? LOW : HIGH);
    volume_set = -1;
  }
  if (!active) return;

  // The codec's clock dividers must follow each file's sample rate
  const uint32_t rate = s_sample_rate;
  if (rate && rate != rate_set && es8311_set_sample_rate(rate)) rate_set = rate;

  int pct = target_pct_;
  if (ramp_sec_ && pct > AUDIO_RAMP_START_PCT) {
    const float t = (millis() - start_ms_) / (ramp_sec_ * 1000.0f);
    if (t < 1) pct = AUDIO_RAMP_START_PCT + (int)((pct - AUDIO_RAMP_START_PCT) * t);
  }
  if (pct != volume_set) {
    volume_set = pct;
    // % -> dB: 100 % = AUDIO_MAX_DB, each % below is AUDIO_VOLUME_RANGE_DB / 100 quieter, 0 % = off
    es8311_set_volume_db(pct == 0 ? -95.5f : AUDIO_MAX_DB - (100 - pct) * AUDIO_VOLUME_RANGE_DB / 100.0f);
  }
}

void AudioManager::loop() { update_output(); }
