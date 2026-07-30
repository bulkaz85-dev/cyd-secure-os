#pragma once
#include <TFT_eSPI.h>
#include <vector>
#include "../touch.h"
#include "../ui_helpers.h"

// Simple grid-based Snake game. Swipe on screen to change direction.
class SnakeGame {
public:
    SnakeGame(TFT_eSPI &tft, Touch &touch) : tft(tft), touch(touch) {}

    void run() {
        reset();
        bool exitGame = false;
        unsigned long lastTick = 0;
        const unsigned long tickMs = 180;

        while (!exitGame && alive) {
            handleSwipe();
            if (millis() - lastTick > tickMs) {
                step();
                draw();
                lastTick = millis();
            }
            // Tap top-left corner to quit
            int16_t x, y;
            if (touch.read(x, y) && x < 30 && y < 20) exitGame = true;
            delay(5);
        }
        gameOverScreen();
    }

private:
    TFT_eSPI &tft;
    Touch &touch;
    static const int cols = 30, rows = 20, cell = 10; // 300x200 play field
    std::vector<std::pair<int,int>> body;
    int dx = 1, dy = 0;
    int foodX, foodY;
    bool alive = true;
    int score = 0;
    int16_t lastTx = -1, lastTy = -1;

    void reset() {
        body.clear();
        body.push_back({cols/2, rows/2});
        dx = 1; dy = 0;
        alive = true;
        score = 0;
        placeFood();
        tft.fillScreen(TFT_BLACK);
    }

    void placeFood() {
        foodX = random(0, cols);
        foodY = random(0, rows);
    }

    void handleSwipe() {
        int16_t x, y;
        if (!touch.read(x, y)) { lastTx = -1; return; }
        if (lastTx < 0) { lastTx = x; lastTy = y; return; }
        int ddx = x - lastTx, ddy = y - lastTy;
        if (abs(ddx) > 15 || abs(ddy) > 15) {
            if (abs(ddx) > abs(ddy)) {
                if (ddx > 0 && dx == 0) { dx = 1; dy = 0; }
                else if (ddx < 0 && dx == 0) { dx = -1; dy = 0; }
            } else {
                if (ddy > 0 && dy == 0) { dx = 0; dy = 1; }
                else if (ddy < 0 && dy == 0) { dx = 0; dy = -1; }
            }
            lastTx = x; lastTy = y;
        }
    }

    void step() {
        auto head = body.front();
        int nx = head.first + dx;
        int ny = head.second + dy;
        if (nx < 0 || ny < 0 || nx >= cols || ny >= rows) { alive = false; return; }
        for (auto &seg : body) {
            if (seg.first == nx && seg.second == ny) { alive = false; return; }
        }
        body.insert(body.begin(), {nx, ny});
        if (nx == foodX && ny == foodY) {
            score++;
            placeFood();
        } else {
            body.pop_back();
        }
    }

    void draw() {
        tft.fillRect(0, 20, cols * cell, rows * cell, TFT_BLACK);
        tft.fillRect(foodX * cell, 20 + foodY * cell, cell, cell, TFT_RED);
        for (auto &seg : body) {
            tft.fillRect(seg.first * cell, 20 + seg.second * cell, cell - 1, cell - 1, TFT_GREEN);
        }
        tft.fillRect(0, 0, 320, 20, TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.drawString("Score: " + String(score) + "  (tap top-left to quit)", 34, 6);
    }

    void gameOverScreen() {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(3);
        tft.drawString("GAME OVER", 160, 90);
        tft.setTextSize(2);
        tft.drawString("Score: " + String(score), 160, 130);
        Rect back{110, 170, 100, 34};
        drawButton(tft, back, "BACK", TFT_MAROON);
        while (true) {
            int16_t x, y;
            if (touch.read(x, y) && back.contains(x, y)) { delay(150); return; }
            power.tick();
            delay(10);
        }
    }
};
