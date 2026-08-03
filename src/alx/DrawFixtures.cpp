#include "DrawFixtures.h"
#include "ParticleSystem.h"
#include "ParticleEmitters.h"
#include "Random.h"
#include "Layer.h"
#include "Game.h"
#include "core/Draw.h"
#include "Player.h"
#include <cmath>
#include <algorithm>

namespace alx {
namespace DrawFixtures {

namespace {

// --- HELPERS ---

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

bool is_player_behind(int world_x, int world_y, const Player* player) {
    if (!player) return false;
    float px = player->transform.x;
    float py = player->transform.y;
    float pw = player->transform.width;
    float ph = player->transform.height;
    float p_bottom_y = py + ph;

    bool overlap_x = (px + pw > world_x) && (px < world_x + 16);
    bool overlap_y = (py + ph > world_y - 8) && (py < world_y + 16);

    return overlap_x && overlap_y && p_bottom_y <= world_y + 16;
}

// Derive primary output direction from flow_out_mask, preferring straight-through
void primary_out_from_mask(uint8_t mask, int in_dx, int in_dy, int& out_dx, int& out_dy) {
    out_dx = 0; out_dy = 0;
    // Prefer straight-through (same direction as incoming flow)
    uint8_t straight = DirectionMask::from_delta(in_dx, in_dy);
    if (mask & straight) { out_dx = in_dx; out_dy = in_dy; return; }
    // Fall back to first set bit
    DirectionMask::to_delta(mask, out_dx, out_dy);
}

// --- DRAW BACKGROUNDS ---

void draw_pipe_bg(const Network& network, int gx, int gy, int world_x, int world_y, int tile_size) {
    uint32_t pipe_color = 0xFF4A4A60;

    // NOTE: sub_len might need to be tweaked if fixtures are taller than 1.5 tiles in future
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
}

void draw_building_bg(FixtureType type, int world_x, int world_y, const Player* player = nullptr) {
    bool fade = is_player_behind(world_x, world_y, player);
    uint32_t top_color, front_color, right_color, outline_color;
    uint32_t window_bg_color, window_border_color;
    uint32_t alpha = fade ? 0x66000000 : 0xFF000000;

    if (type == FixtureType::Refiner) {
        top_color = alpha | 0x004A2C8A;
        front_color = alpha | 0x00241454;
        right_color = alpha | 0x00301C66;
        outline_color = alpha | 0x0010052C;
        window_bg_color = alpha | 0x00120A2A;
        window_border_color = alpha | 0x00090415;
    } else {
        top_color = alpha | 0x0000FF88;
        front_color = alpha | 0x0000A340;
        right_color = alpha | 0x0000D15C;
        outline_color = alpha | 0x00003D18;
        window_bg_color = alpha | 0x00005220;
        window_border_color = alpha | 0x00002810;
    }

    int z_idx = Layer::WorldObj;
    int world_bottom_y = world_y - 4;

    // 1. Draw Front Face (12x20 at x, y-4)
    Draw::rect(world_x, world_bottom_y, 12, 20, front_color, true, 1, z_idx);

    // 2. Draw Right Face (slanted vertical columns from x+12 to x+16)
    for (int dx = 0; dx < 4; ++dx) {
        Draw::rect(world_x + 12 + dx, world_bottom_y - dx, 1, 20, right_color, true, 1, z_idx);
    }

    // 3. Draw Top Face (slanted horizontal rows from y-4 to y-8)
    for (int dy = 0; dy <= 4; ++dy) {
        Draw::rect(world_x + dy, world_bottom_y - dy, 12, 1, top_color, true, 1, z_idx);
    }

    // 5. Draw Frameless Front Window Cutout Interior Background (10x10 at x+1, y+4) (y_sort_override needed)
    int y_sort_override = world_bottom_y + 20;
    Draw::rect(world_x + 1, world_y + 4, 10, 10, window_bg_color, true, 1, z_idx, y_sort_override);
}

void draw_fixture_bg(FixtureType type, int world_x, int world_y, int tile_size) {
    uint32_t color = 0xFF00FF66;
    int z_idx = Layer::GroundFixture;

    if (type == FixtureType::Seep) {
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

void draw_background(const Network& network, FixtureType type, int gx, int gy, int world_x, int world_y, int tile_size, const Player* player = nullptr) {
    if (type == FixtureType::Pipe) {
        draw_pipe_bg(network, gx, gy, world_x, world_y, tile_size);
    } else if (type == FixtureType::Refiner || type == FixtureType::Spire) {
        draw_building_bg(type, world_x, world_y, player);
    } else {
        draw_fixture_bg(type, world_x, world_y, tile_size);
    }
}

// --- DRAW BUILDING MANA ---

void draw_node_dark_mana(int world_x, int world_y, int tile_size, float progress, uint8_t timer, bool fade) {
    uint32_t liquid_color = fade ? 0x669900FF : 0xFF9900FF; // Glowing twilight violet liquid
    int z_idx = Layer::WorldObj + 2;
    // Fills lower 4px of the 10x10 opening (y from y+10 to y+14, width 10)
    Draw::rect(world_x + 1, world_y + 10, 10, 4, liquid_color, true, 1, z_idx);
}

void draw_node_light_mana(int world_x, int world_y, int tile_size, float progress, uint8_t timer, bool fade) {
    uint32_t alpha = fade ? 0x66000000 : 0xFF000000;
    uint32_t aura_color = alpha | 0x0000FFFF;  // Cyan aura
    uint32_t core_color = alpha | 0x00FFFFFF;  // White core

    int z_idx_aura = Layer::WorldObj + 2;
    int z_idx_core = Layer::WorldObj + 3;

    // Fills lower 4px of the 10x10 opening (y from y+10 to y+14, width 10)
    Draw::rect(world_x + 4, world_y + 10, 7, 4, aura_color, true, 1, z_idx_aura);
    Draw::rect(world_x + 6, world_y + 11, 5, 2, core_color, true, 1, z_idx_core);
}

// --- PIPE - LIGHT MANA ---

void draw_pipe_light_mana(const Network& network, const Fixture& fix, int gx, int gy, int world_x, int world_y, int tile_size, float progress) {
    int src_gx = gx - fix.move_dx;
    int src_gy = gy - fix.move_dy;

    int travel_dist = tile_size;
    if (is_node_fixture(network, src_gx, src_gy)) {
        travel_dist = tile_size / 2;
    }

    int anim_offset_x = static_cast<int>(-fix.move_dx * (1.0f - progress) * travel_dist);
    int anim_offset_y = static_cast<int>(-fix.move_dy * (1.0f - progress) * travel_dist);
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

// --- DRAW PIPE - DARK MANA ---

void draw_pipe_dark_mana_straight(int world_x, int world_y, int tile_size, int flow_dx, int flow_dy, float progress, bool is_head_tile, bool is_tail_tile, int stream_w) {
    if (flow_dx == 0 && flow_dy == 0) return;

    uint32_t stream_color = 0xFF4A0088; // Deep Twilight Purple Base
    int hub_cx = world_x + tile_size / 2;
    int hub_cy = world_y + tile_size / 2;
    int half_tile = tile_size / 2;
    int offset = (tile_size - stream_w) / 2;
    float cap_r = static_cast<float>(stream_w) * 0.5f;

    float start_x = static_cast<float>(hub_cx - flow_dx * half_tile);
    float start_y = static_cast<float>(hub_cy - flow_dy * half_tile);
    float target_x = static_cast<float>(hub_cx + flow_dx * half_tile);
    float target_y = static_cast<float>(hub_cy + flow_dy * half_tile);

    float t = std::clamp(progress, 0.0f, 1.0f);

    if (!is_head_tile && !is_tail_tile) {
        // --- FULLY FILLED MIDDLE TILE (Solid 16px Edge-to-Edge Stream) ---
        if (flow_dx != 0) {
            Draw::rect(static_cast<float>(world_x), static_cast<float>(world_y + offset), static_cast<float>(tile_size), static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
        } else {
            Draw::rect(static_cast<float>(world_x + offset), static_cast<float>(world_y), static_cast<float>(stream_w), static_cast<float>(tile_size), stream_color, true, 1, Layer::GroundFixtureItem);
        }
    } else if (is_head_tile && is_tail_tile) {
        // --- SINGLE-TILE MOVING SLUG (Advancing Head + Receding Tail) ---
        float tail_dist = t * static_cast<float>(tile_size);
        float tail_x = start_x + static_cast<float>(flow_dx) * tail_dist;
        float tail_y = start_y + static_cast<float>(flow_dy) * tail_dist;
        float head_x = target_x;
        float head_y = target_y;

        if (flow_dx != 0) {
            float rx = (flow_dx > 0) ? tail_x : head_x;
            float rw = std::abs(head_x - tail_x);
            Draw::rect(rx, static_cast<float>(world_y + offset), rw, static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
        } else {
            float ry = (flow_dy > 0) ? tail_y : head_y;
            float rh = std::abs(head_y - tail_y);
            Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), rh, stream_color, true, 1, Layer::GroundFixtureItem);
        }

        Draw::circle(tail_x, tail_y, cap_r, stream_color, true, 1, Layer::GroundFixtureItem);
        Draw::circle(head_x, head_y, cap_r, stream_color, true, 1, Layer::GroundFixtureItem);
    } else if (is_head_tile) {
        // --- ADVANCING LEADING HEAD TILE ---
        float head_dist = t * static_cast<float>(tile_size);
        float head_x = start_x + static_cast<float>(flow_dx) * head_dist;
        float head_y = start_y + static_cast<float>(flow_dy) * head_dist;

        if (flow_dx != 0) {
            float rx = (flow_dx > 0) ? start_x : head_x;
            float rw = std::abs(head_x - start_x);
            Draw::rect(rx, static_cast<float>(world_y + offset), rw, static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
        } else {
            float ry = (flow_dy > 0) ? start_y : head_y;
            float rh = std::abs(head_y - start_y);
            Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), rh, stream_color, true, 1, Layer::GroundFixtureItem);
        }

        if (t < 0.98f) {
            Draw::circle(head_x, head_y, cap_r, stream_color, true, 1, Layer::GroundFixtureItem);
        }
    } else if (is_tail_tile) {
        // --- RECEDING DRAINING TAIL TILE ---
        float tail_dist = t * static_cast<float>(tile_size);
        float tail_x = start_x + static_cast<float>(flow_dx) * tail_dist;
        float tail_y = start_y + static_cast<float>(flow_dy) * tail_dist;

        if (flow_dx != 0) {
            float rx = (flow_dx > 0) ? tail_x : target_x;
            float rw = std::abs(target_x - tail_x);
            Draw::rect(rx, static_cast<float>(world_y + offset), rw, static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
        } else {
            float ry = (flow_dy > 0) ? tail_y : target_y;
            float rh = std::abs(target_y - tail_y);
            Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), rh, stream_color, true, 1, Layer::GroundFixtureItem);
        }

        if (t < 0.98f) {
            Draw::circle(tail_x, tail_y, cap_r, stream_color, true, 1, Layer::GroundFixtureItem);
        }
    }
}

void draw_pipe_dark_mana_corner(int world_x, int world_y, int tile_size, int in_dx, int in_dy, int out_dx, int out_dy, float progress, bool is_head_tile, bool is_tail_tile, int stream_w) {
    uint32_t stream_color = 0xFF4A0088;
    int hub_cx = world_x + tile_size / 2;
    int hub_cy = world_y + tile_size / 2;
    int half_tile = tile_size / 2;
    int offset = (tile_size - stream_w) / 2;
    float cap_r = static_cast<float>(stream_w) * 0.5f;

    float t = std::clamp(progress, 0.0f, 1.0f);

    if (!is_head_tile && !is_tail_tile) {
        // --- FULLY FILLED CORNER TILE (Seamless 90-degree Hub Connection) ---
        float entry_start_x = static_cast<float>(hub_cx - in_dx * half_tile);
        float entry_start_y = static_cast<float>(hub_cy - in_dy * half_tile);

        if (in_dx != 0) {
            float rx = (in_dx > 0) ? entry_start_x : static_cast<float>(hub_cx);
            Draw::rect(rx, static_cast<float>(world_y + offset), static_cast<float>(half_tile), static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
        } else if (in_dy != 0) {
            float ry = (in_dy > 0) ? entry_start_y : static_cast<float>(hub_cy);
            Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), static_cast<float>(half_tile), stream_color, true, 1, Layer::GroundFixtureItem);
        }

        if (out_dx != 0) {
            float rx = (out_dx > 0) ? static_cast<float>(hub_cx) : static_cast<float>(hub_cx + out_dx * half_tile);
            Draw::rect(rx, static_cast<float>(world_y + offset), static_cast<float>(half_tile), static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
        } else if (out_dy != 0) {
            float ry = (out_dy > 0) ? static_cast<float>(hub_cy) : static_cast<float>(hub_cy + out_dy * half_tile);
            Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), static_cast<float>(half_tile), stream_color, true, 1, Layer::GroundFixtureItem);
        }

        Draw::rect(static_cast<float>(hub_cx - stream_w / 2), static_cast<float>(hub_cy - stream_w / 2), static_cast<float>(stream_w), static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
    } else if (is_head_tile) {
        // --- ADVANCING LEADING HEAD CORNER ---
        float entry_t = std::clamp(t / 0.5f, 0.0f, 1.0f);
        float entry_len = entry_t * static_cast<float>(half_tile);

        float entry_start_x = static_cast<float>(hub_cx - in_dx * half_tile);
        float entry_start_y = static_cast<float>(hub_cy - in_dy * half_tile);
        float entry_head_x = entry_start_x + static_cast<float>(in_dx) * entry_len;
        float entry_head_y = entry_start_y + static_cast<float>(in_dy) * entry_len;

        if (in_dx != 0) {
            float rx = (in_dx > 0) ? entry_start_x : entry_head_x;
            Draw::rect(rx, static_cast<float>(world_y + offset), entry_len, static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
        } else if (in_dy != 0) {
            float ry = (in_dy > 0) ? entry_start_y : entry_head_y;
            Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), entry_len, stream_color, true, 1, Layer::GroundFixtureItem);
        }

        if (t <= 0.5f) {
            Draw::circle(entry_head_x, entry_head_y, cap_r, stream_color, true, 1, Layer::GroundFixtureItem);
        } else {
            float exit_t = std::clamp((t - 0.5f) / 0.5f, 0.0f, 1.0f);
            float exit_len = exit_t * static_cast<float>(half_tile);

            float exit_head_x = static_cast<float>(hub_cx) + static_cast<float>(out_dx) * exit_len;
            float exit_head_y = static_cast<float>(hub_cy) + static_cast<float>(out_dy) * exit_len;

            if (out_dx != 0) {
                float rx = (out_dx > 0) ? static_cast<float>(hub_cx) : exit_head_x;
                Draw::rect(rx, static_cast<float>(world_y + offset), exit_len, static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
            } else if (out_dy != 0) {
                float ry = (out_dy > 0) ? static_cast<float>(hub_cy) : exit_head_y;
                Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), exit_len, stream_color, true, 1, Layer::GroundFixtureItem);
            }

            if (t < 0.98f) {
                Draw::circle(exit_head_x, exit_head_y, cap_r, stream_color, true, 1, Layer::GroundFixtureItem);
            }
        }

