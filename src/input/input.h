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

}  // namespace Input
