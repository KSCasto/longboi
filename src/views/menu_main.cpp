#include "menu_main.h"
#include "../display/display.h"
#include "../display/ui.h"
#include "../input/input.h"
#include "../storage/library.h"
#include "../bookmarks/bookmarks.h"
#include "../settings/settings.h"
#include "../fonts/fonts.h"

namespace MenuMain {

// Build the right-panel preview content for the currently highlighted menu item
static void drawPreviewForItem(int8_t idx, bool hasResume, const BookEntry* resumeBook) {
    // Adjust index if resume is hidden
    int actualIdx = idx;
    if (!hasResume) actualIdx = idx + 1;  // Skip resume slot

    switch (actualIdx) {
        case 0: {
            // Resume — show book details
            if (resumeBook) {
                char pageLine[48];
                snprintf(pageLine, sizeof(pageLine), "Page %d", resumeBook->page + 1);
                char statusLine[32];
                snprintf(statusLine, sizeof(statusLine), "Status: %s",
                         Library::statusToString(resumeBook->status));

                PreviewLine lines[] = {
                    {resumeBook->filename.c_str(), &font_medium},
                    {pageLine, nullptr},
                    {statusLine, nullptr},
                };
                UI::drawRightPreview(lines, 3);
            }
            break;
        }
        case 1: {
            // Library — show book counts
            char unreadLine[32], readingLine[32], readLine[32];
            snprintf(unreadLine, sizeof(unreadLine), "%d unread",
                     Library::countByStatus(BookStatus::UNREAD));
            snprintf(readingLine, sizeof(readingLine), "%d reading",
                     Library::countByStatus(BookStatus::READING));
            snprintf(readLine, sizeof(readLine), "%d read",
                     Library::countByStatus(BookStatus::READ_DONE));

            PreviewLine lines[] = {
                {"Library", &font_medium},
                {unreadLine, nullptr},
                {readingLine, nullptr},
                {readLine, nullptr},
            };
            UI::drawRightPreview(lines, 4);
            break;
        }
        case 2: {
            // Bookmarks — show summary
            auto booksWithBm = Bookmarks::booksWithBookmarks();
            char countLine[48];
            snprintf(countLine, sizeof(countLine), "%d books with bookmarks",
                     (int)booksWithBm.size());

            PreviewLine lines[] = {
                {"Bookmarks", &font_medium},
                {countLine, nullptr},
            };
            UI::drawRightPreview(lines, 2);
            break;
        }
        case 3: {
            // Upload
            PreviewLine lines[] = {
                {"WiFi Upload", &font_medium},
                {"Creates a WiFi network.", nullptr},
                {"Connect and open", nullptr},
                {AP_IP, nullptr},
                {"to upload books.", nullptr},
            };
            UI::drawRightPreview(lines, 5);
            break;
        }
        case 4: {
            // Settings — show current values
            static char fontLine[24], boldLine[24], spaceLine[24], refreshLine[24];
            static char scrollLine[24], resumeLine[24], sleepLine[24];

            const char* fName = "Small";
            if (Settings::fontSize() == 16) fName = "Medium";
            else if (Settings::fontSize() == 18) fName = "Large";
            snprintf(fontLine, sizeof(fontLine), "Font: %s", fName);
            snprintf(boldLine, sizeof(boldLine), "Bold: %s",
                     Settings::boldEnabled() ? "On" : "Off");
            const char* spNames[] = {"Tight", "Normal", "Loose"};
            snprintf(spaceLine, sizeof(spaceLine), "Spacing: %s",
                     spNames[Settings::lineSpacing()]);
            snprintf(refreshLine, sizeof(refreshLine), "Refresh: %d pages",
                     Settings::refreshInterval());
            snprintf(scrollLine, sizeof(scrollLine), "Scroll: %s",
                     Settings::invertScroll() ? "Inverted" : "Normal");
            snprintf(resumeLine, sizeof(resumeLine), "Auto Resume: %s",
                     Settings::autoResume() ? "On" : "Off");
            uint8_t sl = Settings::autoSleepMinutes();
            if (sl == 0)
                snprintf(sleepLine, sizeof(sleepLine), "Auto Sleep: Off");
            else
                snprintf(sleepLine, sizeof(sleepLine), "Auto Sleep: %dm", sl);

            PreviewLine lines[] = {
                {"Settings", &font_medium},
                {fontLine, nullptr},
                {boldLine, nullptr},
                {spaceLine, nullptr},
                {refreshLine, nullptr},
                {scrollLine, nullptr},
                {resumeLine, nullptr},
                {sleepLine, nullptr},
            };
            UI::drawRightPreview(lines, 8);
            break;
        }
    }
}

ViewResult run() {
    const BookEntry* resumeBook = Library::getResumeBook();
    bool hasResume = (resumeBook != nullptr);

    // Build menu items
    char resumeLabel[64] = "";
    if (hasResume) {
        snprintf(resumeLabel, sizeof(resumeLabel), "Resume: %s (p.%d)",
                 resumeBook->filename.c_str(), resumeBook->page + 1);
    }

    const int MAX_ITEMS = 5;
    MenuItem items[MAX_ITEMS];
    int itemCount = 0;

    if (hasResume) {
        items[itemCount++] = {resumeLabel, true};
    }
    items[itemCount++] = {"Library", true};
    items[itemCount++] = {"Bookmarks", true};
    items[itemCount++] = {"Upload", true};
    items[itemCount++] = {"Settings", true};

    int8_t selected = 0;

    auto redraw = [&]() {
        Display::clearBuffer();
        UI::drawLeftPanel(items, itemCount, selected, -1, true);
        UI::drawDivider();
        drawPreviewForItem(selected, hasResume, resumeBook);
        UI::drawBatteryBottomRight();
    };

    // Initial draw
    redraw();
    Display::update();

    while (true) {
        Event e = Input::poll();
        if (e == Event::NONE) { Input::lightSleep(); continue; }

        switch (e) {
            case Event::SCROLL_UP:
                selected = (selected > 0) ? selected - 1 : itemCount - 1;
                redraw();
                Display::update();
                break;

            case Event::SCROLL_DOWN:
                selected = (selected < itemCount - 1) ? selected + 1 : 0;
                redraw();
                Display::update();
                break;

            case Event::SELECT:
            case Event::MENU: {
                int actualIdx = hasResume ? selected : selected + 1;
                switch (actualIdx) {
                    case 0:
                        // Resume — go straight to reading
                        g_bookToOpen = resumeBook->filename;
                        g_pageToOpen = resumeBook->page;
                        return ViewResult::OPEN_BOOK;
                    case 1: return ViewResult::OPEN_LIBRARY;
                    case 2: return ViewResult::OPEN_BOOKMARKS;
                    case 3: return ViewResult::OPEN_UPLOAD;
                    case 4: return ViewResult::OPEN_SETTINGS;
                }
                break;
            }

            default:
                break;
        }
    }
}

}  // namespace MenuMain
