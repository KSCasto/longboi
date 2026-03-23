#pragma once

#include <Arduino.h>

// =============================================================================
// In-reading overlay menu — shown when MENU is pressed during reading
// =============================================================================

enum class ReadingMenuResult {
    RESUME,         // Dismiss overlay, continue reading
    ADD_BOOKMARK,   // Add bookmark at current page
    VIEW_BOOKMARKS, // Open bookmarks for this book
    MARK_AS_READ,   // Mark book as read and exit
    BACK_TO_LIBRARY,// Exit reader, go to library
};

namespace MenuReading {

// Run the in-reading overlay menu.
// bookFilename: current book being read
// currentPage: current page number for bookmark creation
ReadingMenuResult run(const char* bookFilename, uint16_t currentPage);

}  // namespace MenuReading
