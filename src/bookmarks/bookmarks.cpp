#include "bookmarks.h"
#include "../storage/sd_manager.h"
#include <ArduinoJson.h>

namespace Bookmarks {

String bookToFolderName(const String& bookFilename) {
    int dot = bookFilename.lastIndexOf('.');
    if (dot > 0) return bookFilename.substring(0, dot);
    return bookFilename;
}

static String bmDirPath(const String& bookName) {
    return String(PATH_BOOKMARKS) + "/" + bookToFolderName(bookName);
}

std::vector<Bookmark> load(const String& bookName) {
    std::vector<Bookmark> bms;
    String dirPath = bmDirPath(bookName);

    if (!SDManager::exists(dirPath.c_str())) return bms;

    auto files = SDManager::listFiles(dirPath.c_str());
    for (const auto& file : files) {
        if (!file.endsWith(".bm")) continue;

        String filePath = dirPath + "/" + file;
        String content = SDManager::readFile(filePath.c_str());
        if (content.isEmpty()) continue;

        JsonDocument doc;
        if (deserializeJson(doc, content)) continue;

        Bookmark bm;
        bm.name = doc["name"] | file.substring(0, file.length() - 3);
        bm.page = doc["page"] | 0;
        bm.timestamp = doc["timestamp"] | 0;
        bms.push_back(bm);
    }

    // Sort by page number
    std::sort(bms.begin(), bms.end(), [](const Bookmark& a, const Bookmark& b) {
        return a.page < b.page;
    });

    return bms;
}

bool save(const String& bookName, uint16_t page, const String& name) {
    String dirPath = bmDirPath(bookName);
    SDManager::createDir(dirPath.c_str());

    // Auto-name if no name provided
    String bmName = name;
    if (bmName.isEmpty()) {
        bmName = "page_" + String(page);
    }

    String filePath = dirPath + "/" + bmName + ".bm";

    JsonDocument doc;
    doc["name"] = bmName;
    doc["page"] = page;
    doc["timestamp"] = millis() / 1000;

    String json;
    serializeJson(doc, json);
    return SDManager::writeFile(filePath.c_str(), json.c_str());
}

bool remove(const String& bookName, const String& bmName) {
    String filePath = bmDirPath(bookName) + "/" + bmName + ".bm";
    return SDManager::deleteFile(filePath.c_str());
}

bool removeAll(const String& bookName) {
    String dirPath = bmDirPath(bookName);
    if (!SDManager::exists(dirPath.c_str())) return true;
    return SDManager::deleteDir(dirPath.c_str());
}

std::vector<String> booksWithBookmarks() {
    return SDManager::listDirs(PATH_BOOKMARKS);
}

uint16_t count(const String& bookName) {
    String dirPath = bmDirPath(bookName);
    if (!SDManager::exists(dirPath.c_str())) return 0;

    auto files = SDManager::listFiles(dirPath.c_str());
    uint16_t cnt = 0;
    for (const auto& f : files) {
        if (f.endsWith(".bm")) cnt++;
    }
    return cnt;
}

}  // namespace Bookmarks
