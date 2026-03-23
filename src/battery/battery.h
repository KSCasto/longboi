#pragma once

#include <Arduino.h>

namespace Battery {

// Returns battery voltage in volts.
// Currently a stub — returns 0.0 until ADC pin is wired up.
float voltage();

// Returns a formatted string like "3.7V" (static buffer, do not free).
const char* voltageStr();

}  // namespace Battery
