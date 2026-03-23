#pragma once

#include <Arduino.h>

// =============================================================================
// Display — Dual SSD1683 via Elecrow CrowPanel EPD library
//
// IMPORTANT: The Elecrow library uses an 800x272 internal buffer (EPD_W=800).
// There is an 8-pixel gap at x=396..403 between the two SSD1683 controllers.
// Paint_SetPixel() handles this offset automatically when x >= 396.
// Our code works in VISIBLE coordinates: 0..791 x 0..271.
// =============================================================================
#define EPD_WIDTH       792    // Visible pixel width
#define EPD_HEIGHT      272    // Visible pixel height

// Display SPI pins are defined in epd/spi.h (MOSI=11, SCK=12, CS=45, DC=46, RES=47, BUSY=48)
#define EPD_PWR_PIN     7      // HIGH to enable display power
// #define LED_PWR_PIN  ??     // TODO: find LED pin for 5.79" board (not yet identified)

// Two-pane layout: each panel is half the visible width
#define PANEL_LEFT_W    396
#define PANEL_RIGHT_W   396
#define PANEL_DIVIDER_X 396

// =============================================================================
// SD Card — SPI mode
// =============================================================================
#define SD_MOSI_PIN     40
#define SD_MISO_PIN     13
#define SD_SCK_PIN      39
#define SD_CS_PIN       10

// =============================================================================
// Input — Rotary encoder + buttons
// =============================================================================
#define ROTARY_CLK_PIN  6      // "Up" direction
#define ROTARY_DT_PIN   4      // "Down" direction
#define ROTARY_SW_PIN   5      // Rotary press (select)
#define BTN_MENU_PIN    2
#define BTN_EXIT_PIN    1

#define DEBOUNCE_MS     50     // Button/rotary debounce time

// =============================================================================
// File paths on SD card
// =============================================================================
#define PATH_BOOKS      "/books"
#define PATH_BOOKMARKS  "/bookmarks"
#define PATH_STATE      "/state"
#define PATH_LIBRARY    "/state/library.json"

// =============================================================================
// WiFi AP settings
// =============================================================================
#define AP_SSID         "KiraReader"
#define AP_PASSWORD     ""     // Open network (empty = no password)
#define AP_IP           "192.168.4.1"
#define WEB_PORT        80

// =============================================================================
// UI layout constants
// =============================================================================
#define MARGIN_X        8      // Left/right text margin within a panel
#define MARGIN_Y        6      // Top margin
#define LINE_SPACING    2      // Extra pixels between lines
#define PROGRESS_BAR_H  10     // Height of progress bar at bottom
#define INDICATOR_W     16     // Width reserved for > and * indicators

// =============================================================================
// Font sizes (approximate glyph heights — will be set by actual font headers)
// =============================================================================
#define FONT_LARGE_H    28     // H1 headers
#define FONT_MEDIUM_H   22     // H2 headers
#define FONT_REGULAR_H  16     // Body text
#define FONT_BOLD_H     16     // Bold text (same height as regular)

// =============================================================================
// Event queue
// =============================================================================
#define EVENT_QUEUE_SIZE 16

// =============================================================================
// Colors (1-bit e-ink)
// =============================================================================
#define COL_BLACK       0
#define COL_WHITE       1
