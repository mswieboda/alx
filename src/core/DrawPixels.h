#pragma once
#include <vector>
#include <string_view>
#include <cstdint>
#include "Font.h"

#ifndef ALX_DRAW_INTERNAL_ALLOW
#error "DrawPixels.h is a private backend header and must not be included directly. Use core/Draw.h instead."
#endif

namespace DrawPixels {

    void clear(std::vector<uint32_t>& buf, uint32_t color);

    void rect(std::vector<uint32_t>& buf, int rx, int ry, int rw, int rh, uint32_t color, bool fill, int thickness);

    void oval(std::vector<uint32_t>& buf, float cx, float cy, float rx, float ry, uint32_t color, bool fill, int thickness);

    void line(std::vector<uint32_t>& buf, int x1, int y1, int x2, int y2, uint32_t color, int thickness);

    void text(std::vector<uint32_t>& buf, int x, int y, std::string_view text, uint32_t color, int scale, const FontData* font_ptr);

    void text_shadow(std::vector<uint32_t>& buf, int x, int y, std::string_view text, uint32_t color, uint32_t shadow_color, int scale, const FontData* font_ptr);

    void sprite_frame(
        std::vector<uint32_t>& buf,
        int x, int y,
        const uint8_t* pixel_data,
        uint32_t pixel_data_size,
        int tex_w,
        int tex_h,
        int dest_w,
        int dest_h,
        int src_x,
        int src_y,
        int src_w,
        int src_h,
        const uint32_t* palette,
        bool is_flip_h = false,
        bool is_flip_v = false
    );

    void blend(
        std::vector<uint32_t>& pixel_buffer,
        int x, int y,
        const uint32_t* pixel_data,
        uint32_t pixel_data_size,
        int width, int height
    );

    void vignette(
        std::vector<uint32_t>& pixel_buffer,
        float intensity,
        uint32_t color,
        float inner_radius,
        float outer_radius
    );

} // namespace DrawPixels
