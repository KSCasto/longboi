#include "input.h"

// =============================================================================
// Polled input — matches Elecrow's button hardware (external pull-ups on PCB)
//
// The "rotary encoder" on the CrowPanel is actually two discrete buttons
// (UP/DOWN) plus a center press (CONF), not a true quadrature encoder.
// All buttons read LOW when pressed, HIGH when released.
// =============================================================================

static Event eventQueue[EVENT_QUEUE_SIZE];
static uint8_t queueHead = 0;
static uint8_t queueTail = 0;

// Track previous button states for edge detection
static bool prevUp = false;
static bool prevDown = false;
static bool prevSelect = false;
static bool prevMenu = false;
static bool prevExit = false;

static void enqueue(Event e) {
    uint8_t next = (queueHead + 1) % EVENT_QUEUE_SIZE;
    if (next != queueTail) {
        eventQueue[queueHead] = e;
        queueHead = next;
    }
}

namespace Input {

void init() {
    // Elecrow uses INPUT (external pull-ups on the PCB)
    pinMode(ROTARY_CLK_PIN, INPUT);  // Up / Previous
    pinMode(ROTARY_DT_PIN, INPUT);   // Down / Next
    pinMode(ROTARY_SW_PIN, INPUT);   // Confirm / Select
    pinMode(BTN_MENU_PIN, INPUT);    // Menu / Home
    pinMode(BTN_EXIT_PIN, INPUT);    // Exit
}

// Call this from the main loop — scans all buttons for press-release events
static void scan() {
    // Read current states (LOW = pressed)
    bool up     = (digitalRead(ROTARY_CLK_PIN) == LOW);
    bool down   = (digitalRead(ROTARY_DT_PIN) == LOW);
    bool select = (digitalRead(ROTARY_SW_PIN) == LOW);
    bool menu   = (digitalRead(BTN_MENU_PIN) == LOW);
    bool exit_  = (digitalRead(BTN_EXIT_PIN) == LOW);

    // Detect release edges (was pressed, now released) — like Elecrow's example
    if (prevUp && !up)         enqueue(Event::SCROLL_UP);
    if (prevDown && !down)     enqueue(Event::SCROLL_DOWN);
    if (prevSelect && !select) enqueue(Event::SELECT);
    if (prevMenu && !menu)     enqueue(Event::MENU);
    if (prevExit && !exit_)    enqueue(Event::EXIT);

    prevUp = up;
    prevDown = down;
    prevSelect = select;
    prevMenu = menu;
    prevExit = exit_;
}

Event poll() {
    // Scan buttons every poll call
    scan();

    if (queueHead == queueTail) return Event::NONE;

    Event e = eventQueue[queueTail];
    queueTail = (queueTail + 1) % EVENT_QUEUE_SIZE;
    return e;
}

Event peek() {
    scan();
    if (queueHead == queueTail) return Event::NONE;
    return eventQueue[queueTail];
}

void flush() {
    queueTail = queueHead;
}

bool isHeld(Event button) {
    switch (button) {
        case Event::SELECT: return digitalRead(ROTARY_SW_PIN) == LOW;
        case Event::MENU:   return digitalRead(BTN_MENU_PIN) == LOW;
        case Event::EXIT:   return digitalRead(BTN_EXIT_PIN) == LOW;
        default:            return false;
    }
}

}  // namespace Input
