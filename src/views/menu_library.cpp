#include "menu_library.h"
#include "../display/display.h"
#include "../display/ui.h"
#include "../input/input.h"
#include "../storage/library.h"
#include "../storage/sd_manager.h"
#include "../bookmarks/bookmarks.h"
#include "../fonts/fonts.h"
#include <SD.h>
#include <algorithm>

enum class SortMode : uint8_t { NAME, RECENT, PROGRESS };

static const char* sortModeLabel(SortMode m) {
    switch (m) {
        case SortMode::NAME:     return "Sort: A-Z";
        case SortMode::RECENT:   return "Sort: Recent";
        case SortMode::PROGRESS: return "Sort: Progress";
    }
    return "Sort: A-Z";
}

// Calculate reading progress (0-100) for a book entry
static uint8_t calcProgress(const BookEntry& e) {
    if (e.status == BookStatus::READ_DONE) return 100;
    if (e.byteOffset == 0) return 0;
    String path = String(PATH_BOOKS) + "/" + e.filename;
    size_t fSize = SDManager::fileSize(path.c_str());
    if (fSize == 0) return 0;
    return (uint8_t)((e.byteOffset * 100) / fSize);
}

namespace MenuLibrary {

ViewResult run() {
    auto& allEntries = Library::getAllMut();

    if (allEntries.empty()) {
        UI::drawCenteredMessage("No books found", font_regular);
        Display::update(true);
        while (true) {
            Event e = Input::poll();
            if (e != Event::NONE) return ViewResult::MAIN_MENU;
            Input::lightSleep();
        }
    }

    // Storage info for title
    char libraryTitle[48];
    auto buildTitle = [&]() {
        uint64_t total = SD.totalBytes();
        uint64_t used = SD.usedBytes();
        uint8_t pct = (total > 0) ? (uint8_t)((used * 100) / total) : 0;
        snprintf(libraryTitle, sizeof(libraryTitle), "Library %d%% %dMB/%dMB",
                 pct, (int)(used / (1024*1024)), (int)(total / (1024*1024)));
    };
    buildTitle();

    SortMode sortMode = SortMode::RECENT;

    auto sortEntries = [&]() {
        switch (sortMode) {
            case SortMode::NAME:
                std::sort(allEntries.begin(), allEntries.end(),
                    [](const BookEntry& a, const BookEntry& b) {
                        return a.filename < b.filename;
                    });
                break;
            case SortMode::RECENT:
                std::sort(allEntries.begin(), allEntries.end(),
                    [](const BookEntry& a, const BookEntry& b) {
                        return a.updated > b.updated;
                    });
                break;
            case SortMode::PROGRESS:
                std::sort(allEntries.begin(), allEntries.end(),
                    [](const BookEntry& a, const BookEntry& b) {
                        return calcProgress(a) > calcProgress(b);
                    });
                break;
        }
    };
    sortEntries();

    // Build left panel: first item is sort selector, rest are books
    const int MAX_DISPLAY = 33;
    MenuItem leftItems[MAX_DISPLAY];
    char labels[MAX_DISPLAY][48];
    int totalItems = 0;

    auto rebuildLabels = [&]() {
        int bookCount = min((int)allEntries.size(), MAX_DISPLAY - 1);
        snprintf(labels[0], sizeof(labels[0]), "%s", sortModeLabel(sortMode));
        leftItems[0] = {labels[0], true};

        for (int i = 0; i < bookCount; i++) {
            uint8_t pct = calcProgress(allEntries[i]);
            snprintf(labels[i + 1], sizeof(labels[i + 1]), "%3d%% %.20s",
                     pct, allEntries[i].filename.c_str());
            leftItems[i + 1] = {labels[i + 1], true};
        }
        totalItems = bookCount + 1;
    };
    rebuildLabels();

    int8_t selected = 0;
    bool rightActive = false;
    int8_t rightSelected = 0;

    MenuItem bookSubItems[] = {
        {"Open", true},
        {"Cycle Status", true},
        {"Delete", true},
        {"Main Menu", true},
    };
    int bookSubCount = 4;

    // Build the right-panel preview for a book
    auto drawBookPreview = [&](int bookIdx) {
        const BookEntry& entry = allEntries[bookIdx];
        char statusLine[32], pageLine[48], bmLine[32], sizeLine[32];
        snprintf(statusLine, sizeof(statusLine), "Status: %s",
                 Library::statusToString(entry.status));
        snprintf(pageLine, sizeof(pageLine), "Page: %d", entry.page + 1);
        uint16_t bmCount = Bookmarks::count(entry.filename);
        snprintf(bmLine, sizeof(bmLine), "%d bookmarks", bmCount);
        String path = String(PATH_BOOKS) + "/" + entry.filename;
        size_t fSize = SDManager::fileSize(path.c_str());
        snprintf(sizeLine, sizeof(sizeLine), "Size: %dKB", (int)(fSize / 1024));

        PreviewLine lines[] = {
            {entry.filename.c_str(), &font_medium},
            {statusLine, nullptr},
            {pageLine, nullptr},
            {bmLine, nullptr},
            {sizeLine, nullptr},
        };
        UI::drawRightPreview(lines, 5);
    };

    // Initial draw
    Display::clearBuffer();
    UI::drawLeftPanel(leftItems, totalItems, selected, -1, true);
    UI::drawDivider();
    if (totalItems > 1) drawBookPreview(0);
    Display::update(true);

    while (true) {
        Event e = Input::poll();
        if (e == Event::NONE) { Input::lightSleep(); continue; }

        if (!rightActive) {
            switch (e) {
                case Event::SCROLL_UP:
                    selected = (selected > 0) ? selected - 1 : totalItems - 1;
                    UI::drawLeftPanel(leftItems, totalItems, selected, -1, true);
                    if (selected > 0) drawBookPreview(selected - 1);
                    else UI::clearRightPanel();
                    UI::drawDivider();
                    Display::update();
                    break;

                case Event::SCROLL_DOWN:
                    selected = (selected < totalItems - 1) ? selected + 1 : 0;
                    UI::drawLeftPanel(leftItems, totalItems, selected, -1, true);
                    if (selected > 0) drawBookPreview(selected - 1);
                    else UI::clearRightPanel();
                    UI::drawDivider();
                    Display::update();
                    break;

                case Event::SELECT:
                case Event::MENU:
                    if (selected == 0) {
                        // Cycle sort mode
                        switch (sortMode) {
                            case SortMode::NAME:     sortMode = SortMode::RECENT;   break;
                            case SortMode::RECENT:   sortMode = SortMode::PROGRESS; break;
                            case SortMode::PROGRESS: sortMode = SortMode::NAME;     break;
                        }
                        sortEntries();
                        rebuildLabels();
                        Display::clearBuffer();
                        UI::drawLeftPanel(leftItems, totalItems, selected, -1, true);
                        UI::drawDivider();
                        Display::update(true);
                    } else {
                        // Activate right panel submenu
                        rightActive = true;
                        rightSelected = 0;
                        UI::drawLeftPanel(leftItems, totalItems, -1, selected, false);
                        UI::drawRightMenu(bookSubItems, bookSubCount, rightSelected);
                        UI::drawDivider();
                        Display::update(true);
                    }
                    break;

                case Event::EXIT:
                    return ViewResult::MAIN_MENU;

                default: break;
            }
        } else {
            int bookIdx = selected - 1;
            switch (e) {
                case Event::SCROLL_UP:
                    rightSelected = (rightSelected > 0) ? rightSelected - 1 : bookSubCount - 1;
                    UI::drawRightMenu(bookSubItems, bookSubCount, rightSelected);
                    Display::update();
                    break;

                case Event::SCROLL_DOWN:
                    rightSelected = (rightSelected < bookSubCount - 1) ? rightSelected + 1 : 0;
                    UI::drawRightMenu(bookSubItems, bookSubCount, rightSelected);
                    Display::update();
                    break;

                case Event::SELECT:
                case Event::MENU: {
                    const BookEntry& book = allEntries[bookIdx];
                    switch (rightSelected) {
                        case 0:
                            // Open book
                            g_bookToOpen = book.filename;
                            g_pageToOpen = book.page;
                            return ViewResult::OPEN_BOOK;

                        case 1: {
                            // Cycle status
                            BookStatus next;
                            switch (book.status) {
                                case BookStatus::UNREAD:    next = BookStatus::READING; break;
                                case BookStatus::READING:   next = BookStatus::READ_DONE; break;
                                case BookStatus::READ_DONE: next = BookStatus::UNREAD; break;
                            }
                            Library::setEntry(book.filename, next, book.page);
                            rightActive = false;
                            rebuildLabels();
                            if (selected >= totalItems) selected = max(0, totalItems - 1);
                            Display::clearBuffer();
                            UI::drawLeftPanel(leftItems, totalItems, selected, -1, true);
                            UI::drawDivider();
                            if (selected > 0) drawBookPreview(selected - 1);
                            Display::update(true);
                            break;
                        }

                        case 2: {
                            // Delete — confirm
                            Display::clearBuffer();
                            UI::drawConfirmDialog("Delete this book?", true);
                            Display::update(true);

                            bool yesSelected = true;
                            while (true) {
                                Event ce = Input::poll();
                                if (ce == Event::NONE) { Input::lightSleep(); continue; }

                                if (ce == Event::SCROLL_UP || ce == Event::SCROLL_DOWN) {
                                    yesSelected = !yesSelected;
                                    Display::clearBuffer();
                                    UI::drawConfirmDialog("Delete this book?", yesSelected);
                                    Display::update(true);
                                } else if (ce == Event::SELECT || ce == Event::MENU) {
                                    if (yesSelected) {
                                        String path = String(PATH_BOOKS) + "/" + book.filename;
                                        SDManager::deleteFile(path.c_str());
                                        Bookmarks::removeAll(book.filename);
                                        Library::removeEntry(book.filename);
                                    }
                                    break;
                                } else if (ce == Event::EXIT) {
                                    break;
                                }
                            }

                            rightActive = false;
                            buildTitle();
                            rebuildLabels();
                            if (selected >= totalItems) selected = max(0, totalItems - 1);
                            Display::clearBuffer();
                            if (totalItems > 1) {
                                UI::drawLeftPanel(leftItems, totalItems, selected, -1, true);
                                UI::drawDivider();
                                if (selected > 0) drawBookPreview(selected - 1);
                            } else {
                                UI::drawCenteredMessage("No books", font_regular);
                            }
                            Display::update(true);
                            break;
                        }

                        case 3:
                            return ViewResult::MAIN_MENU;
                    }
                    break;
                }

                case Event::EXIT:
                    rightActive = false;
                    UI::drawLeftPanel(leftItems, totalItems, selected, -1, true);
                    if (selected > 0) drawBookPreview(selected - 1);
                    UI::drawDivider();
                    Display::update(true);
                    break;

                default: break;
            }
        }
    }
}

}  // namespace MenuLibrary