        if (t >= 0.45f) {
            Draw::rect(static_cast<float>(hub_cx - stream_w / 2), static_cast<float>(hub_cy - stream_w / 2), static_cast<float>(stream_w), static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
        }
    } else if (is_tail_tile) {
        // --- RECEDING DRAINING TAIL CORNER ---
        if (t <= 0.5f) {
            float entry_t = std::clamp(t / 0.5f, 0.0f, 1.0f);
            float tail_len = entry_t * static_cast<float>(half_tile);
            float entry_start_x = static_cast<float>(hub_cx - in_dx * half_tile);
            float entry_start_y = static_cast<float>(hub_cy - in_dy * half_tile);
            float tail_x = entry_start_x + static_cast<float>(in_dx) * tail_len;
            float tail_y = entry_start_y + static_cast<float>(in_dy) * tail_len;
            float rem_len = static_cast<float>(half_tile) - tail_len;

            if (in_dx != 0) {
                float rx = (in_dx > 0) ? tail_x : static_cast<float>(hub_cx);
                Draw::rect(rx, static_cast<float>(world_y + offset), rem_len, static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
            } else if (in_dy != 0) {
                float ry = (in_dy > 0) ? tail_y : static_cast<float>(hub_cy);
                Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), rem_len, stream_color, true, 1, Layer::GroundFixtureItem);
            }
            Draw::circle(tail_x, tail_y, cap_r, stream_color, true, 1, Layer::GroundFixtureItem);

            if (out_dx != 0) {
                float rx = (out_dx > 0) ? static_cast<float>(hub_cx) : static_cast<float>(hub_cx + out_dx * half_tile);
                Draw::rect(rx, static_cast<float>(world_y + offset), static_cast<float>(half_tile), static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
            } else if (out_dy != 0) {
                float ry = (out_dy > 0) ? static_cast<float>(hub_cy) : static_cast<float>(hub_cy + out_dy * half_tile);
                Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), static_cast<float>(half_tile), stream_color, true, 1, Layer::GroundFixtureItem);
            }
        } else {
            float exit_t = std::clamp((t - 0.5f) / 0.5f, 0.0f, 1.0f);
            float tail_len = exit_t * static_cast<float>(half_tile);
            float tail_x = static_cast<float>(hub_cx) + static_cast<float>(out_dx) * tail_len;
            float tail_y = static_cast<float>(hub_cy) + static_cast<float>(out_dy) * tail_len;
            float rem_len = static_cast<float>(half_tile) - tail_len;

            float exit_end_x = static_cast<float>(hub_cx + out_dx * half_tile);
            float exit_end_y = static_cast<float>(hub_cy + out_dy * half_tile);

            if (out_dx != 0) {
                float rx = (out_dx > 0) ? tail_x : exit_end_x;
                Draw::rect(rx, static_cast<float>(world_y + offset), rem_len, static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
            } else if (out_dy != 0) {
                float ry = (out_dy > 0) ? tail_y : exit_end_y;
                Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), rem_len, stream_color, true, 1, Layer::GroundFixtureItem);
            }
            if (t < 0.98f) {
                Draw::circle(tail_x, tail_y, cap_r, stream_color, true, 1, Layer::GroundFixtureItem);
            }
        }

