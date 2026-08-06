#pragma once
#include <cstdint>
#include <vector>
#include "core/Entity.h"
#include "core/Collision.h"
#include "alx/Layer.h"

namespace alx {

enum class StructureType : uint8_t {
    DarkTower
};

struct WorldStructure : public Entity {
    static constexpr float DARK_TOWER_WIDTH = 48.0f;  // 3 tiles (16px * 3)
    static constexpr float DARK_TOWER_HEIGHT = 64.0f; // 4 tiles (16px * 4)
    static constexpr int DEFAULT_DARK_TOWER_MAX_HP = 8;

    StructureType type = StructureType::DarkTower;
    int hp = DEFAULT_DARK_TOWER_MAX_HP;
    int max_hp = DEFAULT_DARK_TOWER_MAX_HP;
    float spawn_timer = 0.0f;
    float pulse_timer = 0.0f;
    float hit_flash_timer = 0.0f;

    WorldStructure(float px = 0.0f, float py = 0.0f, StructureType struct_type = StructureType::DarkTower);

    float center_x(float alpha = 1.0f) const;
    float center_y(float alpha = 1.0f) const;

    Collision::AABB ground_aabb() const;
    void sync_prev_transforms();
    void take_damage(int amount);

    void update(float dt);
    void draw(std::vector<uint32_t>& screen_buffer, float alpha) const;

private:
    void draw_dark_tower(std::vector<uint32_t>& screen_buffer, float alpha) const;
};

} // namespace alx
