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
    bool bold;           // Use bold glyph variant
};

extern FontDef font_large;           // H1 headers (scales with font setting)
extern FontDef font_medium;          // H2 headers (scales with font setting)
extern FontDef font_regular;         // body text (scales with font setting)
extern FontDef font_bold;            // bold (tracks font_regular)
extern const FontDef font_splash;    // size 25 — bold-oblique 24px (boot splash, fixed)

// Update all font globals based on the body font size and bold flag
void updateFontScale(uint8_t bodySize, bool bold = false);

uint16_t getStringWidth(const char* str, const FontDef& font);
uint8_t  getCharWidth(char c, const FontDef& font);