        if (t <= 0.55f) {
            Draw::rect(static_cast<float>(hub_cx - stream_w / 2), static_cast<float>(hub_cy - stream_w / 2), static_cast<float>(stream_w), static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
        }
    }
}

void draw_junction_branch_stubs(const Network& network, int x, int y, int world_x, int world_y, int tile_size, int in_dx, int in_dy, int out_dx, int out_dy, int stream_w) {
    uint32_t stream_color = 0xFF4A0088;
    int half_tile = tile_size / 2;
    int offset = (tile_size - stream_w) / 2;
    int hub_cx = world_x + tile_size / 2;
    int hub_cy = world_y + tile_size / 2;

    int dirs_dx[] = { 0, 0, -1, 1 };
    int dirs_dy[] = { -1, 1, 0, 0 };

    bool drew_stub = false;

    for (int i = 0; i < 4; ++i) {
        // Skip directions already covered by primary in->out path
        if (dirs_dx[i] == -in_dx && dirs_dy[i] == -in_dy) continue;
        if (dirs_dx[i] == out_dx && dirs_dy[i] == out_dy) continue;

        int nx = x + dirs_dx[i];
        int ny = y + dirs_dy[i];
        if (!network.in_bounds(nx, ny)) continue;

        const Fixture& neighbor = network.fixture(nx, ny);
        bool neighbor_has_dark = (neighbor.mana_state == ManaState::Dark) ||
                                 (neighbor.type == FixtureType::Seep);
        if (!neighbor_has_dark) continue;

        // Draw half-tile stub from center to edge in this direction
        if (dirs_dx[i] != 0) {
            float rx = (dirs_dx[i] > 0) ? static_cast<float>(hub_cx) : static_cast<float>(world_x);
            Draw::rect(rx, static_cast<float>(world_y + offset), static_cast<float>(half_tile), static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
        } else {
            float ry = (dirs_dy[i] > 0) ? static_cast<float>(hub_cy) : static_cast<float>(world_y);
            Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), static_cast<float>(half_tile), stream_color, true, 1, Layer::GroundFixtureItem);
        }
        drew_stub = true;
    }

    // Draw center hub fill to seamlessly connect stubs to primary stream
    if (drew_stub) {
        Draw::rect(static_cast<float>(hub_cx - stream_w / 2), static_cast<float>(hub_cy - stream_w / 2),
                   static_cast<float>(stream_w), static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
    }
}

