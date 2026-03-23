#pragma once

#include <Arduino.h>
#include <vector>
#include "../config.h"

// =============================================================================
// Library state manager — CRUD for /state/library.json
// =============================================================================

enum class BookStatus : uint8_t {
    UNREAD,
    READING,
    READ_DONE,  // "READ" is a macro in some contexts, so we use READ_DONE
};

struct BookEntry {
    String filename;
    BookStatus status;
    uint16_t page;
    uint32_t byteOffset;  // Byte offset in file for reading progress
    uint32_t updated;      // Unix timestamp
};

namespace Library {

// Load library.json from SD card. Call once at startup.
bool load();

// Save library.json to SD card.
bool save();

// Get entry for a book. Returns nullptr if not found.
const BookEntry* getEntry(const String& filename);

// Set/update an entry. Creates if it doesn't exist.
void setEntry(const String& filename, BookStatus status, uint16_t page);

// Remove an entry (when book is deleted)
void removeEntry(const String& filename);

// Get all entries
const std::vector<BookEntry>& getAll();

// Get mutable reference (for sorting in library view)
std::vector<BookEntry>& getAllMut();

// Get the most recently updated book with status READING (for "Resume").
// Returns nullptr if no reading book exists.
const BookEntry* getResumeBook();

// Sync library with SD card book list:
// - New files on SD get added as UNREAD
// - Entries for deleted files get removed
void syncWithSD(const std::vector<String>& sdBooks);

// Get count of books by status
uint16_t countByStatus(BookStatus status);

// Get human-readable status string
const char* statusToString(BookStatus status);

}  // namespace Library
