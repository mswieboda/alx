#pragma once
#include <memory>
#include <vector>
#include <cstdint>

class Scene;

enum class TransitionState {
    None,
    FadingOut,
    FadingIn
};

class SceneManager {
    static constexpr float fade_duration = 0.2f; // secs for fade in/out

private:
    std::unique_ptr<Scene> m_current_scene;
    std::unique_ptr<Scene> m_next_scene;

    TransitionState m_transition_state{TransitionState::None};
    float m_fade_timer{0.0f};

    void fade_and_swap(float dt);
    void draw_fade(float alpha);

public:
    bool m_is_quit = false;

    SceneManager();
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager(SceneManager&&) noexcept;
    SceneManager& operator=(SceneManager&&) noexcept;

    void change_scene(std::unique_ptr<Scene> new_scene, bool force = false);
    void update(float dt);
    void draw(std::vector<uint32_t>& pixel_buffer, float alpha = 1.0f);

    bool is_in_transition() const { return m_transition_state != TransitionState::None; }
};
