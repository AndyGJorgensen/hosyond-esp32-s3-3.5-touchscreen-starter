#pragma once
// User-adjustable settings and board pin map (from the vendor schematic, see board/board.md).

// ---- Display: LCDwiki ES3C35P, HMX035CTFT panel, Sitronix ST77922 TDDI over QSPI ----
#define LCD_NATIVE_W   320        // panel width in its native (portrait) orientation
#define LCD_NATIVE_H   480        // panel height in its native (portrait) orientation
// Screen orientation. The ST77922 has no axis-swap bit, so LVGL rotates in software.
// 0 = portrait 320x480, 1 = landscape upside down, 2 = portrait upside down, 3 = landscape 480x320 (EEZ project)
#define LCD_ROTATION   3
#define LCD_MADCTL     0x00       // MADCTL (0x36): 0x40 mirrors X, 0x80 mirrors Y, 0x08 swaps red/blue
#define LCD_SPI_HZ     80000000   // QSPI clock, as the vendor demo (lower to 40000000 if the image is corrupted)
#define LCD_CHUNK_BYTES 32768     // max bytes per QSPI transaction (internal DMA bounce buffer)

#define LCD_PIN_BL    41   // backlight, active HIGH (drives Q4 BSS138, so PWM dims it)
#define BACKLIGHT_DEFAULT_PCT 100  // brightness at boot (Main screen "Light" slider)
#define BACKLIGHT_MIN_PCT     5    // lowest brightness the slider allows (0 would leave the screen dark)
#define BACKLIGHT_PWM_CHANNEL 7    // LEDC channel for the backlight
#define BACKLIGHT_PWM_FREQ    5000 // Hz
#define LCD_PIN_CS    10
#define LCD_PIN_SCK   12
#define LCD_PIN_D0    11   // LCD_SDA0
#define LCD_PIN_D1    13   // LCD_SDA1
#define LCD_PIN_D2    14   // LCD_SDA2
#define LCD_PIN_D3    9    // LCD_SDA3
#define LCD_PIN_TE    42   // tearing effect output
// LCD reset is tied to the ESP32 chip reset (CHIP_PU, no GPIO). See board/board.md.

// ---- Shared I2C bus: ST77922 touch (0x55) + ES8311 codec (0x18) + header P4. Started once in main.cpp ----
#define I2C_PIN_SDA   38
#define I2C_PIN_SCL   39
#define I2C_BUS_HZ    400000
#define TP_PIN_INT    47
#define TP_PIN_RST    48

// ---- Touch (reported in native portrait coordinates; LVGL applies LCD_ROTATION) ----
#define TOUCH_I2C_ADDR  0x55
#define TOUCH_MIRROR_X  0    // set to 1 if touches register mirrored left/right (in portrait)
#define TOUCH_MIRROR_Y  0    // set to 1 if touches register mirrored top/bottom (in portrait)

// ---- WiFi ----
#define WIFI_HOSTNAME             "esp32-display"  // DHCP/mDNS host name shown on the WiFi screen
#define WIFI_SAVED_MAX            3                // saved networks (matches the WifiSaved screen rows)
#define WIFI_CONNECT_TIMEOUT_MS   15000            // give up on a connection attempt after this long
#define WIFI_SCAN_TIMEOUT_MS      15000            // give up on a scan after this long (a full scan takes ~6.4 s)
#define WIFI_AUTOCONNECT_AT_BOOT  1                // 1 = try saved networks (most recent first) at boot

// ---- OTA (ArduinoOTA over WiFi; host name is WIFI_HOSTNAME) ----
#define OTA_ENABLED_DEFAULT       1                // OTA switch state at boot (1: OTA is the default upload path)
#define OTA_PORT                  3232             // also in platformio.ini [env:ota] upload_flags --port
#define OTA_PASSWORD              "esp32ota"       // also in platformio.ini [env:ota] upload_flags --auth. Change both

