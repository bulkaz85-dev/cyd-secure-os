#pragma once
#include <TFT_eSPI.h>
#include "ui_helpers.h"
#include "lock_manager.h"
#include "touch.h"

// ============================================================
// lock_screen.h — numeric PIN keypad UI
//
// Ships as a numeric PIN for simplicity/usability on a touchscreen.
// A PIN alone (even 6 digits = 1,000,000 combos) is weak against
// an offline attacker who has the flash dump -- see crypto.h notes.
// For real security, prompt for a longer alphanumeric passphrase
// via an on-screen QWERTY keyboard instead; swap this class out,
// the LockManager API (provision/tryUnlock) doesn't care what
// string you pass it.
// ============================================================

class LockScreen {
public:
    LockScreen(TFT_eSPI &tft, Touch &touch, LockManager &lock)
        : tft(tft), touch(touch), lock(lock) {}

    void layoutKeypad() {
        const char *keys[12] = {"1","2","3","4","5","6","7","8","9","","0","DEL"};
        int idx = 0;
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 3; col++) {
                keyRects[idx] = { 20 + col * 100, 90 + row * 35, 90, 30 };
                idx++;
            }
        }
    }

    // Blocks until the user successfully unlocks (or provisions on first run).
    void run() {
        layoutKeypad();
        bool firstRun = !lock.isProvisioned();
        entry = "";
        done = false;
        draw(firstRun ? "Set your passphrase" : "Enter passphrase");

        while (!done) {
            int16_t x, y;
            if (touch.read(x, y)) {
                handleTouch(x, y, firstRun);
                delay(150); // simple debounce
            }
            power.tick();
            delay(10);
        }
    }

private:
    TFT_eSPI &tft;
    Touch &touch;
    LockManager &lock;
    Rect keyRects[12];
    String entry;

    void draw(const char *prompt) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(2);
        tft.drawString(prompt, 160, 40);
        drawDots();
        const char *labels[12] = {"1","2","3","4","5","6","7","8","9","","0","DEL"};
        for (int i = 0; i < 12; i++) {
            if (strlen(labels[i]) == 0) continue;
            drawButton(tft, keyRects[i], labels[i],
                       strcmp(labels[i], "DEL") == 0 ? TFT_MAROON : TFT_DARKGREY);
        }
        Rect okBtn = { 20, 90 + 4 * 35 + 10, 280, 34 };
        drawButton(tft, okBtn, "UNLOCK", TFT_DARKGREEN);
        okRect = okBtn;
    }

    void drawDots() {
        tft.fillRect(60, 60, 200, 16, TFT_BLACK);
        for (size_t i = 0; i < entry.length() && i < 10; i++) {
            tft.fillCircle(70 + i * 18, 68, 5, TFT_WHITE);
        }
    }

    Rect okRect;

    void handleTouch(int16_t x, int16_t y, bool firstRun) {
        const char *labels[12] = {"1","2","3","4","5","6","7","8","9","","0","DEL"};
        for (int i = 0; i < 12; i++) {
            if (keyRects[i].contains(x, y) && strlen(labels[i]) > 0) {
                if (strcmp(labels[i], "DEL") == 0) {
                    if (entry.length() > 0) entry.remove(entry.length() - 1);
                } else if (entry.length() < 32) {
                    entry += labels[i];
                }
                drawDots();
                return;
            }
        }
        if (okRect.contains(x, y)) {
            attemptSubmit(firstRun);
        }
    }

    void attemptSubmit(bool firstRun) {
        if (entry.length() < 4) {
            flashMessage("Min 4 digits", TFT_RED);
            return;
        }
        bool ok = firstRun ? lock.provision(entry) : lock.tryUnlock(entry);
        entry = "";
        if (ok) {
            flashMessage("OK", TFT_DARKGREEN);
            delay(300);
            // returning from run() hands control back to the caller (launcher)
            done = true;
        } else {
            flashMessage("Wrong passphrase", TFT_RED);
            drawDots();
        }
    }

    void flashMessage(const char *msg, uint16_t color) {
        tft.fillRect(0, 200, 320, 30, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(color, TFT_BLACK);
        tft.setTextSize(2);
        tft.drawString(msg, 160, 215);
    }

public:
    bool done = false;
};
