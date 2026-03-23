#pragma once

#include <Arduino.h>
#include "paginator.h"
#include "../config.h"

// =============================================================================
// Reader — reading view controller
//
// Manages the full-screen reading experience: open a book, paginate, render
// pages, handle navigation, track progress.
// =============================================================================

// Result of the reader's run loop — tells the state machine what to do next
enum class ReaderResult {
    EXIT_TO_MENU,       // User pressed MENU and chose to exit
    BOOK_FINISHED,      // Reached last page — show completion screen
    OPEN_READING_MENU,  // User pressed MENU — caller should show reading menu
};

namespace Reader {

// Open a book for reading. Runs pagination (shows loading screen).
// Restores last page from library.json.
// Returns false if file can't be opened or paginated.
bool open(const char* filename);

// Render the current page to the display buffer and refresh.
void renderCurrentPage();

// Navigate to next page. Returns false if already on last page.
bool nextPage();

// Navigate to previous page. Returns false if already on first page.
bool prevPage();

// Jump to a specific page (0-indexed). Returns false if out of range.
bool goToPage(uint16_t page);

// Get current page number (0-indexed)
uint16_t currentPage();

// Get total page count
uint16_t totalPages();

// Get the filename of the currently open book
const char* currentBook();

// Check if we're on the last page
bool isLastPage();

// Force next render to use full refresh (call after overlay menus)
void forceFullRefresh();

// Close the book — saves current page to library.json
void close();

// Main reading loop — handles input and page rendering.
// Returns when the user exits or finishes the book.
ReaderResult run();

}  // namespace Reader
