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
#include "alx/SFX.h"
#include "core/Audio.h"
#include "core/Draw.h"
#include "Debug.h"
#include "assets/Images.h"

namespace alx {

namespace {

void constrain_movement_velocity_to_map(float& move_dx, float& move_dy, const Collision::Circle& ground, float map_w, float map_h) noexcept {
    if (move_dx < 0.0f && (ground.cx - ground.radius + move_dx) < 0.0f) {
        move_dx = 0.0f;
    } else if (move_dx > 0.0f && (ground.cx + ground.radius + move_dx) > map_w) {
        move_dx = 0.0f;
    }

    if (move_dy < 0.0f && (ground.cy - ground.radius + move_dy) < 0.0f) {
        move_dy = 0.0f;
    } else if (move_dy > 0.0f && (ground.cy + ground.radius + move_dy) > map_h) {
        move_dy = 0.0f;
    }
}

void clamp_transform_to_map_bounds(Transform& transform, float ground_radius_ratio, float ground_offset_y_ratio, float map_w, float map_h) noexcept {
    const float r = transform.width * ground_radius_ratio;
    const float cy_offset = (transform.height * ground_offset_y_ratio) - r;
    const float half_w = transform.width * 0.5f;

    const float min_x = r - half_w;
    const float max_x = map_w - half_w - r;
    const float min_y = r - cy_offset;
    const float max_y = map_h - cy_offset - r;

    transform.x = std::clamp(transform.x, min_x, max_x);
    transform.y = std::clamp(transform.y, min_y, max_y);
}

void update_facing_animation(AnimatedSpriteRender& anim, Facing::Type f) {
    switch (f) {
        case Facing::North:     anim.set_frame(0); anim.is_flip_h = false; break;
        case Facing::NorthEast: anim.set_frame(1); anim.is_flip_h = false; break;
        case Facing::East:      anim.set_frame(2); anim.is_flip_h = false; break;
        case Facing::SouthEast: anim.set_frame(3); anim.is_flip_h = false; break;
        case Facing::South:     anim.set_frame(4); anim.is_flip_h = true;  break;
        case Facing::SouthWest: anim.set_frame(3); anim.is_flip_h = true;  break;
        case Facing::West:      anim.set_frame(2); anim.is_flip_h = true;  break;
        case Facing::NorthWest: anim.set_frame(1); anim.is_flip_h = true;  break;
    }
}

void draw_player_sprite(const AnimatedSpriteRender& anim, float world_draw_x, float world_draw_y, float z_index, float world_bottom_y) {
    if (anim.current_anim.frame_indices.empty()) return;

    int frame_pool_index = anim.current_anim.frame_indices[anim.current_sequence_index];
    const SpriteFrame& current_frame = anim.master_frames[frame_pool_index];

    const uint8_t* frame_pixels = anim.sheet_pixels + current_frame.offset;
    uint32_t frame_pixels_size = (current_frame.len > 0) ? static_cast<uint32_t>(current_frame.len) : anim.sheet_pixels_size;

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
        z_index,
        static_cast<int>(world_bottom_y),
        anim.is_flip_h,
        anim.is_flip_v
    );
}

void draw_debug_outlines(const Player& player, float world_draw_x, float world_draw_y, float alpha, float world_bottom_y) {
    if constexpr (Debug::DRAW_GROUND_AREAS) {
        Collision::Circle ground = player.ground_circle(world_draw_x, world_draw_y);
        Draw::circle(
            ground.cx, ground.cy, ground.radius,
            0xFF00FFFF, false, 1,
            player.transform.z_index + 1, static_cast<int>(world_bottom_y)
        );
    }

    if constexpr (Debug::DRAW_HURT_AREAS) {
        Collision::Circle hurt = player.hurt_circle();
        Draw::circle(
            hurt.cx, hurt.cy, hurt.radius,
            0xFFFFFF00, false, 1,
            player.transform.z_index + 1, static_cast<int>(world_bottom_y)
        );
    }

    if constexpr (Debug::DRAW_MELEE_ARCS) {
        if (player.is_attacking()) {
            Collision::Circle hit_c = player.attack_hit_circle(alpha);
            Draw::circle(
                hit_c.cx, hit_c.cy, hit_c.radius,
                0xFF00FFFF, false, 1,
                player.transform.z_index + 2, static_cast<int>(world_bottom_y)
            );
        }
    }
}

void draw_placement_preview(const Player& player, float world_draw_x, float world_draw_y, float world_bottom_y, const Tiles* tiles, const Network* network) {
    if (!Action::is_pressed(Action::BuildMode) && !Action::is_place_fixture_held()) return;

    constexpr float tile_sz = 16.0f;
    Player::PlacementPoint pt = player.placement_fixture_center(world_draw_x, world_draw_y, tile_sz);

    int target_tx = static_cast<int>(std::floor(pt.cx / tile_sz));
    int target_ty = static_cast<int>(std::floor(pt.cy / tile_sz));
    GridPos target_pos{ static_cast<int16_t>(target_tx), static_cast<int16_t>(target_ty) };

    FixtureType selected_type = player.selected_fixture_type();
    MultiTileFootprint fp = get_fixture_footprint(selected_type);
    Collision::AABB proposed_aabb = fixture_ground_aabb(target_tx, target_ty, tile_sz, selected_type);

    bool self_overlap = Collision::circle_vs_aabb(player.ground_circle(world_draw_x, world_draw_y), proposed_aabb);
    bool has_alloy = (player.cursed_alloy() >= Player::fixture_cost(selected_type));
    bool can_place = true;
    if (network && tiles) {
        can_place = network->can_place_fixture(target_pos, selected_type, *tiles);
    }

    bool is_valid = (!self_overlap) && has_alloy && can_place;
    uint32_t border_color = is_valid ? 0xFF00FF00 : 0xFF0000FF; // Green (valid) vs Red (invalid)

    // STUI: 2D Directional Reticle Vector Line & Arrowhead
    float pcx = world_draw_x + (player.transform.width * 0.5f);
    float pcy = world_draw_y + (player.transform.height * 0.5f);
    float tcx = (static_cast<float>(target_tx) + static_cast<float>(fp.width) * 0.5f) * tile_sz;
    float tcy = (static_cast<float>(target_ty) + static_cast<float>(fp.height) * 0.5f) * tile_sz;

    Draw::line(pcx, pcy, tcx, tcy, border_color, 1, player.transform.z_index + 1, static_cast<int>(world_bottom_y));

    float card_dx = player.m_cardinal_facing_dx;
    float card_dy = player.m_cardinal_facing_dy;
    if (card_dx == 0.0f && card_dy == 0.0f) {
        card_dx = player.facing_dx;
        card_dy = player.facing_dy;
    }

    constexpr float wing_len = 3.0f;
    if (std::abs(card_dx) > std::abs(card_dy)) {
        float dir_sign = (card_dx >= 0.0f) ? 1.0f : -1.0f;
        Draw::line(tcx, tcy, tcx - dir_sign * wing_len, tcy - wing_len, border_color, 1, player.transform.z_index + 1, static_cast<int>(world_bottom_y));
        Draw::line(tcx, tcy, tcx - dir_sign * wing_len, tcy + wing_len, border_color, 1, player.transform.z_index + 1, static_cast<int>(world_bottom_y));
    } else {
        float dir_sign = (card_dy >= 0.0f) ? 1.0f : -1.0f;
        Draw::line(tcx, tcy, tcx - wing_len, tcy - dir_sign * wing_len, border_color, 1, player.transform.z_index + 1, static_cast<int>(world_bottom_y));
        Draw::line(tcx, tcy, tcx + wing_len, tcy - dir_sign * wing_len, border_color, 1, player.transform.z_index + 1, static_cast<int>(world_bottom_y));
    }

    Draw::rect(
        static_cast<float>(target_tx) * tile_sz,
        static_cast<float>(target_ty) * tile_sz,
        static_cast<float>(fp.width) * tile_sz,
        static_cast<float>(fp.height) * tile_sz,
        border_color,
        false, 1,
        player.transform.z_index + 1, static_cast<int>(world_bottom_y)
    );
}

} // anonymous namespace

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

    // STCE: Use strict 4-way orthogonal cardinal facing vector
    float card_dx = m_cardinal_facing_dx;
    float card_dy = m_cardinal_facing_dy;
    if (card_dx == 0.0f && card_dy == 0.0f) {
        if (std::abs(facing_dx) >= std::abs(facing_dy)) {
            card_dx = (facing_dx >= 0.0f) ? 1.0f : -1.0f;
            card_dy = 0.0f;
        } else {
            card_dx = 0.0f;
            card_dy = (facing_dy >= 0.0f) ? 1.0f : -1.0f;
        }
    }

    float fallback_cx = g.cx + card_dx * tile_size;
    float fallback_cy = g.cy + card_dy * tile_size;

    bool is_vertical = std::abs(card_dy) >= std::abs(card_dx);

    for (float d = tile_size * 0.5f; d <= max_dist; d += tile_size * 0.25f) {
        int tx = 0;
        int ty = 0;

        if (is_vertical) {
            // Perpendicular axis (X) centered on player tile X
            tx = player_tx - (fp.width / 2);
            if (card_dy >= 0.0f) {
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
            if (card_dx >= 0.0f) {
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
        Audio::play_sfx(SFX::build_snap());
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

bool Player::take_damage(int amount) {
    if (is_invulnerable()) {
        return false;
    }
    state.hp = std::max(0, state.hp - amount);
    state.iframe_timer = State::IFRAME_DURATION;
    if (state.hp <= 0) {
        state.defeated = true;
        state.defeat_timer = State::DEFEAT_DURATION;
        Audio::play_sfx(SFX::player_death());
    } else {
        Audio::play_sfx(SFX::player_hit());
    }
    return true;
}

void Player::update(float dt, const Tiles& tiles, Network& network, const alx::Camera& camera, const std::vector<WorldStructure>* structures) {
    sync_prev_transforms();

    if (state.iframe_timer > 0.0f) {
        state.iframe_timer = std::max(0.0f, state.iframe_timer - dt);
    }

    if (state.defeated) {
        if (state.defeat_timer > 0.0f) {
            state.defeat_timer -= dt;
        }
        return;
    }

    update_movement(dt, tiles, network, camera, structures);
    update_actions(dt, tiles, network);
}

void Player::draw(std::vector<uint32_t>& screen_buffer, float alpha, const Tiles* tiles, const Network* network) {
    if (!active) return;

    float world_draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
    float world_draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
    float world_draw_w = transform.width;
    float world_draw_h = transform.height;
    float world_bottom_y = world_draw_y + world_draw_h;

    const alx::Camera* cam = static_cast<const alx::Camera*>(Draw::active_camera());
    if (cam && !cam->is_aabb_visible(world_draw_x, world_draw_y, world_draw_w, world_draw_h)) {
        return;
    }

    // Player shadow underneath player at bottom Y edge (foreshortened oval)
    DrawFX::shadow(
        world_draw_x,
        world_draw_y,
        world_draw_w,
        world_draw_h,
        Layer::WorldObjBG,
        static_cast<int>(world_bottom_y),
        SHADOW_RX_RATIO,
        SHADOW_RY_RATIO_OF_RX
    );

    // Player body (with i-frame flashing and defeat concealment)
    if (auto* anim = std::get_if<AnimatedSpriteRender>(&visual)) {
        bool skip_sprite = false;
        if (state.iframe_timer > 0.0f) {
            int blink = static_cast<int>(state.iframe_timer * 20.0f) % 2;
            if (blink == 0) {
                skip_sprite = true;
            }
        }
        if (!skip_sprite && !state.defeated) {
            draw_player_sprite(*anim, world_draw_x, world_draw_y, transform.z_index, world_bottom_y);
        }
    }

    // Charging visual feedback ring / aura
    if (is_charging_attack) {
        float cx = world_draw_x + world_draw_w * 0.5f;
        float cy = world_draw_y + world_draw_h * 0.5f;
        if (charge_timer < CHARGE_FULL_DURATION) {
            float progress = charge_timer / CHARGE_FULL_DURATION;
            int r = static_cast<int>(progress * CHARGE_MAX_UNCHARGED_RADIUS);
            if (r > 1) {
                float size = static_cast<float>(r * 2);
                Draw::rect(cx - static_cast<float>(r), cy - static_cast<float>(r), size, size, CHARGE_AURA_COLOR, false, 1, Layer::WorldObjFX, static_cast<int>(world_bottom_y));
            }
        } else {
            // Fully charged! Flash cyan aura ring (25% opacity, decreased size)
            Draw::rect(cx - CHARGE_FULLY_CHARGED_OUTER_OFF, cy - CHARGE_FULLY_CHARGED_OUTER_OFF, CHARGE_FULLY_CHARGED_OUTER_SZ, CHARGE_FULLY_CHARGED_OUTER_SZ, CHARGE_FULL_OUTER_COLOR, false, 2, Layer::WorldObjFX, static_cast<int>(world_bottom_y));
            Draw::rect(cx - CHARGE_FULLY_CHARGED_INNER_OFF, cy - CHARGE_FULLY_CHARGED_INNER_OFF, CHARGE_FULLY_CHARGED_INNER_SZ, CHARGE_FULLY_CHARGED_INNER_SZ, CHARGE_FULL_INNER_COLOR, false, 1, Layer::WorldObjFX, static_cast<int>(world_bottom_y));
        }
    }

    // Ground & hurt collision areas + attack hit arc debug outlines
    draw_debug_outlines(*this, world_draw_x, world_draw_y, alpha, world_bottom_y);

    // Fixture placement preview box
    draw_placement_preview(*this, world_draw_x, world_draw_y, world_bottom_y, tiles, network);
}

int Player::fixture_cost(FixtureType type) {
    switch (type) {
        case FixtureType::Pipe: return 1;
        case FixtureType::Wall: return 2;
        case FixtureType::Refiner: return 5;
        case FixtureType::Spire: return 10;
        default: return 0;
    }
}

void Player::update_movement(float dt, const Tiles& tiles, const Network& network, const alx::Camera& camera, const std::vector<WorldStructure>* structures) {
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

    if (!Action::is_place_fixture_held()) {
        FacingVector facing_vec = input_buffer.update_facing(dt, dx, dy);
        facing_dx = facing_vec.dx;
        facing_dy = facing_vec.dy;

        float ax = std::abs(facing_dx);
        float ay = std::abs(facing_dy);
        if (ax > ay) {
            m_cardinal_facing_dx = (facing_dx > 0.0f) ? 1.0f : -1.0f;
            m_cardinal_facing_dy = 0.0f;
        } else if (ay > ax) {
            m_cardinal_facing_dx = 0.0f;
            m_cardinal_facing_dy = (facing_dy > 0.0f) ? 1.0f : -1.0f;
        }
    }

    auto f = facing();
    if (facing_dx < -0.01f) {
        is_facing_left = true;
    } else if (facing_dx > 0.01f) {
        is_facing_left = false;
    }

    // Update movement animations
    if (auto* anim = std::get_if<AnimatedSpriteRender>(&visual)) {
        update_facing_animation(*anim, f);
    }

    // DIAGONAL SPEED SCALE OPTIONS:
    static constexpr float DIAGONAL_SPEED_SCALE = 0.75f;

    if (dx != 0.0f && dy != 0.0f) {
        dx *= DIAGONAL_SPEED_SCALE;
        dy *= DIAGONAL_SPEED_SCALE;
    }

    float move_dx = dx * speed * dt;
    float move_dy = dy * speed * dt;

    const float map_w = tiles.world_width();
    const float map_h = tiles.world_height();

    constrain_movement_velocity_to_map(move_dx, move_dy, ground_circle(), map_w, map_h);

    auto move_res = WorldCollision::try_move(transform.x, transform.y, move_dx, move_dy, ground_circle(), tiles, network, structures);
    bool currently_blocked = move_res.blocked_x || move_res.blocked_y;
    if (currently_blocked && !m_was_blocked_prev) {
        Audio::play_sfx(SFX::wall_bump());
    }
    m_was_blocked_prev = currently_blocked;

    clamp_transform_to_map_bounds(transform, GROUND_RADIUS_RATIO, GROUND_OFFSET_Y_RATIO, map_w, map_h);
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

    bool btn_just_pressed = Action::is_attack();
    bool btn_held = Action::is_attack_held();
    bool spark_just_pressed = Action::is_just_pressed(Action::ManaSpark);
    bool spark_held = Action::is_pressed(Action::ManaSpark);
    
    if (!is_charging_attack) {
        if (btn_just_pressed && attack_phase == AttackPhase::Idle) {
            sync_prev_transforms();
            attack_phase = AttackPhase::ActiveSweep;
            attack_timer = 0.0f;
            swing_progress_prev = 0.0f;
            swing_progress_curr = 0.0f;
            current_swing_id++;
            is_charging_attack = true;
            m_charging_with_spark = false;
            charge_timer = 0.0f;
            Audio::play_sfx(SFX::sword_swipe());
        } else if (spark_just_pressed) {
            sync_prev_transforms();
            is_charging_attack = true;
            m_charging_with_spark = true;
            charge_timer = 0.0f;
        }
    }
    
    if (is_charging_attack) {
        bool still_holding = m_charging_with_spark ? spark_held : btn_held;
        if (still_holding) {
            charge_timer += dt;
        } else {
            if (charge_timer >= CHARGE_FULL_DURATION) {
                m_pending_spark = true;
            }
            is_charging_attack = false;
            m_charging_with_spark = false;
            charge_timer = 0.0f;
        }
    }

    if (!is_attacking()) {
        if (Action::is_build_cycle()) {
            if (m_selected_fixture_type == FixtureType::Pipe) {
                m_selected_fixture_type = FixtureType::Wall;
            } else if (m_selected_fixture_type == FixtureType::Wall) {
                m_selected_fixture_type = FixtureType::Refiner;
            } else if (m_selected_fixture_type == FixtureType::Refiner) {
                m_selected_fixture_type = FixtureType::Spire;
            } else {
                m_selected_fixture_type = FixtureType::Pipe;
            }
        }

        if constexpr (ALX_ENABLE_DEBUG) {
            if (Action::is_just_pressed(Action::DebugResource)) {
                m_cursed_alloy += 10;
            }
        }

        if (Action::is_place_fixture()) {
            try_build_tile(tiles, network);
        }

        if (Action::is_remove_fixture()) {
            try_remove_tile(tiles, network);
        }

        if (Action::is_build_foundation()) {
            float tile_sz = static_cast<float>(tiles.tile_size());
            PlacementPoint pt = placement_fixture_center(tile_sz);
            GridPos target_pos{
                static_cast<int16_t>(static_cast<int>(std::floor(pt.cx / tile_sz))),
                static_cast<int16_t>(static_cast<int>(std::floor(pt.cy / tile_sz)))
            };
            if (network.in_bounds(target_pos)) {
                Audio::play_sfx(SFX::build_snap());
            }
        }
    }
}

} // namespace alx
