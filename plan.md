# ESP32 E-Reader — Implementation Plan

## Project Overview

A standalone e-reader built on the **Elecrow CrowPanel ESP32 5.79" E-Paper Display**. Reads `.txt` and `.md` files from an SD card, supports bookmarks, basic Markdown rendering, and WiFi-based file upload. Landscape orientation at 792×272px.

---

## Architecture

### PlatformIO Project Structure

```
eReader/
├── platformio.ini
├── src/
│   ├── main.cpp                 # Setup/loop, top-level state machine
│   ├── config.h                 # Pin definitions, display constants, paths
│   │
│   ├── display/
│   │   ├── display.h/.cpp       # Display driver wrapper (init, full/partial refresh, landscape)
│   │   └── ui.h/.cpp            # Drawing primitives (text, progress bar, menus, HR)
│   │
│   ├── input/
│   │   └── input.h/.cpp         # Rotary encoder + buttons (debounce, event queue)
│   │
│   ├── storage/
│   │   ├── sd_manager.h/.cpp    # SD card init, file listing, read/write helpers
│   │   └── library.h/.cpp       # library.json CRUD, book status lifecycle
│   │
│   ├── reader/
│   │   ├── paginator.h/.cpp     # Page pre-calculation, byte-offset mapping
│   │   ├── renderer.h/.cpp      # Markdown line parser + font-mapped rendering
│   │   └── reader.h/.cpp        # Reading view controller (page nav, progress bar)
│   │
│   ├── bookmarks/
│   │   └── bookmarks.h/.cpp     # Bookmark CRUD, naming via rotary alphabet scroll
│   │
│   ├── views/
│   │   ├── menu_main.h/.cpp     # Main menu view
│   │   ├── menu_library.h/.cpp  # Library browser (filter/sort)
│   │   ├── menu_bookmarks.h/.cpp# Bookmark browser (two-level)
│   │   ├── menu_reading.h/.cpp  # In-reading overlay menu
│   │   └── completion.h/.cpp    # "You finished!" screen
│   │
│   ├── wifi/
│   │   └── upload_server.h/.cpp # AP mode, ESPAsyncWebServer, upload handler
│   │
│   └── fonts/
│       ├── font_large.h         # Adafruit GFX bitmap font — H1
│       ├── font_medium.h        # Adafruit GFX bitmap font — H2
│       ├── font_regular.h       # Adafruit GFX bitmap font — body
│       └── font_bold.h          # Adafruit GFX bitmap font — bold
│
├── data/                        # SPIFFS/LittleFS assets (if needed)
│   └── upload.html              # WiFi upload web page
│
└── plan.md                      # This file
```

---

## Module Breakdown

### 1. `config.h` — Pin Definitions & Constants

All hardware pin assignments, display dimensions, file paths, and tuning constants in one place.

```
- SPI pins for dual SSD1683
- Rotary encoder pins (CLK, DT, SW)
- MENU / EXIT button pins
- SD card CS pin
- Display: WIDTH=792, HEIGHT=272
- Paths: "/books/", "/bookmarks/", "/state/library.json"
- Debounce timing, AP SSID/password
```

### 2. `display/display` — Display Driver

Wraps the Elecrow CrowPanel library (dual SSD1683 drivers).

- `init()` — initialize both SSD1683 controllers, set landscape rotation, full clear
- `update(bool full = false)` — single entry point for all display refreshes:
  - `full=true` — clears 0x26/0xA6 (RED plane) to 0x00, writes content to 0x24/0xA4, triggers full update (0xF7), syncs old buffer
  - `full=false` — writes content to 0x24/0xA4, triggers partial update (0xDC), syncs old buffer; auto-promotes to full every `MAX_PARTIALS` calls
- `sleep()` / `wake()` — deep sleep and recovery

**SSD1683 dual-mode detail:** `EPD_Update()` uses 0xF7 (3-color mode) where register 0x26 is the RED color plane and must be 0x00. `EPD_PartUpdate()` uses 0xDC (B/W mode) where 0x26 is the "old image" buffer for pixel diffing. The driver handles clearing/syncing 0x26 correctly between modes.

**Key concern:** The dual-driver setup splits the screen into two halves. The Elecrow library handles this internally — we wrap it, not reimplement it.

### 3. `display/ui` — Drawing Helpers

High-level drawing on top of the display framebuffer. All menu views use a **two-pane layout** that exploits the dual SSD1683 controllers for independent half-screen refreshes.

#### Two-Pane System

The screen is divided into left (396px) and right (396px) panels:

