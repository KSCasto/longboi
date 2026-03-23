#pragma once

#include <Arduino.h>
#include "../config.h"

// =============================================================================
// Input events from rotary encoder and buttons
// =============================================================================

enum class Event : uint8_t {
    NONE,
    SCROLL_UP,      // Rotary turned clockwise
    SCROLL_DOWN,    // Rotary turned counter-clockwise
    SELECT,         // Rotary switch pressed
    MENU,           // MENU button pressed
    EXIT,           // EXIT button pressed
};

namespace Input {

// Initialize GPIO pins and attach interrupts
void init();

// Poll the next event from the queue. Returns Event::NONE if empty.
Event poll();

// Peek at the next event without removing it
Event peek();

// Clear all pending events
void flush();

// Check if a specific button is currently held down (for long-press detection)
bool isHeld(Event button);

// Enter light sleep with GPIO wakeup (any button) + 10ms timer fallback.
// Saves ~35mA vs active polling with delay().
void lightSleep();

// Reset the activity timer (for auto-sleep tracking)
void resetActivity();

// Get milliseconds since last activity
uint32_t idleMillis();

// Enter deep sleep if idle timeout exceeded
void checkAutoSleep();

}  // namespace Input