void draw_pipe_dark_mana(const Network& network, const Fixture& fix, int gx, int gy, int world_x, int world_y, int tile_size, float progress, int stream_w) {
    int in_dx = fix.move_dx;
    int in_dy = fix.move_dy;
    int out_dx = 0, out_dy = 0;
    primary_out_from_mask(fix.flow_out_mask, in_dx, in_dy, out_dx, out_dy);

    if (out_dx == 0 && out_dy == 0) {
        out_dx = in_dx;
        out_dy = in_dy;
    }

    int downstream_x = gx + out_dx;
    int downstream_y = gy + out_dy;
    int upstream_x = gx - in_dx;
    int upstream_y = gy - in_dy;

    bool downstream_has_dark = network.in_bounds(downstream_x, downstream_y) &&
        (network.fixture(downstream_x, downstream_y).mana_state == ManaState::Dark || network.fixture(downstream_x, downstream_y).type == FixtureType::Refiner);

    bool upstream_has_dark = network.in_bounds(upstream_x, upstream_y) &&
        (network.fixture(upstream_x, upstream_y).mana_state == ManaState::Dark || network.fixture(upstream_x, upstream_y).type == FixtureType::Seep);

    bool is_head_tile = !downstream_has_dark;
    bool is_tail_tile = !upstream_has_dark && fix.is_draining;

    bool is_corner = (in_dx != out_dx || in_dy != out_dy);
    if (!is_corner) {
        int flow_dx = (in_dx != 0) ? in_dx : out_dx;
        int flow_dy = (in_dy != 0) ? in_dy : out_dy;
        draw_pipe_dark_mana_straight(world_x, world_y, tile_size, flow_dx, flow_dy, progress, is_head_tile, is_tail_tile, stream_w);
    } else {
        draw_pipe_dark_mana_corner(world_x, world_y, tile_size, in_dx, in_dy, out_dx, out_dy, progress, is_head_tile, is_tail_tile, stream_w);
    }

    // Draw junction branch stubs for T and X intersections
    if (!is_head_tile && !is_tail_tile) {
        draw_junction_branch_stubs(network, gx, gy, world_x, world_y, tile_size, in_dx, in_dy, out_dx, out_dy, stream_w);
    }
}

