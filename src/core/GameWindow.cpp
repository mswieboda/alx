#include "core/GameWindow.h"
#include "core/MiniFBWindow.h"

GameWindow::GameWindow(const char* title, unsigned int width, unsigned int height, int min_width, int min_height)
    : m_window(std::make_unique<MiniFBWindow>(title, width, height, min_width, min_height)) {
}

GameWindow::~GameWindow() = default;

bool GameWindow::is_running() const {
    return m_window ? m_window->is_running() : false;
}

void GameWindow::close() {
    if (m_window) {
        m_window->close();
    }
}

bool GameWindow::is_active() const {
    return m_window ? m_window->is_active() : false;
}

void GameWindow::poll_events() {
    if (m_window) {
        m_window->poll_events();
    }
}

int GameWindow::width() const {
    return m_window ? m_window->width() : 0;
}

int GameWindow::height() const {
    return m_window ? m_window->height() : 0;
}

void GameWindow::present(const std::vector<uint32_t>& buffer, int target_w, int target_h) {
    if (m_window) {
        m_window->present(buffer, target_w, target_h);
    }
}

void GameWindow::set_key_callback(IWindow::KeyCallback cb) {
    if (m_window) {
        m_window->set_key_callback(cb);
    }
}

void GameWindow::set_active_callback(IWindow::ActiveCallback cb) {
    if (m_window) {
        m_window->set_active_callback(cb);
    }
}

void GameWindow::move_to_left_edge() {
    if (m_window) {
        m_window->move_to_left_edge();
    }
}
