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

public:
    GameWindow(const char* title, unsigned int width, unsigned int height)
        : m_width(width), m_height(height), m_running(true) {
        m_window = mfb_open_ex(title, width, height, MFB_WF_RESIZABLE);
        if (!m_window) m_running = false;
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

    // One-liner display buffer swapper
    void present(std::vector<uint32_t>& pixel_buffer) {
        if (!m_running) return;
        if (mfb_update_ex(m_window, pixel_buffer.data(), m_width, m_height) < 0) {
            m_running = false;
        }
    }
};
