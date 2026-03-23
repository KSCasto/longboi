#include "display.h"

#include "../epd/EPD.h"
#include "../epd/EPD_Init.h"

static uint8_t ImageBW[EPD_W * EPD_H / 8];
static uint8_t partialCount = 0;
static const uint8_t MAX_PARTIALS = 3;

// Clear old/RED registers (0x26/0xA6) to 0x00.
// Full refresh uses 3-color mode (0xF7) where 0x26 is the RED plane,
// so it must be 0x00 (no red) before a full update.
static void clearOldRegisters() {
    EPD_SetRAMMP(); EPD_SetRAMMA(); EPD_WR_REG(0x26);
    for (uint32_t i = 0; i < ALLSCREEN_BYTES; i++) EPD_WR_DATA8(0x00);
    EPD_SetRAMSP(); EPD_SetRAMSA(); EPD_WR_REG(0xA6);
    for (uint32_t i = 0; i < ALLSCREEN_BYTES; i++) EPD_WR_DATA8(0x00);
}

namespace Display {

void init() {
    pinMode(EPD_PWR_PIN, OUTPUT);
    digitalWrite(EPD_PWR_PIN, HIGH);
    delay(100);

    EPD_GPIOInit();
    EPD_Init();

    Paint_NewImage(ImageBW, EPD_W, EPD_H, 180, WHITE);
    Paint_Clear(WHITE);

    // Initial full refresh clears display and syncs old buffer
    update(true);
}

uint8_t* getBuffer() {
    return ImageBW;
}

void clearBuffer() {
    Paint_Clear(WHITE);
}

void setPixel(int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) return;
    Paint_SetPixel((uint16_t)x, (uint16_t)y,
                   color == COL_BLACK ? BLACK : WHITE);
}

uint8_t getPixel(int16_t x, int16_t y) {
    if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) return COL_WHITE;
    uint16_t X = (uint16_t)x;
    if (X >= 396) X += 8;
    uint32_t addr = X / 8 + (uint16_t)y * Paint.widthByte;
    uint8_t rdata = ImageBW[addr];
    return (rdata & (0x80 >> (X % 8))) ? COL_WHITE : COL_BLACK;
}

void update(bool full, bool skipCount) {
    EPD_Display(ImageBW);
    if (full || partialCount >= MAX_PARTIALS) {
        // Full refresh (3-color mode 0xF7): 0x26 is RED plane, must be 0x00
        clearOldRegisters();
        EPD_Update();
        partialCount = 0;
    } else {
        // Partial refresh (B/W mode 0xDC): 0x26 is old image for diffing
        EPD_PartUpdate();
        if (!skipCount) partialCount++;
    }
    // Sync old buffer with current content for next partial refresh
    EPD_Display_Old(ImageBW, 0);
}

void sleep() {
    EPD_DeepSleep();
}

void wake() {
    EPD_GPIOInit();
    EPD_Init();
    Paint_NewImage(ImageBW, EPD_W, EPD_H, 180, WHITE);
}

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    for (int16_t row = y; row < y + h && row < EPD_HEIGHT; row++) {
        for (int16_t col = x; col < x + w && col < EPD_WIDTH; col++) {
            setPixel(col, row, color);
        }
    }
}

void drawHLine(int16_t x, int16_t y, int16_t w, uint8_t color) {
    for (int16_t i = x; i < x + w && i < EPD_WIDTH; i++) {
        setPixel(i, y, color);
    }
}

void drawVLine(int16_t x, int16_t y, int16_t h, uint8_t color) {
    for (int16_t i = y; i < y + h && i < EPD_HEIGHT; i++) {
        setPixel(x, i, color);
    }
}

}  // namespace Display
