#pragma once

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <vector>
#include "../config.h"

// =============================================================================
// SD card file I/O manager
// =============================================================================

namespace SDManager {

// Initialize SD card on the dedicated SPI bus. Returns true on success.
bool init();

// Check if the SD card is currently mounted
bool isMounted();

// List all .txt and .md files in /books/
std::vector<String> listBooks();

// Read a chunk of bytes from a file at the given offset.
// Returns the number of bytes actually read.
size_t readChunk(const char* path, uint8_t* buffer, size_t offset, size_t length);

// Read an entire file into a String (use for small files like library.json, .bm)
String readFile(const char* path);

// Write a string to a file (creates or overwrites)
bool writeFile(const char* path, const char* data);

// Write raw bytes to a file (for upload handler)
bool writeFileBytes(const char* path, const uint8_t* data, size_t length);

// Append data to an existing file
bool appendFile(const char* path, const uint8_t* data, size_t length);

// Get file size in bytes. Returns 0 if file doesn't exist.
size_t fileSize(const char* path);

// Check if a file or directory exists
bool exists(const char* path);

// Delete a file
bool deleteFile(const char* path);

// Recursively delete a directory and all contents
bool deleteDir(const char* path);

// Create a directory (and parents if needed)
bool createDir(const char* path);

// List subdirectories in a given path
std::vector<String> listDirs(const char* path);

// List files in a given directory (just filenames, not full paths)
std::vector<String> listFiles(const char* path);

// Ensure required directories exist (/books, /bookmarks, /state)
void ensureDirectories();

}  // namespace SDManager