// --- DRAW SEEP - DARK MANA ---

void draw_seep_dark_mana_connector(int world_x, int world_y, int tile_size, int out_dx, int out_dy, int stream_w) {
    if (out_dx == 0 && out_dy == 0) return;

    uint32_t stream_color = 0xFF4A0088;
    int offset = (tile_size - stream_w) / 2;
    int hub_cx = world_x + tile_size / 2;
    int hub_cy = world_y + tile_size / 2;
    float cap_r = static_cast<float>(stream_w) * 0.5f;

    Draw::circle(static_cast<float>(hub_cx), static_cast<float>(hub_cy), cap_r, stream_color, true, 1, Layer::GroundFixtureItem);

    if (out_dx != 0) {
        float rx = (out_dx > 0) ? static_cast<float>(hub_cx) : static_cast<float>(world_x);
        Draw::rect(rx, static_cast<float>(world_y + offset), static_cast<float>(tile_size / 2), static_cast<float>(stream_w), stream_color, true, 1, Layer::GroundFixtureItem);
    } else if (out_dy != 0) {
        float ry = (out_dy > 0) ? static_cast<float>(hub_cy) : static_cast<float>(world_y);
        Draw::rect(static_cast<float>(world_x + offset), ry, static_cast<float>(stream_w), static_cast<float>(tile_size / 2), stream_color, true, 1, Layer::GroundFixtureItem);
    }
}

