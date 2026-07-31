#include "alx/ParticleSystem.h"
#include "core/Draw.h"

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

        // Kinematic physics update (Route A)
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.render_x = p.x;
        p.render_y = p.y;
    }
}

void ParticleSystem::draw() const {
    for (const auto& p : m_pool) {
        if (!p.active || p.life <= 0.0f) continue;

        float progress = (p.max_life > 0.0f) ? (p.life / p.max_life) : 1.0f;
        uint32_t alpha = static_cast<uint8_t>(progress * 255.0f);
        uint32_t current_color = (p.color & 0x00FFFFFF) | (alpha << 24);

        if (p.size <= 2) {
            float s = static_cast<float>(p.size);
            Draw::rect(p.render_x, p.render_y, s, s, current_color, true, 1, PARTICLE_Z_INDEX);
        } else {
            float r = static_cast<float>(p.size) * 0.5f;
            Draw::circle(p.render_x, p.render_y, r, current_color, true, 1, PARTICLE_Z_INDEX);
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
