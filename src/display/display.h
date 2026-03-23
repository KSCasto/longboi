#pragma once

#include <Arduino.h>
#include "../config.h"

// =============================================================================
// Display driver wrapper for Elecrow CrowPanel 5.79" (dual SSD1683)
//
// Wraps the Elecrow EPD library (src/epd/). The internal buffer is 800x272
// (not 792) because there is an 8-pixel gap between the two controllers.
// Paint_SetPixel() handles this offset automatically.
// Our API uses VISIBLE coordinates: 0..791 x 0..271.
// =============================================================================

namespace Display {

void init();

uint8_t* getBuffer();
void clearBuffer();

void setPixel(int16_t x, int16_t y, uint8_t color);
uint8_t getPixel(int16_t x, int16_t y);

// Send framebuffer to display. Pass full=true for a clean full refresh
// (e.g. entering a new view). Partial refreshes auto-promote to full
// every few updates to clear accumulated ghosting.
// Pass skipCount=true to do a partial refresh without incrementing the counter.
void update(bool full = false, bool skipCount = false);

void sleep();
void wake();

void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void drawHLine(int16_t x, int16_t y, int16_t w, uint8_t color);
void drawVLine(int16_t x, int16_t y, int16_t h, uint8_t color);

}  // namespace Display
