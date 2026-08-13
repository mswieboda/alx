#pragma once
#include <vector>
#include <cstdint>
#include "Layer.h"

namespace alx {

struct ManaSpark {
    static constexpr float DEFAULT_SIZE = 12.0f;
    static constexpr float HALF_SIZE = DEFAULT_SIZE / 2.0f;
    static constexpr float BOUNDARY_PADDING = 24.0f; // Padding buffer past map boundary before despawning
    static constexpr uint32_t COLOR_OUTER = 0xAA00CCFF;
    static constexpr uint32_t COLOR_INNER = 0xAAFFFFFF;
    static constexpr int Z_INDEX = Layer::WorldObjFX;

    // Charge attack timing, dimensions (decreased size), and partial transparent colors
    static constexpr float CHARGE_FULL_DURATION          = 0.5f;
    static constexpr float CHARGE_MAX_UNCHARGED_RADIUS   = DEFAULT_SIZE * 0.5f;
    static constexpr float CHARGE_FULLY_CHARGED_OUTER_SZ = 10.0f;
    static constexpr float CHARGE_FULLY_CHARGED_OUTER_OFF= 5.0f;
    static constexpr float CHARGE_FULLY_CHARGED_INNER_SZ = 6.0f;
    static constexpr float CHARGE_FULLY_CHARGED_INNER_OFF= 3.0f;
    static constexpr uint32_t CHARGE_AURA_COLOR          = 0x5500AAFF; // partial transparent cyan/blue
    static constexpr uint32_t CHARGE_FULL_OUTER_COLOR    = 0x5500FFFF; // partial transparent opacity bright cyan
    static constexpr uint32_t CHARGE_FULL_INNER_COLOR    = 0x55FFFFFF; // partial transparent opacity white

    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float lifetime = 2.0f;
    int damage = 2;

    ManaSpark(float px, float py, float pvx, float pvy);

    void update(float dt);
    void draw(std::vector<uint32_t>& pixel_buffer, float alpha) const;

    static void draw_charging(
        std::vector<uint32_t>& pixel_buffer, float alpha,
        float m_charge_timer, float cx, float cy, int y_sort_override
    );
};

} // namespace alx
