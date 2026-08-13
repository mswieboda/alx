#include "alx/ManaSpark.h"
#include "core/Draw.h"

namespace alx {

namespace {
    // shared method used between charging and sparks
    void _draw(float cx, float cy, float size, int sort_y) {
        float half_size = size * 0.5f;
        float quarter_size = half_size * 0.5f;

        // outer
        Draw::rect(
            cx - half_size, cy - half_size, // x, y
            // cx, cy,
            size, size, // w, h
            ManaSpark::COLOR_OUTER,
            true, 1, // filled, thickness
            ManaSpark::Z_INDEX, sort_y
        );
    }
} // namespace private

ManaSpark::ManaSpark(float px, float py, float pvx, float pvy)
    : x(px), y(py), vx(pvx), vy(pvy) {}

void ManaSpark::update(float dt) {
    x += vx * dt;
    y += vy * dt;
    lifetime -= dt;
}

void ManaSpark::draw(std::vector<uint32_t>& pixel_buffer, float alpha) const {
    int sort_y = static_cast<int>(y);
    float quarter_size = HALF_SIZE * 0.5f;

    _draw(x, y, DEFAULT_SIZE, sort_y);
}

// TODO: maybe tweak this so it's compeletely separate from sparks visuals
void ManaSpark::draw_charging(
    std::vector<uint32_t>& pixel_buffer, float alpha,
    float m_charge_timer, float cx, float cy, int y_sort_override
) {
    if (m_charge_timer < CHARGE_FULL_DURATION) {
        float progress = m_charge_timer / CHARGE_FULL_DURATION;
        int r = static_cast<int>(progress * CHARGE_MAX_UNCHARGED_RADIUS);
        if (r > 1) {
            float size = static_cast<float>(r * 2);

            _draw(cx, cy, size, y_sort_override);
        }
    } else {
        // Fully charged!

        // TODO: do something different like pulsing/flashing blue/white etc
        _draw(cx, cy, DEFAULT_SIZE, y_sort_override);
    }
}

} // namespace alx
