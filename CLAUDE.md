# ESP32 E-Reader — Claude Context

## Hardware
- **MCU:** Elecrow CrowPanel ESP32-S3 5.79" E-Paper Display
- **Display:** 792×272px landscape, dual SSD1683 controllers (handled internally by Elecrow library)
- **Input:** Rotary encoder (CLK/DT/SW) + MENU and EXIT buttons
- **Storage:** SD card (FAT32), SPIFFS/LittleFS for web assets
- **Framework:** Arduino via PlatformIO, ESP32-S3, 8MB flash, PSRAM enabled


## Project Structure

```
eReader/
├── platformio.ini
├── plan.md
├── data/                        # SPIFFS assets
├── src/
│   ├── main.cpp                 # Setup/loop, top-level state machine
│   ├── config.h                 # Pin definitions, display constants, paths
│   │
│   ├── epd/                     # Elecrow CrowPanel library (do not modify)
│   │   ├── EPD.h/.cpp           # Core EPD driver
│   │   ├── EPD_Init.h/.cpp      # Display initialization
│   │   ├── EPDfont.h            # Font types for EPD layer
│   │   └── spi.h/.cpp           # SPI setup (pin defs live here: MOSI=11, SCK=12, CS=45, DC=46, RES=47, BUSY=48)
│   │
│   ├── display/
│   │   ├── display.h/.cpp       # Display driver wrapper (init, full/partial refresh, landscape)
│   │   └── ui.h/.cpp            # Drawing primitives (text, progress bar, menus, HR)
│   │
│   ├── fonts/
│   │   └── fonts.h/.cpp         # Bitmap font definitions (H1/H2/body/bold)
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
│   │   ├── views.h/.cpp         # Shared view utilities
│   │   ├── menu_main.h/.cpp
│   │   ├── menu_library.h/.cpp
│   │   ├── menu_bookmarks.h/.cpp
│   │   ├── menu_reading.h/.cpp  # In-reading overlay menu
│   │   └── completion.h/.cpp
│   │
│   └── wifi/
│       └── upload_server.h/.cpp # AP mode, ESPAsyncWebServer, upload handler
```

---

## Pin Assignments (from config.h)

| Group | Pin | Note |
|-------|-----|------|
| Display SPI | MOSI=11, SCK=12, CS=45, DC=46, RES=47, BUSY=48 | Defined in epd/spi.h |
| Display power | EPD_PWR_PIN=7 | HIGH to enable |
| SD card | MOSI=40, MISO=13, SCK=39, CS=10 | Separate SPI bus |
| Rotary encoder | CLK=6, DT=4, SW=5 | CLK=up, DT=down |
| Buttons | MENU=2, EXIT=1 | |

---

## Critical Hardware Notes

### SSD1683 Dual-Driver Display
- Screen is split into two 396px halves managed by dual SSD1683 controllers
- **Full refresh (0xF7):** 3-color mode — register 0x26 (RED plane) **must be 0x00**; writes content to 0x24/0xA4; triggers via 0xF7; syncs old buffer
- **Partial refresh (0xDC):** B/W mode — 0x26 acts as "old image" buffer for pixel diffing; triggers via 0xDC; auto-promotes to full every `MAX_PARTIALS` calls to clear ghosting
- Single entry point: `Display::update(bool full)` — caller decides full vs partial
- **8-pixel gap at x=396..403** between the two controllers — the Elecrow library uses an 800px internal buffer (EPD_W=800); `Paint_SetPixel()` handles the offset automatically for x≥396. Always work in visible coordinates 0..791 × 0..271, never raw buffer coordinates
- Do not reimplement the dual-driver split; the Elecrow library handles it internally

---

## Architecture & Key Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| State management | Single `library.json` on SD | One read/write point; simpler than separate files |
| Pagination | Pre-calculate byte offsets on file open | O(1) page access; total pages known upfront; array fits in PSRAM |
| Markdown parsing | Line-by-line, no AST | Minimal memory, fast, sufficient for `#`, `##`, `---`, `**bold**` |
| View architecture | State machine + view modules | No framework overhead; each view handles its own input/render loop |
| Fonts | Adafruit GFX bitmaps in flash | No runtime TTF rendering; proven format |
| WiFi upload | AP mode only | No router dependency |
| Menu layout | Two-pane master-detail | Left = master list, right = preview or active submenu |
| Selection indicators | `>` = active focus, `*` = breadcrumb | Only one panel has `>` at a time |
| Refresh strategy | Partial for scroll/nav within a view; full for view transitions and every 3rd page turn | Partial (0xDC) only viable for incremental changes; full (0xF7) required when 0x26 must be cleared |

---

## Two-Pane UI System

Screen divided into left (396px) and right (396px) panels:
- **Left panel** — master list (always visible)
- **Right panel** — passive preview when browsing; becomes active submenu on SELECT
- `>` — current selection in the **active** panel (only one panel at a time)
- `*` — breadcrumb on left showing which item spawned the right submenu
- EXIT on right: returns focus to left, right reverts to preview

### Input Mapping
| Input | Left Active | Right Active |
|-------|-------------|--------------|
| Rotary rotate | Scroll left, right preview updates | Scroll right submenu |
| SELECT | Activate right as submenu | Execute action |
| EXIT | Go back one level | Deactivate right, return to left |
| MENU | Return to main menu | Return to main menu |

---

## State Machine (main.cpp)

```
BOOT → init SD/display/input/library → MAIN MENU
MAIN MENU → RESUME (reader) | LIBRARY browse | BOOKMARKS browse | UPLOAD AP mode
LIBRARY browse → READING (open book)
BOOKMARKS browse → READING (jump to page)
READING → COMPLETION (last page) → MAIN MENU
```

Top-level `loop()` is a switch on current state. Each state calls its view module, which handles rendering and input internally, then returns the next state.

---

## File Paths & Constants (defined in config.h)
- Books: `/books/`
- Bookmarks: `/bookmarks/<bookName>/`
- Library state: `/state/library.json`
- Book status enum: `UNREAD`, `READING`, `READ`
- Bookmark files: `.bm` extension