#include "managers/mic_manager.h"
#include <Arduino.h>
#include <driver/i2s.h>
#include <esp_rom_gpio.h>
#include <soc/gpio_sig_map.h>
#include <soc/io_mux_reg.h>
#include <math.h>
#include "app_config.h"
#include "drivers/es8311.h"

MicManager mic_manager;

static volatile float s_level = -120;  // peak-hold meter, dBFS
static volatile MicManager::SampleHandler s_handler;

// The mic on I2S1, receive-only slave on the clock pins I2S0 (playback, the audio library) drives. Only the clock
// signals are fed in (the pins stay I2S0 outputs); I2S_PIN_DI is the data in. Playback's own setup is never touched.
// Running I2S0 in duplex (TX + RX) instead broke playback and corrupted memory on this board (see board/board.md).
static bool mic_i2s_begin() {
  i2s_config_t c = {};
  c.mode = (i2s_mode_t)(I2S_MODE_SLAVE | I2S_MODE_RX);
  c.sample_rate = 22050;  // follows I2S0's clock in slave mode; used only for internal sizing
  c.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  c.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  c.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  c.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  c.dma_buf_count = MIC_I2S_DMA_COUNT;
  c.dma_buf_len = MIC_I2S_DMA_LEN;
  c.use_apll = false;
  if (i2s_driver_install(I2S_NUM_1, &c, 0, nullptr) != ESP_OK) return false;
  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_PIN_NO_CHANGE;
  pins.ws_io_num = I2S_PIN_NO_CHANGE;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = I2S_PIN_DI;
  if (i2s_set_pin(I2S_NUM_1, &pins) != ESP_OK) return false;
  PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[I2S_PIN_SCK]);
  PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[I2S_PIN_LRC]);
  esp_rom_gpio_connect_in_signal(I2S_PIN_SCK, I2S1I_BCK_IN_IDX, false);
  esp_rom_gpio_connect_in_signal(I2S_PIN_LRC, I2S1I_WS_IN_IDX, false);
  return true;
}

static void mic_task(void *) {
  const int FRAMES = 256;
  static int16_t raw[FRAMES * 2];  // interleaved slot 0 / slot 1
  static int16_t mono[FRAMES];
  uint32_t last_ms = millis();
  for (;;) {
    size_t got = 0;
    i2s_read(I2S_NUM_1, raw, sizeof(raw), &got, portMAX_DELAY);
    const size_t frames = got / 4;
    if (!frames) continue;

    int64_t sum = 0;
    for (size_t i = 0; i < frames; i++) {
      mono[i] = raw[2 * i + MIC_SLOT];
      sum += (int32_t)mono[i] * mono[i];
    }
    const double ms = (double)sum / frames;
    const float db = ms > 0 ? 10 * log10(ms / (32768.0 * 32768.0)) : -120;

    // Peak hold: jump up at once, fall at MIC_METER_DECAY_DB_S
    const uint32_t now = millis();
    const float fallen = s_level - MIC_METER_DECAY_DB_S * (now - last_ms) / 1000.0f;
    last_ms = now;
    s_level = fmaxf(db, fmaxf(fallen, -120.0f));

    const MicManager::SampleHandler handler = s_handler;
    if (handler) {
      float rate = i2s_get_clk(I2S_NUM_0);  // playback's rate drives the clock the mic is sampled on
      handler(mono, frames, rate > 0 ? (uint32_t)rate : 22050);
    }
  }
}

bool MicManager::begin() {
  if (!es8311_mic_begin(MIC_PGA_DB, MIC_ADC_DB)) {
    Serial.println("ES8311 mic setup failed");
    return false;
  }
  if (!mic_i2s_begin()) {
    Serial.println("Mic I2S1 setup failed");
    return false;
  }
  xTaskCreatePinnedToCore(mic_task, "mic", 4096, nullptr, 2, nullptr, 0);
  return true;
}

float MicManager::level_dbfs() const { return s_level; }

int MicManager::level_pct() const {
  const float pct = (s_level - MIC_METER_FLOOR_DBFS) * 100.0f / -MIC_METER_FLOOR_DBFS;
  return constrain((int)lroundf(pct), 0, 100);
}

void MicManager::set_sample_handler(SampleHandler handler) { s_handler = handler; }
