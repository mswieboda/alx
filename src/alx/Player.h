#pragma once
#include <MiniFB.h>
#include "core/Entity.h"
#include "Grid.h"

namespace alx {

struct Player : public Entity {
    float speed = 128.0f;
    float vx = 0.0f;
    float vy = 0.0f;

    Player(float startX, float startY)
        : Entity(
            Transform{ startX, startY, 24, 48, 10 }, // Transform (x, y, w, h, z_index)
            RectangleRender{ 0xFFFF00FF, true, 1 },         // Visual (Magenta box representation)
            true,                                           // Active
            "player"                                        // Tag for easy lookups
          )
    {
    }

    void sync_prev_transforms() {
        transform_prev = transform;
    }

    void update(float dt, Grid& grid) {
        sync_prev_transforms();

        update_movement(dt, grid);
        update_actions(dt, grid);
    }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha) {
        if (!active) return;

        // --- MAIN BODY ---
        int draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
        int draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);

        if (auto* rect = std::get_if<RectangleRender>(&visual)) {
            Draw::rect(
                draw_x,
                draw_y,
                (int)transform.width,
                (int)transform.height,
                rect->color,
                rect->fill,
                rect->thickness,
                transform.z_index
            );
        }

        // --- TARGET box for interactions ---
        float size = transform.height / 4.0f;

        // TODO: cx, cy need to be synced with `update_actions`, make shared method
        float center_x = draw_x + (transform.width / 2.0f);
        float center_y = draw_y + (transform.height / 1.25f);

        float box_x = center_x - (size / 2.0f);
        float box_y = center_y - (size / 2.0f);

        Draw::rect(
            (int)box_x,
            (int)box_y,
            (int)size, // width
            (int)size, // height
            0xFF990099, // color
            true, // fill
            1, // thickness (unused for fill true)
            transform.z_index // same as player
        );
    }

private:
    void update_movement(float dt, const Grid& grid) {
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

        // --- AXIS-BY-AXIS COLLISION RESOLUTION ---

        // Try moving along the X axis first
        float target_x = transform.x + dx * speed * dt;
        if (!is_solid_box(target_x, transform.y, transform.width, transform.height, grid)) {
            transform.x = target_x;
        }

        // Try moving along the Y axis independently
        float target_y = transform.y + dy * speed * dt;
        if (!is_solid_box(transform.x, target_y, transform.width, transform.height, grid)) {
            transform.y = target_y;
        }
    }

    void update_actions(float dt, Grid& grid) {
        // Check for interaction key (e.g., 'E')
        if (Input::is_key_just_pressed(MFB_KB_KEY_E)) {
            // Calculate the center point of the player's bounding box
            float center_x = transform.x + (transform.width / 2.0f);
            float center_y = transform.y + (transform.height / 1.25f);

            // Convert world space coordinates to grid tile indices
            // TODO: maybe make this a helper in Grid.h or something
            int tile_size = grid.get_tile_size();
            int target_tx = static_cast<int>(center_x) / tile_size;
            int target_ty = static_cast<int>(center_y) / tile_size;

            // Tell the grid to toggle the pipe at that location
            // TODO: check for player inventory, if they have pipes available, etc
            grid.toggle_conduit(target_tx, target_ty);
        }
    }

    // Helper method checking the four corners against grid->is_walkable()
    bool is_solid_box(float x, float y, float width, float height, const Grid& grid) const {
        int tile_size = grid.get_tile_size();

        int left   = static_cast<int>(x) / tile_size;
        int right  = static_cast<int>(x + width - 1) / tile_size;
        int top    = static_cast<int>(y) / tile_size;
        int bottom = static_cast<int>(y + height - 1) / tile_size;

        return !grid.is_walkable(left, top) ||
               !grid.is_walkable(right, top) ||
               !grid.is_walkable(left, bottom) ||
               !grid.is_walkable(right, bottom);
    }
};

} // namespace alx
