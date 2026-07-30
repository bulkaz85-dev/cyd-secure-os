#pragma once
#include <TFT_eSPI.h>

struct Rect {
    int x, y, w, h;
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};

inline void drawButton(TFT_eSPI &tft, Rect r, const char *label,
                        uint16_t bg = TFT_DARKGREY, uint16_t fg = TFT_WHITE) {
    tft.fillRoundRect(r.x, r.y, r.w, r.h, 8, bg);
    tft.drawRoundRect(r.x, r.y, r.w, r.h, 8, TFT_BLACK);
    tft.setTextColor(fg, bg);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString(label, r.x + r.w / 2, r.y + r.h / 2);
}

inline void drawStatusBar(TFT_eSPI &tft, const char *title, bool locked, int batteryPct = -1) {
    tft.fillRect(0, 0, 320, 20, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.setTextSize(1);
    tft.drawString(title, 4, 10);
    tft.setTextDatum(MR_DATUM);
    String right = locked ? "[LOCK]" : "[UNLOCKED]";
    if (batteryPct >= 0) {
        right = String(batteryPct) + "% " + right;
    }
    tft.drawString(right, 316, 10);
}
