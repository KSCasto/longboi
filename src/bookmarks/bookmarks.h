#pragma once

#include <Arduino.h>
#include <vector>
#include "../config.h"

// =============================================================================
// Bookmark manager — CRUD for .bm files in /bookmarks/<bookname>/
// =============================================================================

struct Bookmark {
    String name;        // e.g. "page_42" or user-given name
    uint16_t page;      // Page number (0-indexed)
    uint32_t timestamp; // When it was created
};

namespace Bookmarks {

// Load all bookmarks for a specific book
std::vector<Bookmark> load(const String& bookName);

// Save a new bookmark. name can be "" for auto-naming ("page_N")
bool save(const String& bookName, uint16_t page, const String& name = "");

// Delete a specific bookmark
bool remove(const String& bookName, const String& bmName);

// Delete all bookmarks for a book (when book is deleted)
bool removeAll(const String& bookName);

// Get list of books that have bookmarks (subdirectory names in /bookmarks/)
std::vector<String> booksWithBookmarks();

// Count bookmarks for a specific book
uint16_t count(const String& bookName);

// Strip file extension from a book filename to get the bookmark folder name
// e.g. "dune.txt" → "dune"
String bookToFolderName(const String& bookFilename);

}  // namespace Bookmarks
