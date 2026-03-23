#pragma once

#include <Arduino.h>

// =============================================================================
// View results — returned by each view to tell the state machine what's next
// =============================================================================

enum class ViewResult {
    MAIN_MENU,          // Go to main menu
    OPEN_LIBRARY,       // Open library browser
    OPEN_BOOKMARKS,     // Open bookmarks browser
    OPEN_UPLOAD,        // Start WiFi upload mode
    OPEN_SETTINGS,      // Open settings (placeholder)
    OPEN_BOOK,          // Open a book in the reader (bookToOpen is set)
    RESUME_READING,     // Resume last-read book
    BACK,               // Go back to previous view
};

// Global: which book should be opened when ViewResult::OPEN_BOOK is returned
extern String g_bookToOpen;
extern uint16_t g_pageToOpen;  // -1 means use saved page
