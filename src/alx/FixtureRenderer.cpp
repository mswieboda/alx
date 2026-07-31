#include "FixtureRenderer.h"
#include "ParticleSystem.h"
#include "ParticleEmitters.h"
#include "Random.h"
#include "Layer.h"
#include "Game.h"
#include "core/Draw.h"
#include <cmath>
#include <algorithm>

namespace alx {
namespace FixtureRenderer {

namespace {

bool is_connectable_fixture(const Network& network, int gx, int gy) {
    if (!network.in_bounds(gx, gy)) return false;
    const Fixture& fix = network.fixture(gx, gy);
    return !fix.is_empty();
}

bool is_node_fixture(const Network& network, int gx, int gy) {
    if (!network.in_bounds(gx, gy)) return false;
    const Fixture& fix = network.fixture(gx, gy);
    return fix.type == FixtureType::Refiner || fix.type == FixtureType::Spire || fix.type == FixtureType::Seep;
}

void draw_single_fixture_bg(const Network& network, const Fixture& fix, int gx, int gy, int world_x, int world_y, int tile_size) {
    if (fix.type == FixtureType::Pipe) {
        uint32_t pipe_color = 0xFF4A4A60;

        int hub_size = 8;
        int offset = (tile_size - hub_size) / 2;
        int stub_len = offset;

        Draw::rect(world_x + offset, world_y + offset, hub_size, hub_size, pipe_color, true, 1, Layer::GroundFixture);

        if (is_connectable_fixture(network, gx, gy - 1)) {
            Draw::rect(world_x + offset, world_y, hub_size, stub_len, pipe_color, true, 1, Layer::GroundFixture);
        }
        if (is_connectable_fixture(network, gx, gy + 1)) {
            Draw::rect(world_x + offset, world_y + offset + hub_size, hub_size, stub_len, pipe_color, true, 1, Layer::GroundFixture);
        }
        if (is_connectable_fixture(network, gx - 1, gy)) {
            Draw::rect(world_x, world_y + offset, stub_len, hub_size, pipe_color, true, 1, Layer::GroundFixture);
        }
        if (is_connectable_fixture(network, gx + 1, gy)) {
            Draw::rect(world_x + offset + hub_size, world_y + offset, stub_len, hub_size, pipe_color, true, 1, Layer::GroundFixture);
        }
        return;
    }

    uint32_t color = 0xFF00FF66;
    int z_idx = Layer::GroundFixture;
    if (fix.type == FixtureType::Refiner) {
        color = 0xFF301C66;
        z_idx = Layer::WorldObj;
    } else if (fix.type == FixtureType::Spire) {
        color = 0xFF00FF66;
        z_idx = Layer::WorldObj;
    } else if (fix.type == FixtureType::Seep) {
        color = 0xFF00FF66;
        z_idx = Layer::GroundFixture;
    }

    Draw::rect(
        world_x,
        world_y,
        tile_size,
        tile_size,
        color,
        true,
        1,
        z_idx
    );
}

void draw_node_dark_mana(int world_x, int world_y, int tile_size, float progress, uint8_t timer) {
    uint32_t liquid_color = 0xFF9900FF; // Glowing twilight violet liquid

    float pulse = std::sin(progress * 3.14159f);
    int pulse_extra = (timer > 0 && pulse > 0.5f) ? 2 : 0;

    int pool_size = 6 + pulse_extra;
    int offset = (tile_size - pool_size) / 2;
    int z_idx = Layer::WorldObj + 1;

    Draw::rect(world_x + offset, world_y + offset, pool_size, pool_size, liquid_color, true, 1, z_idx);
}

void draw_node_light_mana(int world_x, int world_y, int tile_size, float progress, uint8_t timer) {
    uint32_t aura_color = 0xFF00FFFF;  // Cyan aura
    uint32_t core_color = 0xFFFFFFFF;  // White core

    float pulse = std::sin(progress * 3.14159f);
    int pulse_extra = (timer > 0 && pulse > 0.5f) ? 2 : 0;

    int orb_size = 10 + pulse_extra;
    int offset = (tile_size - orb_size) / 2;

    int z_idx_aura = Layer::WorldObj + 1;
    int z_idx_core = Layer::WorldObj + 2;

    Draw::rect(world_x + offset, world_y + offset, orb_size, orb_size, aura_color, true, 1, z_idx_aura);

    int core_size = std::max(4, orb_size - 4);
    int core_offset = (tile_size - core_size) / 2;
    Draw::rect(world_x + core_offset, world_y + core_offset, core_size, core_size, core_color, true, 1, z_idx_core);
}

void draw_tile_pipe_light_mana(const Fixture& fix, int anim_offset_x, int anim_offset_y, int world_x, int world_y, int tile_size) {
    uint32_t alpha = (fix.mana_ttl * 255) / Game::LIGHT_MANA_TIME_TO_LIFE_TICKS;
    if (alpha > 255) alpha = 255;

    uint32_t aura_color = (alpha << 24) | 0x0000FFFF;
    uint32_t core_color = (alpha << 24) | 0x00FFFFFF;

    int orb_size = 10;
    int offset = (tile_size - orb_size) / 2;

    int orb_x = world_x + offset + anim_offset_x;
    int orb_y = world_y + offset + anim_offset_y;

    Draw::rect(orb_x, orb_y, orb_size, orb_size, aura_color, true, 1, Layer::GroundFixtureItem);
    Draw::rect(orb_x + 2, orb_y + 2, orb_size - 4, orb_size - 4, core_color, true, 1, Layer::GroundFixtureItemFX);
}

void draw_pipe_light_mana(const Network& network, const Fixture& fix, int gx, int gy, int world_x, int world_y, int tile_size, float progress) {
    int src_gx = gx - fix.move_dx;
    int src_gy = gy - fix.move_dy;

    int travel_dist = tile_size;
    if (is_node_fixture(network, src_gx, src_gy)) {
        travel_dist = tile_size / 2;
    }

    int anim_offset_x = static_cast<int>(-fix.move_dx * (1.0f - progress) * travel_dist);
    int anim_offset_y = static_cast<int>(-fix.move_dy * (1.0f - progress) * travel_dist);

    draw_tile_pipe_light_mana(fix, anim_offset_x, anim_offset_y, world_x, world_y, tile_size);
}

} // namespace

void draw_background(const Network& network, int min_tx, int max_tx, int min_ty, int max_ty) {
    int tile_size = network.tile_size();
    for (int y = min_ty; y <= max_ty; ++y) {
        for (int x = min_tx; x <= max_tx; ++x) {
            const Fixture& fix = network.fixture(x, y);
            if (fix.is_empty()) continue;
            draw_single_fixture_bg(network, fix, x, y, x * tile_size, y * tile_size, tile_size);
        }
    }
}

void draw_mana(const Network& network, int min_tx, int max_tx, int min_ty, int max_ty, float progress) {
    int tile_size = network.tile_size();
    for (int y = min_ty; y <= max_ty; ++y) {
        for (int x = min_tx; x <= max_tx; ++x) {
            const Fixture& fix = network.fixture(x, y);
            if (fix.is_empty() || fix.mana_state == ManaState::None) continue;

            int world_x = x * tile_size;
            int world_y = y * tile_size;

            if (fix.type == FixtureType::Pipe) {
                if (fix.mana_state == ManaState::Light) {
                    draw_pipe_light_mana(network, fix, x, y, world_x, world_y, tile_size, progress);
                }
                // Note: Dark mana for pipes is handled via particle system in emit_particles!
                continue;
            }

            if (fix.type == FixtureType::Refiner && fix.mana_state == ManaState::Dark) {
                draw_node_dark_mana(world_x, world_y, tile_size, progress, fix.process_timer);
                continue;
            }

            if (fix.type == FixtureType::Spire && fix.mana_state == ManaState::Light) {
                draw_node_light_mana(world_x, world_y, tile_size, progress, fix.process_timer);
                continue;
            }

            uint32_t color = (fix.mana_state == ManaState::Light) ? 0xFF00FFFF : 0xFF6600FF;
            int size = tile_size / 2;
            int z_idx = (fix.type == FixtureType::Refiner || fix.type == FixtureType::Spire) ? (Layer::WorldObj + 1) : Layer::GroundFixtureItem;
            Draw::rect(
                world_x + size / 2,
                world_y + size / 2,
                size,
                size,
                color,
                true,
                1,
                z_idx
            );
        }
    }
}

void draw_powered_indicators(const Network& network, int min_tx, int max_tx, int min_ty, int max_ty) {
    int tile_size = network.tile_size();
    for (int y = min_ty; y <= max_ty; ++y) {
        for (int x = min_tx; x <= max_tx; ++x) {
            const Fixture& fix = network.fixture(x, y);
            if (!fix.is_powered || (fix.type != FixtureType::Refiner && fix.type != FixtureType::Spire)) {
                continue;
            }
            uint32_t color = 0xFFFFFF00;
            Draw::rect(x * tile_size, y * tile_size, tile_size, tile_size, color, false, 1, Layer::WorldObj);
        }
    }
}

void emit_particles(ParticleSystem& ps, const Network& network, int min_tx, int max_tx, int min_ty, int max_ty, float dt) {
    static float emit_timer = 0.0f;
    emit_timer += dt;

    constexpr float EMIT_INTERVAL = 0.04f; // 25 Hz framerate-independent particle emission rate

    bool should_emit_pipe = false;
    if (emit_timer >= EMIT_INTERVAL) {
        should_emit_pipe = true;
        emit_timer -= EMIT_INTERVAL;
        if (emit_timer > EMIT_INTERVAL * 2.0f) {
            emit_timer = 0.0f; // Clamp accumulator lag spikes
        }
    }

    int tile_size = network.tile_size();
    for (int y = min_ty; y <= max_ty; ++y) {
        for (int x = min_tx; x <= max_tx; ++x) {
            const Fixture& fix = network.fixture(x, y);
            if (fix.is_empty()) continue;

            int world_x = x * tile_size;
            int world_y = y * tile_size;

            if (fix.type == FixtureType::Pipe && fix.mana_state == ManaState::Dark) {
                if (!should_emit_pipe) continue;

                int in_dx = fix.move_dx;
                int in_dy = fix.move_dy;
                int out_dx = fix.out_dx;
                int out_dy = fix.out_dy;

                if (out_dx == 0 && out_dy == 0) {
                    network.downstream_dir(x, y, ManaState::Dark, out_dx, out_dy);
                }
                if (out_dx == 0 && out_dy == 0) {
                    out_dx = in_dx;
                    out_dy = in_dy;
                }

                // If straight segment (or entry segment)
                bool is_corner = (in_dx != out_dx || in_dy != out_dy);
                if (!is_corner) {
                    int flow_dx = (in_dx != 0) ? in_dx : out_dx;
                    int flow_dy = (in_dy != 0) ? in_dy : out_dy;
                    if (flow_dx != 0 || flow_dy != 0) {
                        ParticleEmitters::spawn_straight_pipe_mana(ps, x, y, flow_dx, flow_dy, 1, tile_size);
                    }
                }
            }
            else if (fix.type == FixtureType::Refiner && fix.mana_state == ManaState::Dark) {
                ParticleEmitters::spawn_refiner_embers(ps, static_cast<float>(world_x + tile_size / 2), static_cast<float>(world_y + tile_size / 2));
            }
            else if (fix.type == FixtureType::Spire && fix.mana_state == ManaState::Light) {
                ParticleEmitters::spawn_spire_embers(ps, static_cast<float>(world_x + tile_size / 2), static_cast<float>(world_y + tile_size / 2));
            }
        }
    }
}

} // namespace FixtureRenderer
} // namespace alx