// ---- Audio: ES8311 codec (I2C 0x18 on the shared bus) + SC8002B speaker amp + analog mic ----
// Sounds are MP3/WAV files in /audio on the SD card. With no card or file, a built-in beep plays instead.
#define AUDIO_DEFAULT_VOLUME_PCT 70     // volume used by audio_manager.play() when none is given (0..100)
#define AUDIO_MAX_DB           6        // codec gain at 100 % (0 dB = unity; higher = louder, may distort; ~16 max)
#define AUDIO_VOLUME_RANGE_DB  40       // 1 % is this much quieter than 100 %
#define AUDIO_RAMP_START_PCT   20       // a ramped play() starts at this volume
#define AUDIO_EN_ACTIVE_LOW    1        // SC8002B amp enable (AUDIO_EN_PIN) is on when LOW (vendor Example_16_music)
#define AUDIO_TEST_SOUND       "chime.wav"  // played by the Main screen's "Play SD sound" button (sd_card/audio/)
// Mic (LMA2718B381 -> ES8311 ADC -> I2S1 slave on the playback clock pins, data in on I2S_PIN_DI)
#define MIC_PGA_DB             24       // ES8311 mic preamp gain, 0-30 dB in 3 dB steps (higher = more sensitive)
#define MIC_ADC_DB             0        // ES8311 ADC digital volume, -95.5 .. +32 dB (0 = unity)
#define MIC_SLOT               0        // I2S slot carrying the mic (measured: 0)
#define MIC_METER_FLOOR_DBFS   -70      // mic level shown as 0 % on the meter (0 dBFS = 100 %)
#define MIC_METER_DECAY_DB_S   30       // meter falls this fast after a peak (dB per second)
#define MIC_I2S_DMA_COUNT      4        // mic I2S1 DMA buffers (internal RAM: count x len x 4 bytes)
#define MIC_I2S_DMA_LEN        256
#define ES8311_I2C_ADDR 0x18
#define I2S_PIN_MCK   17   // I2S_MCK -> ES8311 MCLK
#define I2S_PIN_SCK   18   // I2S_SCK -> ES8311 SCLK (bit clock)
#define I2S_PIN_LRC   21   // I2S_LRC -> ES8311 LRCK (word select)
#define I2S_PIN_DO    15   // ESP32 data out -> ES8311 (speaker path). Net "I2S_DI" (named from the codec side);
                           // confirmed by the vendor Example_16_music (I2S_DOUT 15) and on hardware
#define I2S_PIN_DI    16   // ES8311 -> ESP32 data in (mic path). Net "I2S_DO"
#define AUDIO_EN_PIN  1    // SC8002B amplifier enable, active LOW (speaker on JP3)

// ---- microSD card (4-bit SDMMC, 10K pull-ups on board, no card-detect pin) ----
#define SD_FREQ_KHZ               20000   // SDMMC clock; 20000 = default speed (reliable), 40000 = high speed
#define SD_RETRY_MS               5000    // look for a card this often while none is mounted
#define SD_CHECK_MS               1000    // check the mounted card is still there this often (CMD13)
#define SD_MAX_OPEN_FILES         8       // files open at once (each slot is ~4.2 KB of PSRAM)
#define SD_UNMOUNT_WAIT_MS        5000    // before unmounting, wait this long for open files (audio) to close
#define SD_STATUS_REFRESH_MS      300000  // recompute free space this often
#define SD_FORMAT_IF_MOUNT_FAILED 0       // 1 = format an unreadable card (ERASES it). Leave 0
#define SD_FOLDERS                {"/audio"}  // created on every mount
#define SD_PIN_CLK    5
#define SD_PIN_CMD    4
#define SD_PIN_D0     6
#define SD_PIN_D1     7
#define SD_PIN_D2     2
#define SD_PIN_D3     3

// ---- RGB LED ----
#define RGB_LED_PIN   40   // WS2812B data (single LED, powered from 5 V), driven with the core's neopixelWrite()
#define RGB_LED_LEVEL 40   // brightness of each colour channel, 0-255 (255 is very bright)

// ---- Battery: 1-cell LiPo on JP1, TP4054 charger. Only the cell voltage reaches the ESP32 ----
#define BAT_ADC_PIN   8    // battery voltage through 100K/100K divider
#define BAT_ADC_RATIO 2.0f // multiply the ADC pin voltage by this to get battery voltage
#define BAT_CAL_OFFSET_V     0.02f  // added to every reading (this board's ADC read ~0.02 V low vs a multimeter)
#define BAT_SAMPLE_MS        1000   // voltage sample interval
#define BAT_AUDIO_SETTLE_MS  3000   // ignore readings while a sound plays and this long after (speaker current)

// ---- Buttons ----
#define BOOT_BTN_PIN  0    // KEY2 (BOOT), active LOW, 10K pull-up; KEY1 is RESET (CHIP_PU, no GPIO)

// ---- Headers ----
#define EXT_IO45_PIN  45   // header P3 (IO45), free GPIO (strapping pin: leave floating/low at boot)
#define EXT_IO46_PIN  46   // header P3 (IO46), free GPIO (strapping pin: leave floating/low at boot)
#define UART0_TX_PIN  43   // header P2 (TXD0), via 100R
#define UART0_RX_PIN  44   // header P2 (RXD0), via 100R
// Header P4 (I2C) is the shared bus on I2C_PIN_SDA / I2C_PIN_SCL.

// ---- Reserved by hardware (do not use) ----
// GPIO19/20: native USB D-/D+ (USB-C, Serial when ARDUINO_USB_CDC_ON_BOOT=1)
// GPIO26-32: external flash (FSPI); GPIO33-37: in-package octal PSRAM
