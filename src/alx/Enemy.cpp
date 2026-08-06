#include "alx/Enemy.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <utility>
#include "core/Draw.h"
#include "alx/DrawFX.h"
#include "alx/Layer.h"
#include "alx/Random.h"
#include "Debug.h"

namespace alx {

Enemy::Enemy(float px, float py, float w, float h, uint32_t col, int max_hp)
    : Entity(
        Transform{ px, py, w, h, Layer::WorldObj },
        RectangleRender{ col, true, 1 },
        true,
        "enemy"
      ),
      hp(max_hp)
{}

float Enemy::center_x(float alpha) const {
    float draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
    return draw_x + (transform.width * 0.5f);
}

float Enemy::center_y(float alpha) const {
    float draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
    return draw_y + (transform.height * 0.5f);
}

Collision::Circle Enemy::ground_circle(float px, float py) const {
    float r = transform.width * GROUND_RADIUS_RATIO;
    float cy = py + (transform.height * GROUND_OFFSET_Y_RATIO) - r;
    return Collision::Circle{ px + (transform.width * 0.5f), cy, r };
}

Collision::Circle Enemy::hurt_circle(float px, float py) const {
    float r = transform.width * HURT_RADIUS_RATIO;
    float cy = py + (transform.height * HURT_OFFSET_Y_RATIO);
    return Collision::Circle{ px + (transform.width * 0.5f), cy, r };
}

void Enemy::set_steering_vector_8way(float target_world_x, float target_world_y) {
    float dx = target_world_x - (transform.x + transform.width * 0.5f);
    float dy = target_world_y - (transform.y + transform.height * 0.5f);

    float len_sq = dx * dx + dy * dy;
    if (len_sq < 0.0001f) {
        move_dx = 0.0f;
        move_dy = 0.0f;
        is_moving = false;
        return;
    }

    float angle = std::atan2(dy, dx);
    constexpr float pi = 3.14159265358979323846f;
    constexpr float inv_sqrt2 = 0.70710678118f;

    static constexpr std::pair<float, float> dirs8[8] = {
        {1.0f, 0.0f}, {inv_sqrt2, inv_sqrt2}, {0.0f, 1.0f}, {-inv_sqrt2, inv_sqrt2},
        {-1.0f, 0.0f}, {-inv_sqrt2, -inv_sqrt2}, {0.0f, -1.0f}, {inv_sqrt2, -inv_sqrt2}
    };

    float normalized_angle = angle;
    if (normalized_angle < 0.0f) normalized_angle += 2.0f * pi;

    int index = static_cast<int>(std::round(8.0f * (normalized_angle / (2.0f * pi)))) % 8;
    move_dx = dirs8[index].first;
    move_dy = dirs8[index].second;
    facing_dx = move_dx;
    facing_dy = move_dy;
    is_moving = true;
}

void Enemy::take_damage(int amount, float kb_dx, float kb_dy, float kb_speed, float wound_ox, float wound_oy) {
    hp -= amount;
    knockback_dx = kb_dx;
    knockback_dy = kb_dy;
    knockback_speed = kb_speed;
    initial_knockback_speed = kb_speed;
    is_moving = false;
    move_dx = 0.0f;
    move_dy = 0.0f;
    state = EnemyState::HitStun;
    state_timer = HIT_STUN_DURATION;
    hit_wound_offset_x = wound_ox;
    hit_wound_offset_y = wound_oy;
    bleed_waves_left = 2; // Wave 1 emitted immediately; Waves 2 & 3 remaining
    bleed_timer = 0.07f;
}

void Enemy::draw(std::vector<uint32_t>& screen_buffer, float alpha) const {
    if (!active) return;

    float world_draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
    float world_draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
    float world_draw_w = transform.width;
    float world_draw_h = transform.height;
    int world_bottom_y = static_cast<int>(world_draw_y + world_draw_h);

    draw_shadow(world_draw_x, world_draw_y, world_draw_w, world_draw_h, world_bottom_y);
    draw_body(world_draw_x, world_draw_y, world_draw_w, world_draw_h, world_bottom_y);
    draw_debug_overlays(world_draw_x, world_draw_y, world_draw_w, world_draw_h, world_bottom_y);
}

void Enemy::draw_shadow(float draw_x, float draw_y, float draw_w, float draw_h, int sort_y) const {
    DrawFX::shadow(
        draw_x,
        draw_y,
        draw_w,
        draw_h,
        Layer::WorldObjBG,
        sort_y
    );
}

void Enemy::draw_body(float draw_x, float draw_y, float draw_w, float draw_h, int sort_y) const {
    if (auto* rect = std::get_if<RectangleRender>(&visual)) {
        uint32_t body_color = rect->color;
        if (state == EnemyState::HitStun && state_timer > (HIT_STUN_DURATION - 0.08f)) {
            constexpr uint32_t flash_col = 0x66880018;
            uint32_t a = (flash_col >> 24) & 0xFF;
            uint32_t inv_a = 255 - a;

            uint32_t r = (((flash_col >> 16) & 0xFF) * a + ((body_color >> 16) & 0xFF) * inv_a) / 255;
            uint32_t g = (((flash_col >> 8) & 0xFF) * a + ((body_color >> 8) & 0xFF) * inv_a) / 255;
            uint32_t b = ((flash_col & 0xFF) * a + (body_color & 0xFF) * inv_a) / 255;

            body_color = (body_color & 0xFF000000) | (r << 16) | (g << 8) | b;
        }

        Draw::rect(
            draw_x,
            draw_y,
            draw_w,
            draw_h,
            body_color,
            rect->fill,
            rect->thickness,
            transform.z_index,
            sort_y
        );
    }
}

void Enemy::draw_debug_overlays(float draw_x, float draw_y, float draw_w, float draw_h, int sort_y) const {
    // Ground feet collision circle outline (cyan debug)
    if (Debug::DRAW_GROUND_AREAS) {
        Collision::Circle ground = ground_circle(draw_x, draw_y);
        Draw::circle(
            ground.cx,
            ground.cy,
            ground.radius,
            0xFF00FFFF, // Bright Cyan debug outline
            false,      // fill = false (outline only)
            1,          // thickness = 1
            transform.z_index + 1,
            sort_y
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
            sort_y
        );
    }

    // Enemy player aggro detection area (dark red debug circle)
    if (Debug::DRAW_ENEMY_AGGRO_AREAS) {
        float cx = draw_x + draw_w * 0.5f;
        float cy = draw_y + draw_h * 0.5f;
        Draw::circle(
            cx,
            cy,
            AGGRO_DETECTION_RADIUS,
            0xCCAA0000, // Dark Red debug outline
            false,      // fill = false (outline only)
            1,          // thickness = 1
            transform.z_index + 1,
            sort_y
        );
    }

    // Enemy facing direction arrow (lime green 2px thick line + arrowhead - always drawn when debug enabled)
    if (Debug::DRAW_ENEMY_FACING) {
        float cx = draw_x + draw_w * 0.5f;
        float cy = draw_y + draw_h * 0.5f;
        float arrow_len = 12.0f;
        float fdx = facing_dx;
        float fdy = facing_dy;
        if (fdx == 0.0f && fdy == 0.0f) { fdy = 1.0f; }

        float end_x = cx + fdx * arrow_len;
        float end_y = cy + fdy * arrow_len;
        constexpr uint32_t arrow_color = 0xFF00FF00; // Lime green

        // Shaft
        Draw::line(cx, cy, end_x, end_y, arrow_color, 2, transform.z_index + 2, sort_y);

        // Arrowhead tips
        float norm_len = std::sqrt(fdx * fdx + fdy * fdy);
        if (norm_len > 0.001f) {
            float ndx = fdx / norm_len;
            float ndy = fdy / norm_len;

            float px = -ndy;
            float py = ndx;
            float head_len = 4.0f;

            float left_x = end_x - ndx * head_len + px * head_len;
            float left_y = end_y - ndy * head_len + py * head_len;
            float right_x = end_x - ndx * head_len - px * head_len;
            float right_y = end_y - ndy * head_len - py * head_len;

            Draw::line(end_x, end_y, left_x, left_y, arrow_color, 2, transform.z_index + 2, sort_y);
            Draw::line(end_x, end_y, right_x, right_y, arrow_color, 2, transform.z_index + 2, sort_y);
        }
    }
}

} // namespace alx

