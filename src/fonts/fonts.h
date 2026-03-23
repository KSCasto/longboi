#pragma once

#include <Arduino.h>

// =============================================================================
// Font system — wraps Elecrow EPDfont.h built-in bitmap fonts
//
// Available sizes (from EPDfont.h):
//   12 → 6px wide,  12px tall
//   16 → 8px wide,  16px tall
//   24 → 12px wide, 24px tall
//   48 → 24px wide, 48px tall
//
// EPD_ShowChar(x, y, chr, size, color) draws one character.
// Both are called from ui.cpp — no custom bitmap data needed.
// =============================================================================

struct FontDef {
    uint8_t size;        // Elecrow font size (12/16/24/48)
    uint8_t charWidth;   // Pixel width per character = size / 2
    uint8_t lineHeight;  // Pixel height per character = size
    uint8_t baseline;    // Approximate baseline offset
};

extern const FontDef font_large;    // size 24 — H1 headers
extern const FontDef font_medium;   // size 16 — H2 headers
extern const FontDef font_regular;  // size 12 — body text
extern const FontDef font_bold;     // size 12 — bold (same metrics, semantic marker only)

uint16_t getStringWidth(const char* str, const FontDef& font);
uint8_t  getCharWidth(char c, const FontDef& font);
