#include "menu_reading.h"
#include "../display/display.h"
#include "../display/ui.h"
#include "../input/input.h"
#include "../bookmarks/bookmarks.h"
#include "../storage/library.h"
#include "../fonts/fonts.h"

namespace MenuReading {

ReadingMenuResult run(const char* bookFilename, uint16_t currentPage) {
    MenuItem items[] = {
        {"Add Bookmark", true},
        {"View Bookmarks", true},
        {"Mark as Read", true},
        {"Back to Library", true},
    };
    int itemCount = 4;
    int8_t selected = 0;

    // Draw as a centered overlay (full-width single pane for reading context)
    auto drawOverlay = [&]() {
        Display::clearBuffer();

        // Title
        UI::drawString(MARGIN_X, MARGIN_Y, bookFilename, font_medium, COL_BLACK);

        // Menu items with > indicator
        int16_t y = MARGIN_Y + font_medium.lineHeight + LINE_SPACING + 10;
        int16_t lineH = font_regular.lineHeight + LINE_SPACING + 4;

        for (int i = 0; i < itemCount; i++) {
            int16_t x = MARGIN_X + 20;
            if (i == selected) {
                UI::drawChar(x, y, '>', font_regular, COL_BLACK);
            }
            x += INDICATOR_W;
            UI::drawString(x, y, items[i].label, font_regular, COL_BLACK);
            y += lineH;
        }
    };

    drawOverlay();
    Display::update(true);

    while (true) {
        Event e = Input::poll();
        if (e == Event::NONE) { delay(10); continue; }

        switch (e) {
            case Event::SCROLL_UP:
                if (selected > 0) {
                    selected--;
                    drawOverlay();
                    Display::update();
                }
                break;

            case Event::SCROLL_DOWN:
                if (selected < itemCount - 1) {
                    selected++;
                    drawOverlay();
                    Display::update();
                }
                break;

            case Event::SELECT:
                switch (selected) {
                    case 0: {
                        // Add Bookmark — save with auto-name
                        Bookmarks::save(bookFilename, currentPage);
                        UI::drawCenteredMessage("Bookmark saved!", font_regular);
                        Display::update(true);
                        delay(800);
                        return ReadingMenuResult::RESUME;
                    }
                    case 1: return ReadingMenuResult::VIEW_BOOKMARKS;
                    case 2: return ReadingMenuResult::MARK_AS_READ;
                    case 3: return ReadingMenuResult::BACK_TO_LIBRARY;
                }
                break;

            case Event::EXIT:
                return ReadingMenuResult::RESUME;

            default: break;
        }
    }
}

}  // namespace MenuReading
