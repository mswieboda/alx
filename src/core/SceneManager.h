#pragma once
#include <memory>
#include "Scene.h"

class SceneManager {
private:
    std::unique_ptr<Scene> m_current_scene;
    std::unique_ptr<Scene> m_next_scene;

    void process_pending_changes() {
        if (m_next_scene) {
            m_current_scene = std::move(m_next_scene);
            m_current_scene->init(*this);
        }
    }

public:
    void change_scene(std::unique_ptr<Scene> new_scene) {
        // Queue the scene change to avoid deleting a scene mid-update loop
        m_next_scene = std::move(new_scene);
    }

    void update(float dt) {
        process_pending_changes();

        // Inside SceneManager::update (or main loop)
        if (m_current_scene) {
            // Capture current transforms as previous
            m_current_scene->sync_prev_transforms();

            // Run internal entity timing (animations for now)
            m_current_scene->update_entities(dt);

            // Run custom scene game play
            m_current_scene->update(*this, dt);
        }
    }

    void draw(std::vector<uint32_t>& pixel_buffer, float alpha = 1.0f) {
        if (m_current_scene) {
            m_current_scene->draw(pixel_buffer, alpha);

            Draw::flush_pipeline(pixel_buffer, m_current_scene->background_color);
        } else {
            Draw::flush_pipeline(pixel_buffer, 0xFF000000);
        }
    }
};
