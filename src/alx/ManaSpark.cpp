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
    Draw::rect(static_cast<int>(x) - 2, static_cast<int>(y) - 2, 4, 4, 0xFFFFFFFF, true, 1, z_idx, sort_y);
    Draw::rect(static_cast<int>(x) - 1, static_cast<int>(y) - 1, 2, 2, 0xFF00DDFF, true, 1, z_idx, sort_y);
}

} // namespace alx
