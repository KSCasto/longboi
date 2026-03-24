#include <Arduino.h>
#include "config.h"
#include "display/display.h"
#include "display/ui.h"
#include "input/input.h"
#include "storage/sd_manager.h"
#include "storage/library.h"
#include "reader/reader.h"
#include "bookmarks/bookmarks.h"
#include "wifi/upload_server.h"
#include "views/views.h"
#include "views/menu_main.h"
#include "views/menu_library.h"
#include "views/menu_bookmarks.h"
#include "views/menu_reading.h"
#include "views/completion.h"
#include "views/menu_settings.h"
#include "settings/settings.h"
#include "fonts/fonts.h"

// =============================================================================
// State machine
// =============================================================================

enum class AppState {
    BOOT,
    MAIN_MENU,
    LIBRARY,
    BOOKMARKS,
    READING,
    UPLOAD,
    SETTINGS,
};

static AppState state = AppState::BOOT;

// --- Boot sequence -----------------------------------------------------------

static bool bootInit() {
    Serial.begin(115200);
    Serial.println("\n[KiraReader] Booting...");

    // TODO: LED blink on boot — need to identify LED pin for 5.79" board
    // #ifdef LED_PWR_PIN
    // pinMode(LED_PWR_PIN, OUTPUT);
    // digitalWrite(LED_PWR_PIN, HIGH); delay(100);
    // digitalWrite(LED_PWR_PIN, LOW);  delay(100);
    // digitalWrite(LED_PWR_PIN, HIGH); delay(100);
    // digitalWrite(LED_PWR_PIN, LOW);
    // #endif

    // CPU at max for snappy navigation
    setCpuFrequencyMhz(240);

    // Initialize display
    Display::init();
    UI::drawCenteredMessage("THE LOOK", font_splash);
    Display::update();

    // Initialize input
    Input::init();

    // Initialize SD card
    if (!SDManager::init()) {
        UI::drawCenteredMessage("SD Card Error!", font_medium);
        Display::update();
        Serial.println("[Boot] SD card failed");
        // Wait for user to fix and reboot
        while (true) { delay(1000); }
    }

    // Load settings and library
    Settings::load();
    Library::load();
    auto books = SDManager::listBooks();
    Library::syncWithSD(books);

    Serial.printf("[Boot] Ready. %d books on SD.\n", books.size());
    return true;
}

// --- Upload mode view --------------------------------------------------------

static void runUpload() {
    UploadServer::start();

    Display::clearBuffer();

    // Show upload mode info on both panels
    PreviewLine leftLines[] = {
        {"WiFi Upload Mode", &font_medium},
        {"", nullptr},
        {"Network:", nullptr},
        {AP_SSID, &font_bold},
        {"", nullptr},
        {"Open in browser:", nullptr},
        {AP_IP, &font_bold},
    };

    // Draw on left panel area
    int16_t y = MARGIN_Y;
    for (int i = 0; i < 7; i++) {
        const FontDef& f = leftLines[i].font ? *leftLines[i].font : font_regular;
        if (leftLines[i].text[0] != '\0') {
            UI::drawString(MARGIN_X, y, leftLines[i].text, f, COL_BLACK);
        }
        y += f.lineHeight + LINE_SPACING + 2;
    }

    // Right panel: instructions
    int16_t rx = PANEL_DIVIDER_X + MARGIN_X;
    int16_t ry = MARGIN_Y;
    UI::drawString(rx, ry, "Instructions:", font_medium, COL_BLACK);
    ry += font_medium.lineHeight + 8;
    UI::drawString(rx, ry, "1. Connect phone/laptop", font_regular, COL_BLACK);
    ry += font_regular.lineHeight + 4;
    UI::drawString(rx, ry, "   to WiFi network above", font_regular, COL_BLACK);
    ry += font_regular.lineHeight + 4;
    UI::drawString(rx, ry, "2. Open browser to IP", font_regular, COL_BLACK);
    ry += font_regular.lineHeight + 4;
    UI::drawString(rx, ry, "3. Upload .txt or .md files", font_regular, COL_BLACK);
    ry += font_regular.lineHeight + 12;
    UI::drawString(rx, ry, "Press MENU to exit", font_regular, COL_BLACK);

    UI::drawDivider();
    Display::update();

    // Flush any stale events from menu navigation or WiFi init noise
    Input::flush();

    // Wait for MENU or EXIT to leave upload mode
    while (true) {
        Event e = Input::poll();
        if (e == Event::MENU || e == Event::EXIT) break;
        delay(10);
    }

    UploadServer::stop();

    // Re-sync library after uploads/deletes
    auto books = SDManager::listBooks();
    Library::syncWithSD(books);
}

// --- Reading with menu overlay handling --------------------------------------

