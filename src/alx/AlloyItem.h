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
    static constexpr float DEFAULT_WIDTH = 10.0f; // 8px front + 2px right slant
    static constexpr float DEFAULT_HEIGHT = 5.0f; // 3px front + 2px top slant
    static constexpr uint32_t FRONT_COLOR = 0xFF1E242C;     // Dark charcoal steel front face
    static constexpr uint32_t RIGHT_COLOR = 0xFF323A44;     // Dark slate silver right face
    static constexpr uint32_t HIGHLIGHT_COLOR = 0xFF8A94A4; // Sleek metallic silver top sheen face
    static constexpr int Z_INDEX = Layer::GroundItem;
};

struct AlloyItem {
    float x = 0.0f;
    float y = 0.0f;
    float width = AlloyItemConstants::DEFAULT_WIDTH;
    float height = AlloyItemConstants::DEFAULT_HEIGHT;
    uint32_t color = AlloyItemConstants::FRONT_COLOR;
    bool active = true;

    AlloyItem(float px = 0.0f, float py = 0.0f, float w = AlloyItemConstants::DEFAULT_WIDTH, float h = AlloyItemConstants::DEFAULT_HEIGHT, uint32_t col = AlloyItemConstants::FRONT_COLOR)
        : x(px), y(py), width(w), height(h), color(col), active(true) {}

    float center_x() const {
        return x + (width / 2.0f);
    }

    float center_y() const {
        return y + (height / 2.0f);
    }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha) const {
        if (!active) return;

        int z_idx = AlloyItemConstants::Z_INDEX;
        int sort_y = static_cast<int>(y + height);

        // 1. Front Face (8x3 px at x, y+2)
        Draw::rect(
            x,
            y + 2.0f,
            8.0f,
            3.0f,
            color,
            true, // fill
            1,    // thickness
            z_idx,
            sort_y
        );

        // 2. Right Face (2px slanted columns on right side, matching building oblique angle)
        for (int dx = 0; dx < 2; ++dx) {
            Draw::rect(
                x + 8.0f + dx,
                y + 2.0f - dx,
                1.0f,
                3.0f,
                AlloyItemConstants::RIGHT_COLOR,
                true, // fill
                1,    // thickness
                z_idx,
                sort_y
            );
        }

        // 3. Top Face Sheen (8x2 px slanted horizontal sheen face with higher z_index for guaranteed visibility)
        for (int dy = 0; dy < 2; ++dy) {
            Draw::rect(
                x + dy,
                y + 1.0f - dy,
                8.0f,
                1.0f,
                AlloyItemConstants::HIGHLIGHT_COLOR,
                true, // fill
                1,    // thickness
                z_idx + 1,
                sort_y + 1
            );
        }
    }
};

} // namespace alx
