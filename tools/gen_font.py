#!/usr/bin/env python3
"""
Generate a C bitmap font header from a TTF file.
Output format matches ascii_1206: uint8_t array[95][CHAR_H],
one byte per row, MSB = leftmost pixel, for ASCII 32-126.
"""

from PIL import Image, ImageDraw, ImageFont
import sys

FONT_PATH = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
FONT_SIZE  = 9      # PIL point size — tune this if glyphs clip
CHAR_W     = 8      # Cell width in pixels
CHAR_H     = 12     # Cell height in pixels
THRESHOLD  = 64     # Grayscale threshold for 1-bit conversion
NAME       = "dejavu_mono_12"

font = ImageFont.truetype(FONT_PATH, FONT_SIZE)

# Use font metrics to calculate a good vertical offset
ascent, descent = font.getmetrics()
total_h = ascent + descent
# Centre glyphs vertically in the cell; nudge up slightly for visual balance
y_off = max(0, (CHAR_H - total_h) // 2)

rows_out = []
for code in range(32, 127):
    c = chr(code)
    # Render onto a clean canvas
    img = Image.new('L', (CHAR_W, CHAR_H), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, y_off), c, fill=255, font=font)

    row_bytes = []
    for row in range(CHAR_H):
        byte = 0
        for col in range(CHAR_W):
            if img.getpixel((col, row)) > THRESHOLD:
                byte |= (0x80 >> col)
        row_bytes.append(f"0x{byte:02X}")
    rows_out.append((c, row_bytes))

# Write C header
lines = [
    "#pragma once",
    "#include <Arduino.h>",
    "",
    "// DejaVu Sans Mono — 8x12 bitmap font, ASCII 32-126",
    f"// Generated from {FONT_PATH} at size {FONT_SIZE}",
    f"// Format: {NAME}[char - 32][row], MSB = leftmost pixel",
    "",
    f"static const uint8_t {NAME}[][{CHAR_H}] PROGMEM = {{",
]
for c, row_bytes in rows_out:
    safe = c if c not in ('"', '\\') else f'\\{c}'
    lines.append(f"    {{{', '.join(row_bytes)}}},  /* '{safe}' */")
lines += ["};", ""]

header = "\n".join(lines)
print(header, end="")

# Quick ASCII preview to stderr for visual check
print("\n--- Preview ---", file=sys.stderr)
for code in range(32, 127):
    c = chr(code)
    _, row_bytes = rows_out[code - 32]
    rows = [int(b, 16) for b in row_bytes]
    print(f"'{c}': " + "".join("X" if rows[4] & (0x80 >> col) else "."
                                 for col in range(CHAR_W)), file=sys.stderr)
