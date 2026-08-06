#include "alx/Player.h"
#include <algorithm>
#include <cmath>
#include <variant>
#include "alx/Action.h"
#include "alx/Camera.h"
#include "alx/Tiles.h"
#include "alx/Network.h"
#include "alx/Layer.h"
#include "alx/DrawFX.h"
#include "alx/WorldCollision.h"
#include "alx/TrigLUT.h"
#include "core/Draw.h"
#include "Debug.h"
#include "assets/Images.h"

namespace alx {

FacingVector PlayerInputBuffer::update_facing(float dt, float raw_dx, float raw_dy) {
    bool is_diagonal = (raw_dx != 0.0f && raw_dy != 0.0f);
    bool is_moving = (raw_dx != 0.0f || raw_dy != 0.0f);

    if (is_diagonal) {
        // Immediately lock diagonal facing vector and refresh latch timer
        constexpr float inv_sqrt2 = 0.70710678118f;
        current_facing_dx = raw_dx * inv_sqrt2;
        current_facing_dy = raw_dy * inv_sqrt2;
        latched_facing_dx = current_facing_dx;
        latched_facing_dy = current_facing_dy;
        diagonal_latch_timer = FACING_DIAGONAL_LATCH_TIME;
        start_delay_timer = 0.0f;
    } else if (is_moving) {
        // Single-axis input
        if (diagonal_latch_timer > 0.0f) {
            // Technique 2: Diagonal Release Buffer (Stop Latch)
            diagonal_latch_timer -= dt;
            current_facing_dx = latched_facing_dx;
            current_facing_dy = latched_facing_dy;
            start_delay_timer = 0.0f;
        } else if (!was_moving) {
            // Technique 1: Facing Direction Hysteresis (2-Frame Input Latch)
            // Defer updating facing_dx/dy by 2 frames when starting movement from idle
            start_delay_timer = FACING_START_DELAY_TIME;
        } else if (start_delay_timer > 0.0f) {
            start_delay_timer -= dt;
            if (start_delay_timer <= 0.0f) {
                current_facing_dx = raw_dx;
                current_facing_dy = raw_dy;
            }
        } else {
            current_facing_dx = raw_dx;
            current_facing_dy = raw_dy;
        }
    } else {
        // Stopped moving (raw_dx == 0 && raw_dy == 0)
        if (diagonal_latch_timer > 0.0f) {
            diagonal_latch_timer -= dt;
            current_facing_dx = latched_facing_dx;
            current_facing_dy = latched_facing_dy;
        }
        start_delay_timer = 0.0f;
    }

    was_moving = is_moving;
    return FacingVector{ current_facing_dx, current_facing_dy };
}

Player::Player(float x, float y)
    : Entity(
        Transform{ // Transform
            x,
            y,
            static_cast<float>(Assets::Images::mystic_width), // width
            static_cast<float>(Assets::Images::mystic_height), // height
            Layer::WorldObj // z-index
        },
        AnimatedSpriteRender::create(
            Assets::Images::mystic,
            Assets::Images::mystic_len,
            Assets::Images::mystic_frames,
            Assets::Images::mystic_anims
        ),
        true, // active
        "player" // tag
    )
{
}

float Player::center_x(float alpha) const {
    float draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
    return draw_x + (transform.width / 2.0f);
}

float Player::center_y(float alpha) const {
    float draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
    return draw_y + (transform.height / 2.0f);
}

Collision::Circle Player::ground_circle(float px, float py) const {
    float r = transform.width * GROUND_RADIUS_RATIO;
    float cy = py + (transform.height * GROUND_OFFSET_Y_RATIO) - r;
    return Collision::Circle{ px + (transform.width / 2.0f), cy, r };
}

Collision::Circle Player::ground_circle() const {
    return ground_circle(transform.x, transform.y);
}

Player::PlacementPoint Player::placement_fixture_center(float px, float py, float tile_size) const {
    Collision::Circle g = ground_circle(px, py);
    MultiTileFootprint fp = get_fixture_footprint(m_selected_fixture_type);
    float max_dim = static_cast<float>(std::max(fp.width, fp.height));
    float max_dist = (max_dim + 1.5f) * tile_size;

    int player_tx = static_cast<int>(std::floor(g.cx / tile_size));
    int player_ty = static_cast<int>(std::floor(g.cy / tile_size));

    float fallback_cx = g.cx + facing_dx * tile_size;
    float fallback_cy = g.cy + facing_dy * tile_size;

    bool is_vertical = std::abs(facing_dy) >= std::abs(facing_dx);

    for (float d = tile_size * 0.5f; d <= max_dist; d += tile_size * 0.25f) {
        int tx = 0;
        int ty = 0;

        if (is_vertical) {
            // Perpendicular axis (X) centered on player tile X
            tx = player_tx - (fp.width / 2);
            if (facing_dy >= 0.0f) {
                // South (+Y)
                ty = static_cast<int>(std::floor((g.cy + d) / tile_size));
            } else {
                // North (-Y)
                int ty_front = static_cast<int>(std::floor((g.cy - d) / tile_size));
                ty = ty_front - (fp.height - 1);
            }
        } else {
            // Perpendicular axis (Y) centered on player tile Y
            ty = player_ty - (fp.height / 2);
            if (facing_dx >= 0.0f) {
                // East (+X)
                tx = static_cast<int>(std::floor((g.cx + d) / tile_size));
            } else {
                // West (-X)
                int tx_front = static_cast<int>(std::floor((g.cx - d) / tile_size));
                tx = tx_front - (fp.width - 1);
            }
        }

        Collision::AABB proposed_aabb = fixture_ground_aabb(tx, ty, tile_size, m_selected_fixture_type);
        if (!Collision::circle_vs_aabb(g, proposed_aabb)) {
            return PlacementPoint{ static_cast<float>(tx) * tile_size + tile_size * 0.5f, static_cast<float>(ty) * tile_size + tile_size * 0.5f };
        }
    }

    return PlacementPoint{ fallback_cx, fallback_cy };
}

Player::PlacementPoint Player::placement_fixture_center(float tile_size) const {
    return placement_fixture_center(transform.x, transform.y, tile_size);
}

Collision::Circle Player::hurt_circle(float px, float py) const {
    float r = transform.width * HURT_RADIUS_RATIO;
    float cy = py + (transform.height * HURT_OFFSET_Y_RATIO);
    return Collision::Circle{ px + (transform.width / 2.0f), cy, r };
}

Collision::Circle Player::hurt_circle() const {
    return hurt_circle(transform.x, transform.y);
}

void Player::sync_prev_transforms() {
    transform_prev = transform;
}

Facing::Type Player::facing() const {
    return Facing::from_vector(facing_dx, facing_dy);
}

bool Player::is_facing(Facing::Type dir) const {
    return facing() == dir;
}

int Player::base_facing_angle_deg() const {
    return Facing::to_degrees(facing());
}

Collision::Circle Player::calculate_attack_circle_at(float progress, float px, float py) const {
    int base_deg = base_facing_angle_deg();
    int offset_deg = ATTACK_ARC_SWEEP_START_DEG + static_cast<int>(progress * ATTACK_ARC_SWEEP_SWING_DEG);
    int angle_deg = base_deg + offset_deg;

    float pcx = px + (transform.width / 2.0f);
    float pcy = py + (transform.height / 2.0f);
    int reach = static_cast<int>(ATTACK_REACH_RADIUS);

    int cx = static_cast<int>(pcx) + ((reach * TrigLUT::cos(angle_deg)) >> TrigLUT::SHIFT);
    int cy = static_cast<int>(pcy) + ((reach * TrigLUT::sin(angle_deg)) >> TrigLUT::SHIFT);

    return Collision::Circle{ static_cast<float>(cx), static_cast<float>(cy), ATTACK_HIT_RADIUS };
}

bool Player::is_attacking() const {
    return attack_phase == AttackPhase::ActiveSweep;
}

Collision::Circle Player::attack_hit_circle(float alpha) const {
    float px = Draw::interpolate(transform_prev.x, transform.x, alpha);
    float py = Draw::interpolate(transform_prev.y, transform.y, alpha);
    float p = Draw::interpolate(swing_progress_prev, swing_progress_curr, alpha);
    return calculate_attack_circle_at(p, px, py);
}

bool Player::try_build_tile(const Tiles& tiles, Network& network) {
    int cost = fixture_cost(m_selected_fixture_type);
    if (m_cursed_alloy < cost) return false;

    float tile_sz = static_cast<float>(tiles.tile_size());
    PlacementPoint pt = placement_fixture_center(tile_sz);
    GridPos target_pos{
        static_cast<int16_t>(static_cast<int>(std::floor(pt.cx / tile_sz))),
        static_cast<int16_t>(static_cast<int>(std::floor(pt.cy / tile_sz)))
    };

    // Self-overlap guard: reject placement if fixture's solid ground box intersects player's ground circle
    Collision::AABB proposed_aabb = fixture_ground_aabb(target_pos.x, target_pos.y, tile_sz, m_selected_fixture_type);
    if (Collision::circle_vs_aabb(ground_circle(), proposed_aabb)) {
        return false;
    }

    if (network.can_place_fixture(target_pos, m_selected_fixture_type, tiles)) {
        m_cursed_alloy -= cost;
        network.place_fixture(target_pos, m_selected_fixture_type);
        return true;
    }
    return false;
}

bool Player::try_remove_tile(const Tiles& tiles, Network& network) {
    float tile_sz = static_cast<float>(tiles.tile_size());
    PlacementPoint pt = placement_fixture_center(tile_sz);
    GridPos target_pos{
        static_cast<int16_t>(static_cast<int>(std::floor(pt.cx / tile_sz))),
        static_cast<int16_t>(static_cast<int>(std::floor(pt.cy / tile_sz)))
    };

    if (network.in_bounds(target_pos)) {
        const Fixture& fix = network.fixture(target_pos);
        if (fix.type != FixtureType::None && fix.type != FixtureType::Seep) {
            int refund = fixture_cost(fix.type);
            m_cursed_alloy += refund;
            network.remove_fixture(target_pos);
            return true;
        }
    }
    return false;
}

void Player::update(float dt, const Tiles& tiles, Network& network, const alx::Camera& camera) {
    sync_prev_transforms();

    update_movement(dt, tiles, network, camera);
    update_actions(dt, tiles, network);
}

void Player::draw(std::vector<uint32_t>& screen_buffer, float alpha, const Tiles* tiles, const Network* network) {
    if (!active) return;

    float world_draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
    float world_draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
    float world_draw_w = transform.width;
    float world_draw_h = transform.height;

    float world_bottom_y = world_draw_y + world_draw_h;

    // Player shadow underneath player at bottom Y edge (foreshortened oval)
    DrawFX::shadow(
        world_draw_x,
        world_draw_y,
        world_draw_w,
        world_draw_h,
        transform.z_index,
        SHADOW_RX_RATIO,
        SHADOW_RY_RATIO_OF_RX
    );

    // Player body
    if (auto* anim = std::get_if<AnimatedSpriteRender>(&visual)) {
        if (!anim->current_anim.frame_indices.empty()) {
            int frame_pool_index = anim->current_anim.frame_indices[anim->current_sequence_index];
            const SpriteFrame& current_frame = anim->master_frames[frame_pool_index];

            const uint8_t* frame_pixels = anim->sheet_pixels + current_frame.offset;
            uint32_t frame_pixels_size = (current_frame.len > 0) ? static_cast<uint32_t>(current_frame.len) : anim->sheet_pixels_size;

            Draw::sprite_frame(
                world_draw_x,
                world_draw_y,
                frame_pixels,
                frame_pixels_size,
                static_cast<float>(current_frame.width),
                static_cast<float>(current_frame.height),
                current_frame.x,
                current_frame.y,
                current_frame.width,
                current_frame.height,
                transform.z_index,
                static_cast<int>(world_bottom_y), // sort Y override
                anim->is_flip_h,
                anim->is_flip_v
            );
        }
    }

    // Ground ground collision circle outline
    if (Debug::DRAW_GROUND_AREAS) {
        Collision::Circle ground = ground_circle(world_draw_x, world_draw_y);
        Draw::circle(
            ground.cx,
            ground.cy,
            ground.radius,
            0xFF00FFFF, // Bright Cyan debug outline
            false,      // fill = false (outline only)
            1,          // thickness = 1
            transform.z_index + 1,
            static_cast<int>(world_bottom_y)
        );
    }

    // Ground hurt collision circle outline
    if (Debug::DRAW_HURT_AREAS) {
        Collision::Circle hurt = hurt_circle();
        Draw::circle(
            hurt.cx,
            hurt.cy,
            hurt.radius,
            0xFFFFFF00, // Bright Yellow debug outline
            false,      // fill = false (outline only)
            1,          // thickness = 1
            transform.z_index + 1,
            static_cast<int>(world_bottom_y)
        );
    }

    // Attack hit circle (2px inner-outlined circle)
    if (is_attacking() && (Debug::DRAW_MELEE_ARCS)) {
        Collision::Circle hit_c = attack_hit_circle(alpha);
        Draw::circle(
            hit_c.cx,
            hit_c.cy,
            hit_c.radius,
            0xFF00FFFF, // Bright Cyan debug circle
            false,      // fill = false (outline only)
            1,          // thickness = 2 (2px inner-outlined circle)
            transform.z_index + 2,
            static_cast<int>(world_bottom_y) // sort Y override
        );
    }

    if (Action::is_pressed(Action::Build)) {
        PlacementPoint pt = placement_fixture_center(world_draw_x, world_draw_y, 16.0f);

        float tile_sz = 16.0f;
        int target_tx = static_cast<int>(std::floor(pt.cx / tile_sz));
        int target_ty = static_cast<int>(std::floor(pt.cy / tile_sz));
        GridPos target_pos{ static_cast<int16_t>(target_tx), static_cast<int16_t>(target_ty) };

        MultiTileFootprint fp = get_fixture_footprint(m_selected_fixture_type);
        Collision::AABB proposed_aabb = fixture_ground_aabb(target_tx, target_ty, tile_sz, m_selected_fixture_type);

        bool self_overlap = Collision::circle_vs_aabb(ground_circle(world_draw_x, world_draw_y), proposed_aabb);
        bool has_alloy = (m_cursed_alloy >= fixture_cost(m_selected_fixture_type));
        bool can_place = true;
        if (network && tiles) {
            can_place = network->can_place_fixture(target_pos, m_selected_fixture_type, *tiles);
        }

        bool is_valid = (!self_overlap) && has_alloy && can_place;
        uint32_t border_color = is_valid ? 0xFF00FF00 : 0xFF0000FF; // Green/Cyan (valid) vs Red (invalid)

        Draw::rect(
            static_cast<float>(target_tx) * tile_sz,
            static_cast<float>(target_ty) * tile_sz,
            static_cast<float>(fp.width) * tile_sz,
            static_cast<float>(fp.height) * tile_sz,
            border_color,
            false,      // fill = false
            1,          // thickness = 1
            transform.z_index + 1,
            static_cast<int>(world_bottom_y)
        );
    }
}

int Player::fixture_cost(FixtureType type) {
    switch (type) {
        case FixtureType::Pipe: return 1;
        case FixtureType::Refiner: return 5;
        case FixtureType::Spire: return 10;
        default: return 0;
    }
}

void Player::update_movement(float dt, const Tiles& tiles, const Network& network, const alx::Camera& camera) {
    is_panning = Action::is_pan_mode_active();

    if (camera.is_player_movement_locked()) {
        return;
    }

    if (is_attacking()) {
        return;
    }

    float dx = 0.0f;
    float dy = 0.0f;

    if (Action::is_pressed(Action::MoveUp))    dy -= 1.0f;
    if (Action::is_pressed(Action::MoveDown))  dy += 1.0f;
    if (Action::is_pressed(Action::MoveLeft))  dx -= 1.0f;
    if (Action::is_pressed(Action::MoveRight)) dx += 1.0f;

    if (!Action::is_pressed(Action::Build)) {
        FacingVector facing_vec = input_buffer.update_facing(dt, dx, dy);
        facing_dx = facing_vec.dx;
        facing_dy = facing_vec.dy;
    }

    auto f = facing();
    if (facing_dx < -0.01f) {
        is_facing_left = true;
    } else if (facing_dx > 0.01f) {
        is_facing_left = false;
    }

    // --- update movement animations ---
    if (auto* anim = std::get_if<AnimatedSpriteRender>(&visual)) {
        if (f == Facing::North) {
            anim->set_frame(0);
            anim->is_flip_h = false;
        } else if (f == Facing::NorthEast) {
            anim->set_frame(1);
            anim->is_flip_h = false;
        } else if (f == Facing::East) {
            anim->set_frame(2);
            anim->is_flip_h = false;
        } else if (f == Facing::SouthEast) {
            anim->set_frame(3);
            anim->is_flip_h = false;
        } else if (f == Facing::South) {
            anim->set_frame(4);
            anim->is_flip_h = true;
        } else if (f == Facing::SouthWest) {
            anim->set_frame(3);
            anim->is_flip_h = true;
        } else if (f == Facing::West) {
            anim->set_frame(2);
            anim->is_flip_h = true;
        } else if (f == Facing::NorthWest) {
            anim->set_frame(1);
            anim->is_flip_h = true;
        }
    }

    // DIAGONAL SPEED SCALE OPTIONS:
    // 1.00f = Classic 16-bit SNES/Zelda grid-aligned (+41% speed boost, 100% 60Hz smooth)
    // 0.75f = Rational 3/4px sub-pixel step (+6% speed boost, silky 4-frame rational rhythm)
    // 0.70710678f = Euclidean normalized (+0% speed boost, irrational jitter pattern)
    static constexpr float DIAGONAL_SPEED_SCALE = 0.75f;

    if (dx != 0.0f && dy != 0.0f) {
        dx *= DIAGONAL_SPEED_SCALE;
        dy *= DIAGONAL_SPEED_SCALE;
    }

    // NOTE: WorldCollision::try_move() prevents geometry penetration during normal gameplay.
    // Enable ejection safety net if adding heavy knockback, teleports, or phase-dashes.
    // WorldCollision::enforce_solid_ground_ejection(transform.x, transform.y, ground_circle(), tiles, network, 2.0f, tag);

    WorldCollision::try_move(transform.x, transform.y, dx * speed * dt, dy * speed * dt, ground_circle(), tiles, network);
}

void Player::update_actions(float dt, const Tiles& tiles, Network& network) {
    if (attack_phase == AttackPhase::ActiveSweep) {
        swing_progress_prev = swing_progress_curr;
        attack_timer += dt;
        if (attack_timer >= ATTACK_SWEEP_DURATION) {
            swing_progress_curr = 1.0f;
            attack_phase = AttackPhase::Recovery;
            attack_timer = 0.0f;
        } else {
            swing_progress_curr = attack_timer / ATTACK_SWEEP_DURATION;
        }
    } else if (attack_phase == AttackPhase::Recovery) {
        attack_timer += dt;
        if (attack_timer >= ATTACK_RECOVERY_DURATION) {
            attack_phase = AttackPhase::Idle;
            attack_timer = 0.0f;
            swing_progress_prev = 0.0f;
            swing_progress_curr = 0.0f;
        }
    }

    if (Action::is_attack() && attack_phase == AttackPhase::Idle) {
        sync_prev_transforms();
        attack_phase = AttackPhase::ActiveSweep;
        attack_timer = 0.0f;
        swing_progress_prev = 0.0f;
        swing_progress_curr = 0.0f;
        current_swing_id++;
    }

    if (!is_attacking()) {
        if (Action::is_build_cycle()) {
            if (m_selected_fixture_type == FixtureType::Pipe) {
                m_selected_fixture_type = FixtureType::Refiner;
            } else if (m_selected_fixture_type == FixtureType::Refiner) {
                m_selected_fixture_type = FixtureType::Spire;
            } else {
                m_selected_fixture_type = FixtureType::Pipe;
            }
        }

        if (Action::is_just_pressed(Action::DebugResource)) {
            m_cursed_alloy += 10;
        }

        if (Action::is_build_tile()) {
            try_build_tile(tiles, network);
        }

        if (Action::is_remove_tile()) {
            try_remove_tile(tiles, network);
        }
    }
}

} // namespace alx
