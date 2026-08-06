#include "alx/AlloyItem.h"
#include <algorithm>
#include <cmath>
#include "core/Draw.h"

namespace alx {

void AlloyItem::update(float dt) {
    lifetime -= dt;
    if (lifetime <= 10.0f) {
        flash_timer += dt;
        if (flash_timer >= 0.1f) {
            flash_timer = 0.0f;
            flashing = !flashing;
        }
    }
    if (lifetime <= 0.0f) {
        active = false;
    }
}

void AlloyItem::draw(std::vector<uint32_t>& screen_buffer, float alpha) const {
    (void)screen_buffer;
    (void)alpha;

    if (!active || flashing) return;

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

} // namespace alx
