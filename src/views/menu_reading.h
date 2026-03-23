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
    GO_TO_PAGE,     // Jump to a specific page
    OPEN_SETTINGS,  // Open settings menu
    MAIN_MENU,      // Return to main menu
};

namespace MenuReading {

// Run the in-reading overlay menu.
// bookFilename: current book being read
// currentPage: current page number (0-indexed) for bookmark creation
// totalPages: total page count for the go-to-page picker
ReadingMenuResult run(const char* bookFilename, uint16_t currentPage,
                      uint16_t totalPages);

// After GO_TO_PAGE result, get the chosen page (0-indexed)
uint16_t getChosenPage();

}  // namespace MenuReading
