#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "core/Draw.h"
#include "alx/Camera.h"

#include "core/Collision.h"

namespace alx {

struct EnemyConstants {
    static constexpr float DEFAULT_WIDTH = 16.0f;
    static constexpr float DEFAULT_HEIGHT = 16.0f;
    static constexpr uint32_t COLOR = 0xFF800080; // Dusky Purple
    static constexpr int DEFAULT_MAX_HP = 3;
    static constexpr float KNOCKBACK_DIST = 2.0f;
    static constexpr float MOVEMENT_RADIUS = 6.0f;
    static constexpr float HURT_RADIUS = 7.0f;
};

struct Enemy {
    float x = 0.0f;
    float y = 0.0f;
    float width = EnemyConstants::DEFAULT_WIDTH;
    float height = EnemyConstants::DEFAULT_HEIGHT;
    uint32_t color = EnemyConstants::COLOR;
    int hp = EnemyConstants::DEFAULT_MAX_HP;

    Enemy(float px = 0.0f, float py = 0.0f, float w = EnemyConstants::DEFAULT_WIDTH, float h = EnemyConstants::DEFAULT_HEIGHT, uint32_t col = EnemyConstants::COLOR, int max_hp = EnemyConstants::DEFAULT_MAX_HP)
        : x(px), y(py), width(w), height(h), color(col), hp(max_hp) {}

    float center_x() const {
        return x + (width / 2.0f);
    }

    float center_y() const {
        return y + (height / 2.0f);
    }

    Collision::Circle get_movement_circle() const {
        return Collision::Circle{ x + (width / 2.0f), y + 12.0f, EnemyConstants::MOVEMENT_RADIUS };
    }

    Collision::Circle get_hurt_circle() const {
        return Collision::Circle{ x + (width / 2.0f), y + (height / 2.0f), EnemyConstants::HURT_RADIUS };
    }

    void take_damage(int amount, float push_dx, float push_dy) {
        hp -= amount;
        x += push_dx * EnemyConstants::KNOCKBACK_DIST;
        y += push_dy * EnemyConstants::KNOCKBACK_DIST;
    }

    bool is_dead() const {
        return hp <= 0;
    }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha, const alx::Camera& camera) const {
        int draw_x = camera.to_screen_x(x);
        int draw_y = camera.to_screen_y(y);
        int draw_w = std::max(1, static_cast<int>(std::round(width * camera.get_zoom())));
        int draw_h = std::max(1, static_cast<int>(std::round(height * camera.get_zoom())));

        Draw::rect(
            draw_x,
            draw_y,
            draw_w,
            draw_h,
            color,
            true, // fill
            1,    // thickness
            5     // z-index
        );
    }
};

} // namespace alx
