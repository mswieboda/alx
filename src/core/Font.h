#pragma once

#include <cstdint>

struct FontData {
    // these are both overridden by each font
    uint8_t size = 8;
    uint8_t spacing = 8;

    // 128 ASCII chars, up to 16 rows per char
    // can use 8-bit (8 rows) too
    uint16_t data[128][16] = {}; // Guaranteed all zeros
};

namespace Font {
    inline constexpr FontData DEFAULT_BLANK = FontData{};
}