void draw_seep_mana(const Fixture& fix, int world_x, int world_y, int tile_size, int stream_w) {
    // Draw connectors to ALL output directions from flow_out_mask
    uint8_t mask = fix.flow_out_mask;
    if (mask == 0) {
        // Fallback: check move_dx/dy
        int dx = fix.move_dx, dy = fix.move_dy;
        if (dx != 0 || dy != 0) {
            draw_seep_dark_mana_connector(world_x, world_y, tile_size, dx, dy, stream_w);
        }
    } else {
        struct Dir { int dx, dy; uint8_t bit; };
        constexpr Dir dirs[] = {
            { 0, -1, DirectionMask::North },
            { 1,  0, DirectionMask::East  },
            { 0,  1, DirectionMask::South },
            {-1,  0, DirectionMask::West  }
        };
        for (const auto& d : dirs) {
            if (mask & d.bit) {
                draw_seep_dark_mana_connector(world_x, world_y, tile_size, d.dx, d.dy, stream_w);
            }
        }
    }
}

} // namespace

void draw_backgrounds(const Network& network, int min_tx, int max_tx, int min_ty, int max_ty, const Player* player) {
    int tile_size = network.tile_size();
    for (int y = min_ty; y <= max_ty; ++y) {
        for (int x = min_tx; x <= max_tx; ++x) {
            const Fixture& fix = network.fixture(x, y);
            if (fix.is_empty()) continue;
            draw_background(network, fix.type, x, y, x * tile_size, y * tile_size, tile_size, player);
        }
    }
}

