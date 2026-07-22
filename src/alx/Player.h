#pragma once
#include "core/Entity.h"
#include <MiniFB.h>

namespace alx {

struct Player : public Entity {
    float speed = 128.0f;
    float vx = 0.0f;
    float vy = 0.0f;

    Player(float startX, float startY)
        : Entity(
            Transform{ startX, startY, 16.0f, 16.0f, 10 }, // Transform (x, y, w, h, z_index)
            RectangleRender{ 0xFFFF00FF, true, 1 },         // Visual (Magenta box representation)
            true,                                           // Active
            "player"                                        // Tag for easy lookups
          )
    {
    }

    void sync_prev_transforms() {
        transform_prev = transform;
    }

    void update(float dt) {
        sync_prev_transforms();

        float dx = 0.0f;
        float dy = 0.0f;

        if (Input::is_key_pressed(MFB_KB_KEY_W) || Input::is_key_pressed(MFB_KB_KEY_UP))    dy -= 1.0f;
        if (Input::is_key_pressed(MFB_KB_KEY_S) || Input::is_key_pressed(MFB_KB_KEY_DOWN))  dy += 1.0f;
        if (Input::is_key_pressed(MFB_KB_KEY_A) || Input::is_key_pressed(MFB_KB_KEY_LEFT))  dx -= 1.0f;
        if (Input::is_key_pressed(MFB_KB_KEY_D) || Input::is_key_pressed(MFB_KB_KEY_RIGHT)) dx += 1.0f;

        // Normalize diagonal movement so player doesn't move faster diagonally
        if (dx != 0.0f && dy != 0.0f) {
            float inv_length = 0.7071f; // ~1 / sqrt(2)
            dx *= inv_length;
            dy *= inv_length;
        }

        // Apply movement scaled by delta time
        transform.x += dx * speed * dt;
        transform.y += dy * speed * dt;
    }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha) {
        if (!active) return;

        float draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
        float draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);

        if (auto* rect = std::get_if<RectangleRender>(&visual)) {
            Draw::rect(
                (int)draw_x,
                (int)draw_y,
                (int)transform.width,
                (int)transform.height,
                rect->color,
                rect->fill,
                rect->thickness,
                transform.z_index
            );
        }
    }
};

} // namespace alx
