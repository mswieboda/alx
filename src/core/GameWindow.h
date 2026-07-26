#pragma once
#include <MiniFB.h>
#include <vector>
#include <cstdint>

class GameWindow {
private:
    struct mfb_window* m_window;
    unsigned int m_width;
    unsigned int m_height;
    bool m_running;
    std::vector<uint32_t> m_presentation_pixel_buffer;

public:
    GameWindow(const char* title, unsigned int width, unsigned int height, int min_width, int min_height)
        : m_width(width),
        m_height(height),
        m_running(true),
        m_presentation_pixel_buffer(width * height)
    {
        m_window = mfb_open_ex(title, width, height, MFB_WF_RESIZABLE);

        if (!m_window) m_running = false;

        // mfb_set_viewport_min(m_window, min_width, min_height);
    }

    ~GameWindow() {
        if (m_window) mfb_close(m_window);
    }

    // Returns raw pointer for setting input callbacks
    struct mfb_window* raw() { return m_window; }

    // Semantic helper properties
    bool is_running() const { return m_running; }
    void close() { m_running = false; }

    bool is_active() {
        return mfb_is_window_active(m_window);
    }

    // One-liner event poll that internally flips m_running if the user hits the [X] button
    void poll_events() {
        if (!m_running) return;

        if (mfb_update_events(m_window) < 0) m_running = false;
    }

    int width() {
        return mfb_get_window_width(m_window);
    }

    int height() {
        return mfb_get_window_height(m_window);
    }

    // Present pixel buffer to minifb to draw to GPU
    // automatically scales with logical presentation in letter box
    void present(std::vector<uint32_t>& pixel_buffer, int width, int height) {
        if (!m_running) return;

        int window_w = this->width();
        int window_h = this->height();

        // Guard against windows minimized
        if (window_w <= 0 || window_h <= 0) return;

        // Ensure presentation buffer matches physical window size
        size_t required_size = static_cast<size_t>(window_w * window_h);
        if (m_presentation_pixel_buffer.size() < required_size) {
            m_presentation_pixel_buffer.resize(required_size);
        }

        // Clear presentation buffer to black (Letterbox / Pillarbox color)
        std::fill(m_presentation_pixel_buffer.begin(), m_presentation_pixel_buffer.end(), 0xFF000000);

        // Calculate max integer scale factor (S >= 1)
        int scale_x = window_w / width;
        int scale_y = window_h / height;

        // Allow scale to drop to 0 when window < game canvas size (it will crop)
        int scale = std::min(scale_x, scale_y);

        if (scale >= 1) {
            // 4. Calculate centered offsets for letterboxing
            int scaled_w = width * scale;
            int scaled_h = height * scale;
            int offset_x = (window_w - scaled_w) / 2;
            int offset_y = (window_h - scaled_h) / 2;

            // 5. Nearest-Neighbor Integer Scaling Pass (Duplicating pixels)
            for (int ly = 0; ly < height; ++ly) {
                // Pre-calculate destination Y row start
                int py_start = offset_y + (ly * scale);

                for (int lx = 0; lx < width; ++lx) {
                    uint32_t pixel = pixel_buffer[ly * width + lx];
                    int px_start = offset_x + (lx * scale);

                    // Duplicate pixel across scale x scale block
                    for (int sy = 0; sy < scale; ++sy) {
                        int target_y = py_start + sy;
                        uint32_t* row_ptr = &m_presentation_pixel_buffer[target_y * window_w + px_start];

                        for (int sx = 0; sx < scale; ++sx) {
                            row_ptr[sx] = pixel;
                        }
                    }
                }
            }
        } else {
            // --- Scale < 1 ---
            // (Window smaller than game width/height)
            // Draw centered 1-to-1 crop
            int start_lx = (width - window_w) / 2;
            int start_ly = (height - window_h) / 2;

            for (int wy = 0; wy < window_h; ++wy) {
                int ly = start_ly + wy;

                if (ly < 0 || ly >= height) continue;

                uint32_t* dest_row = &m_presentation_pixel_buffer[wy * window_w];
                const uint32_t* src_row = &pixel_buffer[ly * width];

                for (int wx = 0; wx < window_w; ++wx) {
                    int lx = start_lx + wx;
                    if (lx >= 0 && lx < width) {
                        dest_row[wx] = src_row[lx];
                    }
                }
            }
        }

        if (mfb_update_ex(m_window, m_presentation_pixel_buffer.data(), window_w, window_h) < 0) {
            m_running = false;
        }
    }
};
