#include "menu_main.h"
#include "../display/display.h"
#include "../display/ui.h"
#include "../input/input.h"
#include "../storage/library.h"
#include "../bookmarks/bookmarks.h"
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
            // Settings
            PreviewLine lines[] = {
                {"Settings", &font_medium},
                {"Coming soon...", nullptr},
            };
            UI::drawRightPreview(lines, 2);
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
    bool rightActive = false;
    int8_t rightSelected = 0;

    // Right-panel submenu items for Resume
    MenuItem resumeSubItems[] = {
        {"Open book", true},
        {"View bookmarks", true},
        {"Mark as read", true},
    };
    int resumeSubCount = 3;

    // Initial draw
    Display::clearBuffer();
    UI::drawLeftPanel(items, itemCount, selected, -1, true);
    UI::drawDivider();
    drawPreviewForItem(selected, hasResume, resumeBook);
    Display::update(true);

    while (true) {
        Event e = Input::poll();
        if (e == Event::NONE) { delay(10); continue; }

        if (!rightActive) {
            // Left panel is active
            switch (e) {
                case Event::SCROLL_UP:
                    if (selected > 0) {
                        selected--;
                        drawPreviewForItem(selected, hasResume, resumeBook);
                        UI::drawLeftPanel(items, itemCount, selected, -1, true);
                        UI::drawDivider();
                        Display::update(false,true);
                    }
                    break;

                case Event::SCROLL_DOWN:
                    if (selected < itemCount - 1) {
                        selected++;
                        UI::drawLeftPanel(items, itemCount, selected, -1, true);
                        drawPreviewForItem(selected, hasResume, resumeBook);
                        UI::drawDivider();
                        Display::update(false,true);
                    }
                    break;

                case Event::SELECT: {
                    int actualIdx = hasResume ? selected : selected + 1;
                    switch (actualIdx) {
                        case 0:
                            // Resume — activate right panel submenu
                            rightActive = true;
                            rightSelected = 0;
                            UI::drawLeftPanel(items, itemCount, -1, selected, false);
                            UI::drawRightMenu(resumeSubItems, resumeSubCount, rightSelected);
                            UI::drawDivider();
                            Display::update(true);
                            break;
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
        } else {
            // Right panel is active (Resume submenu)
            switch (e) {
                case Event::SCROLL_UP:
                    if (rightSelected > 0) {
                        rightSelected--;
                        UI::drawRightMenu(resumeSubItems, resumeSubCount, rightSelected);
                        Display::update();
                    }
                    break;

                case Event::SCROLL_DOWN:
                    if (rightSelected < resumeSubCount - 1) {
                        rightSelected++;
                        UI::drawRightMenu(resumeSubItems, resumeSubCount, rightSelected);
                        Display::update();
                    }
                    break;

                case Event::SELECT:
                    switch (rightSelected) {
                        case 0:
                            // Open book
                            g_bookToOpen = resumeBook->filename;
                            g_pageToOpen = resumeBook->page;
                            return ViewResult::OPEN_BOOK;
                        case 1:
                            // View bookmarks — go to bookmarks view
                            return ViewResult::OPEN_BOOKMARKS;
                        case 2:
                            // Mark as read
                            Library::setEntry(resumeBook->filename,
                                              BookStatus::READ_DONE, resumeBook->page);
                            return ViewResult::MAIN_MENU;  // Refresh menu
                    }
                    break;

                case Event::EXIT:
                    // Deactivate right panel, return to left
                    rightActive = false;
                    UI::drawLeftPanel(items, itemCount, selected, -1, true);
                    drawPreviewForItem(selected, hasResume, resumeBook);
                    UI::drawDivider();
                    Display::update(true);
                    break;

                case Event::MENU:
                    return ViewResult::MAIN_MENU;

                default:
                    break;
            }
        }
    }
}

}  // namespace MenuMain
