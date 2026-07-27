#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "core/Draw.h"
#include "alx/Camera.h"
#include "alx/Layer.h"

namespace alx {

struct AlloyItemConstants {
    static constexpr float DEFAULT_WIDTH = 8.0f;
    static constexpr float DEFAULT_HEIGHT = 8.0f;
    static constexpr uint32_t COLOR = 0xFFFF9900; // Glowing Amber/Gold Cursed Alloy
    static constexpr int Z_INDEX = Layer::GroundItem;
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

    void draw(std::vector<uint32_t>& screen_buffer, float alpha) const {
        if (!active) return;

        // Draw alloy nugget core
        Draw::rect(
            x,
            y,
            width,
            height,
            color,
            true, // fill
            1,    // thickness
            AlloyItemConstants::Z_INDEX
        );
    }
};

} // namespace alx