static AppState runReading() {
    if (g_bookToOpen.isEmpty()) return AppState::MAIN_MENU;

    if (!Reader::open(g_bookToOpen.c_str())) {
        UI::drawCenteredMessage("Failed to open book", font_regular);
        Display::update();
        delay(1500);
        return AppState::MAIN_MENU;
    }

    if (g_pageToOpen > 0) {
        Reader::goToPage(g_pageToOpen);
    }

    while (true) {
        ReaderResult result = Reader::run();

        switch (result) {
            case ReaderResult::BOOK_FINISHED: {
                CompletionResult cr = Completion::run(Reader::currentBook());
                // Save book name before close() clears it
                String finishedBook = Reader::currentBook();
                Reader::close();  // close() sets READING — must happen first
                if (cr == CompletionResult::MARK_AS_READ) {
                    // Override to READ_DONE with page 0 so rereading starts fresh
                    Library::setEntry(finishedBook, BookStatus::READ_DONE, 0);
                }
                return AppState::MAIN_MENU;
            }

            case ReaderResult::OPEN_READING_MENU: {
                ReadingMenuResult mr = MenuReading::run(Reader::currentBook(),
                                                        Reader::currentPage(),
                                                        Reader::totalPages());
                switch (mr) {
                    case ReadingMenuResult::RESUME:
                        // Continue reading — re-render current page
                        Reader::forceFullRefresh();
                        Reader::renderCurrentPage();
                        break;

                    case ReadingMenuResult::ADD_BOOKMARK:
                        // Already handled in menu_reading.cpp
                        Reader::forceFullRefresh();
                        Reader::renderCurrentPage();
                        break;

                    case ReadingMenuResult::VIEW_BOOKMARKS:
                        // Quick bookmark jump
                        Reader::close();
                        return AppState::BOOKMARKS;

                    case ReadingMenuResult::MARK_AS_READ:
                        Library::setEntry(Reader::currentBook(), BookStatus::READ_DONE,
                                          Reader::currentPage());
                        Reader::close();
                        return AppState::MAIN_MENU;

                    case ReadingMenuResult::BACK_TO_LIBRARY:
                        Reader::close();
                        return AppState::LIBRARY;

                    case ReadingMenuResult::GO_TO_PAGE:
                        Reader::goToPage(MenuReading::getChosenPage());
                        Reader::forceFullRefresh();
                        Reader::renderCurrentPage();
                        break;

                    case ReadingMenuResult::OPEN_SETTINGS:
                        Reader::close();
                        return AppState::SETTINGS;

                    case ReadingMenuResult::MAIN_MENU:
                        Reader::close();
                        return AppState::MAIN_MENU;
                }
                break;
            }

            case ReaderResult::EXIT_TO_MENU:
                Reader::close();
                return AppState::MAIN_MENU;
        }
    }
}

// =============================================================================
// Arduino setup & loop
// =============================================================================

void setup() {
    bootInit();
    state = AppState::MAIN_MENU;
}

void loop() {
    switch (state) {
        case AppState::MAIN_MENU: {
            ViewResult vr = MenuMain::run();
            switch (vr) {
                case ViewResult::OPEN_LIBRARY:    state = AppState::LIBRARY; break;
                case ViewResult::OPEN_BOOKMARKS:  state = AppState::BOOKMARKS; break;
                case ViewResult::OPEN_UPLOAD:      state = AppState::UPLOAD; break;
                case ViewResult::OPEN_SETTINGS:    state = AppState::SETTINGS; break;
                case ViewResult::OPEN_BOOK:        state = AppState::READING; break;
                case ViewResult::RESUME_READING: {
                    const BookEntry* resume = Library::getResumeBook();
                    if (resume) {
                        g_bookToOpen = resume->filename;
                        g_pageToOpen = resume->page;
                        state = AppState::READING;
                    }
                    break;
                }
                default: state = AppState::MAIN_MENU; break;
            }
            break;
        }

        case AppState::LIBRARY: {
            ViewResult vr = MenuLibrary::run();
            switch (vr) {
                case ViewResult::OPEN_BOOK: state = AppState::READING; break;
                default:                    state = AppState::MAIN_MENU; break;
            }
            break;
        }

        case AppState::BOOKMARKS: {
            ViewResult vr = MenuBookmarks::run();
            switch (vr) {
                case ViewResult::OPEN_BOOK: state = AppState::READING; break;
                default:                    state = AppState::MAIN_MENU; break;
            }
            break;
        }

        case AppState::READING:
            state = runReading();
            break;

        case AppState::UPLOAD:
            runUpload();
            state = AppState::MAIN_MENU;
            break;

        case AppState::SETTINGS: {
            ViewResult vr = MenuSettings::run();
            state = AppState::MAIN_MENU;
            break;
        }

        default:
            state = AppState::MAIN_MENU;
            break;
    }
}
