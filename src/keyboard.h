#pragma once
#include <TFT_eSPI.h>
#include "ui_helpers.h"
#include "touch.h"

// ============================================================
// keyboard.h — minimal on-screen QWERTY keyboard
//
// Blocking modal: call Keyboard::prompt() and it returns the
// typed string once the user taps ENTER, or "" if they tap
// CANCEL. Used by the Wiki app's search bar; reusable anywhere
// text input is needed (passphrase, notes, etc.).
// ============================================================

class Keyboard {
public:
    Keyboard(TFT_eSPI &tft, Touch &touch) : tft(tft), touch(touch) {}

    String prompt(const char *title, const String &initial = "") {
        text = initial;
        cancelled = false;
        submitted = false;
        buildKeys();
        draw(title);

        while (!submitted && !cancelled) {
            int16_t x, y;
            if (touch.read(x, y)) {
                handleTap(x, y);
                draw(title);
                delay(120);
            }
            delay(10);
        }
        return cancelled ? String("") : text;
    }

private:
    TFT_eSPI &tft;
    Touch &touch;
    String text;
    bool cancelled = false, submitted = false;

    struct Key { Rect r; String label; };
    std::vector<Key> keys;
    Rect enterBtn, cancelBtn, spaceBtn, delBtn;

    void buildKeys() {
        keys.clear();
        const char *row1 = "1234567890";
        const char *row2 = "qwertyuiop";
        const char *row3 = "asdfghjkl";
        const char *row4 = "zxcvbnm";
        int y = 70;
        addRow(row1, y, 10); y += 26;
        addRow(row2, y, 10); y += 26;
        addRow(row3, y, 25); y += 26;
        addRow(row4, y, 40);
        y += 30;
        spaceBtn = {60, y, 120, 26};
        delBtn   = {190, y, 60, 26};
        enterBtn = {255, y, 60, 26};
        cancelBtn = {5, y, 50, 26};
    }

    void addRow(const char *chars, int y, int xOffset) {
        int n = strlen(chars);
        int w = 28;
        for (int i = 0; i < n; i++) {
            Rect r{xOffset + i * w, y, w - 2, 24};
            keys.push_back({r, String(chars[i])});
        }
    }

    void draw(const char *title) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(1);
        tft.drawString(title, 6, 6);
        tft.fillRect(6, 20, 308, 22, TFT_NAVY);
        tft.drawRoundRect(6, 20, 308, 22, 4, TFT_WHITE);
        tft.setTextColor(TFT_WHITE, TFT_NAVY);
        tft.setTextSize(2);
        tft.drawString(text + "_", 12, 25);

        tft.setTextSize(1);
        for (auto &k : keys) {
            drawButton(tft, k.r, k.label.c_str(), TFT_DARKGREY);
        }
        drawButton(tft, spaceBtn, "SPACE", TFT_DARKGREY);
        drawButton(tft, delBtn, "DEL", TFT_MAROON);
        drawButton(tft, enterBtn, "GO", TFT_DARKGREEN);
        drawButton(tft, cancelBtn, "X", TFT_MAROON);
    }

    void handleTap(int16_t x, int16_t y) {
        for (auto &k : keys) {
            if (k.r.contains(x, y)) {
                if (text.length() < 40) text += k.label;
                return;
            }
        }
        if (spaceBtn.contains(x, y)) { text += " "; return; }
        if (delBtn.contains(x, y)) { if (text.length()) text.remove(text.length() - 1); return; }
        if (enterBtn.contains(x, y)) { submitted = true; return; }
        if (cancelBtn.contains(x, y)) { cancelled = true; return; }
    }
};