- **Left panel** — master list (menu items, book list, etc.)
- **Right panel** — detail/preview that updates as you scroll the left panel
- Pressing SELECT on the left **activates** the right panel as a scrollable submenu
- Pressing EXIT on the right **deactivates** it, returning focus to the left

Both panels are always redrawn to the framebuffer before each refresh. The refresh strategy (full vs partial) is controlled by `Display::update()`.

#### Selection Indicators

```
  LEFT ACTIVE (browsing):           RIGHT ACTIVE (submenu entered):

┌──────────────────────┬──────────────────────┐
│                      │                      │
│ > Resume: Dune       │  Dune                │
│   Library            │  Page 42/314         │
│   Bookmarks          │  ██░░░ 13%           │
│   Upload             │  Last read: 2m ago   │
│                      │                      │
└──────────────────────┴──────────────────────┘

          SELECT on "Resume"...

┌──────────────────────┬──────────────────────┐
│                      │                      │
│ * Resume: Dune       │ > Open book          │
│   Library            │   View bookmarks     │
│   Bookmarks          │   Mark as read       │
│   Upload             │   Delete             │
│                      │                      │
└──────────────────────┴──────────────────────┘
```

- `>` — current selection in the **active** panel (only one panel has `>` at a time)
- `*` — breadcrumb on the left showing which item spawned the right submenu
- No `>` on right when it's just a passive preview
- EXIT on right panel: `*` reverts to `>`, right panel returns to preview mode

#### Input Mapping

| Input | Left Active | Right Active |
|-------|------------|-------------|
| Rotary rotate | Scroll left list, right preview updates | Scroll right submenu |
| SELECT (rotary press) | Activate right panel as submenu | Execute selected action |
| EXIT | Go back / up one level | Deactivate right, return to left |
| MENU | Return to main menu | Return to main menu |

#### Drawing Functions

- `drawText(x, y, text, font)` — render a string with a given font
- `drawProgressBar(current, total)` — thin filled rect at bottom of screen
- `drawPageNumber(current, total)` — "page 4/67" text next to progress bar
- `drawHorizontalRule()` — full-width line for `---`
- `drawLeftPanel(items[], selectedIndex, activeItem, isActive)` — left pane with `>` or `*` indicators
- `drawRightPanel(content)` — right pane for preview (no `>`)
- `drawRightMenu(items[], selectedIndex)` — right pane as active submenu (with `>`)
- `drawStatusLine(text)` — single-line status messages

### 4. `input/input` — Input Handler

Interrupt-driven with a small event queue.

- **Rotary rotation** → `EVENT_SCROLL_UP` / `EVENT_SCROLL_DOWN`
- **Rotary press** → `EVENT_SELECT`
- **MENU button** → `EVENT_MENU`
- **EXIT button** → `EVENT_EXIT`
- Debounce all inputs (hardware bouncing is real on rotary encoders)
- `pollEvent()` — returns next event from queue, or `EVENT_NONE`

### 5. `storage/sd_manager` — SD Card I/O

- `init()` — mount SD card (FAT32)
- `listBooks()` — return list of `.txt`/`.md` files in `/books/`
- `readChunk(path, offset, length)` — read a byte range from a file
- `fileSize(path)` — get file size in bytes
- `writeFile(path, data)` — write/overwrite a file
- `deleteFile(path)` — delete a file
- `deleteDir(path)` — recursive directory delete (for bookmark cleanup)
- `exists(path)` — check if file/dir exists

### 6. `storage/library` — Library State

Manages `/state/library.json` via ArduinoJson.

- `load()` / `save()` — read/write JSON from SD
- `getEntry(filename)` — get status/page/timestamp for a book
- `setEntry(filename, status, page)` — update with current timestamp
- `removeEntry(filename)` — delete entry (when book file deleted)
- `getResumeBook()` — find most recently updated "reading" entry
- `syncWithSD(bookList)` — add new books as "unread", remove entries for deleted books
- Status enum: `UNREAD`, `READING`, `READ`

### 7. `reader/paginator` — Page Calculator

Pre-calculates page boundaries on file open.

- `paginate(filePath, displayWidth, displayHeight, fonts)` — scan entire file
  - Simulates line-by-line rendering with word wrap
  - Accounts for font sizes (H1/H2/bold/regular lines have different heights)
  - Reserves bottom region for progress bar
  - Stores array of byte offsets: `pageOffsets[pageNum] = byteOffset`
  - Returns `totalPages`
- `getPageOffset(pageNum)` — byte offset for start of page N
- `getPageEndOffset(pageNum)` — byte offset for end of page N (or start of N+1)

