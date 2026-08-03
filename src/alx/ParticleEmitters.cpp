#include "alx/ParticleEmitters.h"
#include "alx/ParticleSystem.h"
#include "alx/Random.h"
#include <cmath>

namespace alx {
namespace ParticleEmitters {

void spawn_sword_slash_trail(ParticleSystem& ps, float prev_tip_x, float prev_tip_y, float curr_tip_x, float curr_tip_y, float swing_progress, int count, int z_index, int y_sort_override) {
    if (count <= 0) return;

    // Quadratic Ease-In: keeps first 50% paper-thin (1-2px) and swells to 5px at the apex
    float curved_progress = swing_progress * swing_progress;
    int base_size = 1 + static_cast<int>(std::round(curved_progress * 4.0f));
    base_size = std::clamp(base_size, 1, 5);

    for (int i = 0; i < count; ++i) {
        float t = (count > 1) ? (static_cast<float>(i) / static_cast<float>(count - 1)) : 0.5f;
        float line_x = prev_tip_x + (curr_tip_x - prev_tip_x) * t;
        float line_y = prev_tip_y + (curr_tip_y - prev_tip_y) * t;

        // Primary slice particle along line segment
        if (Particle* p = ps.emit()) {
            p->x = line_x + Random::get_float(-0.5f, 0.5f);
            p->y = line_y + Random::get_float(-0.5f, 0.5f);
            p->render_x = p->x;
            p->render_y = p->y;
            p->vx = Random::get_float(-2.0f, 2.0f);
            p->vy = Random::get_float(-2.0f, 2.0f);
            p->life = Random::get_float(0.12f, 0.18f); // Smooth 7-11 frame lingering line fade
            p->max_life = p->life;

            uint8_t shade = static_cast<uint8_t>(Random::get_int(220, 255));
            p->color = 0xFF000000 | (shade << 16) | (shade << 8) | shade; // Crisp white / light grey
            p->size = static_cast<uint8_t>(base_size);
            p->type = ParticleType::Spark;
            p->z_index = z_index;
            p->y_sort_override = y_sort_override;
        }

        // Secondary inner core layer near apex for rich depth
        if (swing_progress > 0.4f && (i % 2 == 0)) {
            if (Particle* p2 = ps.emit()) {
                p2->x = line_x + Random::get_float(-0.75f, 0.75f);
                p2->y = line_y + Random::get_float(-0.75f, 0.75f);
                p2->render_x = p2->x;
                p2->render_y = p2->y;
                p2->vx = Random::get_float(-1.5f, 1.5f);
                p2->vy = Random::get_float(-1.5f, 1.5f);
                p2->life = Random::get_float(0.10f, 0.15f);
                p2->max_life = p2->life;

                p2->color = 0xFFFFFFFF; // Pure white core
                p2->size = static_cast<uint8_t>(std::max(1, base_size - 1));
                p2->type = ParticleType::Spark;
                p2->z_index = z_index;
                p2->y_sort_override = y_sort_override;
            }
        }
    }
}

void spawn_hit_sparks(ParticleSystem& ps, float x, float y, int count, int z_index, int y_sort_override) {
    for (int i = 0; i < count; ++i) {
        if (Particle* p = ps.emit()) {
            // Tight 1px emit radius at exact contact point
            p->x = x + Random::get_float(-1.5f, 1.5f);
            p->y = y + Random::get_float(-1.5f, 1.5f);
            p->render_x = p->x;
            p->render_y = p->y;
            p->vx = Random::get_float(-25.0f, 25.0f);
            p->vy = Random::get_float(-25.0f, 25.0f);
            p->life = Random::get_float(0.35f, 0.75f);
            p->max_life = p->life;

            // Range of Dark Orange to Dark Red
            uint8_t r = static_cast<uint8_t>(Random::get_int(135, 155)); // Warm red base
            uint8_t g = static_cast<uint8_t>(Random::get_int(10, 55));   // Low = Dark Red, High = Dark Orange
            uint8_t b = static_cast<uint8_t>(Random::get_int(0, 10));    // Keep blue near zero
            uint32_t c = 0xFF000000 | (r << 16) | (g << 8) | b;
            p->color = c;

            p->size = 2;
            p->type = ParticleType::Spark;
            p->z_index = z_index;
            p->y_sort_override = y_sort_override;
        }
    }
}

void spawn_spire_embers(ParticleSystem& ps, float x, float y, int count, int z_index, int y_sort_override) {
    for (int i = 0; i < count; ++i) {
        if (Random::chance(0.35f)) {
            if (Particle* p = ps.emit()) {
                float spawn_x = x + Random::get_float(-4.0f, 4.0f);
                float spawn_y = y + Random::get_float(-4.0f, 4.0f);

                p->x = spawn_x;
                p->y = spawn_y;
                p->render_x = spawn_x;
                p->render_y = spawn_y;
                p->vx = Random::get_float(-8.0f, 8.0f);
                p->vy = Random::get_float(-18.0f, -4.0f);
                p->life = Random::get_float(0.75f, 1.0f);
                p->max_life = p->life;

                uint8_t red = 0xFF;
                uint8_t green = 0xFF;
                uint8_t blue = static_cast<uint8_t>(Random::get_int(100, 255)); // White / Yellow / Yellowish-White
                p->color = 0xFF000000 | (red << 16) | (green << 8) | blue;

                p->size = 3;
                p->type = ParticleType::LightEmber;
                p->z_index = z_index;
                p->y_sort_override = y_sort_override;
            }
        }
    }
}

void spawn_refiner_embers(ParticleSystem& ps, float x, float y, int count, int z_index, int y_sort_override) {
    for (int i = 0; i < count; ++i) {
        if (Random::chance(0.35f)) {
            if (Particle* p = ps.emit()) {
                float spawn_x = x + Random::get_float(-4.0f, 4.0f);
                float spawn_y = y + Random::get_float(-4.0f, 4.0f);

                p->x = spawn_x;
                p->y = spawn_y;
                p->render_x = spawn_x;
                p->render_y = spawn_y;
                p->vx = Random::get_float(-8.0f, 8.0f);
                p->vy = Random::get_float(-18.0f, -4.0f);
                p->life = Random::get_float(0.75f, 1.0f);
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
                p->z_index = z_index;
                p->y_sort_override = y_sort_override;
            }
        }
    }
}

void spawn_alloy_pickup(ParticleSystem& ps, float x, float y, int count, int z_index, int y_sort_override) {
    for (int i = 0; i < count; ++i) {
        if (Particle* p = ps.emit()) {
            p->x = x + Random::get_float(-3.0f, 3.0f);
            p->y = y + Random::get_float(-3.0f, 3.0f);
            p->render_x = p->x;
            p->render_y = p->y;
            p->vx = Random::get_float(-75.0f, 75.0f);
            p->vy = Random::get_float(-75.0f, 75.0f);
            p->life = Random::get_float(0.5f, 0.75f);
            p->max_life = p->life;

            bool is_silver = Random::chance(0.5f);
            if (is_silver) {
                uint8_t s = static_cast<uint8_t>(Random::get_int(150, 200));
                p->color = 0xFF000000 | (s << 16) | (s << 8) | s; // Metallic Silver
            } else {
                p->color = 0xFF00E5FF; // Bright Cyan
            }

            p->size = Random::chance(0.5f) ? 1 : 2;
            p->type = ParticleType::Spark;
            p->z_index = z_index;
            p->y_sort_override = y_sort_override;
        }
    }
}

void spawn_straight_pipe_mana(ParticleSystem& ps, int tile_x, int tile_y, int dir_x, int dir_y, float sim_tick_rate, int count, int tile_size) {
    if (dir_x == 0 && dir_y == 0) return;

    for (int i = 0; i < count; ++i) {
        if (Particle* p = ps.emit()) {
            float center_x = static_cast<float>(tile_x * tile_size + tile_size / 2);
            float center_y = static_cast<float>(tile_y * tile_size + tile_size / 2);
            float half_tile = static_cast<float>(tile_size) * 0.5f;

            float dir_len = std::sqrt(static_cast<float>(dir_x * dir_x + dir_y * dir_y));
            float dx = static_cast<float>(dir_x) / dir_len;
            float dy = static_cast<float>(dir_y) / dir_len;

            // Entry (P0) and Exit (P1) points along pipe axis
            float start_x = center_x - dx * half_tile;
            float start_y = center_y - dy * half_tile;
            float target_x = center_x + dx * half_tile;
            float target_y = center_y + dy * half_tile;

            // Normal perpendicular vector N = (-dy, dx)
            float nx = -dy;
            float ny = dx;

            // Random lateral offset across full 6px stream width (+/- 2.0px)
            float lateral_offset = Random::get_float(-2.0f, 2.0f);

            p->start_x = start_x + nx * lateral_offset;
            p->start_y = start_y + ny * lateral_offset;
            p->target_x = target_x + nx * lateral_offset;
            p->target_y = target_y + ny * lateral_offset;

            p->nx = -dy; // (already have dx, dy as unit dir from lines ~193-194)
            p->ny = dx;

            p->x = p->start_x;
            p->y = p->start_y;
            p->render_x = p->x;
            p->render_y = p->y;

            // Dynamically scale TTL based on sim_tick_rate so particles seamlessly cross tile boundaries
            float base_rate = (sim_tick_rate > 0.01f) ? sim_tick_rate : 0.6f;
            p->life = base_rate * Random::get_float(1.15f, 1.40f);
            p->max_life = p->life;
            p->param_a = Random::get_float(0.0f, 6.28318f); // Initial wave phase

            p->tile_x = static_cast<uint16_t>(tile_x);
            p->tile_y = static_cast<uint16_t>(tile_y);

            // Dominant Liquid Twilight Purple Palette
            float color_roll = Random::get_float(0.0f, 1.0f);
            if (color_roll < 0.85f) {
                uint8_t r = static_cast<uint8_t>(Random::get_int(140, 150));
                uint8_t g = static_cast<uint8_t>(Random::get_int(0, 10));
                uint8_t b = static_cast<uint8_t>(Random::get_int(230, 250));
                p->color = 0xFF000000 | (r << 16) | (g << 8) | b;
            } else if (color_roll < 0.95f) {
                p->color = 0xFF6600BB;
            } else {
                p->color = 0xFFDD44FF;
            }

            p->size = 1;
            p->type = ParticleType::ManaPulseStraight;
        }
    }
}

void spawn_corner_pipe_mana(ParticleSystem& ps, int tile_x, int tile_y, int in_dx, int in_dy, int out_dx, int out_dy, float sim_tick_rate, int count, int tile_size) {
    if (in_dx == 0 && in_dy == 0 && out_dx == 0 && out_dy == 0) return;

    for (int i = 0; i < count; ++i) {
        if (Particle* p = ps.emit()) {
            float hub_cx = static_cast<float>(tile_x * tile_size + tile_size / 2);
            float hub_cy = static_cast<float>(tile_y * tile_size + tile_size / 2);
            float half_tile = static_cast<float>(tile_size) * 0.5f;

            float start_x = hub_cx - static_cast<float>(in_dx) * half_tile;
            float start_y = hub_cy - static_cast<float>(in_dy) * half_tile;
            float target_x = hub_cx + static_cast<float>(out_dx) * half_tile;
            float target_y = hub_cy + static_cast<float>(out_dy) * half_tile;

            // Normal vector perpendicular to incoming direction
            float dx = static_cast<float>(in_dx);
            float dy = static_cast<float>(in_dy);
            float nx = -dy;
            float ny = dx;

            float lateral_offset = Random::get_float(-2.0f, 2.0f);

            p->start_x = start_x + nx * lateral_offset;
            p->start_y = start_y + ny * lateral_offset;
            p->control_x = hub_cx;
            p->control_y = hub_cy;
            p->target_x = target_x + nx * lateral_offset;
            p->target_y = target_y + ny * lateral_offset;

            float chord_dx = p->target_x - p->start_x;
            float chord_dy = p->target_y - p->start_y;
            float chord_len = std::sqrt(chord_dx * chord_dx + chord_dy * chord_dy);
            if (chord_len > 0.001f) {
                p->nx = -chord_dy / chord_len;
                p->ny = chord_dx / chord_len;
            }

            p->x = p->start_x;
            p->y = p->start_y;
            p->render_x = p->x;
            p->render_y = p->y;

            float base_rate = (sim_tick_rate > 0.01f) ? sim_tick_rate : 0.6f;
            p->life = base_rate * Random::get_float(1.15f, 1.40f);
            p->max_life = p->life;
            p->param_a = Random::get_float(0.0f, 6.28318f);

            p->tile_x = static_cast<uint16_t>(tile_x);
            p->tile_y = static_cast<uint16_t>(tile_y);

            float color_roll = Random::get_float(0.0f, 1.0f);
            if (color_roll < 0.85f) {
                uint8_t r = static_cast<uint8_t>(Random::get_int(140, 150));
                uint8_t g = static_cast<uint8_t>(Random::get_int(0, 10));
                uint8_t b = static_cast<uint8_t>(Random::get_int(230, 250));
                p->color = 0xFF000000 | (r << 16) | (g << 8) | b;
            } else if (color_roll < 0.95f) {
                p->color = 0xFF6600BB;
            } else {
                p->color = 0xFFDD44FF;
            }

            p->size = 1;
            p->type = ParticleType::ManaPulseCurved;
        }
    }
}

} // namespace ParticleEmitters
} // namespace alx
