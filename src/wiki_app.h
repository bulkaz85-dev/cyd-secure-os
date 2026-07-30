#pragma once
#include <TFT_eSPI.h>
#include <SD.h>
#include <vector>
#include "ui_helpers.h"
#include "touch.h"
#include "keyboard.h"

// ============================================================
// wiki_app.h — offline reference library
//
// Reads plain .txt articles from /wiki on the SD card. These are
// NOT encrypted on purpose: it's reference material you want to
// search/update easily (drag files onto the SD card from any
// computer), not secret data. If you specifically want an
// article encrypted, that's what the Notes vault is for.
//
// File format expected: first line = title, rest = body text.
// Word-wrapped and paginated for the 320x240 screen.
// ============================================================

#define WIKI_DIR "/wiki"
#define CHARS_PER_LINE 48
#define LINES_PER_PAGE 18

class WikiApp {
public:
    WikiApp(TFT_eSPI &tft, Touch &touch) : tft(tft), touch(touch) {}

    void run() {
        query = "";
        refreshList();
        bool exitApp = false;
        while (!exitApp) {
            drawList();
            exitApp = handleListInput();
        }
    }

private:
    TFT_eSPI &tft;
    Touch &touch;
    String query;
    struct Entry { String filename; String title; };
    std::vector<Entry> allEntries, filtered;
    Rect searchBtn{200, 200, 60, 30}, backBtn{10, 200, 60, 30}, clearBtn{270, 200, 40, 30};

    void refreshList() {
        allEntries.clear();
        File dir = SD.open(WIKI_DIR);
        if (!dir) { filtered.clear(); return; }
        File entry = dir.openNextFile();
        while (entry) {
            String name = String(entry.name());
            if (name.endsWith(".txt")) {
                String title = readFirstLine(WIKI_DIR + String("/") + name);
                allEntries.push_back({name, title});
            }
            entry.close();
            entry = dir.openNextFile();
        }
        dir.close();
        applyFilter();
    }

    String readFirstLine(const String &path) {
        File f = SD.open(path, FILE_READ);
        if (!f) return "(unreadable)";
        String line = f.readStringUntil('\n');
        f.close();
        line.trim();
        return line.length() ? line : "(untitled)";
    }

    void applyFilter() {
        filtered.clear();
        String q = query; q.toLowerCase();
        for (auto &e : allEntries) {
            String t = e.title; t.toLowerCase();
            String fn = e.filename; fn.toLowerCase();
            if (q.length() == 0 || t.indexOf(q) >= 0 || fn.indexOf(q) >= 0) {
                filtered.push_back(e);
            }
        }
    }

    void drawList() {
        tft.fillScreen(TFT_BLACK);
        drawStatusBar(tft, "Offline Wiki", true);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        tft.drawString("Search: " + (query.length() ? query : String("(tap to search)")), 8, 26);

        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        int y = 44;
        int shown = 0;
        for (auto &e : filtered) {
            if (shown >= 8) break;
            tft.drawString("- " + e.title, 8, y);
            y += 18;
            shown++;
        }
        if (filtered.empty()) {
            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft.drawString(allEntries.empty()
                ? "No articles found. Copy .txt files into /wiki on the SD card."
                : "No matches.", 8, y);
        }
        drawButton(tft, backBtn, "BACK", TFT_MAROON);
        drawButton(tft, searchBtn, "SEARCH", TFT_NAVY);
        drawButton(tft, clearBtn, "X", TFT_DARKGREY);
    }

    // returns true if exiting the wiki app
    bool handleListInput() {
        while (true) {
            int16_t x, y;
            if (touch.read(x, y)) {
                delay(150);
                if (backBtn.contains(x, y)) return true;
                if (searchBtn.contains(x, y)) {
                    Keyboard kb(tft, touch);
                    query = kb.prompt("Search articles:", query);
                    applyFilter();
                    return false;
                }
                if (clearBtn.contains(x, y)) {
                    query = "";
                    applyFilter();
                    return false;
                }
                if (y >= 44 && y < 44 + 18 * 8) {
                    int idx = (y - 44) / 18;
                    if (idx >= 0 && idx < (int)filtered.size()) {
                        openArticle(filtered[idx].filename);
                        return false;
                    }
                }
            }
            power.tick();
            delay(10);
        }
    }

    // ---- Article viewer ----
    void openArticle(const String &filename) {
        String path = WIKI_DIR + String("/") + filename;
        File f = SD.open(path, FILE_READ);
        if (!f) return;
        String full;
        full.reserve(f.size() + 1);
        while (f.available()) full += (char)f.read();
        f.close();

        std::vector<String> lines = wrapText(full, CHARS_PER_LINE);
        int page = 0;
        int totalPages = max(1, (int)((lines.size() + LINES_PER_PAGE - 1) / LINES_PER_PAGE));

        bool exitArticle = false;
        while (!exitArticle) {
            drawPage(lines, page, totalPages);
            exitArticle = handleArticleInput(page, totalPages);
        }
    }

    std::vector<String> wrapText(const String &text, int width) {
        std::vector<String> lines;
        int start = 0;
        // Split on explicit newlines first, then word-wrap each paragraph.
        while (start <= (int)text.length()) {
            int nl = text.indexOf('\n', start);
            String para = (nl == -1) ? text.substring(start) : text.substring(start, nl);
            wrapParagraph(para, width, lines);
            if (nl == -1) break;
            start = nl + 1;
        }
        return lines;
    }

    void wrapParagraph(const String &para, int width, std::vector<String> &out) {
        if (para.length() == 0) { out.push_back(""); return; }
        int i = 0;
        String line;
        while (i < (int)para.length()) {
            int spaceIdx = para.indexOf(' ', i);
            String word = (spaceIdx == -1) ? para.substring(i) : para.substring(i, spaceIdx);
            if (line.length() + word.length() + 1 > (unsigned)width) {
                out.push_back(line);
                line = word;
            } else {
                line = line.length() ? (line + " " + word) : word;
            }
            i = (spaceIdx == -1) ? para.length() : spaceIdx + 1;
        }
        if (line.length()) out.push_back(line);
    }

    Rect prevBtn{10, 205, 70, 28}, nextBtn{240, 205, 70, 28}, closeBtn{125, 205, 70, 28};

    void drawPage(std::vector<String> &lines, int page, int totalPages) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(TL_DATUM);
        tft.setTextSize(1);
        int startLine = page * LINES_PER_PAGE;
        int y = 4;
        for (int i = startLine; i < min((int)lines.size(), startLine + LINES_PER_PAGE); i++) {
            tft.drawString(lines[i], 4, y);
            y += 11;
        }
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("Page " + String(page + 1) + "/" + String(totalPages), 4, 195);

        drawButton(tft, prevBtn, "PREV", TFT_DARKGREY);
        drawButton(tft, closeBtn, "CLOSE", TFT_MAROON);
        drawButton(tft, nextBtn, "NEXT", TFT_DARKGREY);
    }

    bool handleArticleInput(int &page, int totalPages) {
        while (true) {
            int16_t x, y;
            if (touch.read(x, y)) {
                delay(150);
                if (closeBtn.contains(x, y)) return true;
                if (prevBtn.contains(x, y) && page > 0) { page--; return false; }
                if (nextBtn.contains(x, y) && page < totalPages - 1) { page++; return false; }
            }
            power.tick();
            delay(10);
        }
    }
};