**Performance note:** For a 500KB book, scanning the whole file on ESP32 at 240MHz with PSRAM should take <1 second. Show a "Loading..." screen during pagination.

### 8. `reader/renderer` — Markdown Parser & Renderer

Line-by-line parser, no AST. Processes raw text into draw calls.

For each line:
1. Check prefix: `# ` → large font, `## ` → medium font, `---` → horizontal rule
2. Scan for `**...**` segments → bold font spans
3. Everything else → regular font
4. Word-wrap to display width using the active font's character widths
5. Draw via `ui` module

### 9. `reader/reader` — Reading View Controller

Owns the reading experience for an open book.

- `open(filename)` — load file, run paginator, restore last page from library.json
- `renderCurrentPage()` — read page bytes, pass to renderer, draw progress bar
- `nextPage()` / `prevPage()` — navigate + partial refresh
- `getCurrentPage()` / `getTotalPages()` — for progress display
- `close()` — save current page to library.json
- Detects last page → triggers completion screen

### 10. `bookmarks/bookmarks` — Bookmark Manager

- `loadBookmarks(bookName)` — list `.bm` files in `/bookmarks/<bookName>/`
- `saveBookmark(bookName, page, name)` — create `.bm` file
- `deleteBookmark(bookName, bmFile)` — remove a bookmark
- `deleteAllBookmarks(bookName)` — remove folder (book deletion cleanup)
- `getBooksWithBookmarks()` — list subdirectories of `/bookmarks/`
- Bookmark naming: default `page_N`, or user-entered via rotary alphabet scroll

### 11. `views/*` — UI Views

Each view is a self-contained screen using the two-pane system. Left panel = master list, right panel = preview that becomes an interactive submenu on SELECT.

**`menu_main`** — Main menu

| Left Panel | Right Panel (preview) | Right Panel (submenu) |
|-----------|----------------------|----------------------|
| > Resume: Dune | Book name, page, progress bar, last read | Open book, View bookmarks, Mark as read |
| Library | Book counts: "3 unread, 1 reading, 5 read" | *(enters library view instead)* |
| Bookmarks | Total bookmarks, most recent bookmark | *(enters bookmarks view instead)* |
| Upload | "Creates WiFi network..." instructions | *(activates upload mode)* |
| Settings | Placeholder | Placeholder options |

- Resume hidden if no "reading" book exists
- Library/Bookmarks/Upload navigate to their own views (no right submenu — SELECT goes directly)
- Resume is the one that benefits most from the right submenu pattern

**`menu_library`** — Library browser

| Left Panel | Right Panel (preview) | Right Panel (submenu) |
|-----------|----------------------|----------------------|
| > Dune [reading] | Status, page, progress, last read, bookmark count | Open, Change status, Delete |
| Moby Dick [unread] | Status: unread, no progress | Open, Delete |
| Dracula [read] | Status: read, page 314/314, 100% | Reopen, Delete |

- MENU button opens filter/sort overlay (All/Unread/Reading/Read, Name/Updated/Status)
- "Delete" in right submenu triggers confirmation before removing book + bookmarks + library entry

**`menu_bookmarks`** — Bookmark browser

| Left Panel | Right Panel (preview) | Right Panel (submenu) |
|-----------|----------------------|----------------------|
| > Dune | List of bookmarks: page_42, good_part | > page_42 (scroll to pick) |
| Moby Dick | List of bookmarks: chapter_5 | > chapter_5 |

- SELECT on right submenu bookmark → jump to that page in reader
- Preview shows bookmark names + page numbers
- Active right panel lets you scroll through and select a specific bookmark

**`menu_reading`** — In-reading overlay
- Triggered by MENU during reading
- Single-pane overlay (not two-pane — reading uses full width)
- Options: Add Bookmark, View Bookmarks, Mark as Read, Back to Library
- EXIT dismisses overlay, returns to reading

**`completion`** — Finish screen
- Full-width screen (not two-pane)
- "You finished [Book]!"
- Options: Mark as Read / Keep as Reading
- Then return to main menu

### 12. `wifi/upload_server` — WiFi Upload

- `start()` — configure AP mode, start ESPAsyncWebServer on port 80
- `stop()` — tear down AP, free resources
- Serves `upload.html` from SPIFFS/LittleFS (or hardcoded string)
- `POST /upload` — receive multipart file, write to `/books/`, add to library.json
- `GET /list` — JSON list of books (for optional file manager)
- `DELETE /book/:name` — delete book + bookmarks + library entry
- Display shows "WiFi Upload Mode" with SSID and IP while active

---

## State Machine (main.cpp)

