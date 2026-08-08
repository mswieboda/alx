#pragma once
#include "core/IWindow.h"
#include <cstdint>
#include <memory>
#include <vector>

class GameWindow {
public:
    GameWindow(const char* title, unsigned int width, unsigned int height, int min_width = 0, int min_height = 0);
    ~GameWindow();

    bool is_running() const;
    void close();
    bool is_active() const;
    void poll_events();
    int width() const;
    int height() const;
    void present(const std::vector<uint32_t>& buffer, int target_w, int target_h);

    void set_key_callback(IWindow::KeyCallback cb);
    void set_active_callback(IWindow::ActiveCallback cb);
    void move_to_left_edge();

private:
    std::unique_ptr<IWindow> m_window;
};
