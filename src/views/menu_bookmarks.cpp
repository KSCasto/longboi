#include "menu_bookmarks.h"
#include "../display/display.h"
#include "../display/ui.h"
#include "../input/input.h"
#include "../bookmarks/bookmarks.h"
#include "../storage/library.h"
#include "../fonts/fonts.h"

namespace MenuBookmarks {

ViewResult run() {
    // Get books that have bookmarks
    auto books = Bookmarks::booksWithBookmarks();

    if (books.empty()) {
        UI::drawCenteredMessage("No bookmarks", font_regular);
        Display::update(true);
        while (true) {
            Event e = Input::poll();
            if (e != Event::NONE) return ViewResult::MAIN_MENU;
            Input::lightSleep();
        }
    }

    // Build left panel items (book names)
    const int MAX_ITEMS = 32;
    MenuItem leftItems[MAX_ITEMS];
    int leftCount = min((int)books.size(), MAX_ITEMS);
    for (int i = 0; i < leftCount; i++) {
        leftItems[i] = {books[i].c_str(), true};
    }

    int8_t selected = 0;
    bool rightActive = false;
    int8_t rightSelected = 0;

    // Current book's bookmarks (loaded when selection changes)
    std::vector<Bookmark> currentBms;
    MenuItem rightItems[MAX_ITEMS];
    char rightLabels[MAX_ITEMS][48];
    int rightCount = 0;

    auto loadBookmarks = [&](int idx) {
        currentBms = Bookmarks::load(books[idx]);
        rightCount = min((int)currentBms.size(), MAX_ITEMS);
        for (int i = 0; i < rightCount; i++) {
            snprintf(rightLabels[i], sizeof(rightLabels[i]), "%s (p.%d)",
                     currentBms[i].name.c_str(), currentBms[i].page + 1);
            rightItems[i] = {rightLabels[i], true};
        }
    };

    auto drawPreview = [&](int idx) {
        loadBookmarks(idx);
        PreviewLine lines[MAX_ITEMS + 1];
        char header[48];
        snprintf(header, sizeof(header), "%s", books[idx].c_str());
        lines[0] = {header, &font_medium};

        int lineCount = 1;
        for (int i = 0; i < rightCount && lineCount < MAX_ITEMS; i++) {
            lines[lineCount++] = {rightLabels[i], nullptr};
        }
        UI::drawRightPreview(lines, lineCount);
    };

    // Initial draw
    Display::clearBuffer();
    UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
    UI::drawDivider();
    drawPreview(selected);
    Display::update(true);

    while (true) {
        Event e = Input::poll();
        if (e == Event::NONE) { Input::lightSleep(); continue; }

        if (!rightActive) {
            switch (e) {
                case Event::SCROLL_UP:
                    if (selected > 0) {
                        selected--;
                        UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
                        drawPreview(selected);
                        UI::drawDivider();
                        Display::update();
                    }
                    break;

                case Event::SCROLL_DOWN:
                    if (selected < leftCount - 1) {
                        selected++;
                        UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
                        drawPreview(selected);
                        UI::drawDivider();
                        Display::update();
                    }
                    break;

                case Event::SELECT:
                    if (rightCount > 0) {
                        rightActive = true;
                        rightSelected = 0;
                        UI::drawLeftPanel(leftItems, leftCount, -1, selected, false);
                        UI::drawRightMenu(rightItems, rightCount, rightSelected);
                        UI::drawDivider();
                        Display::update(true);
                    }
                    break;

                case Event::EXIT:
                case Event::MENU:
                    return ViewResult::MAIN_MENU;

                default: break;
            }
        } else {
            // Right panel active — scrolling through bookmarks
            switch (e) {
                case Event::SCROLL_UP:
                    if (rightSelected > 0) {
                        rightSelected--;
                        UI::drawRightMenu(rightItems, rightCount, rightSelected);
                        Display::update();
                    }
                    break;

                case Event::SCROLL_DOWN:
                    if (rightSelected < rightCount - 1) {
                        rightSelected++;
                        UI::drawRightMenu(rightItems, rightCount, rightSelected);
                        Display::update();
                    }
                    break;

                case Event::SELECT:
                    // Jump to this bookmark's page in the book
                    // Need to find the actual book filename (with extension)
                    // The folder name is without extension, so we search library
                    {
                        const auto& allBooks = Library::getAll();
                        for (const auto& entry : allBooks) {
                            if (Bookmarks::bookToFolderName(entry.filename) == books[selected]) {
                                g_bookToOpen = entry.filename;
                                g_pageToOpen = currentBms[rightSelected].page;
                                return ViewResult::OPEN_BOOK;
                            }
                        }
                    }
                    break;

                case Event::EXIT:
                    rightActive = false;
                    UI::drawLeftPanel(leftItems, leftCount, selected, -1, true);
                    drawPreview(selected);
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

}  // namespace MenuBookmarks
