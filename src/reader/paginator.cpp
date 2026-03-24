#include "paginator.h"
#include "../storage/sd_manager.h"
#include "../settings/settings.h"

// Read buffer size — read file in chunks to avoid loading entire book into RAM
#define READ_BUF_SIZE 4096

static PageInfo emptyPage = {0, 0};

bool Paginator::paginate(const char* filePath, int16_t renderWidth, int16_t renderHeight) {
    _filePath = filePath;
    _pages.clear();

    size_t fSize = SDManager::fileSize(filePath);
    if (fSize == 0) return false;

    uint8_t* buf = (uint8_t*)ps_malloc(READ_BUF_SIZE + 1);
    if (!buf) {
        buf = (uint8_t*)malloc(READ_BUF_SIZE + 1);
        if (!buf) return false;
    }

    size_t filePos = 0;         // Current read position in file
    size_t pageStartByte = 0;   // Byte offset where current page starts
    size_t lineStartByte = 0;   // Byte offset where current line starts
    int16_t yUsed = 0;          // Vertical space used on current page
    bool prevWasCR = false;     // Track \r across chunk boundaries

    // Line accumulator — we build lines character by character
    String currentLine;
    currentLine.reserve(256);

    while (filePos < fSize) {
        size_t toRead = min((size_t)READ_BUF_SIZE, fSize - filePos);
        size_t bytesRead = SDManager::readChunk(filePath, buf, filePos, toRead);
        if (bytesRead == 0) break;
        buf[bytesRead] = '\0';

        for (size_t i = 0; i < bytesRead; i++) {
            char c = (char)buf[i];
            size_t absPos = filePos + i;

            // Handle \r\n across chunk boundaries: if previous char was \r
            // and this char is \n, skip the \n (already processed the line break)
            if (prevWasCR && c == '\n') {
                prevWasCR = false;
                lineStartByte = absPos + 1;
                continue;
            }
            prevWasCR = false;

            if (c == '\n' || c == '\r') {
                if (c == '\r') {
                    prevWasCR = true;
                    // Check within same chunk for \r\n
                    if (i + 1 < bytesRead && buf[i + 1] == '\n') {
                        // Will skip the \n next iteration via prevWasCR
                    }
                }

                // End of line — calculate its rendered height
                int16_t lh = lineHeight(currentLine.c_str(), renderWidth);

                if (yUsed + lh > renderHeight && yUsed > 0) {
                    // This line doesn't fit — end current page before it
                    _pages.push_back({pageStartByte, lineStartByte - pageStartByte});
                    pageStartByte = lineStartByte;
                    yUsed = 0;
                }

                yUsed += lh;
                currentLine = "";
                // Next line starts after this newline (or after \r\n pair)
                lineStartByte = absPos + 1;

            } else {
                if (currentLine.length() == 0) {
                    lineStartByte = absPos;
                }
                currentLine += c;
            }
        }

        filePos += bytesRead;
    }

    // Handle any remaining text in the line buffer
    if (currentLine.length() > 0) {
        int16_t lh = lineHeight(currentLine.c_str(), renderWidth);
        if (yUsed + lh > renderHeight && yUsed > 0) {
            _pages.push_back({pageStartByte, lineStartByte - pageStartByte});
            pageStartByte = lineStartByte;
        }
    }

    // Final page: whatever's left
    if (pageStartByte < fSize) {
        _pages.push_back({pageStartByte, fSize - pageStartByte});
    }

    free(buf);

    // Remove trailing empty/whitespace-only pages
    while (_pages.size() > 1) {
        const PageInfo& last = _pages.back();
        if (last.byteLength == 0) {
            _pages.pop_back();
            continue;
        }
        // Check if last page is only whitespace
        uint8_t check[64];
        size_t checkLen = min(last.byteLength, (size_t)64);
        size_t rd = SDManager::readChunk(filePath, check, last.byteOffset, checkLen);
        bool allWhitespace = true;
        for (size_t i = 0; i < rd; i++) {
            if (check[i] != ' ' && check[i] != '\n' && check[i] != '\r' && check[i] != '\t') {
                allWhitespace = false;
                break;
            }
        }
        if (allWhitespace && checkLen == last.byteLength) {
            _pages.pop_back();
        } else {
            break;
        }
    }

    Serial.printf("[Paginator] %s: %d pages, %d bytes\n",
                  filePath, _pages.size(), fSize);
    return _pages.size() > 0;
}

const PageInfo& Paginator::getPage(uint16_t pageNum) const {
    if (pageNum < _pages.size()) return _pages[pageNum];
    return emptyPage;
}

uint16_t Paginator::pageForByteOffset(size_t offset) const {
    if (_pages.empty()) return 0;
    // Binary search for the page containing this offset
    uint16_t lo = 0, hi = _pages.size() - 1;
    while (lo < hi) {
        uint16_t mid = lo + (hi - lo + 1) / 2;
        if (_pages[mid].byteOffset <= offset) lo = mid;
        else hi = mid - 1;
    }
    return lo;
}

const FontDef& Paginator::fontForLine(const char* line) const {
    // Check Markdown prefixes
    if (strncmp(line, "## ", 3) == 0) return font_medium;
    if (strncmp(line, "# ", 2) == 0)  return font_large;
    return font_regular;
}

int16_t Paginator::lineHeight(const char* line, int16_t maxWidth) const {
    // Empty line = one regular line height (paragraph spacing)
    if (line[0] == '\0') {
        return font_regular.lineHeight + LINE_SPACING;
    }

    // Horizontal rule
    if (strcmp(line, "---") == 0 || strcmp(line, "***") == 0 || strcmp(line, "___") == 0) {
        return font_regular.lineHeight;  // Fixed height for HR
    }

    const FontDef& font = fontForLine(line);
    const char* text = line;

    // Skip Markdown prefix for width calculation
    if (strncmp(text, "## ", 3) == 0) text += 3;
    else if (strncmp(text, "# ", 2) == 0) text += 2;

    int16_t numLines = wrappedLineCount(text, font, maxWidth);
    return numLines * (font.lineHeight + LINE_SPACING);
}

int16_t Paginator::wrappedLineCount(const char* text, const FontDef& font, int16_t maxWidth) const {
    if (text[0] == '\0') return 1;

    int16_t lines = 1;
    int16_t curX = 0;
    const char* p = text;

    while (*p) {
        // Find next word boundary
        const char* wordStart = p;
        while (*p && *p != ' ') p++;

        // Measure word width
        uint16_t wordW = 0;
        for (const char* c = wordStart; c < p; c++) {
            wordW += getCharWidth(*c, font);
        }

        // Check if word fits on current line
        if (curX + wordW > maxWidth && curX > 0) {
            lines++;
            curX = 0;
        }

        curX += wordW;

        // Skip space
        if (*p == ' ') {
            curX += getCharWidth(' ', font);
            p++;
        }
    }

    return lines;
}
