#pragma once

#include <Arduino.h>
#include "../config.h"
#include "../fonts/fonts.h"

// =============================================================================
// Markdown renderer — line-by-line parser that draws to the display buffer
//
// Supported syntax:
//   # Title     → large font
//   ## Title    → medium font
//   **bold**    → bold font
//   ---         → horizontal rule
//   plain text  → regular font
// =============================================================================

namespace Renderer {

// Render a page of text into the display buffer.
// text: raw text content for this page (null-terminated)
// x, y: top-left starting position
// maxX: right boundary for word wrapping
// maxY: bottom boundary (stop rendering if exceeded)
// Returns the y position after the last rendered line.
int16_t renderPage(const char* text, int16_t x, int16_t y, int16_t maxX, int16_t maxY);

// Render a single line with Markdown formatting.
// Returns the y position after this line.
int16_t renderLine(const char* line, int16_t x, int16_t y, int16_t maxX);

}  // namespace Renderer
