# Board notes: 3.5" ESP32-S3 all-in-one display board

Source: the vendor schematic `ESP32-S3原理图.pdf` (3.5inch_ESP32-S3_board.SchDoc, 1/30/2026).

The schematic and datasheets are vendor documents and are not in this repo (the ST77922 spec is marked confidential). Get them here:
- **Board page + data package (schematic, datasheets, Arduino demos):** https://www.lcdwiki.com/3.5inch_ESP32-S3_Display
- **Vendor Arduino examples (mirror):** https://github.com/ydedox/st77922. `Example_01_Simple_test` has the panel init table, `Example_08_LVGL_Demos` has the touch driver.
- **ST77922 datasheet (published by Espressif):** https://dl.espressif.com/AE/esp-iot-solution/ST77922_SPEC_V0.1.pdf
- **Espressif ST77922 driver:** https://components.espressif.com/components/espressif/esp_lcd_st77922

Keep local copies in `board/datasheets/`. That folder and all PDFs are git-ignored.

## Core
- MCU: ESP32-S3R8 (bare chip, 8 MB octal PSRAM in-package; GPIO33–37 used by PSRAM, not available).
- External flash on FSPI pins (GPIO27–32). The symbol reads W25X40; the actual size is unconfirmed.
- 40 MHz crystal. PCB antenna (J1 SMD_ANT).
- USB-C goes to **native USB** (GPIO19 D-, GPIO20 D+), not a USB-UART bridge. `Serial` needs `-DARDUINO_USB_CDC_ON_BOOT=1`.
- UART0 (TXD0/RXD0) is on header P2.
- Buttons: KEY1 = RESET (CHIP_PU), KEY2 = BOOT (GPIO0).

## GPIO map
| GPIO | Net | Notes |
|---|---|---|
| 0 | IO0 | BOOT button |
| 1 | AUDIO_EN | SC8002B amp enable, **active LOW** |
| 2 | SD_D2 | microSD (4-bit SDMMC) |
| 3 | SD_D3 | microSD |
| 4 | SD_CMD | microSD |
| 5 | SD_CLK | microSD |
| 6 | SD_D0 | microSD |
| 7 | SD_D1 | microSD |
| 8 | BAT_ADC | battery voltage via 100K/100K divider (×2) |
| 9 | LCD_SDA3 | QSPI D3 |
| 10 | LCD_CS | QSPI CS |
| 11 | LCD_SDA0 | QSPI D0 |
| 12 | LCD_SCK | QSPI CLK |
| 13 | LCD_SDA1 | QSPI D1 |
| 14 | LCD_SDA2 | QSPI D2 |
| 15 | I2S_DI | **ESP32 data out -> ES8311** (speaker). Net named from the codec side; vendor `I2S_DOUT 15` |
| 16 | I2S_DO | ES8311 -> ESP32 data in (mic) |
| 17 | I2S_MCK | ES8311 MCLK |
| 18 | I2S_SCK | ES8311 SCLK |
| 19/20 | USB D-/D+ | native USB |
| 21 | I2S_LRC | ES8311 LRCK |
| 38 | IIC_SDA | shared: touch + ES8311 + header P4 |
| 39 | IIC_SCL | shared: touch + ES8311 + header P4 |
| 40 | RGB_LED | WS2812B data, driven by the core's `neopixelWrite()` (`src/drivers/rgb_led.cpp`) |
| 41 | LCD_BL | backlight, active HIGH (BSS138 low-side on LEDK). PWM dims it (`src/drivers/backlight.cpp`, LEDC) |
| 42 | LCD_TE | panel tearing-effect output |
| 43 | TXD0 | UART0 TX, header P2 (100R) |
| 44 | RXD0 | UART0 RX, header P2 (100R) |
| 45 | IO45 | free, header P3 |
| 46 | IO46 | free, header P3 |
| 47 | CTP_INT | touch interrupt |
| 48 | CTP_RST | touch reset |

