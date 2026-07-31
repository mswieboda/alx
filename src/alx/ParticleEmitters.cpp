#include "alx/ParticleEmitters.h"
#include "alx/ParticleSystem.h"
#include "alx/Random.h"
#include <cmath>

namespace alx {
namespace ParticleEmitters {

void spawn_sword_slash_trail(ParticleSystem& ps, float arc_cx, float arc_cy, float arc_radius) {
    // Spawn 2 crisp white/light-grey slash particles anchored near the outer edge of the sword attack circle
    for (int i = 0; i < 2; ++i) {
        if (Particle* p = ps.emit()) {
            float angle = Random::get_float(0.0f, 6.283185f);
            float dist = arc_radius - Random::get_float(1.0f, 4.0f);
            float spawn_x = arc_cx + std::cos(angle) * dist;
            float spawn_y = arc_cy + std::sin(angle) * dist;

            p->x = spawn_x;
            p->y = spawn_y;
            p->render_x = spawn_x;
            p->render_y = spawn_y;
            p->vx = Random::get_float(-15.0f, 15.0f);
            p->vy = Random::get_float(-15.0f, 15.0f);
            p->life = Random::get_float(0.1f, 0.2f); // Short lifespan for crisp trail
            p->max_life = p->life;

            uint8_t shade = static_cast<uint8_t>(Random::get_int(220, 255));
            p->color = 0xFF000000 | (shade << 16) | (shade << 8) | shade; // White / light grey
            p->size = 1;
            p->type = ParticleType::Spark;
        }
    }
}

void spawn_hit_sparks(ParticleSystem& ps, float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        if (Particle* p = ps.emit()) {
            p->x = x + Random::get_float(-4.0f, 4.0f);
            p->y = y + Random::get_float(-4.0f, 4.0f);
            p->render_x = p->x;
            p->render_y = p->y;
            p->vx = Random::get_float(-110.0f, 110.0f);
            p->vy = Random::get_float(-110.0f, 110.0f);
            p->life = Random::get_float(0.3f, 0.5f);
            p->max_life = p->life;
            p->color = 0xFFFFD700; // Gold / amber hit spark
            p->size = 2;
            p->type = ParticleType::Spark;
        }
    }
}

void spawn_spire_embers(ParticleSystem& ps, float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        if (Random::chance(0.35f)) {
            if (Particle* p = ps.emit()) {
                float spawn_x = x + Random::get_float(-6.0f, 6.0f);
                float spawn_y = y + Random::get_float(-6.0f, 6.0f);

                p->x = spawn_x;
                p->y = spawn_y;
                p->render_x = spawn_x;
                p->render_y = spawn_y;
                p->vx = Random::get_float(-10.0f, 10.0f);
                p->vy = Random::get_float(-18.0f, -6.0f);
                p->life = Random::get_float(0.8f, 1.4f);
                p->max_life = p->life;

                uint8_t red = 0xFF;
                uint8_t green = 0xFF;
                uint8_t blue = static_cast<uint8_t>(Random::get_int(100, 255)); // White / Yellow / Yellowish-White
                p->color = 0xFF000000 | (red << 16) | (green << 8) | blue;

                p->size = 3;
                p->type = ParticleType::LightEmber;
            }
        }
    }
}

void spawn_refiner_embers(ParticleSystem& ps, float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        if (Random::chance(0.35f)) {
            if (Particle* p = ps.emit()) {
                float spawn_x = x + Random::get_float(-6.0f, 6.0f);
                float spawn_y = y + Random::get_float(-6.0f, 6.0f);

                p->x = spawn_x;
                p->y = spawn_y;
                p->render_x = spawn_x;
                p->render_y = spawn_y;
                p->vx = Random::get_float(-10.0f, 10.0f);
                p->vy = Random::get_float(-18.0f, -6.0f);
                p->life = Random::get_float(0.8f, 1.4f);
                p->max_life = p->life;

                int color_variant = Random::get_int(0, 2);
                uint32_t c = 0xFF550088;
                if (color_variant == 0) {
                    // Vibrant Violet
                    uint8_t r = static_cast<uint8_t>(Random::get_int(130, 170));
                    uint8_t g = static_cast<uint8_t>(Random::get_int(10, 40));
                    uint8_t b = static_cast<uint8_t>(Random::get_int(220, 255));
                    c = 0xFF000000 | (r << 16) | (g << 8) | b;
                } else if (color_variant == 1) {
                    // Dark Purple
                    uint8_t r = static_cast<uint8_t>(Random::get_int(60, 100));
                    uint8_t g = static_cast<uint8_t>(Random::get_int(0, 20));
                    uint8_t b = static_cast<uint8_t>(Random::get_int(120, 160));
                    c = 0xFF000000 | (r << 16) | (g << 8) | b;
                } else {
                    // Very Dark Grey
                    uint8_t v = static_cast<uint8_t>(Random::get_int(30, 50));
                    c = 0xFF000000 | (v << 16) | (v << 8) | (v + 5);
                }
                p->color = c;

                p->size = 3;
                p->type = ParticleType::LightEmber;
            }
        }
    }
}

void spawn_alloy_pickup(ParticleSystem& ps, float x, float y, int count) {
    for (int i = 0; i < count; ++i) {
        if (Particle* p = ps.emit()) {
            p->x = x + Random::get_float(-3.0f, 3.0f);
            p->y = y + Random::get_float(-3.0f, 3.0f);
            p->render_x = p->x;
            p->render_y = p->y;
            p->vx = Random::get_float(-70.0f, 70.0f);
            p->vy = Random::get_float(-70.0f, 70.0f);
            p->life = Random::get_float(0.3f, 0.5f);
            p->max_life = p->life;

            bool is_silver = Random::chance(0.5f);
            if (is_silver) {
                uint8_t s = static_cast<uint8_t>(Random::get_int(200, 245));
                p->color = 0xFF000000 | (s << 16) | (s << 8) | s; // Metallic Silver
            } else {
                p->color = 0xFF00E5FF; // Bright Cyan
            }
            p->size = Random::chance(0.5f) ? 1 : 2;
            p->type = ParticleType::Spark;
        }
    }
}

} // namespace ParticleEmitters
} // namespace alx
