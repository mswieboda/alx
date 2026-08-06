#include "alx/WorldStructure.h"
#include <cmath>
#include <algorithm>
#include "core/Draw.h"
#include "alx/DrawFX.h"
#include "alx/Layer.h"
#include "Debug.h"

namespace alx {

WorldStructure::WorldStructure(float px, float py, StructureType struct_type)
    : Entity(
        Transform{ px, py, DARK_TOWER_WIDTH, DARK_TOWER_HEIGHT, Layer::WorldObj },
        RectangleRender{ 0xFF120B1C, true, 1 },
        true,
        "world_structure"
      ),
      type(struct_type),
      hp(DEFAULT_DARK_TOWER_MAX_HP),
      max_hp(DEFAULT_DARK_TOWER_MAX_HP)
{
}

float WorldStructure::center_x(float alpha) const {
    float draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
    return draw_x + (transform.width * 0.5f);
}

float WorldStructure::center_y(float alpha) const {
    float draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
    return draw_y + (transform.height * 0.5f);
}

Collision::AABB WorldStructure::ground_aabb() const {
    // Full 3x4 tile solid ground footprint (48px width x 64px height)
    return Collision::AABB{
        transform.x,
        transform.y,
        DARK_TOWER_WIDTH,
        DARK_TOWER_HEIGHT
    };
}

void WorldStructure::sync_prev_transforms() {
    transform_prev = transform;
}

void WorldStructure::take_damage(int amount) {
    hp = std::max(0, hp - amount);
    hit_flash_timer = 0.15f;
}

void WorldStructure::update(float dt) {
    sync_prev_transforms();

    pulse_timer += dt;
    if (hit_flash_timer > 0.0f) {
        hit_flash_timer = std::max(0.0f, hit_flash_timer - dt);
    }
}

void WorldStructure::draw(std::vector<uint32_t>& screen_buffer, float alpha) const {
    if (!active) return;

    if (type == StructureType::DarkTower) {
        draw_dark_tower(screen_buffer, alpha);
    }
}

void WorldStructure::draw_dark_tower(std::vector<uint32_t>& screen_buffer, float alpha) const {
    float world_draw_x = Draw::interpolate(transform_prev.x, transform.x, alpha);
    float world_draw_y = Draw::interpolate(transform_prev.y, transform.y, alpha);
    float world_draw_w = transform.width;
    float world_draw_h = transform.height;
    float world_bottom_y = world_draw_y + world_draw_h;

    // Roof baseline Y-sort line (bottom edge of top tile row at world_draw_y + 16px)
    // Allows characters standing alongside middle/lower tiles to render in front of body walls
    int roof_sort_y = static_cast<int>(world_draw_y + 16.0f);

    if (Debug::DRAW_WORLD_STRUCTURE_TEST) {
        // world_bottom_y debug indicator line
        Draw::rect(
            world_draw_x - 8,
            world_bottom_y - 1,
            world_draw_w + 16,
            1, // h
            0xFF00FF00,
            true, 1, // fill, thick (ignored)
            2 // z
        );
    }

    // 1. Base foreshortened shadow oval at bottom Y edge (Layer::WorldObjBG = 9, roof_sort_y)
    DrawFX::shadow(
        world_draw_x,
        world_draw_y,
        world_draw_w,
        world_draw_h,
        Layer::WorldObjBG,
        roof_sort_y,
        0.6f,
        0.3f
    );

    // 2. Main obsidian monolith body column (3x4 tile rect)
    uint32_t body_color = 0xFF120B1C; // Deep Dusky Obsidian
    if (hit_flash_timer > 0.0f) {
        body_color = 0xFF661188; // Vibrant Purple hit flash
    }

    Draw::rect(
        world_draw_x,
        world_draw_y,
        world_draw_w,
        world_draw_h,
        body_color,
        true, 1,
        transform.z_index,
        roof_sort_y
    );

    // 3. Dark trim accent border (uses transform.z_index and roof_sort_y to match building layering)
    Draw::rect(
        world_draw_x,
        world_draw_y,
        world_draw_w,
        world_draw_h,
        0xFF2A153D,
        false, 2,
        transform.z_index,
        roof_sort_y
    );

    // 4. Sharp spired peak extending -12px above top edge into upper tile row
    // Top triangle spire lines use Layer::WorldObjSpireTop (19) so player and attack FX pass behind top spire peak when in upper row
    float top_cx = world_draw_x + world_draw_w * 0.5f;
    float top_y = world_draw_y - 12.0f; // Peak extends 12px above top tile edge
    float shoulder_left_x = world_draw_x;
    float shoulder_right_x = world_draw_x + world_draw_w;
    float shoulder_y = world_draw_y;

    Draw::line(top_cx, top_y, shoulder_left_x, shoulder_y, 0xFF2A153D, 2, Layer::WorldObjSpireTop, roof_sort_y);
    Draw::line(top_cx, top_y, shoulder_right_x, shoulder_y, 0xFF2A153D, 2, Layer::WorldObjSpireTop, roof_sort_y);

    // 5. Pulsing violet core diamond in upper section of tower
    float core_cx = top_cx;
    float core_cy = world_draw_y + 24.0f;
    float pulse = std::sin(pulse_timer * 3.0f) * 0.5f + 0.5f; // 0.0 to 1.0 sine pulse
    float core_r = 6.0f + pulse * 2.0f;

    uint32_t core_color = 0xFF8800AA; // Violet magenta
    Draw::circle(
        core_cx,
        core_cy,
        core_r,
        core_color,
        true, 1,
        transform.z_index,
        roof_sort_y
    );

    Draw::circle(
        core_cx,
        core_cy,
        core_r + 2.0f,
        0xFFCC44FF, // Bright pulse aura outline
        false, 1,
        transform.z_index,
        roof_sort_y
    );

    // Ground footprint debug outline if DRAW_WORLD_STRUCTURE_COLLISION_AREAS enabled
    if (Debug::DRAW_WORLD_STRUCTURE_COLLISION_AREAS) {
        Collision::AABB g_aabb = ground_aabb();
        Draw::rect(
            g_aabb.x, g_aabb.y, g_aabb.w, g_aabb.h,
            0xFFFF00FF, false, 1,
            transform.z_index + 3, roof_sort_y
        );
    }
}

} // namespace alx
