#pragma once
#include <memory>
#include <vector>
#include <cstdint>

class Scene;

class SceneManager {
private:
    std::unique_ptr<Scene> m_current_scene;
    std::unique_ptr<Scene> m_next_scene;

    void process_pending_changes();

public:
    SceneManager();
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager(SceneManager&&) noexcept;
    SceneManager& operator=(SceneManager&&) noexcept;

    void change_scene(std::unique_ptr<Scene> new_scene);
    void update(float dt);
    void draw(std::vector<uint32_t>& pixel_buffer, float alpha = 1.0f);
};
