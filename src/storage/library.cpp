#include "library.h"
#include "sd_manager.h"
#include <ArduinoJson.h>

static std::vector<BookEntry> entries;

namespace Library {

bool load() {
    entries.clear();

    String json = SDManager::readFile(PATH_LIBRARY);
    if (json.isEmpty()) {
        Serial.println("[Library] No library.json found, starting fresh");
        return true;  // Not an error — first boot
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[Library] JSON parse error: %s\n", err.c_str());
        return false;
    }

    JsonObject root = doc.as<JsonObject>();
    for (JsonPair kv : root) {
        BookEntry e;
        e.filename = kv.key().c_str();

        JsonObject val = kv.value().as<JsonObject>();
        String statusStr = val["status"] | "unread";
        if (statusStr == "reading")    e.status = BookStatus::READING;
        else if (statusStr == "read")  e.status = BookStatus::READ_DONE;
        else                           e.status = BookStatus::UNREAD;

        e.page = val["page"] | 0;
        e.byteOffset = val["byteOffset"] | 0;
        e.updated = val["updated"] | 0;

        entries.push_back(e);
    }

    Serial.printf("[Library] Loaded %d entries\n", entries.size());
    return true;
}

bool save() {
    JsonDocument doc;

    for (const auto& e : entries) {
        JsonObject obj = doc[e.filename].to<JsonObject>();

        switch (e.status) {
            case BookStatus::READING:   obj["status"] = "reading"; break;
            case BookStatus::READ_DONE: obj["status"] = "read"; break;
            default:                    obj["status"] = "unread"; break;
        }
        obj["page"] = e.page;
        obj["byteOffset"] = e.byteOffset;
        obj["updated"] = e.updated;
    }

    String json;
    serializeJsonPretty(doc, json);
    return SDManager::writeFile(PATH_LIBRARY, json.c_str());
}

const BookEntry* getEntry(const String& filename) {
    for (const auto& e : entries) {
        if (e.filename == filename) return &e;
    }
    return nullptr;
}

void setEntry(const String& filename, BookStatus status, uint16_t page) {
    uint32_t now = millis() / 1000;  // Approximate — no RTC on board

    for (auto& e : entries) {
        if (e.filename == filename) {
            e.status = status;
            e.page = page;
            e.updated = now;
            save();
            return;
        }
    }

    // New entry
    BookEntry e;
    e.filename = filename;
    e.status = status;
    e.page = page;
    e.updated = now;
    entries.push_back(e);
    save();
}

void removeEntry(const String& filename) {
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        if (it->filename == filename) {
            entries.erase(it);
            save();
            return;
        }
    }
}

const std::vector<BookEntry>& getAll() {
    return entries;
}

std::vector<BookEntry>& getAllMut() {
    return entries;
}

const BookEntry* getResumeBook() {
    const BookEntry* best = nullptr;
    for (const auto& e : entries) {
        if (e.status == BookStatus::READING) {
            if (!best || e.updated > best->updated) {
                best = &e;
            }
        }
    }
    return best;
}

void syncWithSD(const std::vector<String>& sdBooks) {
    // Add new books from SD that aren't in library
    for (const auto& book : sdBooks) {
        bool found = false;
        for (const auto& e : entries) {
            if (e.filename == book) { found = true; break; }
        }
        if (!found) {
            BookEntry e;
            e.filename = book;
            e.status = BookStatus::UNREAD;
            e.page = 0;
            e.updated = millis() / 1000;
            entries.push_back(e);
        }
    }

    // Remove entries for books no longer on SD
    for (auto it = entries.begin(); it != entries.end(); ) {
        bool onSD = false;
        for (const auto& book : sdBooks) {
            if (it->filename == book) { onSD = true; break; }
        }
        if (!onSD) {
            it = entries.erase(it);
        } else {
            ++it;
        }
    }

    save();
}

uint16_t countByStatus(BookStatus status) {
    uint16_t count = 0;
    for (const auto& e : entries) {
        if (e.status == status) count++;
    }
    return count;
}

const char* statusToString(BookStatus status) {
    switch (status) {
        case BookStatus::READING:   return "reading";
        case BookStatus::READ_DONE: return "read";
        default:                    return "unread";
    }
}

}  // namespace Library
