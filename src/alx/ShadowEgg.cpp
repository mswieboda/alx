#include "alx/ShadowEgg.h"
#include <cmath>
#include <algorithm>
#include "core/Audio.h"
#include "core/Draw.h"
#include "alx/Layer.h"
#include "alx/DrawFX.h"
#include "alx/SFX.h"

namespace alx {

ShadowEgg::ShadowEgg(float px, float py)
    : x(px), y(py), start_x(px), start_y(py), target_x(px), target_y(py),
      flight_timer(0.0f), flight_duration(0.0f), invulnerable_timer(0.0f) {}

ShadowEgg::ShadowEgg(float sx, float sy, float tx, float ty, float duration)
    : x(sx), y(sy), start_x(sx), start_y(sy), target_x(tx), target_y(ty),
      flight_timer(duration), flight_duration(duration),
      invulnerable_timer(ShadowEggConstants::INVULNERABLE_DURATION) {}

bool ShadowEgg::in_flight() const {
    return flight_timer > 0.0f;
}

bool ShadowEgg::is_invulnerable() const {
    return invulnerable_timer > 0.0f || in_flight();
}

float ShadowEgg::center_x() const { return x + width * 0.5f; }
float ShadowEgg::center_y() const { return y + height * 0.5f; }

Collision::Circle ShadowEgg::hurt_circle() const {
    return Collision::Circle{ center_x(), center_y(), width * 0.6f };
}

Collision::AABB ShadowEgg::ground_aabb() const {
    // If in flight, ground AABB is at target landing location
    float gx = in_flight() ? target_x : x;
    float gy = in_flight() ? target_y : y;
    return Collision::AABB{ gx, gy, width, height };
}

void ShadowEgg::take_damage(int amount) {
    if (is_invulnerable()) return;

    hp -= amount;
    Audio::play_sfx(SFX::fixture_hit());
    if (hp <= 0) {
        destroyed = true;
    }
}

void ShadowEgg::update(float dt) {
    if (hatched || destroyed) return;
    
    if (in_flight()) {
        flight_timer = std::max(0.0f, flight_timer - dt);
        if (flight_duration > 0.0f) {
            float progress = 1.0f - (flight_timer / flight_duration);
            progress = std::clamp(progress, 0.0f, 1.0f);
            x = start_x + (target_x - start_x) * progress;
            y = start_y + (target_y - start_y) * progress;
        }
        if (flight_timer <= 0.0f) {
            x = target_x;
            y = target_y;
        }
        return;
    }

    if (invulnerable_timer > 0.0f) {
        invulnerable_timer = std::max(0.0f, invulnerable_timer - dt);
    }
    
    incubation_timer -= dt;
    wobble_time += dt;
    
    if (incubation_timer <= 0.0f) {
        incubation_timer = 0.0f;
        hatched = true;
        Audio::play_sfx(SFX::egg_hatch());
    }
}

void ShadowEgg::draw(std::vector<uint32_t>& screen_buffer, float alpha) const {
    if (hatched || destroyed) return;
    
    float draw_x = x;
    float draw_y = y;
    float height_arc_offset = 0.0f;

    if (in_flight() && flight_duration > 0.0f) {
        float progress = 1.0f - (flight_timer / flight_duration);
        progress = std::clamp(progress, 0.0f, 1.0f);
        // Parabolic arc: 4 * h * t * (1 - t)
        height_arc_offset = 4.0f * ShadowEggConstants::PEAK_ARC_HEIGHT * progress * (1.0f - progress);
    } else if (incubation_timer < ShadowEggConstants::INCUBATION_WOBBLE_THRESHOLD) {
        // Wobble logic as it nears hatching
        float intensity = (ShadowEggConstants::INCUBATION_WOBBLE_THRESHOLD - incubation_timer) * 1.5f;
        draw_x += std::sin(wobble_time * 40.0f) * intensity;
    }
    
    int sort_y = static_cast<int>(target_y + height);
    
    // Foreshortened shadow at landing/ground position
    float shadow_x = in_flight() ? target_x : draw_x;
    float shadow_y = in_flight() ? target_y : draw_y;
    DrawFX::shadow(
        shadow_x, shadow_y + height - 4.0f,
        width, 6.0f,
        Layer::WorldObjBG, sort_y,
        0.5f, 0.4f
    );
    
    // Egg Y rendering position shifted upwards by arc flight offset
    float render_y = draw_y - height_arc_offset;

    // Main egg body (Layered blocks for pixel-art oval)
    // Dark base (12x16)
    Draw::rect(draw_x, render_y + 2.0f, width, height - 4.0f, 0xFF1B112C, true, 1, Layer::WorldObj, sort_y);
    Draw::rect(draw_x + 2.0f, render_y, width - 4.0f, height, 0xFF1B112C, true, 1, Layer::WorldObj, sort_y);
    
    // Middle twilight layer (8x12)
    Draw::rect(draw_x + 2.0f, render_y + 3.0f, width - 4.0f, height - 6.0f, 0xFF2A153D, true, 1, Layer::WorldObj, sort_y);
    Draw::rect(draw_x + 3.0f, render_y + 2.0f, width - 6.0f, height - 4.0f, 0xFF2A153D, true, 1, Layer::WorldObj, sort_y);
    
    // Inner pulse layer
    float pulse = std::sin(wobble_time * 5.0f) * 0.5f + 0.5f;
    uint32_t pulse_color = (pulse > 0.5f) ? 0xFF772299 : 0xFF551177;
    if (invulnerable_timer > 0.0f) {
        pulse_color = 0xFF9933CC; // Bright glowing purple when incubating shield is active
    }
    Draw::rect(draw_x + 4.0f, render_y + 4.0f, width - 8.0f, height - 8.0f, pulse_color, true, 1, Layer::WorldObj, sort_y);
    
    // Top highlight (glint)
    Draw::rect(draw_x + 4.0f, render_y + 1.0f, 4.0f, 2.0f, 0xFF8844AA, true, 1, Layer::WorldObj, sort_y);
}

} // namespace alx
