#include "renderer.h"
#include "../display/ui.h"
#include "../display/display.h"
#include "../settings/settings.h"

namespace Renderer {

// Draw a line that may contain **bold** segments intermixed with regular text.
// Uses word wrapping: words that don't fit on the current line are moved to
// the next line. Words wider than the full line are character-wrapped.
static int16_t renderBoldLine(const char* text, int16_t x, int16_t y,
                              int16_t maxX, const FontDef& baseFont) {
    int16_t curX = x;
    int16_t curY = y;
    int16_t lineH = baseFont.lineHeight + LINE_SPACING;
    int16_t lineWidth = maxX - x;
    bool inBold = false;

    const char* p = text;

    while (*p) {
        // Handle ** markers — toggle bold, don't render
        if (p[0] == '*' && p[1] == '*') {
            inBold = !inBold;
            p += 2;
            continue;
        }

        // Handle spaces
        if (*p == ' ') {
            uint8_t cw = getCharWidth(' ', baseFont);
            if (curX + cw > maxX && curX > x) {
                // Space at end of line — wrap, skip drawing the space
                curX = x;
                curY += lineH;
            } else {
                const FontDef& font = inBold ? font_bold : baseFont;
                curX += UI::drawChar(curX, curY, ' ', font, COL_BLACK);
            }
            p++;
            continue;
        }

        // Non-space: measure the full word (skipping ** markers within)
        const char* wordEnd = p;
        int16_t wordWidth = 0;
        bool tempBold = inBold;
        while (*wordEnd && *wordEnd != ' ') {
            if (wordEnd[0] == '*' && wordEnd[1] == '*') {
                tempBold = !tempBold;
                wordEnd += 2;
                continue;
            }
            const FontDef& font = tempBold ? font_bold : baseFont;
            wordWidth += getCharWidth(*wordEnd, font);
            wordEnd++;
        }

        // Word doesn't fit on current line but fits on a full line — wrap first
        if (curX + wordWidth > maxX && curX > x && wordWidth <= lineWidth) {
            curX = x;
            curY += lineH;
        }

        // Draw the word character by character (handles words wider than line)
        while (p < wordEnd) {
            if (p[0] == '*' && p[1] == '*') {
                inBold = !inBold;
                p += 2;
                continue;
            }
            const FontDef& font = inBold ? font_bold : baseFont;
            uint8_t cw = getCharWidth(*p, font);
            if (curX + cw > maxX && curX > x) {
                curX = x;
                curY += lineH;
            }
            curX += UI::drawChar(curX, curY, *p, font, COL_BLACK);
            p++;
        }
    }

    return curY + lineH;
}

int16_t renderLine(const char* line, int16_t x, int16_t y, int16_t maxX) {
    const FontDef& bodyFont = Settings::bodyFont();

    // Empty line — paragraph spacing
    if (line[0] == '\0') {
        return y + bodyFont.lineHeight + LINE_SPACING;
    }

    // Horizontal rule: ---  ***  ___
    if (strcmp(line, "---") == 0 || strcmp(line, "***") == 0 || strcmp(line, "___") == 0) {
        int16_t ruleY = y + bodyFont.lineHeight / 2;
        UI::drawHorizontalRule(x, ruleY, maxX - x);
        return y + bodyFont.lineHeight + LINE_SPACING;
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
    return renderBoldLine(line, x, y, maxX, bodyFont);
}

int16_t renderPage(const char* text, int16_t x, int16_t y, int16_t maxX, int16_t maxY) {
    int16_t curY = y;
    const char* p = text;

    // Static buffer — sized for reflowed paragraphs (multiple source lines joined)
    static char lineBuf[4096];

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
