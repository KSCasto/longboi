#include "fonts.h"

// Elecrow font metrics: charWidth = size/2, lineHeight = size
const FontDef font_large   = { 24, 12, 24, 20 };  // H1
const FontDef font_medium  = { 16,  8, 16, 13 };  // H2
const FontDef font_regular = { 12,  6, 12, 10 };  // body
const FontDef font_bold    = { 12,  6, 12, 10 };  // bold (same size as regular)

uint8_t getCharWidth(char c, const FontDef& font) {
    return font.charWidth;
}

uint16_t getStringWidth(const char* str, const FontDef& font) {
    return strlen(str) * font.charWidth;
}