```
                    ┌─────────┐
                    │  BOOT   │
                    └────┬────┘
                         │ init SD, display, input, load library
                         ▼
                  ┌──────────────┐
           ┌──── │  MAIN MENU   │ ◄──────────────────────┐
           │     └──────┬───────┘                         │
           │            │ select                          │
           ▼            ▼            ▼            ▼       │
      ┌─────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐  │
      │ RESUME  │ │ LIBRARY  │ │BOOKMARKS │ │ UPLOAD │  │
      │ (reader)│ │  browse  │ │  browse  │ │ AP mode│  │
      └────┬────┘ └────┬─────┘ └────┬─────┘ └───┬────┘  │
           │           │ select      │ select     │       │
           │           ▼             ▼            │       │
           │      ┌──────────┐  jump to page      │       │
           ├───── │ READING  │ ◄──────────────────┘       │
           │      └────┬─────┘                            │
           │           │ last page                        │
           │           ▼                                  │
           │     ┌────────────┐                           │
           │     │ COMPLETION │                           │
           │     └─────┬──────┘                           │
           │           │                                  │
           └───────────┴──────────────────────────────────┘
```

Top-level `loop()` is a switch on the current state. Each state calls into its view module, which handles rendering and input internally, then returns a result indicating the next state.

---

## Implementation Order

### Phase 1 — Foundation
1. **PlatformIO project setup** — `platformio.ini` with ESP32-S3, required libraries
2. **config.h** — all pin defs and constants
3. **display/display** — get the screen working in landscape (start from Elecrow examples)
4. **display/ui** — basic text drawing, verify fonts render correctly
5. **input/input** — rotary + button events with debounce

### Phase 2 — Storage & State
6. **storage/sd_manager** — mount SD, list/read/write files
7. **storage/library** — library.json CRUD
8. **fonts/** — generate/import 4 bitmap font variants

### Phase 3 — Core Reading
9. **reader/paginator** — page pre-calculation with word wrap
10. **reader/renderer** — Markdown line parser + rendering
11. **reader/reader** — reading view with navigation and progress bar

### Phase 4 — UI Views
12. **views/menu_main** — main menu with resume
13. **views/menu_library** — library browser with filter/sort
14. **views/completion** — finish screen
15. **State machine in main.cpp** — wire it all together

### Phase 5 — Bookmarks
16. **bookmarks/bookmarks** — bookmark CRUD
17. **views/menu_bookmarks** — bookmark browser
18. **views/menu_reading** — in-reading overlay menu

### Phase 6 — WiFi Upload
19. **wifi/upload_server** — AP mode + web server
20. **data/upload.html** — upload page UI

### Phase 7 — Polish
21. Partial refresh tuning for page turns
22. Sleep/power management
23. Edge cases: empty library, corrupt JSON, SD removal
24. Cleanup rules: book deletion cascades to bookmarks + library

---

## platformio.ini (Draft)

```ini
[env:crowpanel-epaper]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
board_build.arduino.memory_type = qio_opi
board_build.psram = enabled

monitor_speed = 115200

lib_deps =
    bblanchon/ArduinoJson@^7
    ESP Async WebServer
    adafruit/Adafruit GFX Library
    ; Elecrow CrowPanel library — add via lib_extra_dirs or manual include

board_build.partitions = default_8MB.csv
board_upload.flash_size = 8MB
```

---

## Key Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Build system | PlatformIO | Better dependency management than Arduino IDE, CLI-friendly |
| State management | Single `library.json` | Simpler than separate files; one read/write point |
| Pagination | Pre-calculate on open | Avoids re-scanning on every page turn; total pages known upfront |
| Page storage | Byte offset array | O(1) random access to any page; array fits in PSRAM easily |
| Markdown parsing | Line-by-line, no AST | Minimal memory, fast, sufficient for supported subset |
| View architecture | State machine + view functions | Simple, no framework overhead, easy to extend |
| Fonts | Adafruit GFX bitmaps in flash | Proven format, no runtime TTF rendering needed |
| WiFi upload | AP mode only | No router dependency; works anywhere |
| Menu layout | Two-pane master-detail | Rich previews without clutter; left = master list, right = detail or submenu |
| Selection indicators | `>` active, `*` breadcrumb | Unambiguous focus — only one panel has `>` at a time |
| Refresh strategy | Single `update(bool full)` — partial for scroll within a view, full for entering/exiting submenus and view transitions; reader page turns use partial with auto-promotion to full every 3 pages to clear ghosting | Partial waveform (0xDC/B/W mode) only viable for small incremental changes; full waveform (0xF7/3-color mode) required when 0x26 must be 0x00 |
