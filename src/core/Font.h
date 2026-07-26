#pragma once

#include <cstdint>

struct FontData {
    // these are both overridden by each font
    uint8_t size = 8;
    uint8_t spacing = 8;

    // 128 ASCII chars, up to 16 rows per char
    // can use 8-bit (8 rows) too
    uint16_t data[128][16] = {}; // Guaranteed all zeros

    // FUTURE PROPORTIONAL SPACING UPGRADE (Optional):
    // To implement variable-width character spacing (like modern coding/UI fonts):
    // 1. Add `uint8_t widths[128];` to this struct.
    // 2. Update `pack_assets.cr` / `font_txt.cr` to calculate and export the true
    //    pixel width per character (e.g., scanning the right-most active column).
    // 3. In `draw_text_immediate`, advance the horizontal cursor layout offset by
    //    `font.widths[ascii]` instead of a flat fixed width or font.spacing
};

namespace Font {
    inline constexpr FontData DEFAULT_BLANK = FontData{};
}
