# Board notes: 3.5" ESP32-S3 all-in-one display board

Source: `ESP32-S3原理图.pdf` (3.5inch_ESP32-S3_board.SchDoc, 1/30/2026).

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
| 1 | AUDIO_EN | SC8002B amp shutdown |
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
| 15 | I2S_DI | ES8311 ASDOUT |
| 16 | I2S_DO | ES8311 DSDIN |
| 17 | I2S_MCK | ES8311 MCLK |
| 18 | I2S_SCK | ES8311 SCLK |
| 19/20 | USB D-/D+ | native USB |
| 21 | I2S_LRC | ES8311 LRCK |
| 38 | IIC_SDA | shared: touch + ES8311 + header P4 |
| 39 | IIC_SCL | shared: touch + ES8311 + header P4 |
| 40 | RGB_LED | WS2812B data |
| 41 | LCD_BL | backlight, active HIGH (BSS138 low-side on LEDK) |
| 42 | LCD_TE | panel tearing-effect output |
| 43 | TXD0 | UART0 TX, header P2 (100R) |
| 44 | RXD0 | UART0 RX, header P2 (100R) |
| 45 | IO45 | free, header P3 |
| 46 | IO46 | free, header P3 |
| 47 | CTP_INT | touch interrupt |
| 48 | CTP_RST | touch reset |

## Display
- Panel: HMX035CTFT-001, 3.5", driven over **4-line QSPI** (no D/C pin). The LCD RESET line is tied to the ESP32 chip reset.
- Controller: **Sitronix ST77922** TDDI (display + touch in one chip), per `datasheets/`. Native 320×480 assumed.
- QSPI framing: opcode `0x02` + 24-bit address `00 <cmd> 00` for commands (1 line); opcode `0x32` + `00 2C 00` for pixels (data on 4 lines). RGB565 is big-endian.
- TFT_eSPI does not support this panel. The project uses its own driver in `src/lcd_st77922.cpp` (IDF spi_master). The LVGL glue (rotation, 4-px rounding, flush) is in `src/lvgl_port.cpp`.
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
- **Touch: confirmed ST77922 TDDI touch over I2C at 0x55** (the FT6336G datasheet in `datasheets/` does not apply). Driver: `src/touch_st77922.cpp`, following the vendor `Example_08_LVGL_Demos/esp_lcd_st77922.c`.
  - Registers are 16-bit big-endian (write 2 address bytes, repeated start, read).
  - Reset: TP_RST (GPIO48) low 100 ms, then high 100 ms. Done after display init, as the vendor does.
  - `0x0000` fw version, `0x0005..0x0009` max X/Y and max touches, `0x0010` status (bit3 = coordinates available), `0x0009` report count, `0x0014` reports (7 bytes each: b0 bit7 valid, x = (b0&0x3F)<<8|b1, y = b2<<8|b3, b4 area, b5 intensity).
  - Coordinates are native portrait 320×480 and match the display with MADCTL 0x00. LVGL 9 rotates pointer input itself (`lv_display_rotate_point`), so the driver passes native coordinates.
  - Polled on every LVGL indev read. INT (GPIO47) is configured as an input but not used yet.
- Flash confirmed 16 MB (esptool: manufacturer 0x5E, device 0x4018).

## Audio
- ES8311 codec (I2C + I2S), powered from its own 3.3 V LDO.
- SC8002B speaker amp, enabled by AUDIO_EN (GPIO1). Speaker on JP3.
- LMA2718B381 analog mic into the ES8311 MIC1.

## Power
- TP4054 Li-ion charger from VBUS; battery connector JP1.
- ME6217C33 3.3 V LDO.
