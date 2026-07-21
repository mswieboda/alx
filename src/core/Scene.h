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

    // Returns the index of the first entity with a matching tag.
    //
    //
    // --- EXTRA NOTES ---
    // Prefer caching this in init() rather than calling every frame.
    //
    // NOTE: indices (not pointers/references) are the correct long-term handle
    // for an entity in a std::vector — they survive reallocation safely.
    // If no match is found, returns SIZE_MAX as a sentinel (check before use).
    //
    // --- REMOVING / DESPAWNING ENTITIES (e.g. bullets, enemies) ---
    // Removing elements from `entities` invalidates cached indices for any
    // entity that sits AFTER the removed element in the vector (they all shift
    // left by one). There are three practical strategies, pick by use-case:
    //
    // 1. SOFT-DELETE via `entity.active = false`  <-- RECOMMENDED for most games
    //    Don't erase at all. Just flip `entity.active = false` to stop it from
    //    being drawn or updated. draw_entities() already skips inactive entries.
    //    Indices are NEVER invalidated. Recycle slots later if memory matters.
    //    Best for bullets, enemies, particles — anything with high churn.
    //
    // 2. SWAP-AND-POP (erase without shifting)
    //    To truly remove element at index i without shifting others:
    //      std::swap(entities[i], entities.back());
    //      entities.pop_back();
    //    Only the swapped entity (previously at .back()) changes index.
    //    If you hold a cached index to it, re-assign: idx = i after the swap.
    //    Order of entities in the vector is NOT preserved (z_index handles draw order).
    //
    // 3. LINEAR ERASE + RE-CACHE (simple but O(n) per removal)
    //    Call entities.erase(entities.begin() + i), then call entity_index(tag)
    //    again for every cached index you hold. Simple, correct, slow for bulk.
    //    Fine for infrequent removals (boss death, level transition, etc.).
    //
    // --- SCALING UP (if soft-delete isn't enough) ---
    // If the vector grows unboundedly (thousands of bullets/particles), recycle
    // inactive slots instead of always pushing new ones — this is an Object Pool.
    // Scan for the first inactive entry in spawn logic and reuse it; only push_back
    // if none are free. The vector grows to a high-water mark then plateaus,
    // giving you zero heap allocations after warm-up.
    //
    // For AAA-scale needs, look up "Generational Indices" (also called Entity Handles).
    // Used by engines like Unreal, Unity DOTS, and EnTT — worth a search or AI chat.
    size_t entity_index(const std::string& tag) const;

protected:
    // Optional virtual hook for custom raw pixel drawing
    virtual void draw_custom(std::vector<uint32_t>& screen_buffer, float alpha = 1.0f);

    // The shared built-in entity rendering pipeline
    void draw_entities(std::vector<uint32_t>& screen_buffer, float alpha = 1.0f);
};
