#define ALX_DRAW_INTERNAL_ALLOW
#include "DrawPixels.h"
#include "../Game.h"
#include "Font.h"
#include <algorithm>
#include <cmath>

namespace DrawPixels {

namespace {

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

} // namespace

void clear(std::vector<uint32_t>& buf, uint32_t color) {
    std::fill(buf.begin(), buf.end(), color);
}

void rect(std::vector<uint32_t>& buf, int rx, int ry, int rw, int rh, uint32_t color, bool fill, int thickness) {
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

void oval(std::vector<uint32_t>& buf, float cx, float cy, float rx, float ry, uint32_t color, bool fill, int thickness) {
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

void line(std::vector<uint32_t>& buf, int x1, int y1, int x2, int y2, uint32_t color, int thickness) {
    uint32_t alpha = (color >> 24) & 0xFF;
    if (alpha == 0) return;

    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    auto draw_brush = [&](int cx, int cy) {
        int half_t = thickness / 2;
        int start_x = std::max(0, cx - half_t);
        int end_x = std::min(Game::WIDTH, cx - half_t + thickness);
        int start_y = std::max(0, cy - half_t);
        int end_y = std::min(Game::HEIGHT, cy - half_t + thickness);

        for (int y = start_y; y < end_y; ++y) {
            for (int x = start_x; x < end_x; ++x) {
                uint32_t idx = y * Game::WIDTH + x;
                if (alpha == 0xFF) {
                    buf[idx] = color;
                } else {
                    buf[idx] = blend_pixel(buf[idx], color);
                }
            }
        }
    };

    if (thickness <= 1) {
        while (true) {
            if (x1 >= 0 && x1 < Game::WIDTH && y1 >= 0 && y1 < Game::HEIGHT) {
                uint32_t idx = y1 * Game::WIDTH + x1;
                if (alpha == 0xFF) {
                    buf[idx] = color;
                } else {
                    buf[idx] = blend_pixel(buf[idx], color);
                }
            }
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    } else {
        while (true) {
            draw_brush(x1, y1);
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }
}

void text(std::vector<uint32_t>& buf, int x, int y, std::string_view text, uint32_t color, int scale, const FontData* font_ptr) {
    if (!font_ptr) {
        font_ptr = &Font::DEFAULT_BLANK;
    }

    const auto& font = *font_ptr;
    if (scale < 1) scale = 1;

    int cursor_x = x;

    for (char c : text) {
        uint8_t ascii = static_cast<unsigned char>(c);

        if (c == ' ') {
            cursor_x += (font.spacing - 2) * scale;
            continue;
        }

        if (ascii >= 128) continue;

        for (int row = 0; row < font.size; ++row) {
            uint16_t font_row = font.data[ascii][row];

            int base_y = y + (row * scale);

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

        cursor_x += font.spacing * scale;
    }
}

void sprite_frame(
    std::vector<uint32_t>& buf,
    int x, int y,
    const uint8_t* pixel_data,
    uint32_t pixel_data_size,
    int width,
    int height,
    int src_x,
    int src_y,
    int src_w,
    int src_h,
    const uint32_t* palette,
    bool is_flip_h,
    bool is_flip_v
) {
    int px_idx = 0;
    uint32_t cursor = 0;

    while (cursor < pixel_data_size) {
        uint8_t run = pixel_data[cursor++];
        uint8_t pal_idx = pixel_data[cursor++];

        for (uint8_t i = 0; i < run; ++i) {
            int lx = px_idx % width;
            int ly = px_idx / width;

            if (ly >= src_y + src_h) {
                return;
            }

            px_idx++;

            if (lx >= src_x && lx < src_x + src_w &&
                ly >= src_y && ly < src_y + src_h) {

                int local_x = lx - src_x;
                int local_y = ly - src_y;

                if (is_flip_h) {
                    local_x = (src_w - 1) - local_x;
                }
                if (is_flip_v) {
                    local_y = (src_h - 1) - local_y;
                }

                int tx = x + local_x;
                int ty = y + local_y;

                if (tx >= 0 && tx < Game::WIDTH && ty >= 0 && ty < Game::HEIGHT) {
                    uint32_t color = palette ? palette[pal_idx] : 0xFF00FF00;

                    if ((color & 0xFF000000) != 0x00000000) {
                        uint32_t dest_idx = ty * Game::WIDTH + tx;
                        buf[dest_idx] = blend_pixel(buf[dest_idx], color);
                    }
                }
            }
        }
    }
}

void blend(
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

                if ((color & 0xFF000000) != 0x00000000) {
                    uint32_t dest_idx = ty * Game::WIDTH + tx;
                    pixel_buffer[dest_idx] = blend_pixel(pixel_buffer[dest_idx], color);
                }
            }
        }
    }
}

void vignette(
    std::vector<uint32_t>& pixel_buffer,
    float intensity,
    uint32_t color,
    float inner_radius,
    float outer_radius
) {
    float clamped_intensity = std::clamp(intensity, 0.0f, 1.0f);
    if (clamped_intensity <= 0.0f) return;

    int width = Game::WIDTH;
    int height = Game::HEIGHT;
    if (static_cast<int>(pixel_buffer.size()) < width * height) return;

    float center_x = static_cast<float>(width) / 2.0f;
    float center_y = static_cast<float>(height) / 2.0f;

    float rad_diff = outer_radius - inner_radius;
    if (rad_diff <= 0.0001f) rad_diff = 0.0001f;
    float inv_rad_diff = 1.0f / rad_diff;

    uint32_t rgb_color = color & 0x00FFFFFF;

    for (int y = 0; y < height; ++y) {
        float dy = (static_cast<float>(y) + 0.5f - center_y) / center_y;
        float dy_sq = dy * dy;
        int row_offset = y * width;

        for (int x = 0; x < width; ++x) {
            float dx = (static_cast<float>(x) + 0.5f - center_x) / center_x;
            float dist = std::sqrt(dx * dx + dy_sq);

            if (dist <= inner_radius) continue;

            float t = std::clamp((dist - inner_radius) * inv_rad_diff, 0.0f, 1.0f);
            float smooth_t = t * t * (3.0f - 2.0f * t);
            float pixel_alpha = smooth_t * clamped_intensity;

            uint8_t alpha_byte = static_cast<uint8_t>(std::clamp(pixel_alpha * 255.0f, 0.0f, 255.0f));
            if (alpha_byte > 0) {
                uint32_t src_pixel = (static_cast<uint32_t>(alpha_byte) << 24) | rgb_color;
                int idx = row_offset + x;
                pixel_buffer[idx] = blend_pixel(pixel_buffer[idx], src_pixel);
            }
        }
    }
}

} // namespace DrawPixels
