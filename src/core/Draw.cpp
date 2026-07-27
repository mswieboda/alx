#include <algorithm>
#include <cstdio>
#include <cstdarg>
#include "../Game.h"
#include "Draw.h"
#include "Font.h"
#include "assets/Fonts.h"

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

        inline uint32_t blend_pixel(uint32_t dest, uint32_t src) {
            uint32_t src_alpha = (src >> 24) & 0xFF;
            if (src_alpha == 0xFF) return src;
            if (src_alpha == 0x00) return dest;

            uint32_t src_r = (src >> 16) & 0xFF;
            uint32_t src_g = (src >> 8) & 0xFF;
            uint32_t src_b = src & 0xFF;

            uint32_t dest_alpha = (dest >> 24) & 0xFF;
            uint32_t dest_r = (dest >> 16) & 0xFF;
            uint32_t dest_g = (dest >> 8) & 0xFF;
            uint32_t dest_b = dest & 0xFF;

            uint32_t out_r = (src_r * src_alpha + dest_r * (255 - src_alpha)) / 255;
            uint32_t out_g = (src_g * src_alpha + dest_g * (255 - src_alpha)) / 255;
            uint32_t out_b = (src_b * src_alpha + dest_b * (255 - src_alpha)) / 255;
            uint32_t out_a = src_alpha + (dest_alpha * (255 - src_alpha)) / 255;

            return (out_a << 24) | (out_r << 16) | (out_g << 8) | out_b;
        }

        void clear_screen(std::vector<uint32_t>& buf, uint32_t color) {
            std::fill(buf.begin(), buf.end(), color);
        }

        void draw_rect_immediate(std::vector<uint32_t>& buf, int rx, int ry, int rw, int rh, uint32_t color, bool fill, int thickness) {
            int start_x = std::max(0, rx), end_x = std::min(Game::WIDTH, rx + rw);
            int start_y = std::max(0, ry), end_y = std::min(Game::HEIGHT, ry + rh);
            uint32_t alpha = (color >> 24) & 0xFF;

            if (fill) {
                for (int y = start_y; y < end_y; ++y) {
                    for (int x = start_x; x < end_x; ++x) {
                        uint32_t idx = y * Game::WIDTH + x;
                        if (alpha == 0xFF) {
                            buf[idx] = color;
                        } else if (alpha > 0) {
                            buf[idx] = blend_pixel(buf[idx], color);
                        }
                    }
                }
            } else {
                int t = thickness;
                if (t <= 0) return;
                for (int y = start_y; y < end_y; ++y) {
                    for (int x = start_x; x < end_x; ++x) {
                        int dx = x - rx;
                        int dy = y - ry;
                        if (dx < t || dx >= rw - t || dy < t || dy >= rh - t) {
                            uint32_t idx = y * Game::WIDTH + x;
                            if (alpha == 0xFF) {
                                buf[idx] = color;
                            } else if (alpha > 0) {
                                buf[idx] = blend_pixel(buf[idx], color);
                            }
                        }
                    }
                }
            }
        }

        void draw_oval_immediate(std::vector<uint32_t>& buf, float cx, float cy, float rx, float ry, uint32_t color, bool fill, int thickness) {
            if (rx <= 0.0f || ry <= 0.0f) return;

            uint32_t alpha = (color >> 24) & 0xFF;
            if (alpha == 0) return;

            float rx_sq = rx * rx - rx;
            if (rx_sq < 0.0f) rx_sq = 0.0f;

            float ry_sq = ry * ry - ry;
            if (ry_sq < 0.0f) ry_sq = 0.0f;

            float max_val = rx_sq * ry_sq;

            float inner_rx = rx - static_cast<float>(thickness);
            float inner_ry = ry - static_cast<float>(thickness);
            float inner_max_val = 0.0f;
            float inner_rx_sq = 0.0f;
            float inner_ry_sq = 0.0f;

            if (inner_rx > 0.0f && inner_ry > 0.0f) {
                inner_rx_sq = inner_rx * inner_rx - inner_rx;
                if (inner_rx_sq < 0.0f) inner_rx_sq = 0.0f;

                inner_ry_sq = inner_ry * inner_ry - inner_ry;
                if (inner_ry_sq < 0.0f) inner_ry_sq = 0.0f;

                inner_max_val = inner_rx_sq * inner_ry_sq;
            }

            int start_x = std::max(0, static_cast<int>(std::floor(cx - rx)));
            int end_x = std::min(Game::WIDTH, static_cast<int>(std::ceil(cx + rx + 1.0f)));
            int start_y = std::max(0, static_cast<int>(std::floor(cy - ry)));
            int end_y = std::min(Game::HEIGHT, static_cast<int>(std::ceil(cy + ry + 1.0f)));

            for (int y = start_y; y < end_y; ++y) {
                float dy = (static_cast<float>(y) + 0.5f) - cy;
                float dy_sq_rx = dy * dy * rx_sq;
                float dy_sq_inner = (inner_rx_sq > 0.0f && inner_ry_sq > 0.0f) ? (dy * dy * inner_rx_sq) : 0.0f;

                for (int x = start_x; x < end_x; ++x) {
                    float dx = (static_cast<float>(x) + 0.5f) - cx;
                    float val = dx * dx * ry_sq + dy_sq_rx;

                    if (val <= max_val) {
                        bool draw_pixel = fill;
                        if (!draw_pixel) {
                            if (inner_rx <= 0.0f || inner_ry <= 0.0f) {
                                draw_pixel = true;
                            } else {
                                float inner_val = dx * dx * inner_ry_sq + dy_sq_inner;
                                draw_pixel = (inner_val >= inner_max_val);
                            }
                        }

                        if (draw_pixel) {
                            uint32_t idx = y * Game::WIDTH + x;
                            if (alpha == 0xFF) {
                                buf[idx] = color;
                            } else {
                                buf[idx] = blend_pixel(buf[idx], color);
                            }
                        }
                    }
                }
            }
        }

        void draw_text_immediate(std::vector<uint32_t>& buf, int x, int y,
                                std::string_view text, uint32_t color, int scale,
                                const FontData* font_ptr)
        {
            // Fallback to DEFAULT_BLANK if font_ptr is ever null
            if (!font_ptr) {
                font_ptr = &Font::DEFAULT_BLANK;
            }

            const auto& font = *font_ptr;

            if (scale < 1) scale = 1;

            int cursor_x = x;

            for (char c : text) {
                uint8_t ascii = static_cast<unsigned char>(c);

                // Handle space gap using the instance's spacing property
                if (c == ' ') {
                    cursor_x += (font.spacing - 2) * scale;
                    continue;
                }

                if (ascii >= 128) continue;

                // Loop over rows using font.size from the instance
                for (int row = 0; row < font.size; ++row) {
                    uint16_t font_row = font.data[ascii][row];

                    int base_y = y + (row * scale);

                    // Check up to font.size bit columns per row
                    // bit (font.size - 1) = leftmost pixel, bit 0 = rightmost
                    for (int col = 0; col < font.size; ++col) {
                        if ((font_row >> ((font.size - 1) - col)) & 1) {
                            int base_x = cursor_x + (col * scale);

                            for (int sy = 0; sy < scale; ++sy) {
                                int current_y = base_y + sy;
                                if (current_y < 0 || current_y >= Game::HEIGHT) continue;

                                for (int sx = 0; sx < scale; ++sx) {
                                    int current_x = base_x + sx;
                                    if (current_x < 0 || current_x >= Game::WIDTH) continue;

                                    buf[current_y * Game::WIDTH + current_x] = color;
                                }
                            }
                        }
                    }
                }

                // Advance cursor using this font instance's specific spacing
                cursor_x += font.spacing * scale;
            }
        }

        void draw_sprite_frame_immediate(
            std::vector<uint32_t>& buf,
            int x, int y,
            const uint8_t* pixel_data,
            uint32_t pixel_data_size,
            int width,
            int height,
            int src_x,
            int src_y,
            int src_w,
            int src_h
        ) {
            int px_idx = 0;
            uint32_t cursor = 0;

            while (cursor < pixel_data_size) {
                // RLE Decompression: [run_length, palette_index]
                uint8_t run = pixel_data[cursor++];
                uint8_t pal_idx = pixel_data[cursor++];

                for (uint8_t i = 0; i < run; ++i) {
                    // Local 2D coordinates inside the entire master sheet texture
                    int lx = px_idx % width;
                    int ly = px_idx / width;

                    if (ly >= src_y + src_h) {
                        return;
                    }

                    px_idx++;

                    // Check if this pixel falls inside the cropped frame's source bounds
                    if (lx >= src_x && lx < src_x + src_w &&
                        ly >= src_y && ly < src_y + src_h) {

                        // Calculate destination screen coordinates
                        // Offset by (lx - src_x) and (ly - src_y) so the sub-frame aligns to (x, y)
                        int tx = x + (lx - src_x);
                        int ty = y + (ly - src_y);

                        // Screen bounds check
                        if (tx >= 0 && tx < Game::WIDTH && ty >= 0 && ty < Game::HEIGHT) {
                            uint32_t color = g_palette ? g_palette[pal_idx] : 0xFF00FF00;

                            // Alpha check & Blending
                            if ((color & 0xFF000000) != 0x00000000) {
                                uint32_t dest_idx = ty * Game::WIDTH + tx;
                                buf[dest_idx] = blend_pixel(buf[dest_idx], color);
                            }
                        }
                    }
                }
            }
        }

        void draw_blend_immediate(
            std::vector<uint32_t>& pixel_buffer,
            int x, int y,
            const uint32_t* pixel_data,
            uint32_t pixel_data_size,
            int width, int height
        ) {
            for (int row = 0; row < height; ++row) {
                for (int col = 0; col < width; ++col) {
                    int tx = x + col;
                    int ty = y + row;

                    if (tx >= 0 && tx < Game::WIDTH && ty >= 0 && ty < Game::HEIGHT) {
                        int src_idx = row * width + col;

                        if (src_idx * sizeof(uint32_t) >= pixel_data_size) continue;

                        uint32_t color = pixel_data[src_idx];

                        // Blending, skipping pure transparency (0x00XXXXXX)
                        if ((color & 0xFF000000) != 0x00000000) {
                            uint32_t dest_idx = ty * Game::WIDTH + tx;
                            pixel_buffer[dest_idx] = blend_pixel(pixel_buffer[dest_idx], color);
                        }
                    }
                }
            }
        }
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
        const FontData* f = font ? font : &Font::DEFAULT_BLANK;
        int sort_y = calc_sort_y(y, f->size * scale, sort_y_override);
        g_queue.push_back({ static_cast<float>(x), static_cast<float>(y), z_index, sort_y, TextData{ text, color, scale, f } });
    }

    void rect(int x, int y, int width, int height, uint32_t color, bool fill, int thickness, int z_index, int sort_y_override) {
        int sort_y = calc_sort_y(y, height, sort_y_override);
        g_queue.push_back({ static_cast<float>(x), static_cast<float>(y), z_index, sort_y, RectData{ width, height, color, fill, thickness } });
    }

    void oval(float cx, float cy, float rx, float ry, uint32_t color, bool fill, int thickness, int z_index, int sort_y_override) {
        int base_y = static_cast<int>(std::round(cy));
        int base_ry = static_cast<int>(std::round(ry));
        int sort_y = calc_sort_y(base_y, base_ry, sort_y_override, -base_ry);
        g_queue.push_back({ cx, cy, z_index, sort_y, OvalData{ cx, cy, rx, ry, color, fill, thickness } });
    }

    void circle(float cx, float cy, float radius, uint32_t color, bool fill, int thickness, int z_index, int sort_y_override) {
        oval(cx, cy, radius, radius, color, fill, thickness, z_index, sort_y_override);
    }

    void sprite(int x, int y, const uint8_t* pixel_data, uint32_t pixel_data_size, int width, int height, int z_index, int sort_y_override) {
        int sort_y = calc_sort_y(y, height, sort_y_override);
        g_queue.push_back({ static_cast<float>(x), static_cast<float>(y), z_index, sort_y,
            SpriteData{ pixel_data, pixel_data_size, width, height, 0, 0, width, height }
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
        int sort_y = calc_sort_y(screen_y, src_h, sort_y_override);
        g_queue.push_back({ static_cast<float>(screen_x), static_cast<float>(screen_y), z_index, sort_y,
            SpriteData{ pixels, pixels_size, width, height, src_x, src_y, src_w, src_h }
        });
    }

    void blend_pixels(
        int screen_x, int screen_y,
        const uint32_t* pixel_data, uint32_t pixel_data_size,
        int width, int height,
        int z_index,
        int sort_y_override
    ) {
        int sort_y = calc_sort_y(screen_y, height, sort_y_override);
        g_queue.push_back({ static_cast<float>(screen_x), static_cast<float>(screen_y), z_index, sort_y,
            BlendPixelsData{ pixel_data, pixel_data_size, width, height }
        });
    }

    void flush_pipeline(std::vector<uint32_t>& buffer, uint32_t background_color) {
        clear_screen(buffer, background_color);

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
                    draw_text_immediate(buffer, static_cast<int>(cmd.x), static_cast<int>(cmd.y), arg.text, arg.color, arg.scale, arg.font);
                }
                else if constexpr (std::is_same_v<T, RectData>) {
                    draw_rect_immediate(buffer, static_cast<int>(cmd.x), static_cast<int>(cmd.y), arg.width, arg.height, arg.color, arg.fill, arg.thickness);
                }
                else if constexpr (std::is_same_v<T, OvalData>) {
                    draw_oval_immediate(buffer, arg.cx, arg.cy, arg.rx, arg.ry, arg.color, arg.fill, arg.thickness);
                }
                else if constexpr (std::is_same_v<T, SpriteData>) {
                    draw_sprite_frame_immediate(
                        buffer,
                        cmd.x, cmd.y,
                        arg.pixel_data,
                        arg.pixel_data_size,
                        arg.width,
                        arg.height,
                        arg.src_x,
                        arg.src_y,
                        arg.src_w,
                        arg.src_h
                    );
                }
                else if constexpr (std::is_same_v<T, BlendPixelsData>) {
                    draw_blend_immediate(
                        buffer,
                        cmd.x, cmd.y,
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
