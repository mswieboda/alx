#pragma once
#include "core/IWindow.h"
#include <cstdint>
#include <vector>

struct mfb_window;

class MiniFBWindow : public IWindow {
public:
    MiniFBWindow(const char* title, unsigned int width, unsigned int height, int min_width = 0, int min_height = 0);
    ~MiniFBWindow() override;

    bool is_running() const override;
    void close() override;
    bool is_active() const override;
    void poll_events() override;
    int width() const override;
    int height() const override;
    void present(const std::vector<uint32_t>& buffer, int target_w, int target_h) override;

    void set_key_callback(KeyCallback cb) override;
    void set_active_callback(ActiveCallback cb) override;
    void move_to_left_edge() override;

    void dispatch_key_callback(KeyCode key, KeyModifier mod, bool is_pressed);
    void dispatch_active_callback(bool is_active);

private:
    struct mfb_window* m_window{nullptr};
    bool m_running{true};
    std::vector<uint32_t> m_presentation_pixel_buffer{};
    KeyCallback m_key_callback{};
    ActiveCallback m_active_callback{};
};
