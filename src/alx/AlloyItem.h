#pragma once
#include <cstdint>
#include <vector>
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
    float lifetime = 30.0f;
    bool flashing = false;
    float flash_timer = 0.0f;
    bool active = true;

    AlloyItem(float px = 0.0f, float py = 0.0f, float w = AlloyItemConstants::DEFAULT_WIDTH, float h = AlloyItemConstants::DEFAULT_HEIGHT, uint32_t col = AlloyItemConstants::FRONT_COLOR)
        : x(px), y(py), width(w), height(h), color(col), active(true) {}

    void update(float dt);

    float center_x() const {
        return x + (width / 2.0f);
    }

    float center_y() const {
        return y + (height / 2.0f);
    }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha) const;
};

} // namespace alx
