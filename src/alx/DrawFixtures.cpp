#include <algorithm>
#include <cmath>
#include "core/Draw.h"
#include "Game.h"
#include "Layer.h"
#include "DrawFixtures.h"
#include "ParticleSystem.h"
#include "ParticleEmitters.h"
#include "Random.h"

namespace alx {
namespace DrawFixtures {

namespace {

// 6px stream width (1px grey wall margin on each side inside 8px pipe channel)
constexpr int STREAM_WIDTH = 6;

constexpr uint32_t BUILDING_ALPHA_OPAQUE = 0xFF000000;

static float s_emit_timer = 0.0f;
static bool s_should_emit_pipe = false;

// --- HELPERS ---

bool is_connectable_fixture(const Network& network, int from_gx, int from_gy, int to_gx, int to_gy) {
    return network.is_valid_port_connection(from_gx, from_gy, to_gx, to_gy);
}

bool is_node_fixture(const Network& network, int gx, int gy) {
    if (!network.in_bounds(gx, gy)) return false;
    const Fixture& fix = network.fixture(gx, gy);
    return fix.type == FixtureType::Refiner || fix.type == FixtureType::Spire || fix.type == FixtureType::Seep;
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

// --- DRAW BUILDING ---

void building_bg(FixtureType type, int world_x, int world_y, int world_bottom_y, int y_sort_override, uint32_t alpha) {
    int z_idx = Layer::WorldObj;

    if (type == FixtureType::Refiner) {
        // Refiner (3x3 tiles = 48x48 px)
        uint32_t top_color       = alpha | 0x005C38AA; // Bright Beveled Roof Cap
        uint32_t top_highlight   = alpha | 0x007B4CE3; // Roof Bevel Highlight
        uint32_t body_color      = alpha | 0x00341C66; // Medium Violet Core Body
        uint32_t base_color      = alpha | 0x001F1240; // Dark Foundation Base
        uint32_t window_bg_color = alpha | 0x00120A2A; // Inset Skylight Window BG
        uint32_t flange_color    = alpha | 0x003C247B;

        // Bottom Row Base (y+32 to y+48, 48x16 px)
        Draw::rect(world_x, world_y + 32, 48, 16, base_color, true, 1, z_idx);

        // 4 Perimeter Port Flanges (Refiner 3x3 midpoint ports)
        Draw::rect(world_x - 2, world_y + 20, 4, 8, flange_color, true, 1, z_idx);  // West Port (root_x, root_y+1)
        Draw::rect(world_x + 46, world_y + 20, 4, 8, flange_color, true, 1, z_idx); // East Port (root_x+2, root_y+1)
        Draw::rect(world_x + 20, world_y - 2, 8, 4, flange_color, true, 1, z_idx);  // North Port (root_x+1, root_y)
        Draw::rect(world_x + 20, world_y + 46, 8, 4, flange_color, true, 1, z_idx); // South Port (root_x+1, root_y+2)

        // Middle Row Core (y+16 to y+32, 44x16 px centered at x+2)
        Draw::rect(world_x + 2, world_y + 16, 44, 16, body_color, true, 1, z_idx);

        // Inset Skylight Cutout Window (24x10 px at world_x+12, world_y+19)
        Draw::rect(world_x + 12, world_y + 19, 24, 10, window_bg_color, true, 1, z_idx, y_sort_override);

        // Top Row Roof Cap (y+2 to y+16, 40x14 px centered at x+4)
        Draw::rect(world_x + 4, world_y + 2, 40, 14, top_color, true, 1, z_idx);
        Draw::rect(world_x + 4, world_y + 2, 40, 2, top_highlight, true, 1, z_idx);

        // Roof Vent Vanes (two 6x4 px dark purple vent rects)
        Draw::rect(world_x + 10, world_y + 6, 6, 4, alpha | 0x00241454, true, 1, z_idx);
        Draw::rect(world_x + 32, world_y + 6, 6, 4, alpha | 0x00241454, true, 1, z_idx);
    } else {
        // Twilight Spire (2x3 tiles = 32x48 px)
        uint32_t peak_tip_color  = alpha | 0x0088FFCC; // Glowing Spire Crystal Tip
        uint32_t top_color       = alpha | 0x0000FF88; // Bright Emerald Upper Peak
        uint32_t body_color      = alpha | 0x0000A350; // Purifying Teal Shaft
        uint32_t base_color      = alpha | 0x00004520; // Dark Foundation Base
        uint32_t window_bg_color = alpha | 0x00002810; // Inset Skylight Window BG
        uint32_t flange_color    = alpha | 0x00006B33;

        // Bottom Row Base (y+32 to y+48, 32x16 px)
        Draw::rect(world_x, world_y + 32, 32, 16, base_color, true, 1, z_idx);

        // Single Bottom Port Flange on South face (root_x+1, root_y+2 at x+20, y+46)
        Draw::rect(world_x + 20, world_y + 46, 8, 4, flange_color, true, 1, z_idx);

        // Middle Row Purifying Shaft (y+16 to y+32, 26x16 px centered at x+3)
        Draw::rect(world_x + 3, world_y + 16, 26, 16, body_color, true, 1, z_idx);

        // Inset Skylight Window (16x10 px at world_x+8, world_y+19)
        Draw::rect(world_x + 8, world_y + 19, 16, 10, window_bg_color, true, 1, z_idx, y_sort_override);

        // Top Row Tapered Spire Crystal Peak (y to y+16)
        Draw::rect(world_x + 6, world_y + 12, 20, 4, top_color, true, 1, z_idx);
        Draw::rect(world_x + 9, world_y + 6, 14, 6, top_color, true, 1, z_idx);
        Draw::rect(world_x + 12, world_y, 8, 6, peak_tip_color, true, 1, z_idx);
    }
}

// --- DRAW BUILDING MANA ---

void building_dark_mana(int world_x, int world_y, int y_sort_override, uint32_t alpha) {
    uint32_t liquid_color = alpha | 0x009900FF; // Glowing twilight violet liquid
    int z_idx = Layer::WorldObj;

    // Fills lower 6px of the 24x10 skylight opening for Refiner (at world_x+12, world_y+23)
    Draw::rect(world_x + 12, world_y + 23, 24, 6, liquid_color, true, 1, z_idx, y_sort_override);
}

void building_light_mana(int world_x, int world_y, int y_sort_override, uint32_t alpha) {
    uint32_t aura_color = alpha | 0x0000FFFF;  // Cyan aura
    uint32_t core_color = alpha | 0x00FFFFFF;  // White core
    int z_idx = Layer::WorldObj;

    // Fills skylight window (Refiner: 24x10 at x+12, y+19; Spire: 16x10 at x+8, y+19)
    Draw::rect(world_x + 8, world_y + 21, 16, 6, aura_color, true, 1, z_idx, y_sort_override);
    Draw::rect(world_x + 12, world_y + 23, 8, 3, core_color, true, 1, z_idx, y_sort_override);
}

// --- DRAW PIPE ---

void pipe_bg(const Network& network, int gx, int gy, int world_x, int world_y, int tile_size) {
    uint32_t pipe_color = 0xFF4A4A60;

    int hub_size = 8;
    int offset = (tile_size - hub_size) / 2;
    int stub_len = offset;

    Draw::rect(world_x + offset, world_y + offset, hub_size, hub_size, pipe_color, true, 1, Layer::GroundFixture);

    if (network.is_valid_port_connection(gx, gy, gx, gy - 1)) {
        Draw::rect(world_x + offset, world_y, hub_size, stub_len, pipe_color, true, 1, Layer::GroundFixture);
    }
    if (network.is_valid_port_connection(gx, gy, gx, gy + 1)) {
        Draw::rect(world_x + offset, world_y + offset + hub_size, hub_size, stub_len, pipe_color, true, 1, Layer::GroundFixture);
    }
    if (network.is_valid_port_connection(gx, gy, gx - 1, gy)) {
        Draw::rect(world_x, world_y + offset, stub_len, hub_size, pipe_color, true, 1, Layer::GroundFixture);
    }
    if (network.is_valid_port_connection(gx, gy, gx + 1, gy)) {
        Draw::rect(world_x + offset + hub_size, world_y + offset, stub_len, hub_size, pipe_color, true, 1, Layer::GroundFixture);
    }
}

// --- PIPE - LIGHT MANA ---

void pipe_light_mana(const Network& network, const Fixture& fix, int gx, int gy, int world_x, int world_y, int tile_size, float progress) {
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

    int aura_w = 10;
    int aura_h = 4;
    int core_w = 6;
    int core_h = 2;

    int offset_x = (tile_size - aura_w) / 2;
    int offset_y = (tile_size - aura_h) / 2;

    int orb_x = world_x + offset_x + anim_offset_x;
    int orb_y = world_y + offset_y + anim_offset_y;

    Draw::rect(orb_x, orb_y, aura_w, aura_h, aura_color, true, 1, Layer::GroundFixtureItem);
    Draw::rect(orb_x + 2, orb_y + 1, core_w, core_h, core_color, true, 1, Layer::GroundFixtureItemFX);
}

// --- DRAW PIPE - DARK MANA ---

void pipe_dark_mana_straight(int world_x, int world_y, int tile_size, int flow_dx, int flow_dy, float progress, bool is_head_tile, bool is_tail_tile, int stream_w) {
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

void pipe_dark_mana_corner(int world_x, int world_y, int tile_size, int in_dx, int in_dy, int out_dx, int out_dy, float progress, bool is_head_tile, bool is_tail_tile, int stream_w) {
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

void pipe_dark_mana_junction_branch_stubs(const Network& network, int x, int y, int world_x, int world_y, int tile_size, int in_dx, int in_dy, int out_dx, int out_dy, int stream_w) {
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

void pipe_dark_mana_particles(
    ParticleSystem& ps, const Fixture& fix,
    int gx, int gy, int tile_size,
    float sim_tick_rate
) {
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
        ParticleEmitters::spawn_corner_pipe_mana(ps, gx, gy, in_dx, in_dy, out_dx, out_dy, sim_tick_rate, 1, tile_size);
    } else {
        int flow_dx = (in_dx != 0) ? in_dx : out_dx;
        int flow_dy = (in_dy != 0) ? in_dy : out_dy;

        if (flow_dx == 0 && flow_dy == 0) {
            flow_dx = 1; // Fallback rightwards flow
        }

        ParticleEmitters::spawn_straight_pipe_mana(ps, gx, gy, flow_dx, flow_dy, sim_tick_rate, 1, tile_size);
    }
}

void pipe_dark_mana(
    const Network& network, ParticleSystem& ps, const Fixture& fix,
    int gx, int gy, int world_x, int world_y, int tile_size,
    float progress, int stream_w, float last_dt, float sim_tick_rate
) {
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
        pipe_dark_mana_straight(world_x, world_y, tile_size, flow_dx, flow_dy, progress, is_head_tile, is_tail_tile, stream_w);
    } else {
        pipe_dark_mana_corner(world_x, world_y, tile_size, in_dx, in_dy, out_dx, out_dy, progress, is_head_tile, is_tail_tile, stream_w);
    }

    // Draw junction branch stubs for T and X intersections
    if (!is_head_tile && !is_tail_tile) {
        pipe_dark_mana_junction_branch_stubs(network, gx, gy, world_x, world_y, tile_size, in_dx, in_dy, out_dx, out_dy, stream_w);
    }

    // Emit Particles
    if (s_should_emit_pipe) {
        pipe_dark_mana_particles(ps, fix, gx, gy, tile_size, sim_tick_rate);
    }
}

// --- DRAW SEEP ---

void seep_bg(int world_x, int world_y, int tile_size) {
    int z_idx = Layer::GroundFixture;

    // Seep (3x2 tiles = 48x32 px)
    uint32_t base_color   = 0xFF0A2218; // Dark Twilight Mold Base
    uint32_t pool_color   = 0xFF00552B; // Deep Twilight Mana Pit Pool
    uint32_t core_color   = 0xFF00381B; // Inset Core Pit
    uint32_t flange_color = 0xFF00AA55; // Port Ring Indicator

    // Full 48x32 px base ground rect
    Draw::rect(world_x, world_y, 48, 32, base_color, true, 1, z_idx);
    // Inset liquid pool (42x26 px)
    Draw::rect(world_x + 3, world_y + 3, 42, 26, pool_color, true, 1, z_idx);
    // Dark core pit (32x16 px centered)
    Draw::rect(world_x + 8, world_y + 8, 32, 16, core_color, true, 1, z_idx);

    // Top Center Port Indicator (North at root_x+1, root_y)
    Draw::rect(world_x + 20, world_y - 2, 8, 4, flange_color, true, 1, z_idx);
    // Bottom Center Port Indicator (South at root_x+1, root_y+1)
    Draw::rect(world_x + 20, world_y + 30, 8, 4, flange_color, true, 1, z_idx);
}

void seep_dark_mana_connector(int world_x, int world_y, int out_dy, int stream_w) {
    if (out_dy == 0) return;

    uint32_t stream_color = 0xFF4A0088;
    int port_cx = world_x + 24; // Center of tile x+1 (16 to 32 -> center 24)
    int offset_x = port_cx - stream_w / 2;

    if (out_dy == -1) {
        // Top Center Port (North): line from center of pit (y+16) up to top edge (y)
        Draw::rect(static_cast<float>(offset_x), static_cast<float>(world_y), static_cast<float>(stream_w), 16.0f, stream_color, true, 1, Layer::GroundFixtureItem);
    } else if (out_dy == 1) {
        // Bottom Center Port (South): line from center of pit (y+16) down to bottom edge (y+32)
        Draw::rect(static_cast<float>(offset_x), static_cast<float>(world_y + 16), static_cast<float>(stream_w), 16.0f, stream_color, true, 1, Layer::GroundFixtureItem);
    }
}

void seep_mana(const Fixture& fix, int world_x, int world_y, int tile_size, int stream_w) {
    uint8_t mask = fix.flow_out_mask;
    if (mask & DirectionMask::North) {
        seep_dark_mana_connector(world_x, world_y, -1, stream_w);
    }
    if (mask & DirectionMask::South) {
        seep_dark_mana_connector(world_x, world_y, 1, stream_w);
    }
}

} // namespace

void begin_frame(float dt, float sim_tick_rate) {
    s_emit_timer += dt;
    float emit_interval = (sim_tick_rate > 0.01f) ? (sim_tick_rate / 2.5f) : 0.24f;

    s_should_emit_pipe = false;
    if (s_emit_timer >= emit_interval) {
        s_should_emit_pipe = true;
        s_emit_timer -= emit_interval;
        if (s_emit_timer > emit_interval * 2.0f) {
            s_emit_timer = 0.0f; // Clamp accumulator lag spikes
        }
    }
}

void pipe(
    const Network& network, ParticleSystem& ps, const Fixture& fix,
    int gx, int gy, int world_x, int world_y, int tile_size,
    float progress, float dt, float sim_tick_rate
) {
    // bg
    pipe_bg(network, gx, gy, world_x, world_y, tile_size);

    // mana
    if (fix.mana_state == ManaState::Dark) {
        pipe_dark_mana(
            network, ps, fix,
            gx, gy, world_x, world_y, tile_size,
            progress, STREAM_WIDTH, dt, sim_tick_rate
        );
    } else if (fix.mana_state == ManaState::Light) {
        pipe_light_mana(network, fix, gx, gy, world_x, world_y, tile_size, progress);
    }
}


void building(
    const Network& network, ParticleSystem& ps, const Fixture& fix,
    int world_x, int world_y,
    float progress, float last_dt, float sim_tick_rate
) {
    int world_bottom_y = world_y - 4;
    int y_sort_override = world_bottom_y + 20;
    uint32_t alpha = BUILDING_ALPHA_OPAQUE;

    building_bg(fix.type, world_x, world_y, world_bottom_y, y_sort_override, alpha);

    // mana
    if (fix.type == FixtureType::Refiner && fix.mana_state == ManaState::Dark) {
        building_dark_mana(world_x, world_y, y_sort_override, alpha);
    } else if (fix.mana_state == ManaState::Light) {
        building_light_mana(world_x, world_y, y_sort_override, alpha);
    }

    // particles
    if (fix.type == FixtureType::Refiner && fix.mana_state == ManaState::Dark) {
        ParticleEmitters::spawn_refiner_embers(ps, static_cast<float>(world_x + 6), static_cast<float>(world_y + 8), 1, Layer::WorldObj, y_sort_override);
    } else if (fix.type == FixtureType::Spire && fix.mana_state == ManaState::Light) {
        ParticleEmitters::spawn_spire_embers(ps, static_cast<float>(world_x + 6), static_cast<float>(world_y + 8), 1, Layer::WorldObj, y_sort_override);
    }
}

void seep(const Fixture& fix, int world_x, int world_y, int tile_size) {
    seep_bg(world_x, world_y, tile_size);

    // seep_mana
    seep_mana(fix, world_x, world_y, tile_size, STREAM_WIDTH);
}

} // namespace DrawFixtures
} // namespace alx
