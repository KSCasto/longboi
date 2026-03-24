#pragma once

#include <Arduino.h>
#include "../config.h"
#include "../fonts/fonts.h"

// =============================================================================
// Persistent settings — stored in /state/settings.json on SD card
// =============================================================================

namespace Settings {

// Load from SD card (call once at boot). Uses defaults if file missing.
void load();

// Save current settings to SD card.
void save();

// --- Font size (body text) ---------------------------------------------------
// 12 = small (7px wide, 12px tall, DejaVu Mono) — default
// 16 = large (8px wide, 16px tall, EPD font)
uint8_t fontSize();
void setFontSize(uint8_t size);

// --- Line spacing ------------------------------------------------------------
// 0 = tight, 1 = normal (default), 2 = loose
uint8_t lineSpacing();
void setLineSpacing(uint8_t spacing);

// --- Refresh interval --------------------------------------------------------
// Number of partial refreshes before forcing a full refresh.
// Options: 5, 10, 20, 50, 100. Default: 10.
uint8_t refreshInterval();
void setRefreshInterval(uint8_t interval);

// --- Scroll direction --------------------------------------------------------
// false = rotary up is prev page (default)
// true  = rotary up is next page
bool invertScroll();
void setInvertScroll(bool invert);

// --- Auto resume -------------------------------------------------------------
// true = boot directly into last book instead of main menu
bool autoResume();
void setAutoResume(bool enabled);

// --- Auto sleep --------------------------------------------------------------
// Minutes of inactivity before deep sleep. 0 = disabled.
// Options: 0, 5, 10, 30.
uint8_t autoSleepMinutes();
void setAutoSleepMinutes(uint8_t minutes);

// --- Bold text ---------------------------------------------------------------
// true = use bold font variants everywhere
bool boldEnabled();
void setBoldEnabled(bool enabled);

}  // namespace Settings
