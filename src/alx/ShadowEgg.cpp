#include "alx/ShadowEgg.h"
#include <cmath>
#include <algorithm>
#include "core/Draw.h"
#include "alx/Layer.h"
#include "alx/DrawFX.h"

namespace alx {

ShadowEgg::ShadowEgg(float px, float py) : x(px), y(py) {}

float ShadowEgg::center_x() const { return x + width * 0.5f; }
float ShadowEgg::center_y() const { return y + height * 0.5f; }

Collision::Circle ShadowEgg::hurt_circle() const {
    return Collision::Circle{ center_x(), center_y(), width * 0.6f };
}

void ShadowEgg::take_damage(int amount) {
    hp -= amount;
    if (hp <= 0) {
        destroyed = true;
    }
}

void ShadowEgg::update(float dt) {
    if (hatched || destroyed) return;
    
    incubation_timer -= dt;
    wobble_time += dt;
    
    if (incubation_timer <= 0.0f) {
        incubation_timer = 0.0f;
        hatched = true;
    }
}

void ShadowEgg::draw(std::vector<uint32_t>& screen_buffer, float alpha) const {
    if (hatched || destroyed) return;
    
    float draw_x = x;
    float draw_y = y;
    
    // Wobble logic as it nears hatching
    if (incubation_timer < 2.0f) {
        float intensity = (2.0f - incubation_timer) * 1.5f; // Up to 3px shake
        draw_x += std::sin(wobble_time * 40.0f) * intensity;
    }
    
    int sort_y = static_cast<int>(draw_y + height);
    
    // Foreshortened shadow
    DrawFX::shadow(
        draw_x, draw_y + height - 4.0f,
        width, 6.0f,
        Layer::WorldObjBG, sort_y,
        0.5f, 0.4f
    );
    
    // Main egg body (Layered blocks for pixel-art oval)
    // Dark base (12x16)
    Draw::rect(draw_x, draw_y + 2.0f, width, height - 4.0f, 0xFF1B112C, true, 1, Layer::WorldObj, sort_y);
    Draw::rect(draw_x + 2.0f, draw_y, width - 4.0f, height, 0xFF1B112C, true, 1, Layer::WorldObj, sort_y);
    
    // Middle twilight layer (8x12)
    Draw::rect(draw_x + 2.0f, draw_y + 3.0f, width - 4.0f, height - 6.0f, 0xFF2A153D, true, 1, Layer::WorldObj, sort_y);
    Draw::rect(draw_x + 3.0f, draw_y + 2.0f, width - 6.0f, height - 4.0f, 0xFF2A153D, true, 1, Layer::WorldObj, sort_y);
    
    // Inner pulse layer
    float pulse = std::sin(wobble_time * 5.0f) * 0.5f + 0.5f;
    uint32_t pulse_color = (pulse > 0.5f) ? 0xFF772299 : 0xFF551177;
    Draw::rect(draw_x + 4.0f, draw_y + 4.0f, width - 8.0f, height - 8.0f, pulse_color, true, 1, Layer::WorldObj, sort_y);
    
    // Top highlight (glint)
    Draw::rect(draw_x + 4.0f, draw_y + 1.0f, 4.0f, 2.0f, 0xFF8844AA, true, 1, Layer::WorldObj, sort_y);
}

} // namespace alx
