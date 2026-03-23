#include "menu_settings.h"
#include "../display/display.h"
#include "../display/ui.h"
#include "../input/input.h"
#include "../settings/settings.h"
#include "../config.h"
#include "../storage/sd_manager.h"
#include "../storage/library.h"
#include "../fonts/fonts.h"
#include <SD.h>

static void drawDeviceInfo() {
    char buf[48];
    PreviewLine lines[7];
    char ssidBuf[32], macBuf[32], storageBuf[32], booksBuf[24], ramBuf[24];

    lines[0] = {"Device Info", &font_medium};

    snprintf(ssidBuf, sizeof(ssidBuf), "SSID: %s", AP_SSID);
    lines[1] = {ssidBuf, nullptr};

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(macBuf, sizeof(macBuf), "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    lines[2] = {macBuf, nullptr};

    uint64_t total = SD.totalBytes();
    uint64_t used = SD.usedBytes();
    snprintf(storageBuf, sizeof(storageBuf), "SD: %dMB / %dMB",
             (int)(used / (1024*1024)), (int)(total / (1024*1024)));
    lines[3] = {storageBuf, nullptr};

    snprintf(booksBuf, sizeof(booksBuf), "Books: %d", (int)Library::getAll().size());
    lines[4] = {booksBuf, nullptr};

    snprintf(ramBuf, sizeof(ramBuf), "Free RAM: %dKB", (int)(ESP.getFreeHeap() / 1024));
    lines[5] = {ramBuf, nullptr};

    lines[6] = {"", nullptr};

    UI::drawRightPreview(lines, 6);
}

namespace MenuSettings {

ViewResult run() {
    char fontLabel[24];
    char spacingLabel[24];
    char refreshLabel[24];
    char scrollLabel[24];
    char resumeLabel[24];
    char sleepLabel[24];

    auto buildLabels = [&]() {
        snprintf(fontLabel, sizeof(fontLabel), "Font: %s",
                 Settings::fontSize() == 16 ? "Large" : "Small");

        const char* spacingNames[] = {"Tight", "Normal", "Loose"};
        snprintf(spacingLabel, sizeof(spacingLabel), "Spacing: %s",
                 spacingNames[Settings::lineSpacing()]);

        snprintf(refreshLabel, sizeof(refreshLabel), "Refresh: %d",
                 Settings::refreshInterval());

        snprintf(scrollLabel, sizeof(scrollLabel), "Scroll: %s",
                 Settings::invertScroll() ? "Inverted" : "Normal");

        snprintf(resumeLabel, sizeof(resumeLabel), "Auto Resume: %s",
                 Settings::autoResume() ? "On" : "Off");

        uint8_t sl = Settings::autoSleepMinutes();
        if (sl == 0)
            snprintf(sleepLabel, sizeof(sleepLabel), "Auto Sleep: Off");
        else
            snprintf(sleepLabel, sizeof(sleepLabel), "Auto Sleep: %dm", sl);
    };

    buildLabels();

    const int itemCount = 6;
    MenuItem items[itemCount] = {
        {fontLabel, true},
        {spacingLabel, true},
        {refreshLabel, true},
        {scrollLabel, true},
        {resumeLabel, true},
        {sleepLabel, true},
    };

    int8_t selected = 0;

    auto redraw = [&]() {
        Display::clearBuffer();
        UI::drawLeftPanel(items, itemCount, selected, -1, true);
        UI::drawDivider();
        drawDeviceInfo();
    };

    redraw();
    Display::update(true);

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
            case Event::MENU:
                switch (selected) {
                    case 0:
                        Settings::setFontSize(Settings::fontSize() == 12 ? 16 : 12);
                        break;
                    case 1: {
                        uint8_t cur = Settings::lineSpacing();
                        Settings::setLineSpacing((cur + 1) % 3);
                        break;
                    }
                    case 2: {
                        uint8_t cur = Settings::refreshInterval();
                        static const uint8_t opts[] = {5, 10, 20, 50, 100};
                        uint8_t next = opts[0];
                        for (int i = 0; i < 4; i++) {
                            if (cur == opts[i]) { next = opts[i + 1]; break; }
                        }
                        Settings::setRefreshInterval(next);
                        break;
                    }
                    case 3:
                        Settings::setInvertScroll(!Settings::invertScroll());
                        break;
                    case 4:
                        Settings::setAutoResume(!Settings::autoResume());
                        break;
                    case 5: {
                        uint8_t cur = Settings::autoSleepMinutes();
                        static const uint8_t opts[] = {0, 5, 10, 30};
                        uint8_t next = opts[0];
                        for (int i = 0; i < 3; i++) {
                            if (cur == opts[i]) { next = opts[i + 1]; break; }
                        }
                        Settings::setAutoSleepMinutes(next);
                        Input::resetActivity();
                        break;
                    }
                }
                buildLabels();
                redraw();
                Display::update();
                break;

            case Event::EXIT:
                return ViewResult::MAIN_MENU;

            default: break;
        }
    }
}

}  // namespace MenuSettings
