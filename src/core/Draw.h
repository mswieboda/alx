#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <cstdint>
#include "Font.h"

#if defined(__GNUC__) || defined(__clang__)
  #define ALX_PRINTF_FORMAT(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
  #define ALX_PRINTF_FORMAT(fmt_idx, arg_idx)
#endif

namespace core { struct Camera; }
using Camera = core::Camera;

namespace Draw {

    // --- WORLD CAMERA CONTEXT ---
    void world_begin(const Camera& camera);
    void world_end();
    bool is_world_mode();
    const Camera* active_camera();

    // --- COMMAND VARIANTS ---

    // --- COMMAND VARIANTS ---
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

    struct OvalData {
        float cx;
        float cy;
        float rx;
        float ry;
        uint32_t color;
        bool fill;
        int thickness;
    };

    struct SpriteData {
        const uint8_t* pixel_data;
        uint32_t pixel_data_size;
        int tex_w;
        int tex_h;
        int dest_w;
        int dest_h;
        int src_x;
        int src_y;
        int src_w;
        int src_h;
        bool is_flip_h = false;
        bool is_flip_v = false;
    };

    struct BlendPixelsData {
        const uint32_t* pixel_data; // can contain with transparent 0x00 gaps
        uint32_t pixel_data_size;
        int width;
        int height;
    };

    struct LineData {
        float x2;
        float y2;
        uint32_t color;
        int thickness;
    };

    struct VignetteData {
        float intensity;
        uint32_t color;
        float inner_radius;
        float outer_radius;
    };

    // --- COMMAND ---
    struct Command {
        float x;
        float y;
        int z_index;
        int sort_y;
        std::variant<TextData, RectData, OvalData, SpriteData, BlendPixelsData, LineData, VignetteData> data;
    };

    // --- 3. PUBLIC PIPELINE INTERFACE ---
    enum class YSortMode {
        None,
        TopY,
        YPlusHeight
    };

    void set_y_sort_mode(YSortMode mode);
    YSortMode y_sort_mode();

    void set_palette(const uint32_t* palette);

    // Frame string arena: formats text directly into frame scratch pool and returns frame-valid std::string_view
    std::string_view fmt(const char* format, ...) ALX_PRINTF_FORMAT(1, 2);

    int text_width(std::string_view text, int scale = 1, const FontData* font = &Font::DEFAULT_BLANK);

    constexpr int NO_SORT_Y_OVERRIDE = INT32_MIN;

    // Submit actions to the frame queue
    void text(float x, float y, std::string_view text, uint32_t color,
              int scale = 1, int z_index = 1,
              const FontData* font = &Font::DEFAULT_BLANK,
              int sort_y_override = NO_SORT_Y_OVERRIDE,
              uint32_t shadow_color = 0x00000000);
    void text_shadow(float x, float y, std::string_view text, uint32_t color,
                     uint32_t shadow_color = 0xFF000000,
                     int scale = 1, int z_index = 1,
                     const FontData* font = &Font::DEFAULT_BLANK,
                     int sort_y_override = NO_SORT_Y_OVERRIDE);
    void rect(float x, float y, float width, float height, uint32_t color, bool fill = true, int thickness = 1, int z_index = 1, int sort_y_override = NO_SORT_Y_OVERRIDE);
    void oval(float cx, float cy, float rx, float ry, uint32_t color, bool fill = true, int thickness = 1, int z_index = 1, int sort_y_override = NO_SORT_Y_OVERRIDE);
    void circle(float cx, float cy, float radius, uint32_t color, bool fill = true, int thickness = 1, int z_index = 1, int sort_y_override = NO_SORT_Y_OVERRIDE);
    void line(float x1, float y1, float x2, float y2, uint32_t color, int thickness = 1, int z_index = 1, int sort_y_override = NO_SORT_Y_OVERRIDE);
    void sprite(
        float x, float y,
        const uint8_t* pixel_data, uint32_t pixel_data_size,
        float width, float height,
        int z_index = 1,
        int sort_y_override = NO_SORT_Y_OVERRIDE,
        bool is_flip_h = false,
        bool is_flip_v = false
    );
    void sprite_frame(
        float screen_x, float screen_y,
        const uint8_t* sheet_pixels, uint32_t sheet_pixels_size,
        float sheet_width, float sheet_height,
        int src_x, int src_y, int src_w, int src_h,
        int z_index = 1,
        int sort_y_override = NO_SORT_Y_OVERRIDE,
        bool is_flip_h = false,
        bool is_flip_v = false
    );
    void blend_pixels(
        float screen_x, float screen_y,
        const uint32_t* pixel_data, uint32_t pixel_data_size,
        float width, float height,
        int z_index = 1,
        int sort_y_override = NO_SORT_Y_OVERRIDE
    );
    void vignette(
        float intensity,
        uint32_t color = 0x00130C1A,
        float inner_radius = 0.4f,
        float outer_radius = 1.1f,
        int z_index = 90,
        int sort_y_override = NO_SORT_Y_OVERRIDE
    );

    // Process, order, and draw everything to the screen buffer
    void flush_pipeline(std::vector<uint32_t>& buffer, uint32_t background_color);

    inline float interpolate(float prev, float curr, float alpha) {
        return prev + (curr - prev) * alpha;
    }
}
