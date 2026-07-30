#pragma once
#include <TFT_eSPI.h>
#include "ui_helpers.h"
#include "touch.h"
#include "lock_manager.h"
#include "vault.h"
#include "notes_app.h"
#include "wiki_app.h"
#include "games/snake.h"
#include "power.h"

enum class AppId { NOTES, WIKI, GALLERY, SNAKE, SETTINGS, SLEEP };

class Launcher {
public:
    Launcher(TFT_eSPI &tft, Touch &touch, LockManager &lock, Vault &vault)
        : tft(tft), touch(touch), lock(lock), vault(vault) {}

    void run() {
        buildTiles();
        draw();
        while (lock.unlocked()) {
            int16_t x, y;
            if (touch.read(x, y)) {
                delay(150);
                handleTap(x, y);
                if (lock.unlocked()) draw(); // redraw unless we just locked out
            }
            power.tick();
            delay(10);
        }
    }

private:
    TFT_eSPI &tft;
    Touch &touch;
    LockManager &lock;
    Vault &vault;

    struct Tile { Rect r; const char *label; AppId id; uint16_t color; };
    std::vector<Tile> tiles;

    void buildTiles() {
        tiles.clear();
        const char *labels[] = {"Notes", "Wiki", "Gallery", "Snake", "Settings", "Sleep Now"};
        AppId ids[] = {AppId::NOTES, AppId::WIKI, AppId::GALLERY, AppId::SNAKE, AppId::SETTINGS, AppId::SLEEP};
        uint16_t colors[] = {TFT_BLUE, TFT_OLIVE, TFT_PURPLE, TFT_DARKGREEN, TFT_DARKGREY, TFT_NAVY};
        int idx = 0;
        for (int row = 0; row < 2; row++) {
            for (int col = 0; col < 3; col++) {
                Rect r{10 + col * 103, 30 + row * 78, 96, 68};
                tiles.push_back({r, labels[idx], ids[idx], colors[idx]});
                idx++;
            }
        }
    }

    void draw() {
        tft.fillScreen(TFT_BLACK);
        drawStatusBar(tft, "CYD Secure OS", !lock.unlocked(), power.batteryPercent());
        for (auto &t : tiles) {
            drawButton(tft, t.r, t.label, t.color);
        }
        Rect lockBtn{110, 190, 100, 26};
        drawButton(tft, lockBtn, "LOCK", TFT_MAROON);
        lockButtonRect = lockBtn;
    }

    Rect lockButtonRect;

    void handleTap(int16_t x, int16_t y) {
        if (lockButtonRect.contains(x, y)) {
            lock.lock();
            return;
        }
        for (auto &t : tiles) {
            if (t.r.contains(x, y)) {
                launch(t.id);
                return;
            }
        }
    }

    void launch(AppId id) {
        switch (id) {
            case AppId::NOTES: {
                NotesApp app(tft, touch, vault, lock.getSessionKey());
                app.run();
                break;
            }
            case AppId::SNAKE: {
                SnakeGame game(tft, touch);
                game.run();
                break;
            }
            case AppId::WIKI: {
                WikiApp wiki(tft, touch);
                wiki.run();
                break;
            }
            case AppId::GALLERY:
                showPlaceholder("Gallery",
                    "Phase 2: encrypted photo viewer.\nSee README for extension notes.");
                break;
            case AppId::SETTINGS:
                showPlaceholder("Settings",
                    "Phase 2: change passphrase,\nWi-Fi, panic wipe toggle.");
                break;
            case AppId::SLEEP:
                tft.fillScreen(TFT_BLACK);
                tft.setTextDatum(MC_DATUM);
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.setTextSize(2);
                tft.drawString("Sleeping...", 160, 120);
                tft.drawString("Tap screen to wake", 160, 150);
                delay(600);
                lock.lock(); // wipe session key before sleeping, same as manual lock
                power.sleepNow(); // does not return -- deep sleep, chip resets on wake
                break;
            default:
                break;
        }
    }

    void showPlaceholder(const char *title, const char *msg) {
        tft.fillScreen(TFT_BLACK);
        drawStatusBar(tft, title, true);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(2);
        String s(msg);
        int y = 40;
        int start = 0;
        for (int i = 0; i <= (int)s.length(); i++) {
            if (i == (int)s.length() || s[i] == '\n') {
                tft.drawString(s.substring(start, i), 10, y);
                y += 20;
                start = i + 1;
            }
        }
        Rect back{110, 200, 100, 30};
        drawButton(tft, back, "BACK", TFT_MAROON);
        while (true) {
            int16_t x, y;
            if (touch.read(x, y) && back.contains(x, y)) { delay(150); return; }
            power.tick();
            delay(10);
        }
    }
};
