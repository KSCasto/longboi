#include "settings.h"
#include "../storage/sd_manager.h"
#include <ArduinoJson.h>

static const char* SETTINGS_PATH = "/state/settings.json";

// Current values (defaults)
static uint8_t _fontSize = 12;
static uint8_t _lineSpacing = 1;
static uint8_t _refreshInterval = 10;
static bool _invertScroll = false;
static bool _autoResume = false;
static uint8_t _autoSleepMinutes = 0;

// Font defs for body text at each size
static const FontDef bodySmall = { 12, 7, 12, 10 };  // DejaVu Mono 8x12
static const FontDef bodyLarge = { 16, 8, 16, 13 };  // EPD font 8x16

namespace Settings {

void load() {
    String content = SDManager::readFile(SETTINGS_PATH);
    if (content.isEmpty()) return;

    JsonDocument doc;
    if (deserializeJson(doc, content)) return;

    _fontSize = doc["fontSize"] | 12;
    _lineSpacing = doc["lineSpacing"] | 1;
    _refreshInterval = doc["refreshInterval"] | 10;
    _invertScroll = doc["invertScroll"] | false;
    _autoResume = doc["autoResume"] | false;
    _autoSleepMinutes = doc["autoSleepMinutes"] | 0;

    // Validate
    if (_fontSize != 12 && _fontSize != 16) _fontSize = 12;
    if (_lineSpacing > 2) _lineSpacing = 1;
    if (_refreshInterval != 5 && _refreshInterval != 10 &&
        _refreshInterval != 20 && _refreshInterval != 50 &&
        _refreshInterval != 100) _refreshInterval = 10;
    if (_autoSleepMinutes != 0 && _autoSleepMinutes != 5 &&
        _autoSleepMinutes != 10 && _autoSleepMinutes != 30) _autoSleepMinutes = 0;

    Serial.printf("[Settings] Loaded: font=%d, spacing=%d, refresh=%d, invert=%d, resume=%d, sleep=%d\n",
                  _fontSize, _lineSpacing, _refreshInterval, _invertScroll, _autoResume, _autoSleepMinutes);
}

void save() {
    JsonDocument doc;
    doc["fontSize"] = _fontSize;
    doc["lineSpacing"] = _lineSpacing;
    doc["refreshInterval"] = _refreshInterval;
    doc["invertScroll"] = _invertScroll;
    doc["autoResume"] = _autoResume;
    doc["autoSleepMinutes"] = _autoSleepMinutes;

    String json;
    serializeJson(doc, json);
    SDManager::writeFile(SETTINGS_PATH, json.c_str());
}

uint8_t fontSize()          { return _fontSize; }
uint8_t lineSpacing()       { return _lineSpacing; }
uint8_t refreshInterval()   { return _refreshInterval; }
bool invertScroll()         { return _invertScroll; }
bool autoResume()           { return _autoResume; }
uint8_t autoSleepMinutes()  { return _autoSleepMinutes; }

void setFontSize(uint8_t size) {
    _fontSize = (size == 16) ? 16 : 12;
    save();
}

void setLineSpacing(uint8_t spacing) {
    _lineSpacing = (spacing <= 2) ? spacing : 1;
    save();
}

void setRefreshInterval(uint8_t interval) {
    _refreshInterval = interval;
    save();
}

void setInvertScroll(bool invert) {
    _invertScroll = invert;
    save();
}

void setAutoResume(bool enabled) {
    _autoResume = enabled;
    save();
}

void setAutoSleepMinutes(uint8_t minutes) {
    _autoSleepMinutes = minutes;
    save();
}

const FontDef& bodyFont() {
    return (_fontSize == 16) ? bodyLarge : bodySmall;
}

}  // namespace Settings
