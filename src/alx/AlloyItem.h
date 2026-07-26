#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "core/Draw.h"
#include "alx/Camera.h"

namespace alx {

struct AlloyItemConstants {
    static constexpr float DEFAULT_WIDTH = 8.0f;
    static constexpr float DEFAULT_HEIGHT = 8.0f;
    static constexpr uint32_t COLOR = 0xFFFF9900; // Glowing Amber/Gold Cursed Alloy
    static constexpr int Z_INDEX = 4; // Below player (z=10), above floor (z=0)
};

struct AlloyItem {
    float x = 0.0f;
    float y = 0.0f;
    float width = AlloyItemConstants::DEFAULT_WIDTH;
    float height = AlloyItemConstants::DEFAULT_HEIGHT;
    uint32_t color = AlloyItemConstants::COLOR;
    bool active = true;

    AlloyItem(float px = 0.0f, float py = 0.0f, float w = AlloyItemConstants::DEFAULT_WIDTH, float h = AlloyItemConstants::DEFAULT_HEIGHT, uint32_t col = AlloyItemConstants::COLOR)
        : x(px), y(py), width(w), height(h), color(col), active(true) {}

    float center_x() const {
        return x + (width / 2.0f);
    }

    float center_y() const {
        return y + (height / 2.0f);
    }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha, const alx::Camera& camera) const {
        if (!active) return;

        int draw_x = camera.to_screen_x(x);
        int draw_y = camera.to_screen_y(y);
        int draw_w = std::max(1, static_cast<int>(std::round(width * camera.get_zoom())));
        int draw_h = std::max(1, static_cast<int>(std::round(height * camera.get_zoom())));

        // Draw alloy nugget core
        Draw::rect(
            draw_x,
            draw_y,
            draw_w,
            draw_h,
            color,
            true, // fill
            1,    // thickness
            AlloyItemConstants::Z_INDEX
        );
    }
};

} // namespace alx
