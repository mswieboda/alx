#include "alx/ManaSpark.h"
#include "core/Draw.h"

namespace alx {

ManaSpark::ManaSpark(float px, float py, float pvx, float pvy)
    : x(px), y(py), vx(pvx), vy(pvy) {}

void ManaSpark::update(float dt) {
    x += vx * dt;
    y += vy * dt;
    lifetime -= dt;
}

void ManaSpark::draw(std::vector<uint32_t>& pixel_buffer, float alpha) const {
    int z_idx = 16; // Layer::WorldObjFX
    int sort_y = static_cast<int>(y);
    float quarter_size = HALF_SIZE * 0.5f;
    Draw::rect(static_cast<int>(x - HALF_SIZE), static_cast<int>(y - HALF_SIZE), DEFAULT_SIZE, DEFAULT_SIZE, COLOR_OUTER, true, 1, z_idx, sort_y);
    Draw::rect(static_cast<int>(x - quarter_size), static_cast<int>(y - quarter_size), HALF_SIZE, HALF_SIZE, COLOR_INNER, true, 1, z_idx, sort_y);
}

} // namespace alx
