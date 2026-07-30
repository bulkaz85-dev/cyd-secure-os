#pragma once
#include <Arduino.h>
#include <driver/adc.h>
#include <esp_sleep.h>
#include "board_pins.h"

// ============================================================
// power.h — battery monitoring + aggressive power saving
//
// Honesty check: the ESP32-2432S028 has NO built-in battery
// charge circuit, no fuel gauge, and no solar input. To run this
// off solar you need to add, externally:
//   1. A small solar panel (5-6V, a few hundred mA is plenty)
//   2. A solar LiPo charge controller (e.g. a TP4056-based solar
//      charge board, or a dedicated MPPT-lite module like a
//      CN3065/CN3791 board for better panel utilization)
//   3. A single-cell LiPo/Li-ion battery (1000-2000mAh is a
//      reasonable size for this board)
//   4. A voltage divider (two 100k resistors) from the battery's
//      + terminal to an ADC-capable, otherwise-unused pin, so
//      the ESP32 can measure battery voltage without exceeding
//      its 3.3V ADC limit. See README for exact wiring.
// This file assumes that hardware exists on BATTERY_ADC_PIN.
// Without it, battery% will read garbage -- harmless, just
// ignore the reading if you haven't wired the divider yet.
// ============================================================

#define BATTERY_ADC_PIN   35   // free ADC1-capable pin on most CYD boards
#define BATTERY_DIVIDER_RATIO 2.0f   // two equal resistors = 2:1 divider
#define BATTERY_ADC_VREF  3.3f
#define BATTERY_FULL_V    4.2f
#define BATTERY_EMPTY_V   3.3f

#define IDLE_DIM_MS       30000    // dim backlight after 30s idle
#define IDLE_SLEEP_MS     120000   // deep sleep after 2 min idle
#define BACKLIGHT_PIN     21

class PowerManager {
public:
    void begin() {
        pinMode(BACKLIGHT_PIN, OUTPUT);
        digitalWrite(BACKLIGHT_PIN, HIGH);
        lastActivity = millis();
        dimmed = false;

        // If we just woke from deep sleep via touch, log it (useful for debugging).
        esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
        wokeFromTouch = (cause == ESP_SLEEP_WAKEUP_EXT0);
    }

    // Call this anywhere a touch was just detected.
    void notifyActivity() {
        if (dimmed) {
            digitalWrite(BACKLIGHT_PIN, HIGH);
            dimmed = false;
        }
        lastActivity = millis();
    }

    // Call frequently (e.g. once per idle loop iteration, ~every 10ms) from
    // every screen's event loop. Dims backlight, then deep-sleeps on longer idle.
    void tick() {
        unsigned long idleFor = millis() - lastActivity;
        if (!dimmed && idleFor > IDLE_DIM_MS) {
            // PWM dimming isn't wired here for simplicity -- this just
            // drops backlight to off/on toggle at low duty via delay,
            // which is a crude but real power saving. For smooth dimming,
            // swap digitalWrite for ledcWrite on a PWM channel bound to
            // BACKLIGHT_PIN (see README).
            digitalWrite(BACKLIGHT_PIN, LOW);
            dimmed = true;
        }
        if (idleFor > IDLE_SLEEP_MS) {
            sleepNow();
        }
    }

    // Forces immediate deep sleep, e.g. from a "Sleep Now" menu tile.
    void sleepNow() {
        digitalWrite(BACKLIGHT_PIN, LOW);
        // XPT2046 IRQ pin pulls LOW on touch -- wake the ESP32 when that happens.
        esp_sleep_enable_ext0_wakeup((gpio_num_t)XPT2046_IRQ, 0 /* wake on LOW */);
        esp_deep_sleep_start();
        // Execution does not return here -- deep sleep resets the chip;
        // setup() runs again on wake, just like a fresh boot. The session
        // key (RAM only) is gone, so the user re-enters their passphrase --
        // this is a feature, not a bug, for a device that leaves your pocket.
    }

    bool justWokeFromTouch() const { return wokeFromTouch; }

    // Rough LiPo state-of-charge from voltage. LiPo discharge curves are
    // not linear -- treat this as "low / ok / good", not a precise %.
    int batteryPercent() {
        float v = readBatteryVoltage();
        if (v < 1.0f) return -1; // no divider wired / no battery -- can't tell
        float pct = (v - BATTERY_EMPTY_V) / (BATTERY_FULL_V - BATTERY_EMPTY_V) * 100.0f;
        return (int)constrain(pct, 0.0f, 100.0f);
    }

    float readBatteryVoltage() {
        int raw = analogRead(BATTERY_ADC_PIN);           // 0-4095, 12-bit
        float vAtPin = (raw / 4095.0f) * BATTERY_ADC_VREF;
        return vAtPin * BATTERY_DIVIDER_RATIO;
    }

private:
    unsigned long lastActivity = 0;
    bool dimmed = false;
    bool wokeFromTouch = false;
};

// Single global instance so any file can call power.tick()/notifyActivity()
// without threading a reference through every constructor.
extern PowerManager power;
