#include "alx/WorldCollision.h"
#include "core/Log.h"

namespace alx::WorldCollision {

bool is_solid_ground(const Collision::Circle& ground, const Tiles& tiles, const Network& network) {
    float tile_size = static_cast<float>(tiles.tile_size());

    int min_tx = static_cast<int>(std::floor((ground.cx - ground.radius) / tile_size));
    int max_tx = static_cast<int>(std::floor((ground.cx + ground.radius) / tile_size));
    int min_ty = static_cast<int>(std::floor((ground.cy - ground.radius) / tile_size));
    int max_ty = static_cast<int>(std::floor((ground.cy + ground.radius) / tile_size));

    // Wall tile checks (1x1 precision)
    for (int ty = min_ty; ty <= max_ty; ++ty) {
        for (int tx = min_tx; tx <= max_tx; ++tx) {
            if (tiles.in_bounds(tx, ty) && tiles.is_wall(tx, ty)) {
                if (Collision::circle_vs_aabb(ground, tx * tile_size, ty * tile_size, tile_size, tile_size)) {
                    return true;
                }
            }
        }
    }

    // Network solid fixture checks (multi-tile AABB expanded query range computed dynamically)
    int max_dim = max_fixture_footprint_dimension();
    int search_expand = (max_dim > 1) ? (max_dim - 1) : 0;
    int net_min_tx = min_tx - search_expand;
    int net_max_tx = max_tx + search_expand;
    int net_min_ty = min_ty - search_expand;
    int net_max_ty = max_ty + search_expand;

    for (int ty = net_min_ty; ty <= net_max_ty; ++ty) {
        for (int tx = net_min_tx; tx <= net_max_tx; ++tx) {
            if (network.in_bounds(tx, ty) && network.is_solid(tx, ty)) {
                const Fixture& fix = network.fixture(tx, ty);
                int root_tx = tx - fix.root_offset_x;
                int root_ty = ty - fix.root_offset_y;
                const Fixture& root_fix = network.in_bounds(root_tx, root_ty) ? network.fixture(root_tx, root_ty) : fix;
                Collision::AABB fixture_aabb = fixture_ground_aabb(root_tx, root_ty, tile_size, root_fix.type);
                if (Collision::circle_vs_aabb(ground, fixture_aabb)) {
                    return true;
                }
            }
        }
    }
    return false;
}

MoveResult try_move(float& x, float& y, float dx, float dy, const Collision::Circle& ground, const Tiles& tiles, const Network& network) {
    MoveResult res;

    if (dx != 0.0f) {
        float test_x = x + dx;
        Collision::Circle test_ground{ ground.cx + dx, ground.cy, ground.radius };
        if (!is_solid_ground(test_ground, tiles, network)) {
            x = test_x;
            res.moved_dx = dx;
        } else {
            res.blocked_x = true;
        }
    }

    if (dy != 0.0f) {
        float test_y = y + dy;
        Collision::Circle test_ground{ ground.cx + res.moved_dx, ground.cy + dy, ground.radius };
        if (!is_solid_ground(test_ground, tiles, network)) {
            y = test_y;
            res.moved_dy = dy;
        } else {
            res.blocked_y = true;
        }
    }

    return res;
}

bool enforce_solid_ground_ejection(float& x, float& y, const Collision::Circle& ground, const Tiles& tiles, const Network& network, float nudge_dist, std::string tag) {
    float tile_size = static_cast<float>(tiles.tile_size());

    int min_tx = static_cast<int>(std::floor((ground.cx - ground.radius) / tile_size));
    int max_tx = static_cast<int>(std::floor((ground.cx + ground.radius) / tile_size));
    int min_ty = static_cast<int>(std::floor((ground.cy - ground.radius) / tile_size));
    int max_ty = static_cast<int>(std::floor((ground.cy + ground.radius) / tile_size));

    float total_push_x = 0.0f;
    float total_push_y = 0.0f;
    int count = 0;

    for (int ty = min_ty; ty <= max_ty; ++ty) {
        for (int tx = min_tx; tx <= max_tx; ++tx) {
            bool solid = false;
            Collision::AABB obs_aabb;

            if (tiles.in_bounds(tx, ty) && tiles.is_wall(tx, ty)) {
                solid = true;
                obs_aabb = Collision::AABB{ tx * tile_size, ty * tile_size, tile_size, tile_size };
            } else if (network.in_bounds(tx, ty) && network.is_solid(tx, ty)) {
                solid = true;
                const Fixture& fix = network.fixture(tx, ty);
                int root_tx = tx - fix.root_offset_x;
                int root_ty = ty - fix.root_offset_y;
                const Fixture& root_fix = network.in_bounds(root_tx, root_ty) ? network.fixture(root_tx, root_ty) : fix;
                obs_aabb = fixture_ground_aabb(root_tx, root_ty, tile_size, root_fix.type);
            }

            if (solid && Collision::circle_vs_aabb(ground, obs_aabb)) {
                Log::warn_fmt_t("enforce_solid_ground_ejection SOLID && COLLIDE:\n  t: %s x: %.1f y: %.1f gcx: %.1f gcy: %.1f gr: %f\n  rtxy: {%d..%d}, {%d, %d}", tag.c_str(), x, y, ground.cx, ground.cy, ground.radius, min_tx, max_tx, min_ty, max_ty);
                float obs_cx = obs_aabb.x + obs_aabb.w * 0.5f;
                float obs_cy = obs_aabb.y + obs_aabb.h * 0.5f;
                float push_x = ground.cx - obs_cx;
                float push_y = ground.cy - obs_cy;
                float len = std::sqrt(push_x * push_x + push_y * push_y);
                if (len > 0.001f) {
                    push_x /= len;
                    push_y /= len;
                } else {
                    push_x = 0.0f;
                    push_y = -1.0f;
                }
                total_push_x += push_x;
                total_push_y += push_y;
                count++;
            }
        }
    }

    if (count > 0) {
        float len = std::sqrt(total_push_x * total_push_x + total_push_y * total_push_y);
        if (len > 0.001f) {
            total_push_x /= len;
            total_push_y /= len;
        } else {
            total_push_x = 0.0f;
            total_push_y = -1.0f;
        }

        x += total_push_x * nudge_dist;
        y += total_push_y * nudge_dist;
        return true;
    }

    return false;
}

} // namespace alx::WorldCollision
