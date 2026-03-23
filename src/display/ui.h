#pragma once

#include <Arduino.h>
#include "../config.h"
#include "../fonts/fonts.h"

// =============================================================================
// UI drawing helpers — two-pane menu system + reading view primitives
// =============================================================================

// A menu item for the left or right panel
struct MenuItem {
    const char* label;
    bool enabled;
    MenuItem() : label(""), enabled(true) {}
    MenuItem(const char* l, bool e = true) : label(l), enabled(e) {}
};

// Content block for the right preview panel
struct PreviewLine {
    const char* text;
    const FontDef* font;    // nullptr = use font_regular
};

namespace UI {

// --- Text rendering ----------------------------------------------------------

// Draw a single character at (x,y) using the given font, returns advance width
uint8_t drawChar(int16_t x, int16_t y, char c, const FontDef& font, uint8_t color = COL_BLACK);

// Draw a string at (x,y), returns the x position after the last character
int16_t drawString(int16_t x, int16_t y, const char* str, const FontDef& font, uint8_t color = COL_BLACK);

// Draw a string word-wrapped within [x, maxX], starting at y.
// Returns the y position after the last line.
int16_t drawStringWrapped(int16_t x, int16_t y, int16_t maxX, const char* str,
                          const FontDef& font, uint8_t color = COL_BLACK);

// --- Two-pane menu system ----------------------------------------------------

// Draw the left panel with a list of menu items.
// selectedIdx: which item has the cursor
// activeIdx: which item has the breadcrumb (*), -1 if none
// isActive: true if left panel is the active panel (shows >), false (shows nothing or *)
void drawLeftPanel(const MenuItem* items, uint8_t count,
                   int8_t selectedIdx, int8_t activeIdx, bool isActive);

// Draw the right panel as a preview (no > indicator, just content lines)
void drawRightPreview(const PreviewLine* lines, uint8_t count);

// Draw the right panel as an interactive submenu (with > indicator)
void drawRightMenu(const MenuItem* items, uint8_t count, int8_t selectedIdx);

// Draw a vertical divider line between the two panels
void drawDivider();

// --- Reading view elements ---------------------------------------------------

// Draw the progress bar at the bottom of the screen
// currentPage and totalPages are 1-based
void drawProgressBar(uint16_t currentPage, uint16_t totalPages);

// Draw a horizontal rule (full width or within a panel)
void drawHorizontalRule(int16_t x, int16_t y, int16_t width);

// --- Utility -----------------------------------------------------------------

// Clear only the left panel region in the buffer
void clearLeftPanel();

// Clear only the right panel region in the buffer
void clearRightPanel();

// Clear the full screen buffer
void clearAll();

// Draw a simple centered message on the full screen (for loading, errors, etc.)
void drawCenteredMessage(const char* msg, const FontDef& font);

// Draw a confirmation dialog (e.g., "Delete book?") with Yes/No options
// Returns nothing — the view handles input; this just draws.
void drawConfirmDialog(const char* message, bool yesSelected);

}  // namespace UI
