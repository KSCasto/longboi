#include "completion.h"
#include "../display/display.h"
#include "../display/ui.h"
#include "../input/input.h"
#include "../fonts/fonts.h"
#include "../config.h"

namespace Completion {

CompletionResult run(const char* bookFilename) {
    int8_t selected = 0;  // 0 = Mark as read, 1 = Keep reading

    auto draw = [&]() {
        Display::clearBuffer();

        // Title
        char title[64];
        snprintf(title, sizeof(title), "You finished %s!", bookFilename);
        uint16_t titleW = getStringWidth(title, font_medium);
        int16_t titleX = (EPD_WIDTH - titleW) / 2;
        UI::drawString(titleX, EPD_HEIGHT / 2 - 50, title, font_medium, COL_BLACK);

        // Options
        int16_t optY = EPD_HEIGHT / 2;
        int16_t optX = EPD_WIDTH / 2 - 80;
        int16_t lineH = font_regular.lineHeight + LINE_SPACING + 6;

        // Mark as read
        if (selected == 0) UI::drawChar(optX, optY, '>', font_regular, COL_BLACK);
        UI::drawString(optX + INDICATOR_W, optY, "Mark as read", font_regular, COL_BLACK);

        // Keep as reading
        optY += lineH;
        if (selected == 1) UI::drawChar(optX, optY, '>', font_regular, COL_BLACK);
        UI::drawString(optX + INDICATOR_W, optY, "Keep as reading", font_regular, COL_BLACK);
    };

    draw();
    Display::update(true);

    while (true) {
        Event e = Input::poll();
        if (e == Event::NONE) { Input::lightSleep(); continue; }

        switch (e) {
            case Event::SCROLL_UP:
            case Event::SCROLL_DOWN:
                selected = 1 - selected;
                draw();
                Display::update(true);
                break;

            case Event::SELECT:
            case Event::MENU:
                return selected == 0 ? CompletionResult::MARK_AS_READ
                                     : CompletionResult::KEEP_READING;

            default: break;
        }
    }
}

}  // namespace Completion
