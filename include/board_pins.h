#pragma once
// ============================================================
// ESP32-2432S028 (CYD) hardware pin map
// Confirm against the silkscreen/rev of your specific board --
// there are 2-3 known CYD PCB revisions with minor pin diffs.
// ============================================================

// ---- Touch (XPT2046, resistive, separate SPI bus) ----
#define XPT2046_CLK   25
#define XPT2046_MOSI  32
#define XPT2046_MISO  39
#define XPT2046_CS    33
#define XPT2046_IRQ   36

// ---- SD Card (HSPI) ----
#define SD_CS         5
#define SD_MOSI       23
#define SD_MISO       19
#define SD_SCK        18

// ---- Misc onboard peripherals ----
#define PIN_LDR       34   // light dependent resistor (ambient light)
#define PIN_RGB_RED   4
#define PIN_RGB_GREEN 16
#define PIN_RGB_BLUE  17
#define PIN_SPEAKER   26   // DAC-capable, passive buzzer/speaker pad
#define PIN_BOOT_BTN  0    // onboard BOOT button, usable as a physical key

// ---- Touch calibration (adjust after first run using a calibration sketch) ----
#define TOUCH_MIN_X 200
#define TOUCH_MAX_X 3700
#define TOUCH_MIN_Y 240
#define TOUCH_MAX_Y 3800
