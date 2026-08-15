#pragma once

#include "assets/Fonts.h"

namespace TextStyles {
    inline constexpr const FontData& font = Assets::Fonts::fant_8;

    constexpr uint32_t color = 0xFF00CCCC;
    constexpr uint32_t color_shadow = 0xFF003344;

    constexpr int scale = 1;
    constexpr int scale_med = 2;
    constexpr int scale_big = 3;
    constexpr int menu_item_text_line_height = 12;
}
