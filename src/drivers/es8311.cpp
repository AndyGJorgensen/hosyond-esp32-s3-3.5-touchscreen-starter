#include "drivers/es8311.h"
#include <Arduino.h>
#include <Wire.h>
#include "app_config.h"

#define REG_RESET   0x00
#define REG_CLK01   0x01  // clock enables, MCLK source
#define REG_CLK02   0x02  // pre-divider / multiplier
#define REG_CLK03   0x03  // ADC fs mode + OSR
#define REG_CLK04   0x04  // DAC OSR
#define REG_CLK05   0x05  // ADC / DAC clock dividers
#define REG_CLK06   0x06  // BCLK divider / inversion
#define REG_CLK07   0x07  // LRCK divider high
#define REG_CLK08   0x08  // LRCK divider low
#define REG_SDPIN   0x09  // DAC serial port format
#define REG_SDPOUT  0x0A  // ADC serial port format
#define REG_SYS0D   0x0D  // analog power
#define REG_SYS0E   0x0E  // PGA / ADC modulator power
#define REG_SYS12   0x12  // DAC power
#define REG_SYS13   0x13  // output driver
#define REG_SYS14   0x14  // mic input select + PGA gain
#define REG_ADC17   0x17  // ADC volume
#define REG_ADC1C   0x1C  // ADC equalizer / DC offset
#define REG_DAC31   0x31  // DAC mute
#define REG_DAC32   0x32  // DAC volume
#define REG_DAC37   0x37  // DAC ramp / equalizer bypass

static bool write_reg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(ES8311_I2C_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool read_reg(uint8_t reg, uint8_t *value) {
  Wire.beginTransmission(ES8311_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)ES8311_I2C_ADDR, (uint8_t)1) != 1) return false;
  *value = Wire.read();
  return true;
}

// With MCLK = 256 x fs, Espressif's coefficient table gives the same dividers for every supported rate
// (pre_div 1, adc/dac div 1, single speed, LRCK 256, BCLK /4, OSR 0x10), except 8 kHz which doubles MCLK.
bool es8311_set_sample_rate(uint32_t fs) {
  static const uint32_t RATES[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};
  bool ok_rate = false;
  for (uint32_t r : RATES) ok_rate |= r == fs;
  if (!ok_rate) return false;

  const uint8_t pre_multi = fs == 8000 ? 1 : 0;  // x2 for 8 kHz, else x1
  uint8_t v;
  if (!read_reg(REG_CLK02, &v)) return false;
  v = (v & 0x07) | (0 << 5) | (pre_multi << 3);  // pre_div 1 -> field 0
  bool ok = write_reg(REG_CLK02, v);
  ok &= write_reg(REG_CLK03, 0x10);              // single speed, ADC OSR 0x10
  ok &= write_reg(REG_CLK04, 0x10);              // DAC OSR 0x10
  ok &= write_reg(REG_CLK05, 0x00);              // ADC / DAC div 1
  if (!read_reg(REG_CLK06, &v)) return false;
  ok &= write_reg(REG_CLK06, (v & 0xE0) | (4 - 1));  // BCLK = MCLK / 4
  if (!read_reg(REG_CLK07, &v)) return false;
  ok &= write_reg(REG_CLK07, v & 0xC0);          // LRCK divider high = 0x00
  ok &= write_reg(REG_CLK08, 0xFF);              // LRCK divider low = 0xFF (256)
  return ok;
}

bool es8311_begin(uint32_t sample_rate) {
  bool ok = write_reg(REG_RESET, 0x1F);  // reset
  delay(20);
  ok &= write_reg(REG_RESET, 0x00);
  ok &= write_reg(REG_RESET, 0x80);      // power on
  ok &= write_reg(REG_CLK01, 0x3F);      // all clocks on, MCLK from the MCLK pin, not inverted
  uint8_t v;
  if (!read_reg(REG_CLK06, &v)) return false;
  ok &= write_reg(REG_CLK06, v & ~0x20); // BCLK not inverted
  ok &= es8311_set_sample_rate(sample_rate);
  if (!read_reg(REG_RESET, &v)) return false;
  ok &= write_reg(REG_RESET, v & 0xBF);  // serial port in slave mode
  ok &= write_reg(REG_SDPIN, 3 << 2);    // DAC input: I2S, 16-bit
  ok &= write_reg(REG_SDPOUT, 3 << 2);   // ADC output: I2S, 16-bit
  ok &= write_reg(REG_SYS0D, 0x01);      // power up analog circuitry
  ok &= write_reg(REG_SYS0E, 0x02);      // enable analog PGA / ADC modulator
  ok &= write_reg(REG_SYS12, 0x00);      // power up DAC
  ok &= write_reg(REG_SYS13, 0x10);      // enable output driver
  ok &= write_reg(REG_ADC1C, 0x6A);      // ADC equalizer bypass, cancel DC offset
  ok &= write_reg(REG_DAC37, 0x08);      // bypass DAC equalizer
  return ok;
}

// Register 0x32: 0x00 = -95.5 dB (off), 0xBF = 0 dB, 0xFF = +32 dB, 0.5 dB per step
bool es8311_set_volume_db(float db) {
  int reg = 0xBF + (int)lroundf(db * 2);
  return write_reg(REG_DAC32, (uint8_t)constrain(reg, 0, 0xFF));
}

bool es8311_mute(bool mute) {
  uint8_t v;
  if (!read_reg(REG_DAC31, &v)) return false;
  v = mute ? (v | 0x60) : (v & ~0x60);
  return write_reg(REG_DAC31, v);
}

// Mic path (analog mic on MIC1 -> PGA -> ADC -> ASDOUT, GPIO16). The ADC is already powered and clocked by
// es8311_begin(); this selects the mic input and sets the gains (as Espressif's esp_codec_dev does: reg 0x14 =
// MIC1 select | PGA gain in 3 dB steps, reg 0x17 = ADC volume like the DAC's).
bool es8311_mic_begin(int pga_db, float adc_db) {
  const uint8_t pga = (uint8_t)constrain(pga_db / 3, 0, 10);
  bool ok = write_reg(REG_SYS14, 0x10 | pga);
  ok &= write_reg(REG_ADC17, (uint8_t)constrain(0xBF + (int)lroundf(adc_db * 2), 0, 0xFF));
  return ok;
}
