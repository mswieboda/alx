#pragma once
#include "alx/Action.h"
#include "alx/Camera.h"

namespace alx {

struct Player : public Entity {
    // Integer pixels moved per physics tick
    // (MUST be an integer: 1 = 1px/tick, 2 = 2px/tick for pixel-perfect sync without tick wobble)
    int pixels_per_tick = 1;
    float speed = static_cast<float>(pixels_per_tick * Game::TARGET_FPS);
    int wand_radius = 96;
    bool is_panning = false;

    Player(float x = 128.0f, float y = 128.0f)
        : Entity(
            Transform{ x, y, 12, 24, 10 }, // Transform (x, y, w, h, z_index)
            RectangleRender{ 0xFFFF00FF, true, 1 },         // Visual (Magenta box representation)
            true,                                           // Active
            "player"                                        // Tag for easy lookups
          )
    {
    }

    float center_x(float alpha = 1.0f) const {
        float draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
        return draw_x + (transform.width / 2.0f);
    }

    float center_y(float alpha = 1.0f) const {
        float draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
        return draw_y + (transform.height / 2.0f);
    }

    void sync_prev_transforms() {
        transform_prev = transform;
    }

    void update(float dt, Grid& grid, const alx::Camera& camera) {
        sync_prev_transforms();

        update_movement(dt, grid, camera);
        update_actions(dt, grid);
    }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha, const alx::Camera& camera) {
        if (!active) return;

        // --- MAIN BODY ---
        float world_draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
        float world_draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);

        int draw_x = camera.to_screen_x(world_draw_x);
        int draw_y = camera.to_screen_y(world_draw_y);
        int draw_w = std::max(1, static_cast<int>(std::round(transform.width * camera.get_zoom())));
        int draw_h = std::max(1, static_cast<int>(std::round(transform.height * camera.get_zoom())));

        if (auto* rect = std::get_if<RectangleRender>(&visual)) {
            Draw::rect(
                draw_x,
                draw_y,
                draw_w,
                draw_h,
                rect->color,
                rect->fill,
                rect->thickness,
                transform.z_index
            );
        }

        // --- TARGET box for interactions ---
        float size = (transform.height / 4.0f) * camera.get_zoom();

        float target_center_x = draw_x + (draw_w / 2.0f);
        float target_center_y = draw_y + (draw_h / 1.25f);

        float box_x = target_center_x - (size / 2.0f);
        float box_y = target_center_y - (size / 2.0f);

        Draw::rect(
            static_cast<int>(std::round(box_x)),
            static_cast<int>(std::round(box_y)),
            std::max(1, static_cast<int>(std::round(size))), // width
            std::max(1, static_cast<int>(std::round(size))), // height
            0xFF990099, // color
            true, // fill
            1, // thickness (unused for fill true)
            transform.z_index // same as player
        );
    }

    int get_cursed_alloy() const { return m_cursed_alloy; }
    void add_cursed_alloy(int amount) { m_cursed_alloy += amount; }
    TileType get_selected_build_type() const { return m_selected_build_type; }

private:
    int m_cursed_alloy = 5;
    TileType m_selected_build_type = TileType::Pipe;

    void update_movement(float dt, const Grid& grid, const alx::Camera& camera) {
        // PanMode state is active while PanMode key (Shift / Q) is held
        is_panning = Action::is_pressed(Action::PanMode);

        // Suppress player entity movement while camera scouting or return decay is active
        if (camera.is_player_movement_locked()) {
            return;
        }

        float dx = 0.0f;
        float dy = 0.0f;

        if (Action::is_pressed(Action::MoveUp))    dy -= 1.0f;
        if (Action::is_pressed(Action::MoveDown))  dy += 1.0f;
        if (Action::is_pressed(Action::MoveLeft))  dx -= 1.0f;
        if (Action::is_pressed(Action::MoveRight)) dx += 1.0f;

        // Normalize diagonal movement so player doesn't move faster diagonally
        if (dx != 0.0f && dy != 0.0f) {
            constexpr float inv_sqrt2 = 0.70710678118f; // 1 / sqrt(2)
            dx *= inv_sqrt2;
            dy *= inv_sqrt2;
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
        // Cycle active build type (R-Shoulder + D / Right forward, R-Shoulder + A / Left backward)
        if (Action::is_cycle_right()) {
            if (m_selected_build_type == TileType::Pipe) {
                m_selected_build_type = TileType::Refiner;
            } else if (m_selected_build_type == TileType::Refiner) {
                m_selected_build_type = TileType::Spire;
            } else {
                m_selected_build_type = TileType::Pipe;
            }
        } else if (Action::is_cycle_left()) {
            if (m_selected_build_type == TileType::Pipe) {
                m_selected_build_type = TileType::Spire;
            } else if (m_selected_build_type == TileType::Spire) {
                m_selected_build_type = TileType::Refiner;
            } else {
                m_selected_build_type = TileType::Pipe;
            }
        }

        // Key 5 / Action DebugResource: Debug cheat +10 alloy
        if (Action::is_just_pressed(Action::DebugResource)) {
            m_cursed_alloy += 10;
        }

        // Calculate target tile index in front/center of player
        float center_x = transform.x + (transform.width / 2.0f);
        float center_y = transform.y + (transform.height / 1.25f);
        int tile_size = grid.get_tile_size();
        int target_tx = static_cast<int>(center_x) / tile_size;
        int target_ty = static_cast<int>(center_y) / tile_size;

        // Build currently selected tile type (R-Shoulder + ActionBtn)
        if (Action::is_build_tile()) {
            grid.try_place_tile(target_tx, target_ty, m_selected_build_type, m_cursed_alloy);
        }

        // Button B / Action Cancel: Drain active mana or destroy idle buildable tile with refund
        if (Action::is_just_pressed(Action::Cancel)) {
            grid.try_drain_or_destroy_tile(target_tx, target_ty, m_cursed_alloy);
        }
    }

    // Helper method checking the four corners against grid->is_walkable()
    bool is_solid_box(float x, float y, float width, float height, const Grid& grid) const {
        int tile_size = grid.get_tile_size();

        int left   = static_cast<int>(x) / tile_size;
        int right  = static_cast<int>(x + width - 0.01f) / tile_size;
        int top    = static_cast<int>(y) / tile_size;
        int bottom = static_cast<int>(y + height - 0.01f) / tile_size;

        return !grid.is_walkable(left, top) ||
               !grid.is_walkable(right, top) ||
               !grid.is_walkable(left, bottom) ||
               !grid.is_walkable(right, bottom);
    }
};

} // namespace alx
