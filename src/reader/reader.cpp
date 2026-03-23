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

namespace Reader {

bool open(const char* filename) {
    bookFilename = filename;

    // Show loading screen
    UI::drawCenteredMessage("Loading...", font_medium);
    Display::update(true);

    // Build full path
    String fullPath = String(PATH_BOOKS) + "/" + filename;

    // Calculate render area (full width minus margins, height minus progress bar)
    int16_t renderW = EPD_WIDTH - MARGIN_X * 2;
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

    // Restore last read page from library
    const BookEntry* entry = Library::getEntry(filename);
    if (entry && entry->page > 0 && entry->page < paginator.totalPages()) {
        curPage = entry->page;
    } else {
        curPage = 0;
    }

    // Mark as reading
    Library::setEntry(filename, BookStatus::READING, curPage);

    needsFullRefresh = true;  // First page render should do full refresh
    return true;
}

void renderCurrentPage() {
    if (!pageTextBuf) return;

    const PageInfo& page = paginator.getPage(curPage);
    String fullPath = String(PATH_BOOKS) + "/" + bookFilename;

    // Read page content from file
    size_t toRead = min(page.byteLength, PAGE_TEXT_BUF_SIZE - 1);
    size_t bytesRead = SDManager::readChunk(fullPath.c_str(),
                                            (uint8_t*)pageTextBuf,
                                            page.byteOffset, toRead);
    pageTextBuf[bytesRead] = '\0';

    // Clear buffer, render page content, then refresh display
    Display::clearBuffer();
    Renderer::renderPage(pageTextBuf,
                         MARGIN_X, MARGIN_Y,
                         EPD_WIDTH - MARGIN_X,
                         EPD_HEIGHT - PROGRESS_BAR_H - 4);
    UI::drawProgressBar(curPage + 1, paginator.totalPages());

    Display::update(needsFullRefresh);
    needsFullRefresh = false;
}

bool nextPage() {
    if (curPage + 1 >= paginator.totalPages()) return false;
    curPage++;
    Library::setEntry(bookFilename, BookStatus::READING, curPage);
    return true;
}

bool prevPage() {
    if (curPage == 0) return false;
    curPage--;
    Library::setEntry(bookFilename, BookStatus::READING, curPage);
    return true;
}

bool goToPage(uint16_t page) {
    if (page >= paginator.totalPages()) return false;
    curPage = page;
    needsFullRefresh = true;
    Library::setEntry(bookFilename, BookStatus::READING, curPage);
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
    return curPage + 1 >= paginator.totalPages();
}

void forceFullRefresh() {
    needsFullRefresh = true;
}

void close() {
    Library::setEntry(bookFilename, BookStatus::READING, curPage);
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
            case Event::SELECT:
                // Next page
                if (isLastPage()) {
                    return ReaderResult::BOOK_FINISHED;
                }
                if (nextPage()) {
                    renderCurrentPage();
                }
                break;

            case Event::SCROLL_UP:
            case Event::EXIT:
                // Previous page
                if (prevPage()) {
                    renderCurrentPage();
                }
                break;

            case Event::MENU:
                return ReaderResult::OPEN_READING_MENU;

            case Event::NONE:
                delay(10);  // Small sleep to avoid busy-waiting
                break;
        }
    }
}

}  // namespace Reader
