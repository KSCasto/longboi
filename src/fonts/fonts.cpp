#include "fonts.h"

// DejaVu Sans Mono bitmap fonts — scaled by updateFontScale()
// Default to small (12px body), not bold
FontDef font_large   = { 24, 14, 24, 20, false };
FontDef font_medium  = { 16,  8, 16, 13, false };
FontDef font_regular = { 12,  7, 12, 10, false };
FontDef font_bold    = { 12,  7, 12, 10, true  };

// Boot splash — fixed, never changes
const FontDef font_splash = { 25, 15, 24, 20, true };

static const FontDef FONT_12     = { 12,  7, 12, 10, false };
static const FontDef FONT_12B    = { 12,  7, 12, 10, true  };
static const FontDef FONT_16     = { 16,  8, 16, 13, false };
static const FontDef FONT_16B    = { 16,  8, 16, 13, true  };
static const FontDef FONT_18     = { 18, 10, 18, 15, false };
static const FontDef FONT_18B    = { 18, 10, 18, 15, true  };
static const FontDef FONT_24     = { 24, 14, 24, 20, false };
static const FontDef FONT_24B    = { 24, 14, 24, 20, true  };

void updateFontScale(uint8_t bodySize, bool bold) {
    if (bodySize == 18) {
        font_regular = bold ? FONT_18B : FONT_18;
        font_bold    = FONT_18B;
        font_medium  = bold ? FONT_24B : FONT_24;
        font_large   = bold ? FONT_24B : FONT_24;
    } else if (bodySize == 16) {
        font_regular = bold ? FONT_16B : FONT_16;
        font_bold    = FONT_16B;
        font_medium  = bold ? FONT_24B : FONT_24;
        font_large   = bold ? FONT_24B : FONT_24;
    } else {
        font_regular = bold ? FONT_12B : FONT_12;
        font_bold    = FONT_12B;
        font_medium  = bold ? FONT_16B : FONT_16;
        font_large   = bold ? FONT_24B : FONT_24;
    }
}

uint8_t getCharWidth(char c, const FontDef& font) {
    return font.charWidth;
}

uint16_t getStringWidth(const char* str, const FontDef& font) {
    return strlen(str) * font.charWidth;
}
