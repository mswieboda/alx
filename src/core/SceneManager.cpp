#include "core/SceneManager.h"
#include "core/Scene.h"
#include "core/Draw.h"
#include "core/Log.h"

#include "Game.h" // TODO: causes reliance on game-specific
#include "alx/Layer.h" // TODO: causes reliance on game-specific

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

SceneManager::SceneManager(SceneManager&&) noexcept = default;
SceneManager& SceneManager::operator=(SceneManager&&) noexcept = default;

void SceneManager::fade_and_swap(float dt) {
    if (m_transition_state == TransitionState::None) {
        return;
    }

    m_fade_timer += dt;

    if (m_fade_timer < FADE_DURATION) return;

    if (m_transition_state == TransitionState::FadingOut) {
        Log::debug_t("[SceneManager::fade_and_swap] >>> swap to m_next_scene");
        m_current_scene = std::move(m_next_scene);
        m_current_scene->init(*this);
        m_transition_state = TransitionState::FadingIn;
        m_fade_timer = 0.0f;
    } else { // FadingIn (can't be None via way `update` calls `fade_and_swap`)
        m_transition_state = TransitionState::None;
    }
}

void SceneManager::change_scene(std::unique_ptr<Scene> new_scene) {
    Log::debug_t("[SceneManager::change_scene] >>> init fade, set m_next_scene");
    m_transition_state = TransitionState::FadingOut;
    m_fade_timer = 0.0f;
    m_next_scene = std::move(new_scene);
}

void SceneManager::update(float dt) {
    fade_and_swap(dt);

    if (m_current_scene) {
        m_current_scene->sync_prev_transforms();
        m_current_scene->update_entities(dt);
        m_current_scene->update(*this, dt);
    }
}

void SceneManager::draw(std::vector<uint32_t>& pixel_buffer, float alpha) {
    if (m_current_scene) {
        m_current_scene->draw(pixel_buffer, alpha);
        draw_fade(alpha);
        Draw::flush_pipeline(pixel_buffer, m_current_scene->background_color);
    } else {
        Draw::flush_pipeline(pixel_buffer, 0xFF000000);
    }
}

void SceneManager::draw_fade(float alpha) {
    if (m_transition_state == TransitionState::None) return;

    float dft = std::clamp(m_fade_timer / FADE_DURATION, 0.0f, 1.0f);
    bool fading_in = m_transition_state == TransitionState::FadingIn;
    float fade_percent = fading_in ? (1.0f - dft) : dft;

    // if (fade_percent <= 0.0f || fade_percent >= 1.0f) return;

    uint32_t a_color = (uint32_t) (fade_percent * 255.0f + 0.5f); // rounded

    // NOTE: assumes a full black RBG, if we want a custom fade color
    // we need to pack an alpha color into an existing AARBG (overriding AA) via different bitwise math
    uint32_t fade_color = a_color << 24;

    int per_i = (int) (fade_percent * 100 + 0.5f);
    if (per_i % 5 == 0) {
        Log::debug_fmt_t("per_i: %d dft: %3.3f fade_percent: %3.3f fade_color: 0x%08X", per_i, dft, fade_percent, fade_color);
    }

    Draw::rect(
        0, 0,
        Game::WIDTH, Game::HEIGHT, // TODO: causes reliance on game-specific
        fade_color,
        true, // fill
        1, // thickness (ignored)
        alx::Layer::SceneFade // TODO: z-index - causes reliance on game-specific
    );
}