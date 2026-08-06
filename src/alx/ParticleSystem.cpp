#include "alx/ParticleSystem.h"
#include "alx/Random.h"
#include "alx/Layer.h"
#include "alx/Camera.h"
#include "Game.h"
#include "core/Draw.h"
#include <cmath>

namespace alx {

Particle* ParticleSystem::emit() {
    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        size_t idx = (m_next_slot + i) % POOL_CAPACITY;
        if (!m_pool[idx].active) {
            m_next_slot = (idx + 1) % POOL_CAPACITY;
            m_pool[idx] = Particle{};
            m_pool[idx].active = true;
            return &m_pool[idx];
        }
    }

    size_t idx = m_next_slot;
    m_next_slot = (m_next_slot + 1) % POOL_CAPACITY;

    m_pool[idx] = Particle{};
    m_pool[idx].active = true;
    return &m_pool[idx];
}

void ParticleSystem::clear() {
    for (auto& p : m_pool) {
        p.active = false;
    }
    m_next_slot = 0;
}

void ParticleSystem::update(float dt) {
    for (auto& p : m_pool) {
        if (!p.active) continue;

        p.life -= dt;
        if (p.life <= 0.0f) {
            p.active = false;
            continue;
        }

        switch (p.type) {
        case ParticleType::Spark: {
            constexpr float drag = 5.0f;
            p.vx -= p.vx * drag * dt;
            p.vy -= p.vy * drag * dt;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.render_x = p.x;
            p.render_y = p.y;
            break;
        }
        case ParticleType::Blood:
        case ParticleType::OozeDrip: {
            constexpr float drag = 3.0f;
            constexpr float gravity = 220.0f;
            p.vx -= p.vx * drag * dt;
            p.vy -= p.vy * drag * dt;
            p.vy += gravity * dt;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.render_x = p.x;
            p.render_y = p.y;
            break;
        }
        case ParticleType::LightEmber: {
            p.vy -= 15.0f * dt;
            p.vx += Random::get_float(-10.0f, 10.0f) * dt;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.render_x = p.x;
            p.render_y = p.y;
            break;
        }
        case ParticleType::ManaPulseStraight: {
            float t = p.progress();

            // Base Linear Position
            float base_x = p.start_x + (p.target_x - p.start_x) * t;
            float base_y = p.start_y + (p.target_y - p.start_y) * t;

            constexpr float frequency = 6.0f;
            constexpr float amplitude = 1.2f;
            float ripple = std::sin(t * frequency + p.param_a) * amplitude;

            p.x = base_x + p.nx * ripple;
            p.y = base_y + p.ny * ripple;

            p.render_x = p.x;
            p.render_y = p.y;
            break;
        }
        case ParticleType::ManaPulseCurved: {
            float t = p.progress();

            // Quadratic Bezier Formula: P(t) = (1-t)^2 * P0 + 2(1-t)t * P_control + t^2 * P1
            float u = 1.0f - t;
            float tt = t * t;
            float uu = u * u;

            float base_x = uu * p.start_x + 2.0f * u * t * p.control_x + tt * p.target_x;
            float base_y = uu * p.start_y + 2.0f * u * t * p.control_y + tt * p.target_y;

            constexpr float frequency = 6.0f;
            constexpr float amplitude = 1.2f;
            float ripple = std::sin(t * frequency + p.param_a) * amplitude;

            p.x = base_x + p.nx * ripple;
            p.y = base_y + p.ny * ripple;

            p.render_x = p.x;
            p.render_y = p.y;
            break;
        }
        default: {
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.render_x = p.x;
            p.render_y = p.y;
            break;
        }
        }
    }
}

void ParticleSystem::draw(const Camera* camera) const {
    for (const auto& p : m_pool) {
        if (!p.active || p.life <= 0.0f) continue;

        // Viewport Culling Check (Option 2 padded AABB check)
        if (camera && !camera->is_aabb_visible(p.render_x - p.size, p.render_y - p.size, p.size * 2.0f, p.size * 2.0f)) {
            continue;
        }

        float progress = (p.max_life > 0.0f) ? (p.life / p.max_life) : 1.0f;
        uint32_t base_alpha = (p.color >> 24) & 0xFF;
        uint32_t alpha = static_cast<uint8_t>(progress * static_cast<float>(base_alpha));
        uint32_t current_color = (p.color & 0x00FFFFFF) | (alpha << 24);

        int z_idx = p.z_index;
        if (p.type == ParticleType::ManaPulseStraight || p.type == ParticleType::ManaPulseCurved) {
            z_idx = Layer::GroundFixtureItemFX; // Z = 3 (sloshing ripples render on top of solid underlay stream Z = 2)

            // Render flowing liquid streak line oriented along flow direction
            float dx = p.target_x - p.start_x;
            float dy = p.target_y - p.start_y;
            if (std::abs(dx) >= std::abs(dy)) {
                // Horizontal liquid streak (3.5px wide, 1px high)
                Draw::rect(p.render_x - 1.75f, p.render_y, 3.5f, 1.0f, current_color, true, 1, z_idx, p.y_sort_override);
            } else {
                // Vertical liquid streak (1px wide, 3.5px high)
                Draw::rect(p.render_x, p.render_y - 1.75f, 1.0f, 3.5f, current_color, true, 1, z_idx, p.y_sort_override);
            }
            continue;
        }

        if (p.type == ParticleType::OozeDrip) {
            // [VTR]: Dynamic vertical teardrop rendering (3.0px to 5.0px height based on downward speed vy)
            float speed_ratio = std::clamp((p.vy - 10.0f) / 100.0f, 0.0f, 1.0f);
            float teardrop_h = 3.0f + speed_ratio * 2.0f; // 3.0px to 5.0px
            float teardrop_w = static_cast<float>(p.size); // 1.0px or 2.0px
            Draw::rect(p.render_x, p.render_y, teardrop_w, teardrop_h, current_color, true, 1, z_idx, p.y_sort_override);
            continue;
        }

        if (p.size <= 2) {
            float s = static_cast<float>(p.size);
            Draw::rect(p.render_x, p.render_y, s, s, current_color, true, 1, z_idx, p.y_sort_override);
        } else {
            float r = static_cast<float>(p.size) * 0.5f;
            Draw::circle(p.render_x, p.render_y, r, current_color, true, 1, z_idx, p.y_sort_override);
        }
    }
}

size_t ParticleSystem::active_count() const {
    size_t count = 0;
    for (const auto& p : m_pool) {
        if (p.active) ++count;
    }
    return count;
}

} // namespace alx
