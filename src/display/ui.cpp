#include "ui.h"
#include "display.h"
#include "../epd/EPD.h"
#include "../battery/battery.h"
#include "../fonts/DejaVuMono12.h"
#include "../fonts/DejaVuMono12Bold.h"
#include "../fonts/DejaVuMono16.h"
#include "../fonts/DejaVuMono16Bold.h"
#include "../fonts/DejaVuMono18.h"
#include "../fonts/DejaVuMono18Bold.h"
#include "../fonts/DejaVuMono24.h"
#include "../fonts/DejaVuMono24Bold.h"
#include "../fonts/DejaVuMono24BoldOblique.h"
#include <pgmspace.h>

namespace UI {

// =============================================================================
// Text rendering — Elecrow EPD_ShowChar for size-16/24 fonts,
// custom PROGMEM bitmap for size-12 (DejaVu Sans Mono 8x12)
// =============================================================================

static uint8_t drawCharMono12(int16_t x, int16_t y, char c, uint8_t color, bool bold) {
    if (c < 32 || c > 126) c = 32;
    const uint8_t* glyph = bold ? dejavu_mono_12_bold[c - 32] : dejavu_mono_12[c - 32];
    uint8_t col_color = (color == COL_BLACK) ? BLACK : WHITE;
    for (int row = 0; row < 12; row++) {
        uint8_t b = pgm_read_byte(&glyph[row]);
        for (int col = 0; col < 8; col++) {
            if (b & (0x80 >> col))
                Paint_SetPixel(x + col, y + row, col_color);
        }
    }
    return 8;
}

static uint8_t drawCharMono16(int16_t x, int16_t y, char c, uint8_t color, bool bold) {
    if (c < 32 || c > 126) c = 32;
    const uint8_t* glyph = bold ? dejavu_mono_16_bold[c - 32] : dejavu_mono_16[c - 32];
    uint8_t col_color = (color == COL_BLACK) ? BLACK : WHITE;
    for (int row = 0; row < 16; row++) {
        uint8_t b = pgm_read_byte(&glyph[row]);
        for (int col = 0; col < 8; col++) {
            if (b & (0x80 >> col))
                Paint_SetPixel(x + col, y + row, col_color);
        }
    }
    return 8;
}

static uint8_t drawCharMono18(int16_t x, int16_t y, char c, uint8_t color, bool bold) {
    if (c < 32 || c > 126) c = 32;
    const uint16_t* glyph = bold ? dejavu_mono_18_bold[c - 32] : dejavu_mono_18[c - 32];
    uint8_t col_color = (color == COL_BLACK) ? BLACK : WHITE;
    for (int row = 0; row < 18; row++) {
        uint16_t b = pgm_read_word(&glyph[row]);
        for (int col = 0; col < 10; col++) {
            if (b & (0x8000 >> col))
                Paint_SetPixel(x + col, y + row, col_color);
        }
    }
    return 10;
}

static uint8_t drawCharMono24(int16_t x, int16_t y, char c, uint8_t color, bool bold) {
    if (c < 32 || c > 126) c = 32;
    const uint16_t* glyph = bold ? dejavu_mono_24_bold[c - 32] : dejavu_mono_24[c - 32];
    uint8_t col_color = (color == COL_BLACK) ? BLACK : WHITE;
    for (int row = 0; row < 24; row++) {
        uint16_t b = pgm_read_word(&glyph[row]);
        for (int col = 0; col < 14; col++) {
            if (b & (0x8000 >> col))
                Paint_SetPixel(x + col, y + row, col_color);
        }
    }
    return 14;
}

static uint8_t drawCharMono24BI(int16_t x, int16_t y, char c, uint8_t color) {
    if (c < 32 || c > 126) c = 32;
    const uint16_t* glyph = dejavu_mono_24bi[c - 32];
    uint8_t col_color = (color == COL_BLACK) ? BLACK : WHITE;
    for (int row = 0; row < 24; row++) {
        uint16_t b = pgm_read_word(&glyph[row]);
        for (int col = 0; col < 15; col++) {
            if (b & (0x8000 >> col))
                Paint_SetPixel(x + col, y + row, col_color);
        }
    }
    return 15;
}

uint8_t drawChar(int16_t x, int16_t y, char c, const FontDef& font, uint8_t color) {
    if (x < 0 || y < 0) return font.charWidth;
    if (font.size == 12) { drawCharMono12(x, y, c, color, font.bold); return font.charWidth; }
    if (font.size == 16) { drawCharMono16(x, y, c, color, font.bold); return font.charWidth; }
    if (font.size == 18) { drawCharMono18(x, y, c, color, font.bold); return font.charWidth; }
    if (font.size == 24) { drawCharMono24(x, y, c, color, font.bold); return font.charWidth; }
    if (font.size == 25) { drawCharMono24BI(x, y, c, color); return font.charWidth; }
    EPD_ShowChar((uint16_t)x, (uint16_t)y, (uint16_t)c, font.size, color);
    return font.charWidth;
}

int16_t drawString(int16_t x, int16_t y, const char* str, const FontDef& font, uint8_t color) {
    int16_t curX = x;
    while (*str) {
        curX += drawChar(curX, y, *str, font, color);
        str++;
    }
    return curX;
}

int16_t drawStringWrapped(int16_t x, int16_t y, int16_t maxX, const char* str,
                          const FontDef& font, uint8_t color) {
    int16_t curX = x;
    int16_t curY = y;
    int16_t lineH = font.lineHeight + LINE_SPACING;
    int16_t wrapWidth = maxX - x;

    const char* wordStart = str;
    while (*wordStart) {
        // Find next word boundary
        const char* wordEnd = wordStart;
        while (*wordEnd && *wordEnd != ' ' && *wordEnd != '\n') wordEnd++;

        // Measure word width
        uint16_t wordW = 0;
        for (const char* p = wordStart; p < wordEnd; p++) {
            wordW += getCharWidth(*p, font);
        }

        // Check if word fits on current line
        if (curX + wordW > maxX && curX > x) {
            // Wrap to next line
            curX = x;
            curY += lineH;
        }

        // Draw the word
        for (const char* p = wordStart; p < wordEnd; p++) {
            curX += drawChar(curX, curY, *p, font, color);
        }

        // Handle the delimiter
        if (*wordEnd == ' ') {
            curX += getCharWidth(' ', font);
            wordStart = wordEnd + 1;
        } else if (*wordEnd == '\n') {
            curX = x;
            curY += lineH;
            wordStart = wordEnd + 1;
        } else {
            wordStart = wordEnd;  // End of string
        }
    }

    return curY + lineH;
}

// =============================================================================
// Two-pane menu system
// =============================================================================

void drawLeftPanel(const MenuItem* items, uint8_t count,
                   int8_t selectedIdx, int8_t activeIdx, bool isActive) {
    clearLeftPanel();

    int16_t y = MARGIN_Y;
    int16_t lineH = font_regular.lineHeight + LINE_SPACING + 4;

    for (uint8_t i = 0; i < count; i++) {
        if (!items[i].enabled) continue;

        int16_t x = MARGIN_X;
        uint8_t color = COL_BLACK;

        // Draw indicator
        if (isActive && i == selectedIdx) {
            // Active panel, current selection: show ">"
            drawChar(x, y, '>', font_regular, color);
        } else if (!isActive && i == activeIdx) {
            // Inactive panel, breadcrumb: show "*"
            drawChar(x, y, '*', font_regular, color);
        }
        // Otherwise: no indicator (blank space)

        // Draw label after indicator space
        x += INDICATOR_W;
        drawString(x, y, items[i].label, font_regular, color);

        y += lineH;
    }
}

void drawRightPreview(const PreviewLine* lines, uint8_t count) {
    clearRightPanel();

    int16_t y = MARGIN_Y;
    int16_t x = PANEL_DIVIDER_X + MARGIN_X;

    for (uint8_t i = 0; i < count; i++) {
        const FontDef& font = lines[i].font ? *lines[i].font : font_regular;
        int16_t lineH = font.lineHeight + LINE_SPACING + 2;

        drawString(x, y, lines[i].text, font, COL_BLACK);
        y += lineH;
    }
}

void drawRightMenu(const MenuItem* items, uint8_t count, int8_t selectedIdx) {
    clearRightPanel();

    int16_t y = MARGIN_Y;
    int16_t lineH = font_regular.lineHeight + LINE_SPACING + 4;

    for (uint8_t i = 0; i < count; i++) {
        if (!items[i].enabled) continue;

        int16_t x = PANEL_DIVIDER_X + MARGIN_X;

        // Draw ">" indicator for selected item
        if (i == selectedIdx) {
            drawChar(x, y, '>', font_regular, COL_BLACK);
        }

        x += INDICATOR_W;
        drawString(x, y, items[i].label, font_regular, COL_BLACK);

        y += lineH;
    }
}

void drawDivider() {
    Display::drawVLine(PANEL_DIVIDER_X, 0, EPD_HEIGHT, COL_BLACK);
}

// =============================================================================
// Reading view elements
// =============================================================================

void drawProgressBar(uint16_t currentPage, uint16_t totalPages) {
    // Use a fixed small font for the status bar so it never clips off screen
    static const FontDef barFont = { 12, 7, 12, 10, false };
    int16_t barY = EPD_HEIGHT - PROGRESS_BAR_H - 2;

    // Clear the progress bar region
    Display::fillRect(0, barY - 2, EPD_WIDTH, PROGRESS_BAR_H + 4, COL_WHITE);

    // Battery voltage on the left
    const char* batStr = Battery::voltageStr();
    int16_t batW = getStringWidth(batStr, barFont);
    drawString(MARGIN_X, barY, batStr, barFont, COL_BLACK);

    // Compute page text width so bar fits exactly between battery and page text
    char pageStr[16];
    snprintf(pageStr, sizeof(pageStr), "%d/%d", currentPage, totalPages);
    int16_t pageTextW = getStringWidth(pageStr, barFont);

    int16_t barX = MARGIN_X + batW + 3;
    int16_t barW = EPD_WIDTH - barX - 3 - pageTextW;

    // Draw bar outline
    Display::drawHLine(barX, barY, barW, COL_BLACK);
    Display::drawHLine(barX, barY + PROGRESS_BAR_H - 1, barW, COL_BLACK);
    Display::drawVLine(barX, barY, PROGRESS_BAR_H, COL_BLACK);
    Display::drawVLine(barX + barW - 1, barY, PROGRESS_BAR_H, COL_BLACK);

    // Fill progress
    if (totalPages > 0) {
        int16_t fillW = ((uint32_t)(currentPage) * (barW - 2)) / totalPages;
        Display::fillRect(barX + 1, barY + 1, fillW, PROGRESS_BAR_H - 2, COL_BLACK);
    }

    // Page number text flush to right edge
    drawString(EPD_WIDTH - pageTextW, barY, pageStr, barFont, COL_BLACK);
}

void drawBatteryTopRight() {
    const char* batStr = Battery::voltageStr();
    int16_t w = getStringWidth(batStr, font_regular);
    drawString(EPD_WIDTH - MARGIN_X - w, MARGIN_Y, batStr, font_regular, COL_BLACK);
}

void drawBatteryBottomRight() {
    const char* batStr = Battery::voltageStr();
    int16_t w = getStringWidth(batStr, font_regular);
    int16_t y = EPD_HEIGHT - MARGIN_Y - font_regular.lineHeight;
    drawString(EPD_WIDTH - MARGIN_X - w, y, batStr, font_regular, COL_BLACK);
}

void drawHorizontalRule(int16_t x, int16_t y, int16_t width) {
    Display::drawHLine(x, y, width, COL_BLACK);
    Display::drawHLine(x, y + 1, width, COL_BLACK);  // 2px thick
}

// =============================================================================
// Utility
// =============================================================================

void clearLeftPanel() {
    Display::fillRect(0, 0, PANEL_DIVIDER_X, EPD_HEIGHT, COL_WHITE);
}

void clearRightPanel() {
    Display::fillRect(PANEL_DIVIDER_X, 0, PANEL_RIGHT_W, EPD_HEIGHT, COL_WHITE);
}

void clearAll() {
    Display::clearBuffer();
}

void drawCenteredMessage(const char* msg, const FontDef& font) {
    clearAll();
    uint16_t textW = getStringWidth(msg, font);
    int16_t x = (EPD_WIDTH - textW) / 2;
    int16_t y = (EPD_HEIGHT - font.lineHeight) / 2;
    drawString(x, y, msg, font, COL_BLACK);
}

void drawConfirmDialog(const char* message, bool yesSelected) {
    // Draw a centered dialog box
    int16_t boxW = 300;
    int16_t boxH = 100;
    int16_t boxX = (EPD_WIDTH - boxW) / 2;
    int16_t boxY = (EPD_HEIGHT - boxH) / 2;

    // White background with black border
    Display::fillRect(boxX, boxY, boxW, boxH, COL_WHITE);
    Display::drawHLine(boxX, boxY, boxW, COL_BLACK);
    Display::drawHLine(boxX, boxY + boxH - 1, boxW, COL_BLACK);
    Display::drawVLine(boxX, boxY, boxH, COL_BLACK);
    Display::drawVLine(boxX + boxW - 1, boxY, boxH, COL_BLACK);

    // Message text
    drawString(boxX + 10, boxY + 10, message, font_regular, COL_BLACK);

    // Yes / No buttons
    int16_t btnY = boxY + boxH - 35;
    const char* yesLabel = "Yes";
    const char* noLabel = "No";

    if (yesSelected) {
        drawString(boxX + 40, btnY, "> ", font_regular, COL_BLACK);
        drawString(boxX + 56, btnY, yesLabel, font_bold, COL_BLACK);
        drawString(boxX + 180, btnY, "  ", font_regular, COL_BLACK);
        drawString(boxX + 196, btnY, noLabel, font_regular, COL_BLACK);
    } else {
        drawString(boxX + 40, btnY, "  ", font_regular, COL_BLACK);
        drawString(boxX + 56, btnY, yesLabel, font_regular, COL_BLACK);
        drawString(boxX + 180, btnY, "> ", font_regular, COL_BLACK);
        drawString(boxX + 196, btnY, noLabel, font_bold, COL_BLACK);
    }
}

}  // namespace UI
