#pragma once
#include <TFT_eSPI.h>
#include <vector>
#include "ui_helpers.h"
#include "vault.h"
#include "touch.h"
#include "keyboard.h"

class NotesApp {
public:
    NotesApp(TFT_eSPI &tft, Touch &touch, Vault &vault, const uint8_t *key)
        : tft(tft), touch(touch), vault(vault), key(key) {}

    // Runs a simple list-view of encrypted notes until the user taps "Back".
    void run() {
        bool exit = false;
        while (!exit) {
            auto names = vault.list();
            drawList(names);
            exit = waitForListTap(names);
        }
    }

private:
    TFT_eSPI &tft;
    Touch &touch;
    Vault &vault;
    const uint8_t *key;
    Rect backBtn{10, 200, 90, 30};
    Rect newBtn{220, 200, 90, 30};

    void drawList(std::vector<String> &names) {
        tft.fillScreen(TFT_BLACK);
        drawStatusBar(tft, "Encrypted Notes", true);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(2);
        int y = 30;
        int count = 0;
        for (auto &n : names) {
            if (n.startsWith("note_")) {
                tft.drawString(n.substring(5), 10, y);
                y += 24;
                count++;
            }
            if (count >= 6) break;
        }
        if (count == 0) {
            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft.drawString("(empty - tap NEW)", 10, y);
        }
        drawButton(tft, backBtn, "BACK", TFT_MAROON);
        drawButton(tft, newBtn, "NEW", TFT_DARKGREEN);
    }

    // returns true if user wants to exit the notes app
    bool waitForListTap(std::vector<String> &names) {
        while (true) {
            int16_t x, y;
            if (touch.read(x, y)) {
                delay(150);
                if (backBtn.contains(x, y)) return true;
                if (newBtn.contains(x, y)) {
                    createNoteFlow();
                    return false;
                }
                // Row tap -> open note (rows start at y=30, 24px tall)
                int idx = (y - 30) / 24;
                int count = 0;
                for (auto &n : names) {
                    if (!n.startsWith("note_")) continue;
                    if (count == idx) {
                        viewNoteFlow(n.substring(5));
                        return false;
                    }
                    count++;
                }
            }
            power.tick();
            delay(10);
        }
    }

    void createNoteFlow() {
        Keyboard kb(tft, touch);
        String title = kb.prompt("Note title:");
        if (title.length() == 0) return; // cancelled
        String body = kb.prompt("Note text:");
        vault.saveNote(key, title, body);
    }

    void viewNoteFlow(const String &title) {
        String body;
        bool ok = vault.loadNote(key, title, body);
        tft.fillScreen(TFT_BLACK);
        drawStatusBar(tft, ("Note: " + title).c_str(), true);
        tft.setTextColor(ok ? TFT_WHITE : TFT_RED, TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.setTextWrap(true);
        tft.drawString(ok ? body : "DECRYPTION FAILED (tampered or wrong key)", 10, 30);
        Rect back{10, 200, 300, 30};
        drawButton(tft, back, "BACK", TFT_MAROON);
        while (true) {
            int16_t x, y;
            if (touch.read(x, y) && back.contains(x, y)) { delay(150); return; }
            power.tick();
            delay(10);
        }
    }
};
