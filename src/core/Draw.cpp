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

        inline ScreenRect transform_rect(int x, int y, int w, int h) {
            if (g_active_camera) {
                return {
                    g_active_camera->to_screen_x(static_cast<float>(x)),
                    g_active_camera->to_screen_y(static_cast<float>(y)),
                    std::max(1, static_cast<int>(std::round(w * g_active_camera->zoom))),
                    std::max(1, static_cast<int>(std::round(h * g_active_camera->zoom)))
                };
            }
            return { x, y, w, h };
        }

        inline float transform_x(float x) {
            return g_active_camera ? static_cast<float>(g_active_camera->to_screen_x(x)) : x;
        }

        inline float transform_y(float y) {
            return g_active_camera ? static_cast<float>(g_active_camera->to_screen_y(y)) : y;
        }

        inline float scale_dim(float dim) {
            return g_active_camera ? std::max(1.0f, std::round(dim * g_active_camera->zoom)) : dim;
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

    YSortMode get_y_sort_mode() {
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

    void text(int x, int y, std::string_view text, uint32_t color, int scale, int z_index, const FontData* font, int sort_y_override) {
        float draw_x = transform_x(static_cast<float>(x));
        float draw_y = transform_y(static_cast<float>(y));
        int override_y = transform_sort_y_override(sort_y_override);

        const FontData* f = font ? font : &Font::DEFAULT_BLANK;
        int sort_y = calc_sort_y(static_cast<int>(draw_y), f->size * scale, override_y);
        g_queue.push_back({ draw_x, draw_y, z_index, sort_y, TextData{ text, color, scale, f } });
    }

    void rect(int x, int y, int width, int height, uint32_t color, bool fill, int thickness, int z_index, int sort_y_override) {
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

    void sprite(int x, int y, const uint8_t* pixel_data, uint32_t pixel_data_size, int width, int height, int z_index, int sort_y_override) {
        auto [dx, dy, dw, dh] = transform_rect(x, y, width, height);
        int override_y = transform_sort_y_override(sort_y_override);

        int sort_y = calc_sort_y(dy, dh, override_y);
        g_queue.push_back({ static_cast<float>(dx), static_cast<float>(dy), z_index, sort_y,
            SpriteData{ pixel_data, pixel_data_size, dw, dh, 0, 0, dw, dh }
        });
    }

    void sprite_frame(
        int screen_x, int screen_y,
        const uint8_t* pixels, uint32_t pixels_size,
        int width, int height,
        int src_x, int src_y, int src_w, int src_h,
        int z_index,
        int sort_y_override
    ) {
        auto [dx, dy, dw, dh] = transform_rect(screen_x, screen_y, width, height);
        int override_y = transform_sort_y_override(sort_y_override);

        int sort_y = calc_sort_y(dy, src_h, override_y);
        g_queue.push_back({ static_cast<float>(dx), static_cast<float>(dy), z_index, sort_y,
            SpriteData{ pixels, pixels_size, dw, dh, src_x, src_y, src_w, src_h }
        });
    }

    void blend_pixels(
        int screen_x, int screen_y,
        const uint32_t* pixel_data, uint32_t pixel_data_size,
        int width, int height,
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
                        g_palette
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
            }, cmd.data);
        }

        // Reset queue and string pool sizes back to zero for the next frame,
        // but preserve capacity to avoid heap re-allocations
        g_queue.clear();
        g_string_pool.clear();
    }
}
