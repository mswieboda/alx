#pragma once
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <cstdint>
#include <string>
#include "Entity.h"
#include "Draw.h"

class SceneManager; // Forward declaration

class Scene {
protected:
    std::vector<Entity> entities; // Common entity storage for any scene
    mutable std::unordered_map<std::string, size_t> m_tag_to_index;
    mutable size_t m_cached_entities_size = 0;
    mutable bool m_tag_map_dirty = true;

public:
    uint32_t background_color = 0xFF000000;

    virtual ~Scene() = default;

    // Core Lifecycle Hooks
    virtual void init(SceneManager& sm) = 0;
    virtual void update(SceneManager& sm, float dt) = 0;

    // High-level draw wrapper called by SceneManager
    virtual void draw(std::vector<uint32_t>& screen_buffer, float alpha = 1.0f);

    // High-level update wrapper to process internal engine systems (like animations)
    // Called automatically by SceneManager; child scenes should not call this.
    void update_entities(float dt);

    // Copy current transform to transform_prev for all active entities
    void sync_prev_transforms();

    // Get the entity via tag from list of entities, cached
    size_t entity_index(const std::string& tag) const;

protected:
    // Optional virtual hook for custom raw pixel drawing
    virtual void draw_custom(std::vector<uint32_t>& screen_buffer, float alpha = 1.0f);

    // The shared built-in entity rendering pipeline
    void draw_entities(std::vector<uint32_t>& screen_buffer, float alpha = 1.0f);
};
