#include "alx/ParticleSystem.h"
#include "alx/Random.h"
#include "alx/Layer.h"
#include "alx/Camera.h"
#include "Game.h"
#include "core/Draw.h"
#include <algorithm>
#include <cmath>

namespace alx {

namespace {

namespace ParticleConfig {
    constexpr float SPARK_DRAG            = 5.0f;
    constexpr float GRAVITY_DRAG          = 3.0f;
    constexpr float GRAVITY_ACCEL         = 220.0f;
    constexpr float EMBER_LIFT_ACCEL      = 15.0f;
    constexpr float EMBER_JITTER_RANGE    = 10.0f;
    constexpr float MANA_RIPPLE_FREQ      = 6.0f;
    constexpr float MANA_RIPPLE_AMP       = 1.2f;
    constexpr float OOZE_SPEED_MIN        = 10.0f;
    constexpr float OOZE_SPEED_SCALE      = 100.0f;
    constexpr float OOZE_BASE_HEIGHT      = 3.0f;
    constexpr float OOZE_EXTRA_HEIGHT     = 2.0f;
    constexpr float STREAK_WIDTH          = 3.5f;
    constexpr float STREAK_HALF_WIDTH     = 1.75f;
    constexpr float STREAK_THICKNESS      = 1.0f;
    constexpr uint8_t RECT_SIZE_THRESHOLD = 2;
    constexpr float CULL_PADDING_FACTOR   = 2.0f;
} // namespace ParticleConfig

void update_spark_physics(Particle& p, float dt) {
    p.vx -= p.vx * ParticleConfig::SPARK_DRAG * dt;
    p.vy -= p.vy * ParticleConfig::SPARK_DRAG * dt;
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    p.render_x = p.x;
    p.render_y = p.y;
}

void update_gravity_physics(Particle& p, float dt) {
    p.vx -= p.vx * ParticleConfig::GRAVITY_DRAG * dt;
    p.vy -= p.vy * ParticleConfig::GRAVITY_DRAG * dt;
    p.vy += ParticleConfig::GRAVITY_ACCEL * dt;
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    p.render_x = p.x;
    p.render_y = p.y;
}

void update_ember_physics(Particle& p, float dt) {
    p.vy -= ParticleConfig::EMBER_LIFT_ACCEL * dt;
    p.vx += Random::get_float(-ParticleConfig::EMBER_JITTER_RANGE, ParticleConfig::EMBER_JITTER_RANGE) * dt;
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    p.render_x = p.x;
    p.render_y = p.y;
}

void update_mana_straight_physics(Particle& p) {
    float t = p.progress();
    float base_x = p.start_x + (p.target_x - p.start_x) * t;
    float base_y = p.start_y + (p.target_y - p.start_y) * t;

    float ripple = std::sin(t * ParticleConfig::MANA_RIPPLE_FREQ + p.param_a) * ParticleConfig::MANA_RIPPLE_AMP;

    p.x = base_x + p.nx * ripple;
    p.y = base_y + p.ny * ripple;
    p.render_x = p.x;
    p.render_y = p.y;
}

void update_mana_curved_physics(Particle& p) {
    float t = p.progress();
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;

    float base_x = uu * p.start_x + 2.0f * u * t * p.control_x + tt * p.target_x;
    float base_y = uu * p.start_y + 2.0f * u * t * p.control_y + tt * p.target_y;

    float ripple = std::sin(t * ParticleConfig::MANA_RIPPLE_FREQ + p.param_a) * ParticleConfig::MANA_RIPPLE_AMP;

    p.x = base_x + p.nx * ripple;
    p.y = base_y + p.ny * ripple;
    p.render_x = p.x;
    p.render_y = p.y;
}

void update_default_physics(Particle& p, float dt) {
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    p.render_x = p.x;
    p.render_y = p.y;
}

void update_particle_physics(Particle& p, float dt) {
    switch (p.type) {
    case ParticleType::Spark:
        update_spark_physics(p, dt);
        break;
    case ParticleType::Blood:
    case ParticleType::OozeDrip:
        update_gravity_physics(p, dt);
        break;
    case ParticleType::LightEmber:
        update_ember_physics(p, dt);
        break;
    case ParticleType::ManaPulseStraight:
        update_mana_straight_physics(p);
        break;
    case ParticleType::ManaPulseCurved:
        update_mana_curved_physics(p);
        break;
    default:
        update_default_physics(p, dt);
        break;
    }
}

void render_mana_pulse_streak(const Particle& p, uint32_t color) {
    int z_idx = Layer::GroundFixtureItemFX;
    float dx = p.target_x - p.start_x;
    float dy = p.target_y - p.start_y;

    if (std::abs(dx) >= std::abs(dy)) {
        Draw::rect(p.render_x - ParticleConfig::STREAK_HALF_WIDTH, p.render_y, ParticleConfig::STREAK_WIDTH, ParticleConfig::STREAK_THICKNESS, color, true, 1, z_idx, p.y_sort_override);
    } else {
        Draw::rect(p.render_x, p.render_y - ParticleConfig::STREAK_HALF_WIDTH, ParticleConfig::STREAK_THICKNESS, ParticleConfig::STREAK_WIDTH, color, true, 1, z_idx, p.y_sort_override);
    }
}

void render_ooze_teardrop(const Particle& p, uint32_t color) {
    float speed_ratio = std::clamp((p.vy - ParticleConfig::OOZE_SPEED_MIN) / ParticleConfig::OOZE_SPEED_SCALE, 0.0f, 1.0f);
    float teardrop_h = ParticleConfig::OOZE_BASE_HEIGHT + speed_ratio * ParticleConfig::OOZE_EXTRA_HEIGHT;
    float teardrop_w = static_cast<float>(p.size);
    Draw::rect(p.render_x, p.render_y, teardrop_w, teardrop_h, color, true, 1, p.z_index, p.y_sort_override);
}

void render_standard_particle(const Particle& p, uint32_t color) {
    if (p.size <= ParticleConfig::RECT_SIZE_THRESHOLD) {
        float s = static_cast<float>(p.size);
        Draw::rect(p.render_x, p.render_y, s, s, color, true, 1, p.z_index, p.y_sort_override);
    } else {
        float r = static_cast<float>(p.size) * 0.5f;
        Draw::circle(p.render_x, p.render_y, r, color, true, 1, p.z_index, p.y_sort_override);
    }
}

void render_single_particle(const Particle& p, const Camera* camera) {
    if (camera && !camera->is_aabb_visible(p.render_x - p.size, p.render_y - p.size, p.size * ParticleConfig::CULL_PADDING_FACTOR, p.size * ParticleConfig::CULL_PADDING_FACTOR)) {
        return;
    }

    float life_fraction = (p.max_life > 0.0f) ? (p.life / p.max_life) : 1.0f;
    uint32_t base_alpha = (p.color >> 24) & 0xFF;
    uint32_t alpha = static_cast<uint8_t>(life_fraction * static_cast<float>(base_alpha));
    uint32_t current_color = (p.color & 0x00FFFFFF) | (alpha << 24);

    if (p.type == ParticleType::ManaPulseStraight || p.type == ParticleType::ManaPulseCurved) {
        render_mana_pulse_streak(p, current_color);
        return;
    }

    if (p.type == ParticleType::OozeDrip) {
        render_ooze_teardrop(p, current_color);
        return;
    }

    render_standard_particle(p, current_color);
}

} // namespace

ParticleSystem::ParticleSystem() {
    init_free_list();
}

void ParticleSystem::init_free_list() {
    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        m_free_slots[i] = static_cast<uint16_t>(i);
    }
    m_free_count = POOL_CAPACITY;
    m_active_count = 0;
    m_next_slot = 0;
}

Particle* ParticleSystem::emit() {
    size_t slot = 0;
    if (m_free_count > 0) {
        slot = m_free_slots[--m_free_count];
        ++m_active_count;
    } else {
        slot = m_next_slot;
        m_next_slot = (m_next_slot + 1) % POOL_CAPACITY;
    }

    m_pool[slot] = Particle{};
    m_pool[slot].active = true;
    return &m_pool[slot];
}

void ParticleSystem::clear() {
    for (auto& p : m_pool) {
        p.active = false;
    }
    init_free_list();
}

void ParticleSystem::update(float dt) {
    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        auto& p = m_pool[i];
        if (!p.active) continue;

        p.life -= dt;
        if (p.life <= 0.0f) {
            p.active = false;
            m_free_slots[m_free_count++] = static_cast<uint16_t>(i);
            if (m_active_count > 0) {
                --m_active_count;
            }
            continue;
        }

        update_particle_physics(p, dt);
    }
}

void ParticleSystem::draw(const Camera* camera) const {
    for (const auto& p : m_pool) {
        if (!p.active || p.life <= 0.0f) continue;
        render_single_particle(p, camera);
    }
}

} // namespace alx

