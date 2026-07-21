#pragma once

#include <cstdint>

struct FontData {
    uint8_t size = 16;
    uint8_t spacing = 10;
    using RowType = uint16_t;

    // 128 ASCII chars, up to 16 rows per char
    RowType data[128][16] = {}; // Guaranteed all zeros
};

namespace Font {
    inline constexpr FontData DEFAULT_BLANK = FontData{};
}
