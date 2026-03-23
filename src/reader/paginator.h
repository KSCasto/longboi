#pragma once

#include <Arduino.h>
#include <vector>
#include "../config.h"
#include "../fonts/fonts.h"

// =============================================================================
// Paginator — pre-calculates page boundaries for a book file
//
// Scans the entire file simulating word-wrap and Markdown formatting to build
// an array of byte offsets. pageOffsets[N] = byte position where page N starts.
// =============================================================================

struct PageInfo {
    size_t byteOffset;   // Start byte in the file
    size_t byteLength;   // Number of bytes on this page
};

class Paginator {
public:
    // Paginate a file. Call once when opening a book.
    // renderWidth: pixel width available for text (full screen minus margins)
    // renderHeight: pixel height available for text (screen height minus progress bar)
    // Returns true on success.
    bool paginate(const char* filePath, int16_t renderWidth, int16_t renderHeight);

    // Total number of pages
    uint16_t totalPages() const { return _pages.size(); }

    // Get info for a specific page (0-indexed)
    const PageInfo& getPage(uint16_t pageNum) const;

    // Get the file path that was paginated
    const char* getFilePath() const { return _filePath.c_str(); }

    // Find the page containing a given byte offset (binary search)
    uint16_t pageForByteOffset(size_t offset) const;

private:
    String _filePath;
    std::vector<PageInfo> _pages;

    // Determine which font a line uses based on Markdown prefix
    const FontDef& fontForLine(const char* line) const;

    // Calculate the height a line will consume after word-wrapping
    int16_t lineHeight(const char* line, int16_t maxWidth) const;

    // Count wrapped lines for a string in a given font within maxWidth
    int16_t wrappedLineCount(const char* text, const FontDef& font, int16_t maxWidth) const;
};
