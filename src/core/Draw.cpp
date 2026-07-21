#include <algorithm>
#include "../Game.h"
#include "Draw.h"
#include "Font.h"
#include "assets/Fonts.h"
#include "helpers.h"

namespace Draw {
    // NOTE: these are private, and invisible to public consumers
    namespace {
        std::vector<Command> g_queue;
        YSortMode g_y_sort_mode = YSortMode::TopY;
        const uint32_t* g_palette = nullptr;



        void clear_screen(std::vector<uint32_t>& buf, uint32_t color) {
            std::fill(buf.begin(), buf.end(), color);
        }

        void draw_rect_immediate(std::vector<uint32_t>& buf, int rx, int ry, int rw, int rh, uint32_t color, bool fill, int thickness) {
            int start_x = std::max(0, rx), end_x = std::min(Game::WIDTH, rx + rw);
            int start_y = std::max(0, ry), end_y = std::min(Game::HEIGHT, ry + rh);
            if (fill) {
                for (int y = start_y; y < end_y; ++y) {
                    for (int x = start_x; x < end_x; ++x) {
                        buf[y * Game::WIDTH + x] = color;
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
                            buf[y * Game::WIDTH + x] = color;
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
                    FontData::RowType font_row = font.data[ascii][row];

                    int base_y = y + (row * scale);

                    // Check up to 16 bit columns per row (bit 15 = leftmost pixel, bit 0 = rightmost)
                    for (int col = 0; col < 16; ++col) {
                        if ((font_row >> (15 - col)) & 1) {
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

    void text(int x, int y, std::string_view text, uint32_t color, int scale, int z_index, const FontData* font) {
        const FontData* f = font ? font : &Font::DEFAULT_BLANK;
        int sort_y = 0;
        if (g_y_sort_mode == YSortMode::TopY) {
            sort_y = y;
        } else if (g_y_sort_mode == YSortMode::YPlusHeight) {
            sort_y = y + f->size * scale;
        }
        g_queue.push_back({ x, y, z_index, sort_y, TextData{ text, color, scale, f } });
    }

    void rect(int x, int y, int width, int height, uint32_t color, bool fill, int thickness, int z_index) {
        int sort_y = 0;
        if (g_y_sort_mode == YSortMode::TopY) {
            sort_y = y;
        } else if (g_y_sort_mode == YSortMode::YPlusHeight) {
            sort_y = y + height;
        }
        g_queue.push_back({ x, y, z_index, sort_y, RectData{ width, height, color, fill, thickness } });
    }

    void sprite(int x, int y, const uint8_t* pixel_data, uint32_t pixel_data_size, int width, int height, int z_index) {
        int sort_y = 0;
        if (g_y_sort_mode == YSortMode::TopY) {
            sort_y = y;
        } else if (g_y_sort_mode == YSortMode::YPlusHeight) {
            sort_y = y + height;
        }
        g_queue.push_back({
            x, y, z_index, sort_y,
            SpriteData{ pixel_data, pixel_data_size, width, height, 0, 0, width, height }
        });
    }

    void sprite_frame(
        int screen_x, int screen_y,
        const uint8_t* pixels, uint32_t pixels_size,
        int width, int height,
        int src_x, int src_y, int src_w, int src_h,
        int z_index
    ) {
        int sort_y = 0;
        if (g_y_sort_mode == YSortMode::TopY) {
            sort_y = screen_y;
        } else if (g_y_sort_mode == YSortMode::YPlusHeight) {
            sort_y = screen_y + src_h;
        }
        g_queue.push_back({
            screen_x, screen_y, z_index, sort_y,
            SpriteData{ pixels, pixels_size, width, height, src_x, src_y, src_w, src_h }
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
                    draw_text_immediate(buffer, cmd.x, cmd.y, arg.text, arg.color, arg.scale, arg.font);
                } 
                else if constexpr (std::is_same_v<T, RectData>) {
                    draw_rect_immediate(buffer, cmd.x, cmd.y, arg.width, arg.height, arg.color, arg.fill, arg.thickness);
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
            }, cmd.data);
        }

        // Reset the vector size back to zero for the next frame,
        // but preserve the capacity so we don't trigger heap re-allocations
        g_queue.clear();
    }
}