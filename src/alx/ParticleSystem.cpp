#include "alx/ParticleSystem.h"
#include "alx/Random.h"
#include "alx/Layer.h"
#include "alx/Camera.h"
#include "Game.h"
#include "core/Draw.h"
#include "core/Log.h"
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
    if (m_pool[idx].active && m_pool[idx].life > 0.0f) {
        Log::warn_fmt_t("[ParticleSystem] Capacity (%zu) reached! Overwriting active particle (type=%d, remaining_life=%.2fs)",
                        POOL_CAPACITY, static_cast<int>(m_pool[idx].type), m_pool[idx].life);
    }
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

            // Viscous Perpendicular Ripple Modulation
            float dx = p.target_x - p.start_x;
            float dy = p.target_y - p.start_y;
            float len = std::sqrt(dx * dx + dy * dy);

            if (len > 0.001f) {
                float nx = -dy / len;
                float ny = dx / len;

                constexpr float frequency = 16.0f;
                constexpr float amplitude = 2.0f;
                float ripple = std::sin(t * frequency + p.param_a) * amplitude;

                p.x = base_x + nx * ripple;
                p.y = base_y + ny * ripple;
            } else {
                p.x = base_x;
                p.y = base_y;
            }

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
    float cam_min_x = -99999.0f, cam_max_x = 99999.0f;
    float cam_min_y = -99999.0f, cam_max_y = 99999.0f;

    if (camera) {
        float view_w = static_cast<float>(Game::WIDTH) / camera->zoom;
        float view_h = static_cast<float>(Game::HEIGHT) / camera->zoom;
        constexpr float margin = 16.0f;

        cam_min_x = camera->x - margin;
        cam_max_x = camera->x + view_w + margin;
        cam_min_y = camera->y - margin;
        cam_max_y = camera->y + view_h + margin;
    }

    for (const auto& p : m_pool) {
        if (!p.active || p.life <= 0.0f) continue;

        // Viewport Culling Check
        if (p.render_x < cam_min_x || p.render_x > cam_max_x ||
            p.render_y < cam_min_y || p.render_y > cam_max_y) {
            continue;
        }

        float progress = (p.max_life > 0.0f) ? (p.life / p.max_life) : 1.0f;
        uint32_t alpha = static_cast<uint8_t>(progress * 255.0f);
        uint32_t current_color = (p.color & 0x00FFFFFF) | (alpha << 24);

        int z_idx = PARTICLE_Z_INDEX;
        if (p.type == ParticleType::ManaPulseStraight || p.type == ParticleType::ManaPulseCurved) {
            z_idx = Layer::GroundFixtureItemFX; // Z = 3 (sloshing ripples render on top of solid underlay stream Z = 2)
        }

        if (p.size <= 2) {
            float s = static_cast<float>(p.size);
            Draw::rect(p.render_x, p.render_y, s, s, current_color, true, 1, z_idx);
        } else {
            float r = static_cast<float>(p.size) * 0.5f;
            Draw::circle(p.render_x, p.render_y, r, current_color, true, 1, z_idx);
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