void draw_mana(const Network& network, int min_tx, int max_tx, int min_ty, int max_ty, float progress, const Player* player) {
    int tile_size = network.tile_size();
    constexpr int STREAM_WIDTH = 6; // 6px stream width (1px grey wall margin on each side inside 8px pipe channel)

    for (int y = min_ty; y <= max_ty; ++y) {
        for (int x = min_tx; x <= max_tx; ++x) {
            const Fixture& fix = network.fixture(x, y);
            if (fix.is_empty() || fix.mana_state == ManaState::None) continue;

            int world_x = x * tile_size;
            int world_y = y * tile_size;

            if (fix.type == FixtureType::Pipe) {
                if (fix.mana_state == ManaState::Light) {
                    draw_pipe_light_mana(network, fix, x, y, world_x, world_y, tile_size, progress);
                } else if (fix.mana_state == ManaState::Dark) {
                    draw_pipe_dark_mana(network, fix, x, y, world_x, world_y, tile_size, progress, STREAM_WIDTH);
                }
            } else if (fix.type == FixtureType::Seep && fix.mana_state == ManaState::Dark) {
                draw_seep_mana(fix, world_x, world_y, tile_size, STREAM_WIDTH);
            } else if (fix.type == FixtureType::Refiner || fix.type == FixtureType::Spire) {
                bool fade = is_player_behind(world_x, world_y, player);

                if (fix.type == FixtureType::Refiner && fix.mana_state == ManaState::Dark) {
                    draw_node_dark_mana(world_x, world_y, tile_size, progress, fix.process_timer, fade);
                } else if (fix.type == FixtureType::Refiner && fix.mana_state == ManaState::Light) {
                    draw_node_light_mana(world_x, world_y, tile_size, progress, fix.process_timer, fade);
                }
            }
        }
    }
}

void emit_particles(ParticleSystem& ps, const Network& network, int min_tx, int max_tx, int min_ty, int max_ty, float dt, float sim_tick_rate) {
    static float emit_timer = 0.0f;
    emit_timer += dt;

    float emit_interval = (sim_tick_rate > 0.01f) ? (sim_tick_rate / 2.5f) : 0.24f;

    bool should_emit_pipe = false;
    if (emit_timer >= emit_interval) {
        should_emit_pipe = true;
        emit_timer -= emit_interval;
        if (emit_timer > emit_interval * 2.0f) {
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
                int out_dx = 0, out_dy = 0;
                primary_out_from_mask(fix.flow_out_mask, in_dx, in_dy, out_dx, out_dy);

                if (out_dx == 0 && out_dy == 0) {
                    out_dx = in_dx;
                    out_dy = in_dy;
                }

                bool is_corner = (in_dx != out_dx || in_dy != out_dy) && (in_dx != 0 || in_dy != 0) && (out_dx != 0 || out_dy != 0);
                if (is_corner) {
                    ParticleEmitters::spawn_corner_pipe_mana(ps, x, y, in_dx, in_dy, out_dx, out_dy, sim_tick_rate, 1, tile_size);
                } else {
                    int flow_dx = (in_dx != 0) ? in_dx : out_dx;
                    int flow_dy = (in_dy != 0) ? in_dy : out_dy;

                    if (flow_dx == 0 && flow_dy == 0) {
                        flow_dx = 1; // Fallback rightwards flow
                    }

                    ParticleEmitters::spawn_straight_pipe_mana(ps, x, y, flow_dx, flow_dy, sim_tick_rate, 1, tile_size);
                }
            }
            else if (fix.type == FixtureType::Refiner && fix.mana_state == ManaState::Dark) {
                // Temporarily commented out for testing visual geometry:
                int world_bottom_y = world_y - 4;
                int y_sort_override = world_bottom_y + 20;
                ParticleEmitters::spawn_refiner_embers(ps, static_cast<float>(world_x + 6), static_cast<float>(world_y + 8), 1, Layer::WorldObj, y_sort_override);
            }
            else if (fix.type == FixtureType::Spire && fix.mana_state == ManaState::Light) {
                // Temporarily commented out for testing visual geometry:
                int world_bottom_y = world_y - 4;
                int y_sort_override = world_bottom_y + 20;
                ParticleEmitters::spawn_spire_embers(ps, static_cast<float>(world_x + 6), static_cast<float>(world_y + 8), 1, Layer::WorldObj, y_sort_override);
            }
        }
    }
}

} // namespace DrawFixtures
} // namespace alx
