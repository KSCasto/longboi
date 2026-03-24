#include "display.h"

#include "../epd/EPD.h"
#include "../epd/EPD_Init.h"
#include "../settings/settings.h"

static uint8_t ImageBW[EPD_W * EPD_H / 8];
static uint8_t partialCount = 0;

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

    update();
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
    if (full || partialCount >= Settings::refreshInterval()) {
        // FULL REFRESH: HW reset → configure → send data → 0xF7 flash
        EPD_Init();                      // HW+SW reset (clean slate)
        EPD_WR_REG(0x18);               // Temperature sensor control
        EPD_WR_DATA8(0x80);             // Internal sensor (default 0x48 = garbage)
        EPD_WR_REG(0x3C);               // Border waveform control
        EPD_WR_DATA8(0x01);             // Follow LUT
        EPD_Display(ImageBW);            // Write image to 0x24/0xA4
        clearOldRegisters();             // Clear RED plane (0x26/0xA6 = 0x00)
        EPD_Update();                    // 0xF7 full update (loads temp+LUT itself)
        partialCount = 0;
    } else {
        // PARTIAL REFRESH: write new image + inverted old → 0xDC drives ALL pixels
        // The inverted old buffer makes every pixel appear "changed" to the controller,
        // forcing it to drive all pixels to their correct state (no half-gray text).
        // This is what the Elecrow library's invert parameter was designed for.
        EPD_Display(ImageBW);            // Write new image to 0x24/0xA4
        EPD_Display_Old(ImageBW, 1);     // Write ~image to 0x26/0xA6 (all pixels "changed")
        EPD_PartUpdate();                // 0xDC drives every pixel
        if (!skipCount) partialCount++;
    }
}

void sleep() {
    EPD_DeepSleep();
}

void wake() {
    EPD_GPIOInit();
    EPD_Init();
    Paint_NewImage(ImageBW, EPD_W, EPD_H, 180, WHITE);
    partialCount = 255;  // Force full refresh on next update()
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
