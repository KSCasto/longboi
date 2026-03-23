#include "menu_library.h"
#include "../display/display.h"
#include "../display/ui.h"
#include "../input/input.h"
#include "../storage/library.h"
#include "../storage/sd_manager.h"
#include "../bookmarks/bookmarks.h"
#include "../fonts/fonts.h"

namespace MenuLibrary {

// Filter modes
enum class Filter : uint8_t { ALL, UNREAD, READING, READ_DONE };

static const char* filterNames[] = {"All", "Unread", "Reading", "Read"};

static Filter currentFilter = Filter::ALL;

static bool matchesFilter(const BookEntry& e, Filter f) {
    switch (f) {
        case Filter::UNREAD:    return e.status == BookStatus::UNREAD;
        case Filter::READING:   return e.status == BookStatus::READING;
        case Filter::READ_DONE: return e.status == BookStatus::READ_DONE;
        default:                return true;
    }
}

// Build the preview content for a book entry
static void drawBookPreview(const BookEntry& entry) {
    char statusLine[32], pageLine[48], bmLine[32];
    snprintf(statusLine, sizeof(statusLine), "Status: %s",
             Library::statusToString(entry.status));
    snprintf(pageLine, sizeof(pageLine), "Page: %d", entry.page + 1);

    uint16_t bmCount = Bookmarks::count(entry.filename);
    snprintf(bmLine, sizeof(bmLine), "%d bookmarks", bmCount);

    PreviewLine lines[] = {
        {entry.filename.c_str(), &font_medium},
        {statusLine, nullptr},
        {pageLine, nullptr},
        {bmLine, nullptr},
    };
    UI::drawRightPreview(lines, 4);
}

ViewResult run() {
    // Get filtered book list
    const auto& allEntries = Library::getAll();

    // Build displayable list based on filter
    auto rebuildList = [&](std::vector<const BookEntry*>& filtered) {
        filtered.clear();
        for (const auto& e : allEntries) {
            if (matchesFilter(e, currentFilter)) {
                filtered.push_back(&e);
            }
        }
    };

    std::vector<const BookEntry*> filtered;
    rebuildList(filtered);

    if (filtered.empty()) {
        UI::drawCenteredMessage("No books found", font_regular);
        Display::update(true);
        // Wait for any button to go back
        while (true) {
            Event e = Input::poll();
            if (e != Event::NONE) return ViewResult::MAIN_MENU;
            delay(10);
        }
    }

    // Build menu items from filtered list
    const int MAX_DISPLAY = 32;
    MenuItem leftItems[MAX_DISPLAY];
    char labels[MAX_DISPLAY][48];
    int leftCount = min((int)filtered.size(), MAX_DISPLAY);

    auto rebuildLabels = [&]() {
        leftCount = min((int)filtered.size(), MAX_DISPLAY);
        for (int i = 0; i < leftCount; i++) {
            snprintf(labels[i], sizeof(labels[i]), "%s [%s]",
                     filtered[i]->filename.c_str(),
                     Library::statusToString(filtered[i]->status));
            leftItems[i] = {labels[i], true};
        }
    };
    rebuildLabels();

    int8_t selected = 0;
    bool rightActive = false;
    int8_t rightSelected = 0;

    // Right submenu for a selected book
    MenuItem bookSubItems[] = {
        {"Open", true},
        {"Mark Unread", true},
        {"Delete", true},
    };
    int bookSubCount = 3;

    // Initial draw
    Display::clearBuffer();
    UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
    UI::drawDivider();
    if (leftCount > 0) drawBookPreview(*filtered[selected]);
    Display::update();

    while (true) {
        Event e = Input::poll();
        if (e == Event::NONE) { delay(10); continue; }

        if (!rightActive) {
            switch (e) {
                case Event::SCROLL_UP:
                    if (selected > 0) {
                        selected--;
                        UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
                        drawBookPreview(*filtered[selected]);
                        UI::drawDivider();
                        Display::update();
                    }
                    break;

                case Event::SCROLL_DOWN:
                    if (selected < leftCount - 1) {
                        selected++;
                        UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
                        drawBookPreview(*filtered[selected]);
                        UI::drawDivider();
                        Display::update();
                    }
                    break;

                case Event::SELECT:
                    // Activate right panel submenu
                    rightActive = true;
                    rightSelected = 0;
                    UI::drawLeftPanel(leftItems, leftCount, -1, selected, false);
                    UI::drawRightMenu(bookSubItems, bookSubCount, rightSelected);
                    UI::drawDivider();
                    Display::update(true);
                    break;

                case Event::EXIT:
                    return ViewResult::MAIN_MENU;

                case Event::MENU: {
                    // Cycle filter
                    currentFilter = (Filter)(((int)currentFilter + 1) % 4);
                    rebuildList(filtered);
                    rebuildLabels();
                    selected = 0;
                    Display::clearBuffer();
                    UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
                    UI::drawDivider();
                    if (leftCount > 0) drawBookPreview(*filtered[selected]);

                    // Show current filter at top of right panel
                    char filterMsg[32];
                    snprintf(filterMsg, sizeof(filterMsg), "Filter: %s",
                             filterNames[(int)currentFilter]);
                    // Briefly show filter name then redraw
                    Display::update(true);
                    break;
                }

                default: break;
            }
        } else {
            // Right panel active — book action submenu
            switch (e) {
                case Event::SCROLL_UP:
                    if (rightSelected > 0) {
                        rightSelected--;
                        UI::drawRightMenu(bookSubItems, bookSubCount, rightSelected);
                        Display::update();
                    }
                    break;

                case Event::SCROLL_DOWN:
                    if (rightSelected < bookSubCount - 1) {
                        rightSelected++;
                        UI::drawRightMenu(bookSubItems, bookSubCount, rightSelected);
                        Display::update();
                    }
                    break;

                case Event::SELECT: {
                    const BookEntry* book = filtered[selected];
                    switch (rightSelected) {
                        case 0:
                            // Open book
                            g_bookToOpen = book->filename;
                            g_pageToOpen = book->page;
                            return ViewResult::OPEN_BOOK;

                        case 1: {
                            // Cycle status: unread → reading → read → unread
                            BookStatus next;
                            switch (book->status) {
                                case BookStatus::UNREAD:    next = BookStatus::READING; break;
                                case BookStatus::READING:   next = BookStatus::READ_DONE; break;
                                case BookStatus::READ_DONE: next = BookStatus::UNREAD; break;
                            }
                            Library::setEntry(book->filename, next, book->page);

                            // Rebuild and redraw
                            rebuildList(filtered);
                            rebuildLabels();
                            rightActive = false;
                            if (selected >= leftCount) selected = max(0, leftCount - 1);
                            Display::clearBuffer();
                            UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
                            UI::drawDivider();
                            if (leftCount > 0) drawBookPreview(*filtered[selected]);
                            Display::update(true);
                            break;
                        }

                        case 2: {
                            // Delete — show confirmation
                            Display::clearBuffer();
                            UI::drawConfirmDialog("Delete this book?", true);
                            Display::update(true);

                            bool yesSelected = true;
                            while (true) {
                                Event ce = Input::poll();
                                if (ce == Event::NONE) { delay(10); continue; }

                                if (ce == Event::SCROLL_UP || ce == Event::SCROLL_DOWN) {
                                    yesSelected = !yesSelected;
                                    Display::clearBuffer();
                                    UI::drawConfirmDialog("Delete this book?", yesSelected);
                                    Display::update(true);
                                } else if (ce == Event::SELECT) {
                                    if (yesSelected) {
                                        String path = String(PATH_BOOKS) + "/" + book->filename;
                                        SDManager::deleteFile(path.c_str());
                                        Bookmarks::removeAll(book->filename);
                                        Library::removeEntry(book->filename);
                                    }
                                    break;
                                } else if (ce == Event::EXIT) {
                                    break;
                                }
                            }

                            // Rebuild and redraw
                            rebuildList(filtered);
                            rebuildLabels();
                            rightActive = false;
                            if (selected >= leftCount) selected = max(0, leftCount - 1);
                            Display::clearBuffer();
                            if (leftCount > 0) {
                                UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
                                UI::drawDivider();
                                drawBookPreview(*filtered[selected]);
                            } else {
                                UI::drawCenteredMessage("No books", font_regular);
                            }
                            Display::update(true);
                            break;
                        }
                    }
                    break;
                }

                case Event::EXIT:
                    rightActive = false;
                    UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
                    drawBookPreview(*filtered[selected]);
                    UI::drawDivider();
                    Display::update(true);
                    break;

                case Event::MENU:
                    return ViewResult::MAIN_MENU;

                default: break;
            }
        }
    }
}

}  // namespace MenuLibrary