## Display
- Panel: HMX035CTFT-001, 3.5", driven over **4-line QSPI** (no D/C pin). The LCD RESET line is tied to the ESP32 chip reset.
- Controller: **Sitronix ST77922** TDDI (display + touch in one chip), per its datasheets. Native 320×480 assumed.
- QSPI framing: opcode `0x02` + 24-bit address `00 <cmd> 00` for commands (1 line); opcode `0x32` + `00 2C 00` for pixels (data on 4 lines). RGB565 is big-endian.
- TFT_eSPI does not support this panel. The project uses its own driver in `src/drivers/lcd_st77922.cpp` (IDF spi_master). The LVGL glue (rotation, 4-px rounding, flush) is in `src/drivers/lvgl_port.cpp`.
- Espressif's `esp_lcd_st77922` default init sequence is for a 532×300 panel. Don't reuse its power/gamma values here.
- Board is the **LCDwiki ES3C35P** (https://www.lcdwiki.com/3.5inch_ESP32-S3_Display). The vendor examples are mirrored at https://github.com/ydedox/st77922. `Example_01_Simple_test/Simple_test.ino` holds the full vendor init table, which the panel needs (it stays black with only SLPOUT/DISPON). Background: https://github.com/espressif/arduino-esp32/issues/12694
- The vendor table ends with INVON (0x21), COLMOD 0x01, MADCTL 0x00, TEON 0x01.
- **Window column start and width must be multiples of 4** (vendor rounds `sx`, `w`; ESPHome uses `draw_rounding: 4`).
- **No MV (axis swap) bit in MADCTL** (bits: D7 MY, D6 MX, D4 ML, D3 RGB, D2 MH). Landscape has to be a software rotation in LVGL.
- **LCD reset is tied to CHIP_PU.** A USB-JTAG reset (esptool hard reset, `esp_restart`, OTA) does not reset the panel. This project's init works from a hard reset, so no power-cycle is needed after flashing. Just be aware that a soft reset can hide init bugs (see the command framing note below).
- **Pixel writes must use opcode `0x32` + address `0x3C` (RAMWRC)**, as the vendor driver does. With `0x2C` (RAMWR) the panel stays black, even though the datasheet suggests either should work. Confirmed on hardware.
- Orientation: `LCD_ROTATION 3` (LVGL 270°) with `LCD_MADCTL 0x00` gives the correct upright landscape. `1` is upside down. The vendor's MADCTL `0x44` flips the landscape image top-to-bottom.
- **Command framing must match the vendor exactly:** set cmd/addr widths per transaction (`spi_transaction_ext_t` + `SPI_TRANS_VARIABLE_CMD|ADDR`) and send params from a RAM buffer. With device-level `command_bits/address_bits` and `SPI_TRANS_USE_TXDATA`, the panel stays black after a hard reset. It only *seemed* to work while a previous vendor init was still latched in the panel.
- Testing tip: a USB soft reset/upload doesn't reset the panel (the old init stays latched). Press the board's RESET button (CHIP_PU) to test a true cold init.
- **Touch: confirmed ST77922 TDDI touch over I2C at 0x55** (the FT6336G datasheet in the vendor package does not apply). Driver: `src/drivers/touch_st77922.cpp`, following the vendor `Example_08_LVGL_Demos/esp_lcd_st77922.c`.
  - Registers are 16-bit big-endian (write 2 address bytes, repeated start, read).
  - Reset: TP_RST (GPIO48) low 100 ms, then high 100 ms. Done after display init, as the vendor does.
  - `0x0000` fw version, `0x0005..0x0009` max X/Y and max touches, `0x0010` status (bit3 = coordinates available), `0x0009` report count, `0x0014` reports (7 bytes each: b0 bit7 valid, x = (b0&0x3F)<<8|b1, y = b2<<8|b3, b4 area, b5 intensity).
  - Coordinates are native portrait 320×480 and match the display with MADCTL 0x00. LVGL 9 rotates pointer input itself (`lv_display_rotate_point`), so the driver passes native coordinates.
  - Polled on every LVGL indev read. INT (GPIO47) is configured as an input but not used yet.
- Flash confirmed 16 MB (esptool: manufacturer 0x5E, device 0x4018).

## Audio (`src/drivers/es8311.cpp`, `src/managers/audio_manager.cpp`, `src/managers/mic_manager.cpp`)
- ES8311 codec (I2C 0x18 + I2S), powered from its own 3.3 V LDO.
- SC8002B speaker amp, enabled by AUDIO_EN (GPIO1), **active LOW** (vendor Example_16_music writes LOW). Speaker on JP3.
- **I2S data direction:** the schematic nets are named from the codec's side. ESP32 audio out is **GPIO15** (net I2S_DI), mic in is **GPIO16** (net I2S_DO). With them swapped the decoder runs but the speaker stays silent.
- I2S: BCLK 18, LRCK 21, MCLK 17. The legacy I2S driver in ESP32-audioI2S 2.3.0 (`esphome/ESP32-audioI2S`, for Arduino core 2.x) outputs MCLK = 256 × fs. `es8311_set_sample_rate()` sets the matching dividers whenever a file's sample rate changes.
- The vendor example uses ESP32-audioI2S 3.0.x, which needs Arduino core 3.x. This project is on core 2.0.17, so it uses the esphome fork instead.
- LMA2718B381 analog mic into the ES8311 MIC1. `es8311_mic_begin()` sets reg 0x14 (MIC1 + PGA gain) and reg 0x17 (ADC volume). The ADC output comes in on GPIO16 in **I2S slot 0** (measured).
- Playback owns **I2S0** exactly as the audio library sets it up (TX master). The mic reads on **I2S1** as an RX **slave** on the same clock pins. Only the BCK / WS input signals are routed from GPIO18 / GPIO21 (the pads stay I2S0 outputs); data comes in on GPIO16. So the mic needs `audio_manager.begin()` first, and its sample rate follows the last played file (22.05 kHz at boot). Read the current rate with `i2s_get_clk(I2S_NUM_0)`.
- **Don't run I2S0 in duplex (TX + RX) with the legacy driver on this board.** In an earlier project on this board, this made playback distorted or silent, and a recorder writing to SD crashed with a corrupted FATFS file object. The I2S1 slave fixed all of it.
- The speaker's current dips the battery reading, so `battery_manager` ignores samples while a sound plays and for `BAT_AUDIO_SETTLE_MS` after.

## Power (`src/managers/battery_manager.cpp`)
- TP4054 Li-ion charger from VBUS; battery connector JP1.
- ME6217C33 3.3 V LDO.
- Q3 (SL2305) power path: on USB the load runs from USB and the battery only charges.
- No battery protection IC, no charge-status pin and no VBUS-sense pin. The only signal is the battery voltage on GPIO8 (1:2 divider). The ADC reads about 0.02 V low (`BAT_CAL_OFFSET_V`).
- `battery_manager` reports only the smoothed voltage. With no cell fitted, the reading is the charger output: above 4.25 V, rippling by about 0.05 V. Charging / on-battery can be inferred from voltage steps (plug / unplug) and the trend over ~15 minutes; this base keeps it simple.
- Telling USB from battery instantly: switch the backlight between full and off and measure the sag. On USB it's about 1 mV, on battery about 8 mV (measured; a 4 mV threshold separates them). Not in this base because it flashes the screen.

## microSD card (`src/managers/storage_manager.cpp`; 4-bit SDMMC: CLK 5, CMD 4, D0 6, D1 7, D2 2, D3 3)
- 10K pull-ups on the board. There's **no card-detect pin**, so a card can only be found by trying to mount it.
- Mounted with the IDF `esp_vfs_fat_sdmmc_mount()` (4-bit slot, pins through the S3 GPIO matrix) instead of the Arduino `SD_MMC` class, because `SD_MMC` hides the `sdmmc_card_t` handle needed for presence checks. The clock is `SD_FREQ_KHZ` (20 MHz by default).
- Mounting with no card can block while the card init times out, so the mount runs in its own FreeRTOS task (core 0) and retries every `SD_RETRY_MS`. `loop()` and the UI never wait on it.
- Removal: the task sends CMD13 (`sdmmc_get_status`) every `SD_CHECK_MS`. A pulled card stops answering, so the task unmounts it (under a mutex shared with file operations) and waits for a card again. A failed read/write triggers the same unmount.
- Free space (FatFs `f_getfree`) can scan the whole FAT on a large card, so it's only computed in the mount task.
- **FatFs keeps its open-file table in PSRAM** in this SDK (`CONFIG_FATFS_ALLOC_PREFER_EXTRAM`), about 4.2 KB per `SD_MAX_OPEN_FILES` slot. Moving it to internal RAM starved WiFi.
- **File descriptor 0 is an ordinary file here.** The console is native USB (HWCDC), not a VFS device, so the first file opened gets fd 0. Any library that calls `close(0)` by mistake closes a real SD file. The Arduino 2.0.17 `WiFiClientSecure` does exactly that after `stop_ssl_socket()`. If you add HTTPS, use a small subclass of `WiFiClientSecure` whose `stop()` override calls the base `stop()` and then sets `sslclient->socket = -1`, and whose destructor calls `stop()`. Otherwise the base destructor's `stop()` closes whichever file holds fd 0.
- Sample files: copy `sd_card/` from this repo to the card root (`/audio/chime.wav` is the Main screen's test sound).

## Memory (~320 KB internal RAM + 8 MB PSRAM)
- The Arduino core puts every `malloc` under 4 KB in internal RAM (`SPIRAM_MALLOC_ALWAYSINTERNAL`). LVGL objects are small, so with the C library allocator all widgets landed in internal RAM and starved WiFi/TLS (`esp-sha: Failed to allocate buf memory`).
- Fix: `LV_STDLIB_CUSTOM` + `src/drivers/lvgl_mem_psram.cpp` puts LVGL's heap in PSRAM, falling back to internal only if PSRAM is full.
- If you add HTTPS, also make mbedTLS allocate from PSRAM (register a PSRAM `calloc`/`free` pair with `mbedtls_platform_set_calloc_free()` before the first connection). This Arduino build defaults it to internal RAM; moving it raised the internal minimum free from 24 KB to 76 KB with WiFi, audio and SD running.

## WiFi
- A full async scan (`WiFi.scanNetworks(true)`) takes about **6.4 s** on this board. That's measured by timestamping `ARDUINO_EVENT_WIFI_SCAN_DONE`.
- Arduino core 2.0.17 gives up on an async scan after `max_ms_per_chan × 20` = 6 s, so `scanComplete()` returns `WIFI_SCAN_FAILED` about 0.4 s before the results arrive. `src/managers/wifi_manager.cpp` ignores that early FAILED and waits up to `WIFI_SCAN_TIMEOUT_MS`. When SCAN_DONE arrives the core resets its timer and `scanComplete()` returns the real count.
