#include "SceneManager.h"
#include "Scene.h"
#include "Draw.h"

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

SceneManager::SceneManager(SceneManager&&) noexcept = default;
SceneManager& SceneManager::operator=(SceneManager&&) noexcept = default;

void SceneManager::process_pending_changes() {
    if (m_next_scene) {
        m_current_scene = std::move(m_next_scene);
        m_current_scene->init(*this);
    }
}

void SceneManager::change_scene(std::unique_ptr<Scene> new_scene) {
    m_next_scene = std::move(new_scene);
}

void SceneManager::update(float dt) {
    process_pending_changes();

    if (m_current_scene) {
        m_current_scene->sync_prev_transforms();
        m_current_scene->update_entities(dt);
        m_current_scene->update(*this, dt);
    }
}

void SceneManager::draw(std::vector<uint32_t>& pixel_buffer, float alpha) {
    if (m_current_scene) {
        m_current_scene->draw(pixel_buffer, alpha);
        Draw::flush_pipeline(pixel_buffer, m_current_scene->background_color);
    } else {
        Draw::flush_pipeline(pixel_buffer, 0xFF000000);
    }
}
