#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <SPI.h>
#include "board_pins.h"
#include "power.h"
#include "touch.h"
#include "lock_manager.h"
#include "vault.h"
#include "lock_screen.h"
#include "launcher.h"

TFT_eSPI tft = TFT_eSPI();
Touch touch;
LockManager lockMgr;
Vault vault;
PowerManager power; // definition of the extern declared in power.h

bool sdOk = false;

void bootScreen(const char *msg, uint16_t color = TFT_WHITE) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(color, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString(msg, 160, 120);
}

void setup() {
    Serial.begin(115200);

    // --- Display ---
    tft.init();
    tft.setRotation(1); // landscape, 320x240
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH); // backlight on
    bootScreen("CYD Secure OS booting...");

    // --- Touch ---
    touch.begin();

    // --- Power (battery ADC + idle/sleep timers) ---
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db); // allows reading up to ~3.3V at the pin
    power.begin();
    if (power.justWokeFromTouch()) {
        bootScreen("Waking up...");
        delay(300);
    }

    // --- SD card (separate HSPI bus from the display's VSPI) ---
    SPIClass sdSPI(HSPI);
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    sdOk = SD.begin(SD_CS, sdSPI);
    if (!sdOk) {
        bootScreen("SD CARD NOT FOUND", TFT_RED);
        delay(2500);
        // We still continue -- lock/PIN still works via NVS, but vault
        // features (notes, gallery) will fail gracefully until an SD
        // card is inserted and the device is restarted.
    } else {
        vault.begin();
    }

    // --- Lock manager (NVS-backed passphrase) ---
    lockMgr.begin();

    randomSeed(esp_random());
}

void loop() {
    // Lock screen blocks until unlocked/provisioned, then hands off to launcher.
    LockScreen lockScreen(tft, touch, lockMgr);
    lockScreen.run();

    Launcher launcher(tft, touch, lockMgr, vault);
    launcher.run(); // returns when user taps LOCK

    // loop() repeats -> back to lock screen
}
