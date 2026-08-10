#pragma once
#include <vector>
#include <cstdint>

namespace alx {

struct ManaSpark {
    static constexpr float DEFAULT_SIZE = 4.0f;
    static constexpr float HALF_SIZE = 2.0f;
    static constexpr float BOUNDARY_PADDING = 24.0f; // Padding buffer past map boundary before despawning
    static constexpr uint32_t COLOR_OUTER = 0xFFFFFFFF; // Opaque white
    static constexpr uint32_t COLOR_INNER = 0xFF00DDFF; // Opaque cyan/blue

    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float lifetime = 2.0f;
    int damage = 2;

    ManaSpark(float px, float py, float pvx, float pvy);

    void update(float dt);
    void draw(std::vector<uint32_t>& pixel_buffer, float alpha) const;
};

} // namespace alx
