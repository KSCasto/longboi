#include "sd_manager.h"

static SPIClass sdSPI(HSPI);
static bool mounted = false;

namespace SDManager {

bool init() {
    // GPIO 42 is a second power enable pin required by the CrowPanel hardware
    pinMode(42, OUTPUT);
    digitalWrite(42, HIGH);
    delay(10);

    sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN);

    if (!SD.begin(SD_CS_PIN, sdSPI, 80000000)) {
        Serial.println("[SD] Mount failed");
        mounted = false;
        return false;
    }

    Serial.printf("[SD] Mounted. Type: %d, Size: %lluMB\n",
                  SD.cardType(), SD.cardSize() / (1024 * 1024));
    mounted = true;
    ensureDirectories();
    return true;
}

bool isMounted() {
    return mounted;
}

std::vector<String> listBooks() {
    std::vector<String> books;
    if (!mounted) return books;

    File dir = SD.open(PATH_BOOKS);
    if (!dir || !dir.isDirectory()) return books;

    File entry;
    while ((entry = dir.openNextFile())) {
        if (entry.isDirectory()) continue;

        String name = entry.name();
        // entry.name() returns just the filename on ESP32 SD library
        if (name.endsWith(".txt") || name.endsWith(".md") ||
            name.endsWith(".TXT") || name.endsWith(".MD")) {
            books.push_back(name);
        }
        entry.close();
    }
    dir.close();
    return books;
}

size_t readChunk(const char* path, uint8_t* buffer, size_t offset, size_t length) {
    if (!mounted) return 0;

    File f = SD.open(path, FILE_READ);
    if (!f) return 0;

    if (!f.seek(offset)) {
        f.close();
        return 0;
    }

    size_t bytesRead = f.read(buffer, length);
    f.close();
    return bytesRead;
}

String readFile(const char* path) {
    if (!mounted) return "";

    File f = SD.open(path, FILE_READ);
    if (!f) return "";

    String content = f.readString();
    f.close();
    return content;
}

bool writeFile(const char* path, const char* data) {
    if (!mounted) return false;

    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;

    size_t written = f.print(data);
    f.close();
    return written > 0;
}

bool writeFileBytes(const char* path, const uint8_t* data, size_t length) {
    if (!mounted) return false;

    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;

    size_t written = f.write(data, length);
    f.close();
    return written == length;
}

bool appendFile(const char* path, const uint8_t* data, size_t length) {
    if (!mounted) return false;

    File f = SD.open(path, FILE_APPEND);
    if (!f) return false;

    size_t written = f.write(data, length);
    f.close();
    return written == length;
}

size_t fileSize(const char* path) {
    if (!mounted) return 0;

    File f = SD.open(path, FILE_READ);
    if (!f) return 0;

    size_t sz = f.size();
    f.close();
    return sz;
}

bool exists(const char* path) {
    if (!mounted) return false;
    return SD.exists(path);
}

bool deleteFile(const char* path) {
    if (!mounted) return false;
    return SD.remove(path);
}

bool deleteDir(const char* path) {
    if (!mounted) return false;

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) return false;

    // Delete all files and subdirectories first
    File entry;
    while ((entry = dir.openNextFile())) {
        String entryPath = String(path) + "/" + entry.name();
        if (entry.isDirectory()) {
            entry.close();
            deleteDir(entryPath.c_str());
        } else {
            entry.close();
            SD.remove(entryPath.c_str());
        }
    }
    dir.close();

    return SD.rmdir(path);
}

bool createDir(const char* path) {
    if (!mounted) return false;
    if (SD.exists(path)) return true;
    return SD.mkdir(path);
}

std::vector<String> listDirs(const char* path) {
    std::vector<String> dirs;
    if (!mounted) return dirs;

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) return dirs;

    File entry;
    while ((entry = dir.openNextFile())) {
        if (entry.isDirectory()) {
            dirs.push_back(entry.name());
        }
        entry.close();
    }
    dir.close();
    return dirs;
}

std::vector<String> listFiles(const char* path) {
    std::vector<String> files;
    if (!mounted) return files;

    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) return files;

    File entry;
    while ((entry = dir.openNextFile())) {
        if (!entry.isDirectory()) {
            files.push_back(entry.name());
        }
        entry.close();
    }
    dir.close();
    return files;
}

void ensureDirectories() {
    createDir(PATH_BOOKS);
    createDir(PATH_BOOKMARKS);
    createDir(PATH_STATE);
}

}  // namespace SDManager
