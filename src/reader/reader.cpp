#include "reader.h"
#include "renderer.h"
#include "../display/display.h"
#include "../display/ui.h"
#include "../input/input.h"
#include "../storage/sd_manager.h"
#include "../storage/library.h"

static Paginator paginator;
static uint16_t curPage = 0;
static String bookFilename;
static char* pageTextBuf = nullptr;
static const size_t PAGE_TEXT_BUF_SIZE = 8192;  // Max bytes per page
static bool needsFullRefresh = true;  // First render after open uses full refresh

// Helper: get byte offset of current page for saving progress
static uint32_t curByteOffset() {
    return paginator.getPage(curPage).byteOffset;
}

namespace Reader {

bool open(const char* filename) {
    bookFilename = filename;

    // Show loading screen
    UI::drawCenteredMessage("Loading...", font_medium);
    Display::update(true);

    // Build full path
    String fullPath = String(PATH_BOOKS) + "/" + filename;

    // Calculate render area: one panel width minus margins (two-page spread)
    int16_t renderW = PANEL_LEFT_W - MARGIN_X * 2;
    int16_t renderH = EPD_HEIGHT - PROGRESS_BAR_H - MARGIN_Y * 2 - 4;

    if (!paginator.paginate(fullPath.c_str(), renderW, renderH)) {
        Serial.printf("[Reader] Failed to paginate %s\n", filename);
        return false;
    }

    // Allocate page text buffer in PSRAM if available
    if (pageTextBuf) free(pageTextBuf);
    pageTextBuf = (char*)ps_malloc(PAGE_TEXT_BUF_SIZE);
    if (!pageTextBuf) {
        pageTextBuf = (char*)malloc(PAGE_TEXT_BUF_SIZE);
        if (!pageTextBuf) return false;
    }

    // Restore last read position from library (use byte offset for accuracy)
    curPage = 0;
    const BookEntry* entry = Library::getEntry(filename);
    if (entry) {
        if (entry->byteOffset > 0) {
            curPage = paginator.pageForByteOffset(entry->byteOffset);
        } else if (entry->page > 0 && entry->page < paginator.totalPages()) {
            curPage = entry->page;
        }
    }
    // Snap to even page for two-page spread (left page is always even)
    curPage &= ~1;

    // Mark as reading
    Library::setEntry(filename, BookStatus::READING, curPage, curByteOffset());

    needsFullRefresh = true;  // First page render should do full refresh
    return true;
}

void renderCurrentPage() {
    if (!pageTextBuf) return;

    String fullPath = String(PATH_BOOKS) + "/" + bookFilename;
    int16_t contentBottom = EPD_HEIGHT - PROGRESS_BAR_H - 4;

    Display::clearBuffer();

    // Left page (curPage)
    {
        const PageInfo& page = paginator.getPage(curPage);
        size_t toRead = min(page.byteLength, PAGE_TEXT_BUF_SIZE - 1);
        size_t bytesRead = SDManager::readChunk(fullPath.c_str(),
                                                (uint8_t*)pageTextBuf,
                                                page.byteOffset, toRead);
        pageTextBuf[bytesRead] = '\0';
        Renderer::renderPage(pageTextBuf,
                             MARGIN_X, MARGIN_Y,
                             PANEL_LEFT_W - MARGIN_X,
                             contentBottom);
    }

    // Right page (curPage + 1) — if it exists
    if (curPage + 1 < paginator.totalPages()) {
        const PageInfo& page = paginator.getPage(curPage + 1);
        size_t toRead = min(page.byteLength, PAGE_TEXT_BUF_SIZE - 1);
        size_t bytesRead = SDManager::readChunk(fullPath.c_str(),
                                                (uint8_t*)pageTextBuf,
                                                page.byteOffset, toRead);
        pageTextBuf[bytesRead] = '\0';
        Renderer::renderPage(pageTextBuf,
                             PANEL_DIVIDER_X + MARGIN_X, MARGIN_Y,
                             EPD_WIDTH - MARGIN_X,
                             contentBottom);
    }

    // Divider line between the two pages
    Display::drawVLine(PANEL_DIVIDER_X, 0, contentBottom, COL_BLACK);

    // Progress bar — show the rightmost page being displayed
    uint16_t rightPage = min((uint16_t)(curPage + 2), paginator.totalPages());
    UI::drawProgressBar(rightPage, paginator.totalPages());

    Display::update(needsFullRefresh);
    needsFullRefresh = false;
}

bool nextPage() {
    if (curPage + 2 >= paginator.totalPages()) return false;
    curPage += 2;
    Library::setEntry(bookFilename, BookStatus::READING, curPage, curByteOffset());
    return true;
}

bool prevPage() {
    if (curPage < 2) return false;
    curPage -= 2;
    Library::setEntry(bookFilename, BookStatus::READING, curPage, curByteOffset());
    return true;
}

bool goToPage(uint16_t page) {
    page &= ~1;  // Snap to even for spread
    if (page >= paginator.totalPages()) return false;
    curPage = page;
    needsFullRefresh = true;
    Library::setEntry(bookFilename, BookStatus::READING, curPage, curByteOffset());
    return true;
}

uint16_t currentPage() {
    return curPage;
}

uint16_t totalPages() {
    return paginator.totalPages();
}

const char* currentBook() {
    return bookFilename.c_str();
}

bool isLastPage() {
    return curPage + 2 >= paginator.totalPages();
}

void forceFullRefresh() {
    needsFullRefresh = true;
}

void close() {
    Library::setEntry(bookFilename, BookStatus::READING, curPage, curByteOffset());
    if (pageTextBuf) {
        free(pageTextBuf);
        pageTextBuf = nullptr;
    }
    bookFilename = "";
}

ReaderResult run() {
    renderCurrentPage();

    while (true) {
        Event e = Input::poll();

        switch (e) {
            case Event::SCROLL_DOWN:
            case Event::MENU:
                // Next page spread
                if (isLastPage()) {
                    return ReaderResult::BOOK_FINISHED;
                }
                if (nextPage()) {
                    renderCurrentPage();
                }
                break;

            case Event::SCROLL_UP:
            case Event::EXIT:
                // Previous page spread
                if (prevPage()) {
                    renderCurrentPage();
                }
                break;

            case Event::SELECT:
                return ReaderResult::OPEN_READING_MENU;

            case Event::NONE:
                Input::lightSleep();
                break;
        }
    }
}

}  // namespace Reader
