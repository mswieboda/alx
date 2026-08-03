#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include "../Game.h"
#include "Draw.h"
#include "Font.h"
#include "Camera.h"
#include "assets/Fonts.h"

#define ALX_DRAW_INTERNAL_ALLOW
#include "DrawPixels.h"
#undef ALX_DRAW_INTERNAL_ALLOW

namespace Draw {
    // NOTE: these are private, and invisible to public consumers
    namespace {
        // queue of draw commands
        std::vector<Command> g_queue;

        // pool of shared strings
        std::vector<char> g_string_pool;

        // current Y sort mode
        YSortMode g_y_sort_mode = YSortMode::TopY;

        // global reused palette between images/sprites
        const uint32_t* g_palette = nullptr;

        // active camera context for world-space coordinates
        const Camera* g_active_camera = nullptr;

        struct ScreenRect {
            int x, y, w, h;
        };

        // Two-Point Boundary Projection Algorithm:
        // Converts world-space rectangles to screen space by projecting BOTH the top-left (x, y)
        // and bottom-right (x + w, y + h) endpoints through to_screen_x/to_screen_y.
        // This guarantees that adjacent tiles (e.g. Tile A at x, Tile B at x + w) share the exact
        // same integer screen pixel boundary (to_screen_x(x + w)), preventing 1-pixel visual tearing
        // or gaps across all zoom levels, sub-pixel movements, and deadzone lerps.
        //
        // RASTERIZATION ALIASING NOTE (Sub-Pixel Remainder Rolling / Pixel Snapping Beats):
        // In GBA-style CPU software rendering without GPU sub-pixel bilinear filtering, mapping
        // continuous floating-point camera positions onto a discrete integer pixel buffer causes
        // fractional remainders to roll across discrete tile dimensions. This results in standard,
        // expected retro rasterization aliasing (where a tile row/column dynamically alternates
        // between N and N-1 pixels as sub-pixel camera movement rolls across buffer coordinates).
        // This is 100% gapless, authentic, and standard behavior for CPU software pixel buffers.
        // TL;DR: Dimensions dynamically shift +/-1px as float camera positions roll across pixel buffers,
        //   resulting in a temporary squeeze/shrink of element (tile/sprite/rect) heights and widths,
        //   more noticeable in large repeated patterns like tiles vs individual sprites/drawn entities.
        inline ScreenRect transform_rect(float x, float y, float w, float h) {
            if (g_active_camera) {
                int x1 = g_active_camera->to_screen_x(x);
                int x2 = g_active_camera->to_screen_x(x + w);
                int y1 = g_active_camera->to_screen_y(y);
                int y2 = g_active_camera->to_screen_y(y + h);
                return {
                    x1,
                    y1,
                    std::max(1, x2 - x1),
                    std::max(1, y2 - y1)
                };
            }
            int x1 = static_cast<int>(std::floor(x + 0.5f));
            int x2 = static_cast<int>(std::floor(x + w + 0.5f));
            int y1 = static_cast<int>(std::floor(y + 0.5f));
            int y2 = static_cast<int>(std::floor(y + h + 0.5f));
            return {
                x1,
                y1,
                std::max(1, x2 - x1),
                std::max(1, y2 - y1)
            };
        }

        inline float transform_x(float x) {
            return g_active_camera ? static_cast<float>(g_active_camera->to_screen_x(x)) : x;
        }

        inline float transform_y(float y) {
            return g_active_camera ? static_cast<float>(g_active_camera->to_screen_y(y)) : y;
        }

        inline float scale_dim(float dim) {
            return g_active_camera ? std::max(1.0f, std::floor(dim * g_active_camera->zoom + 0.5f)) : dim;
        }

        inline int transform_sort_y_override(int sort_y_override) {
            if (g_active_camera && sort_y_override != NO_SORT_Y_OVERRIDE) {
                return g_active_camera->to_screen_y(static_cast<float>(sort_y_override));
            }
            return sort_y_override;
        }
    }

    void world_begin(const Camera& camera) {
        g_active_camera = &camera;
    }

    void world_end() {
        g_active_camera = nullptr;
    }

    bool is_world_mode() {
        return g_active_camera != nullptr;
    }

    void set_y_sort_mode(YSortMode mode) {
        g_y_sort_mode = mode;
    }

    YSortMode y_sort_mode() {
        return g_y_sort_mode;
    }

    void set_palette(const uint32_t* palette) {
        g_palette = palette;
    }

