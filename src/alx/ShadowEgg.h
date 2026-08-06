#pragma once
#include <cstdint>
#include <vector>
#include "core/Collision.h"

namespace alx {

struct ShadowEgg {
    static constexpr float EGG_WIDTH = 12.0f;
    static constexpr float EGG_HEIGHT = 16.0f;
    static constexpr float MAX_INCUBATION_TIME = 5.0f;
    
    float x = 0.0f;
    float y = 0.0f;
    float width = EGG_WIDTH;
    float height = EGG_HEIGHT;
    
    float incubation_timer = MAX_INCUBATION_TIME;
    int hp = 1;
    bool hatched = false;
    bool destroyed = false;
    float wobble_time = 0.0f;
    uint32_t last_hit_swing_id = 0;
    
    ShadowEgg(float px, float py);
    
    float center_x() const;
    float center_y() const;
    Collision::Circle hurt_circle() const;
    
    void take_damage(int amount);
    void update(float dt);
    void draw(std::vector<uint32_t>& screen_buffer, float alpha) const;
};

} // namespace alx
