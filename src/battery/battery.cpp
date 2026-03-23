#include "battery.h"
#include <cstdio>

namespace Battery {

// TODO: Replace with actual ADC read once the battery voltage pin is identified.
float voltage() {
    return 0.0f;
}

const char* voltageStr() {
    static char buf[8];
    snprintf(buf, sizeof(buf), "%.1fV", voltage());
    return buf;
}

}  // namespace Battery