    std::string_view fmt(const char* format, ...) {
        va_list args;
        va_start(args, format);

        va_list args_copy;
        va_copy(args_copy, args);
        int needed = std::vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (needed <= 0) {
            va_end(args);
            return {};
        }

        size_t start_idx = g_string_pool.size();
        g_string_pool.resize(start_idx + needed + 1);

        std::vsnprintf(g_string_pool.data() + start_idx, needed + 1, format, args);
        va_end(args);

        return std::string_view(g_string_pool.data() + start_idx, static_cast<size_t>(needed));
    }

    int text_width(std::string_view text, int scale, const FontData* font) {
        const FontData* f = font ? font : &Font::DEFAULT_BLANK;
        if (scale < 1) scale = 1;
        int width = 0;
        for (char c : text) {
            uint8_t ascii = static_cast<unsigned char>(c);
            if (c == ' ') {
                width += (f->spacing - 2) * scale;
            } else if (ascii < 128) {
                width += f->spacing * scale;
            }
        }
        return width;
    }

    static inline int calc_sort_y(int base_y, int height, int sort_y_override, int top_y_offset = 0) {
        if (sort_y_override != NO_SORT_Y_OVERRIDE) {
            return sort_y_override;
        }
        if (g_y_sort_mode == YSortMode::TopY) {
            return base_y + top_y_offset;
        } else if (g_y_sort_mode == YSortMode::YPlusHeight) {
            return base_y + height;
        }
        return 0;
    }

    void text(float x, float y, std::string_view text, uint32_t color, int scale, int z_index, const FontData* font, int sort_y_override) {
        float draw_x = transform_x(x);
        float draw_y = transform_y(y);
        int override_y = transform_sort_y_override(sort_y_override);

        const FontData* f = font ? font : &Font::DEFAULT_BLANK;
        int sort_y = calc_sort_y(static_cast<int>(draw_y), f->size * scale, override_y);
        g_queue.push_back({ draw_x, draw_y, z_index, sort_y, TextData{ text, color, scale, f } });
    }

    void rect(float x, float y, float width, float height, uint32_t color, bool fill, int thickness, int z_index, int sort_y_override) {
        auto [dx, dy, dw, dh] = transform_rect(x, y, width, height);
        int override_y = transform_sort_y_override(sort_y_override);

        int sort_y = calc_sort_y(dy, dh, override_y);
        g_queue.push_back({ static_cast<float>(dx), static_cast<float>(dy), z_index, sort_y, RectData{ dw, dh, color, fill, thickness } });
    }

    void oval(float cx, float cy, float rx, float ry, uint32_t color, bool fill, int thickness, int z_index, int sort_y_override) {
        float dcx = transform_x(cx);
        float dcy = transform_y(cy);
        float drx = scale_dim(rx);
        float dry = scale_dim(ry);
        int override_y = transform_sort_y_override(sort_y_override);

        int base_y = static_cast<int>(std::round(dcy));
        int base_ry = static_cast<int>(std::round(dry));
        int sort_y = calc_sort_y(base_y, base_ry, override_y, -base_ry);
        g_queue.push_back({ dcx, dcy, z_index, sort_y, OvalData{ dcx, dcy, drx, dry, color, fill, thickness } });
    }

    void circle(float cx, float cy, float radius, uint32_t color, bool fill, int thickness, int z_index, int sort_y_override) {
        oval(cx, cy, radius, radius, color, fill, thickness, z_index, sort_y_override);
    }

    void line(float x1, float y1, float x2, float y2, uint32_t color, int thickness, int z_index, int sort_y_override) {
        float dx1 = transform_x(x1);
        float dy1 = transform_y(y1);
        float dx2 = transform_x(x2);
        float dy2 = transform_y(y2);
        int override_y = transform_sort_y_override(sort_y_override);

        int sort_y = calc_sort_y(static_cast<int>(std::max(dy1, dy2)), 0, override_y);
        g_queue.push_back({ dx1, dy1, z_index, sort_y, LineData{ dx2, dy2, color, thickness } });
    }

    void sprite(float x, float y, const uint8_t* pixel_data, uint32_t pixel_data_size, float width, float height, int z_index, int sort_y_override, bool is_flip_h, bool is_flip_v) {
        auto [dx, dy, dw, dh] = transform_rect(x, y, width, height);
        int override_y = transform_sort_y_override(sort_y_override);

        int sort_y = calc_sort_y(dy, dh, override_y);
        g_queue.push_back({ static_cast<float>(dx), static_cast<float>(dy), z_index, sort_y,
            SpriteData{ pixel_data, pixel_data_size, dw, dh, 0, 0, dw, dh, is_flip_h, is_flip_v }
        });
    }

