#include "menu_reading.h"
#include "../display/display.h"
#include "../display/ui.h"
#include "../input/input.h"
#include "../bookmarks/bookmarks.h"
#include "../storage/library.h"
#include "../fonts/fonts.h"

static uint16_t s_chosenPage = 0;

// Page number picker — returns 0-indexed page
// MENU/EXIT: hold to scroll continuously. SCROLL: ±1. SELECT: confirm.
static uint16_t runPagePicker(uint16_t currentPage, uint16_t totalPages) {
    uint16_t page = currentPage + 1;  // Display 1-indexed

    auto drawPicker = [&]() {
        Display::clearBuffer();
        char buf[32];
        const char* title = "Go to Page";
        int16_t tw = getStringWidth(title, font_medium);
        UI::drawString((EPD_WIDTH - tw) / 2, 40, title, font_medium);
        snprintf(buf, sizeof(buf), "%d", page);
        int16_t nw = getStringWidth(buf, font_large);
        UI::drawString((EPD_WIDTH - nw) / 2, 90, buf, font_large);
        snprintf(buf, sizeof(buf), "of %d", totalPages);
        int16_t ow = getStringWidth(buf, font_regular);
        UI::drawString((EPD_WIDTH - ow) / 2, 130, buf, font_regular);
        UI::drawString((EPD_WIDTH - nw) / 2 - 20, 90, "<", font_large);
        UI::drawString((EPD_WIDTH + nw) / 2 + 8, 90, ">", font_large);
    };

    drawPicker();
    Display::update(true);

    while (true) {
        // Check raw GPIO before event poll for hold-to-repeat
        if (Input::isHeld(Event::MENU)) {
            if (page < totalPages) page++;
            drawPicker();
            Display::update(false, true);
            delay(100);
            continue;
        }
        if (Input::isHeld(Event::EXIT)) {
            if (page > 1) page--;
            drawPicker();
            Display::update(false, true);
            delay(100);
            continue;
        }

        Event e = Input::poll();
        if (e == Event::NONE) { Input::lightSleep(); continue; }

        switch (e) {
            case Event::SCROLL_UP:
                if (page > 1) page--;
                drawPicker();
                Display::update(false, true);
                break;
            case Event::SCROLL_DOWN:
                if (page < totalPages) page++;
                drawPicker();
                Display::update(false, true);
                break;
            case Event::SELECT:
            case Event::MENU:
                return page - 1;  // Convert back to 0-indexed
            default: break;
        }
    }
}

namespace MenuReading {

ReadingMenuResult run(const char* bookFilename, uint16_t currentPage,
                      uint16_t totalPages) {
    MenuItem items[] = {
        {"Add Bookmark", true},
        {"View Bookmarks", true},
        {"Go to Page", true},
        {"Mark as Read", true},
        {"Settings", true},
        {"Back to Library", true},
        {"Main Menu", true},
    };
    int itemCount = 7;
    int8_t selected = 0;

    auto drawOverlay = [&]() {
        Display::clearBuffer();

        // Title
        UI::drawString(MARGIN_X, MARGIN_Y, bookFilename, font_medium, COL_BLACK);

        // Page info
        char pageInfo[32];
        snprintf(pageInfo, sizeof(pageInfo), "Page %d of %d", currentPage + 1, totalPages);
        UI::drawString(MARGIN_X, MARGIN_Y + font_medium.lineHeight + 4,
                       pageInfo, font_regular, COL_BLACK);

        // Menu items with > indicator
        int16_t y = MARGIN_Y + font_medium.lineHeight + font_regular.lineHeight + LINE_SPACING + 14;
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
        if (e == Event::NONE) { Input::lightSleep(); continue; }

        switch (e) {
            case Event::SCROLL_UP:
                selected = (selected > 0) ? selected - 1 : itemCount - 1;
                drawOverlay();
                Display::update();
                break;

            case Event::SCROLL_DOWN:
                selected = (selected < itemCount - 1) ? selected + 1 : 0;
                drawOverlay();
                Display::update();
                break;

            case Event::SELECT:
            case Event::MENU:
                switch (selected) {
                    case 0: {
                        // Add Bookmark
                        Bookmarks::save(bookFilename, currentPage);
                        UI::drawCenteredMessage("Bookmark saved!", font_regular);
                        Display::update(true);
                        delay(800);
                        return ReadingMenuResult::RESUME;
                    }
                    case 1: return ReadingMenuResult::VIEW_BOOKMARKS;
                    case 2: {
                        // Go to Page
                        s_chosenPage = runPagePicker(currentPage, totalPages);
                        return ReadingMenuResult::GO_TO_PAGE;
                    }
                    case 3: return ReadingMenuResult::MARK_AS_READ;
                    case 4: return ReadingMenuResult::OPEN_SETTINGS;
                    case 5: return ReadingMenuResult::BACK_TO_LIBRARY;
                    case 6: return ReadingMenuResult::MAIN_MENU;
                }
                break;

            case Event::EXIT:
                return ReadingMenuResult::RESUME;

            default: break;
        }
    }
}

uint16_t getChosenPage() {
    return s_chosenPage;
}

}  // namespace MenuReading
