#include "upload_server.h"
#include "../config.h"
#include "../storage/sd_manager.h"
#include "../storage/library.h"
#include "../bookmarks/bookmarks.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>

static AsyncWebServer* server = nullptr;
static bool running = false;

// HTML page served at root — embedded as a string to avoid SPIFFS dependency
static const char UPLOAD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>KiraReader</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: #1a1a2e; color: #eee; padding: 20px;
            max-width: 600px; margin: 0 auto;
        }
        h1 { color: #e94560; margin-bottom: 20px; }
        h2 { color: #aaa; font-size: 0.9em; margin: 20px 0 10px; text-transform: uppercase; }
        .upload-area {
            border: 2px dashed #444; border-radius: 12px; padding: 40px;
            text-align: center; margin-bottom: 20px; transition: border-color 0.3s;
        }
        .upload-area:hover { border-color: #e94560; }
        input[type="file"] { margin: 10px 0; }
        button {
            background: #e94560; color: white; border: none; padding: 12px 24px;
            border-radius: 8px; cursor: pointer; font-size: 1em; margin: 5px;
        }
        button:hover { background: #c73650; }
        button.danger { background: #666; }
        button.danger:hover { background: #e94560; }
        .book-list { list-style: none; }
        .book-list li {
            background: #16213e; padding: 12px 16px; margin: 6px 0;
            border-radius: 8px; display: flex; justify-content: space-between;
            align-items: center;
        }
        .book-name { flex: 1; }
        .status { color: #888; font-size: 0.85em; margin-left: 10px; }
        .msg { padding: 10px; border-radius: 6px; margin: 10px 0; display: none; }
        .msg.ok { background: #0f3d0f; color: #6f6; display: block; }
        .msg.err { background: #3d0f0f; color: #f66; display: block; }
        #progress { width: 100%; height: 6px; background: #333; border-radius: 3px;
                     margin: 10px 0; display: none; }
        #progress-bar { height: 100%; background: #e94560; border-radius: 3px;
                        width: 0%; transition: width 0.3s; }
    </style>
</head>
<body>
    <h1>KiraReader</h1>

    <div class="upload-area">
        <p>Upload .txt or .md files</p>
        <form id="uploadForm">
            <input type="file" id="fileInput" accept=".txt,.md" multiple><br>
            <button type="submit">Upload</button>
        </form>
        <div id="progress"><div id="progress-bar"></div></div>
        <div id="msg" class="msg"></div>
    </div>

    <h2>Library</h2>
    <ul class="book-list" id="bookList"></ul>

    <script>
        const msg = document.getElementById('msg');
        const progress = document.getElementById('progress');
        const progressBar = document.getElementById('progress-bar');

        function showMsg(text, ok) {
            msg.textContent = text;
            msg.className = 'msg ' + (ok ? 'ok' : 'err');
            setTimeout(() => msg.className = 'msg', 3000);
        }

        async function loadBooks() {
            try {
                const res = await fetch('/list');
                const books = await res.json();
                const list = document.getElementById('bookList');
                list.innerHTML = '';
                books.forEach(b => {
                    const li = document.createElement('li');
                    li.innerHTML = `
                        <span class="book-name">${b.name}</span>
                        <span class="status">${b.status} &middot; p.${b.page + 1}</span>
                        <button class="danger" onclick="deleteBook('${b.name}')">Delete</button>
                    `;
                    list.appendChild(li);
                });
            } catch(e) { console.error(e); }
        }

        document.getElementById('uploadForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const files = document.getElementById('fileInput').files;
            if (!files.length) return;

            progress.style.display = 'block';
            for (let i = 0; i < files.length; i++) {
                const formData = new FormData();
                formData.append('file', files[i]);
                progressBar.style.width = ((i / files.length) * 100) + '%';

                try {
                    const res = await fetch('/upload', { method: 'POST', body: formData });
                    if (res.ok) showMsg(files[i].name + ' uploaded!', true);
                    else showMsg('Failed: ' + files[i].name, false);
                } catch(e) {
                    showMsg('Error uploading ' + files[i].name, false);
                }
            }
            progressBar.style.width = '100%';
            setTimeout(() => { progress.style.display = 'none'; progressBar.style.width = '0%'; }, 1000);
            document.getElementById('fileInput').value = '';
            loadBooks();
        });

        async function deleteBook(name) {
            if (!confirm('Delete ' + name + '?')) return;
            try {
                const res = await fetch('/delete?name=' + encodeURIComponent(name));
                if (res.ok) { showMsg(name + ' deleted', true); loadBooks(); }
                else showMsg('Failed to delete', false);
            } catch(e) { showMsg('Error', false); }
        }

        loadBooks();
    </script>
</body>
</html>
)rawliteral";

namespace UploadServer {

void start() {
    if (running) return;

    // Configure AP
    setCpuFrequencyMhz(240);  // Max CPU for fast uploads
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, strlen(AP_PASSWORD) > 0 ? AP_PASSWORD : nullptr);
    delay(100);

    Serial.printf("[WiFi] AP started: %s, IP: %s\n", AP_SSID,
                  WiFi.softAPIP().toString().c_str());

    server = new AsyncWebServer(WEB_PORT);

    // Serve upload page
    server->on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", UPLOAD_HTML);
    });

    // List books as JSON
    server->on("/list", HTTP_GET, [](AsyncWebServerRequest* req) {
        String json = "[";
        const auto& entries = Library::getAll();
        for (size_t i = 0; i < entries.size(); i++) {
            if (i > 0) json += ",";
            json += "{\"name\":\"" + entries[i].filename + "\",";
            json += "\"status\":\"" + String(Library::statusToString(entries[i].status)) + "\",";
            json += "\"page\":" + String(entries[i].page) + "}";
        }
        json += "]";
        req->send(200, "application/json", json);
    });

    // File upload handler
    server->on("/upload", HTTP_POST,
        // Response callback (called after upload completes)
        [](AsyncWebServerRequest* req) {
            req->send(200, "text/plain", "OK");
        },
        // Upload handler (called for each chunk)
        [](AsyncWebServerRequest* req, const String& filename,
           size_t index, uint8_t* data, size_t len, bool final) {
            String path = String(PATH_BOOKS) + "/" + filename;

            if (index == 0) {
                Serial.printf("[Upload] Start: %s\n", filename.c_str());
                // Create/overwrite file
                SDManager::writeFileBytes(path.c_str(), data, len);
            } else {
                // Append subsequent chunks
                SDManager::appendFile(path.c_str(), data, len);
            }

            if (final) {
                Serial.printf("[Upload] Complete: %s (%d bytes)\n",
                              filename.c_str(), index + len);
                // Add to library
                Library::setEntry(filename, BookStatus::UNREAD, 0);
            }
        }
    );

    // Delete a book — /delete?name=filename.txt
    server->on("/delete", HTTP_GET,
        [](AsyncWebServerRequest* req) {
            if (!req->hasParam("name")) {
                req->send(400, "text/plain", "Missing name param");
                return;
            }
            String bookName = req->getParam("name")->value();
            String path = String(PATH_BOOKS) + "/" + bookName;

            if (SDManager::exists(path.c_str())) {
                SDManager::deleteFile(path.c_str());
                Bookmarks::removeAll(bookName);
                Library::removeEntry(bookName);
                req->send(200, "text/plain", "Deleted");
            } else {
                req->send(404, "text/plain", "Not found");
            }
        }
    );

    server->begin();
    running = true;
    Serial.println("[WiFi] Web server started");
}

void stop() {
    if (!running) return;

    if (server) {
        server->end();
        delete server;
        server = nullptr;
    }

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    setCpuFrequencyMhz(240);  // Back to normal
    running = false;
    Serial.println("[WiFi] Server stopped");
}

bool isRunning() {
    return running;
}

}  // namespace UploadServer