    void sprite_frame(
        float screen_x, float screen_y,
        const uint8_t* pixels, uint32_t pixels_size,
        float width, float height,
        int src_x, int src_y, int src_w, int src_h,
        int z_index,
        int sort_y_override,
        bool is_flip_h,
        bool is_flip_v
    ) {
        auto [dx, dy, dw, dh] = transform_rect(screen_x, screen_y, width, height);
        int override_y = transform_sort_y_override(sort_y_override);

        int sort_y = calc_sort_y(dy, src_h, override_y);
        g_queue.push_back({ static_cast<float>(dx), static_cast<float>(dy), z_index, sort_y,
            SpriteData{ pixels, pixels_size, dw, dh, src_x, src_y, src_w, src_h, is_flip_h, is_flip_v }
        });
    }

    void blend_pixels(
        float screen_x, float screen_y,
        const uint32_t* pixel_data, uint32_t pixel_data_size,
        float width, float height,
        int z_index,
        int sort_y_override
    ) {
        auto [dx, dy, dw, dh] = transform_rect(screen_x, screen_y, width, height);
        int override_y = transform_sort_y_override(sort_y_override);

        int sort_y = calc_sort_y(dy, dh, override_y);
        g_queue.push_back({ static_cast<float>(dx), static_cast<float>(dy), z_index, sort_y,
            BlendPixelsData{ pixel_data, pixel_data_size, dw, dh }
        });
    }

    void flush_pipeline(std::vector<uint32_t>& buffer, uint32_t background_color) {
        DrawPixels::clear(buffer, background_color);

        // Sort the commands
        // Primary key: Z-Index (lower draws first).
        // Secondary key: Y coordinate (Classic 2.5D top-to-bottom depth layering)
        std::stable_sort(g_queue.begin(), g_queue.end(), [](const Command& a, const Command& b) {
            if (a.z_index != b.z_index) {
                return a.z_index < b.z_index;
            }
            return a.sort_y < b.sort_y;
        });

        // Dispatch drawing functions using pattern matching
        for (const auto& cmd : g_queue) {
            std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;

                if constexpr (std::is_same_v<T, TextData>) {
                    DrawPixels::text(buffer, static_cast<int>(cmd.x), static_cast<int>(cmd.y), arg.text, arg.color, arg.scale, arg.font);
                }
                else if constexpr (std::is_same_v<T, RectData>) {
                    DrawPixels::rect(buffer, static_cast<int>(cmd.x), static_cast<int>(cmd.y), arg.width, arg.height, arg.color, arg.fill, arg.thickness);
                }
                else if constexpr (std::is_same_v<T, OvalData>) {
                    DrawPixels::oval(buffer, arg.cx, arg.cy, arg.rx, arg.ry, arg.color, arg.fill, arg.thickness);
                }
                else if constexpr (std::is_same_v<T, SpriteData>) {
                    DrawPixels::sprite_frame(
                        buffer,
                        static_cast<int>(cmd.x), static_cast<int>(cmd.y),
                        arg.pixel_data,
                        arg.pixel_data_size,
                        arg.width,
                        arg.height,
                        arg.src_x,
                        arg.src_y,
                        arg.src_w,
                        arg.src_h,
                        g_palette,
                        arg.is_flip_h,
                        arg.is_flip_v
                    );
                }
                else if constexpr (std::is_same_v<T, BlendPixelsData>) {
                    DrawPixels::blend(
                        buffer,
                        static_cast<int>(cmd.x), static_cast<int>(cmd.y),
                        arg.pixel_data,
                        arg.pixel_data_size,
                        arg.width,
                        arg.height
                    );
                }
                else if constexpr (std::is_same_v<T, LineData>) {
                    DrawPixels::line(
                        buffer,
                        static_cast<int>(cmd.x), static_cast<int>(cmd.y),
                        static_cast<int>(arg.x2), static_cast<int>(arg.y2),
                        arg.color,
                        arg.thickness
                    );
                }
            }, cmd.data);
        }

        // Reset queue and string pool sizes back to zero for the next frame,
        // but preserve capacity to avoid heap re-allocations
        g_queue.clear();
        g_string_pool.clear();
    }
}
