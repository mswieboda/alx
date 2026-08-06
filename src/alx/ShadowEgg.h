#pragma once
#include <cstdint>
#include <vector>
#include "core/Collision.h"

namespace alx {

namespace ShadowEggConstants {
    static constexpr float EJECT_FLIGHT_DURATION = 0.35f;
    static constexpr float INVULNERABLE_DURATION = 0.5f;
    static constexpr float PEAK_ARC_HEIGHT = 16.0f;
    static constexpr float INCUBATION_WOBBLE_THRESHOLD = 2.0f;
}

struct ShadowEgg {
    static constexpr float EGG_WIDTH = 12.0f;
    static constexpr float EGG_HEIGHT = 16.0f;
    static constexpr float MAX_INCUBATION_TIME = 5.0f;
    
    float x = 0.0f;
    float y = 0.0f;
    float width = EGG_WIDTH;
    float height = EGG_HEIGHT;
    
    float start_x = 0.0f;
    float start_y = 0.0f;
    float target_x = 0.0f;
    float target_y = 0.0f;
    float flight_timer = 0.0f;
    float flight_duration = 0.0f;
    float invulnerable_timer = 0.0f;
    
    float incubation_timer = MAX_INCUBATION_TIME;
    int hp = 1;
    bool hatched = false;
    bool destroyed = false;
    float wobble_time = 0.0f;
    uint32_t last_hit_swing_id = 0;
    
    ShadowEgg(float px, float py);
    ShadowEgg(float sx, float sy, float tx, float ty, float duration = ShadowEggConstants::EJECT_FLIGHT_DURATION);
    
    bool in_flight() const;
    bool is_invulnerable() const;
    float center_x() const;
    float center_y() const;
    Collision::Circle hurt_circle() const;
    Collision::AABB ground_aabb() const;
    
    void take_damage(int amount);
    void update(float dt);
    void draw(std::vector<uint32_t>& screen_buffer, float alpha) const;
};

} // namespace alx
