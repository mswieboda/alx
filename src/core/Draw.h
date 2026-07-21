#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <cstdint>
#include "Font.h"

namespace Draw {

    // --- 1. DATA PAYLOADS ---
    struct TextData {
        std::string_view text;
        uint32_t color;
        int scale;
        const FontData* font;
    };

    struct RectData {
        int width;
        int height;
        uint32_t color;
        bool fill;
        int thickness;
    };

    struct SpriteData {
        const uint8_t* pixel_data;
        uint32_t pixel_data_size;
        int width;
        int height;
        int src_x;
        int src_y;
        int src_w;
        int src_h;
    };

    // --- 2. UNIFIED PACKET ---
    struct Command {
        int x;
        int y;
        int z_index;
        int sort_y;
        std::variant<TextData, RectData, SpriteData> data;
    };

    // --- 3. PUBLIC PIPELINE INTERFACE ---
    enum class YSortMode {
        None,
        TopY,
        YPlusHeight
    };

    void set_y_sort_mode(YSortMode mode);
    YSortMode get_y_sort_mode();
    
    void set_palette(const uint32_t* palette);

    // Submit actions to the frame queue
    void text(int x, int y, std::string_view text, uint32_t color,
              int scale = 1, int z_index = 1,
              const FontData* font = &Font::DEFAULT_BLANK);
    void rect(int x, int y, int width, int height, uint32_t color, bool fill = true, int thickness = 1, int z_index = 1);
    void sprite(int x, int y, const uint8_t* pixel_data, uint32_t pixel_data_size, int width, int height, int z_index = 1);
    void sprite_frame(
        int screen_x, int screen_y,
        const uint8_t* sheet_pixels, uint32_t sheet_pixels_size,
        int sheet_width, int sheet_height,
        int src_x, int src_y, int src_w, int src_h,
        int z_index = 1
    );

    // Process, order, and draw everything to the screen buffer
    void flush_pipeline(std::vector<uint32_t>& buffer, uint32_t background_color);
}