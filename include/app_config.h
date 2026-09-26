#pragma once
// User-adjustable settings and board pin map (from ESP32-S3原理图.pdf, see board.md).

// ---- Display: LCDwiki ES3C35P, HMX035CTFT panel, Sitronix ST77922 TDDI over QSPI ----
#define LCD_NATIVE_W   320        // panel width in its native (portrait) orientation
#define LCD_NATIVE_H   480        // panel height in its native (portrait) orientation
// Screen orientation. The ST77922 has no axis-swap bit, so LVGL rotates in software.
// 0 = portrait 320x480, 1 = landscape upside down, 2 = portrait upside down, 3 = landscape 480x320 (EEZ project)
#define LCD_ROTATION   3
#define LCD_MADCTL     0x00       // MADCTL (0x36): 0x40 mirrors X, 0x80 mirrors Y, 0x08 swaps red/blue
#define LCD_SPI_HZ     80000000   // QSPI clock, as the vendor demo (lower to 40000000 if the image is corrupted)
#define LCD_CHUNK_BYTES 32768     // max bytes per QSPI transaction (internal DMA bounce buffer)

#define LCD_PIN_BL    41   // backlight enable, active HIGH (drives Q4 BSS138)
#define LCD_PIN_CS    10
#define LCD_PIN_SCK   12
#define LCD_PIN_D0    11   // LCD_SDA0
#define LCD_PIN_D1    13   // LCD_SDA1
#define LCD_PIN_D2    14   // LCD_SDA2
#define LCD_PIN_D3    9    // LCD_SDA3
#define LCD_PIN_TE    42   // tearing effect output
// LCD reset is tied to the ESP32 chip reset (CHIP_PU, no GPIO). See board.md.

// ---- Shared I2C bus: ST77922 touch (0x55) + ES8311 codec (0x18) ----
#define I2C_PIN_SDA   38
#define I2C_PIN_SCL   39
#define TP_PIN_INT    47
#define TP_PIN_RST    48

// ---- Touch (reported in native portrait coordinates; LVGL applies LCD_ROTATION) ----
#define TOUCH_I2C_ADDR  0x55
#define TOUCH_I2C_HZ    400000
#define TOUCH_MIRROR_X  0    // set to 1 if touches register mirrored left/right (in portrait)
#define TOUCH_MIRROR_Y  0    // set to 1 if touches register mirrored top/bottom (in portrait)

// ---- Other peripherals ----
#define AUDIO_EN_PIN  1    // SC8002B amplifier shutdown control
#define RGB_LED_PIN   40   // WS2812B
#define BAT_ADC_PIN   8
