#include "renderer.h"
#include "../display/ui.h"
#include "../display/display.h"

namespace Renderer {

// Draw a line that may contain **bold** segments intermixed with regular text
static int16_t renderBoldLine(const char* text, int16_t x, int16_t y,
                              int16_t maxX, const FontDef& baseFont) {
    int16_t curX = x;
    int16_t curY = y;
    int16_t lineH = baseFont.lineHeight + LINE_SPACING;
    bool inBold = false;

    const char* p = text;
    const char* segStart = p;

    while (*p) {
        // Check for ** delimiter
        if (p[0] == '*' && p[1] == '*') {
            // Render accumulated segment
            if (p > segStart) {
                const FontDef& font = inBold ? font_bold : baseFont;
                for (const char* c = segStart; c < p; c++) {
                    uint8_t cw = getCharWidth(*c, font);
                    if (curX + cw > maxX) {
                        curX = x;
                        curY += lineH;
                    }
                    curX += UI::drawChar(curX, curY, *c, font, COL_BLACK);
                }
            }
            inBold = !inBold;
            p += 2;
            segStart = p;
            continue;
        }

        // Word wrap check
        uint8_t cw = getCharWidth(*p, inBold ? font_bold : baseFont);
        if (curX + cw > maxX && curX > x) {
            // Render what we have, then wrap
            const FontDef& font = inBold ? font_bold : baseFont;
            for (const char* c = segStart; c < p; c++) {
                uint8_t w = getCharWidth(*c, font);
                if (curX + w > maxX) {
                    curX = x;
                    curY += lineH;
                }
                curX += UI::drawChar(curX, curY, *c, font, COL_BLACK);
            }
            segStart = p;
        }

        p++;
    }

    // Render remaining segment
    if (p > segStart) {
        const FontDef& font = inBold ? font_bold : baseFont;
        for (const char* c = segStart; c < p; c++) {
            uint8_t cw = getCharWidth(*c, font);
            if (curX + cw > maxX) {
                curX = x;
                curY += lineH;
            }
            curX += UI::drawChar(curX, curY, *c, font, COL_BLACK);
        }
    }

    return curY + lineH;
}

int16_t renderLine(const char* line, int16_t x, int16_t y, int16_t maxX) {
    // Empty line — paragraph spacing
    if (line[0] == '\0') {
        return y + font_regular.lineHeight + LINE_SPACING;
    }

    // Horizontal rule: ---  ***  ___
    if (strcmp(line, "---") == 0 || strcmp(line, "***") == 0 || strcmp(line, "___") == 0) {
        int16_t ruleY = y + font_regular.lineHeight / 2;
        UI::drawHorizontalRule(x, ruleY, maxX - x);
        return y + font_regular.lineHeight + LINE_SPACING;
    }

    // H1: # Title
    if (strncmp(line, "# ", 2) == 0) {
        const char* text = line + 2;
        return renderBoldLine(text, x, y, maxX, font_large);
    }

    // H2: ## Title
    if (strncmp(line, "## ", 3) == 0) {
        const char* text = line + 3;
        return renderBoldLine(text, x, y, maxX, font_medium);
    }

    // Regular text (may contain **bold** segments)
    return renderBoldLine(line, x, y, maxX, font_regular);
}

int16_t renderPage(const char* text, int16_t x, int16_t y, int16_t maxX, int16_t maxY) {
    int16_t curY = y;
    const char* p = text;

    // Static buffer to avoid alloca stack overflow on long lines
    static char lineBuf[512];

    // Skip UTF-8 BOM if present at start
    if ((uint8_t)p[0] == 0xEF && (uint8_t)p[1] == 0xBB && (uint8_t)p[2] == 0xBF) {
        p += 3;
    }

    // Process line by line
    while (*p && curY < maxY) {
        // Find end of current line
        const char* lineEnd = p;
        while (*lineEnd && *lineEnd != '\n') lineEnd++;

        // Copy line into fixed buffer, stripping non-ASCII bytes
        size_t srcLen = lineEnd - p;
        size_t j = 0;
        for (size_t i = 0; i < srcLen && j < sizeof(lineBuf) - 1; i++) {
            uint8_t ch = (uint8_t)p[i];
            if (ch >= 0x80) {
                // Skip multi-byte UTF-8 sequences
                if (ch >= 0xF0) i += 3;       // 4-byte
                else if (ch >= 0xE0) i += 2;  // 3-byte (includes BOM)
                else if (ch >= 0xC0) i += 1;  // 2-byte
                continue;
            }
            if (ch == '\r') continue;  // Strip \r
            lineBuf[j++] = (char)ch;
        }
        lineBuf[j] = '\0';

        curY = renderLine(lineBuf, x, curY, maxX);

        // Move past the newline
        p = lineEnd;
        if (*p == '\n') p++;
    }

    return curY;
}

}  // namespace Renderer
