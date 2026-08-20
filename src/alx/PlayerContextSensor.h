#pragma once

#include <cstdint>

namespace alx {

class PromptOverlay;
struct Player;
class Network;
class EnemyManager;
class Tiles;

namespace sensor_config {
    inline constexpr float update_interval_sec = 0.25f;
    inline constexpr float proximity_distance_px = 48.0f; // 3 tiles @ 16px
    inline constexpr float proximity_distance_sq = proximity_distance_px * proximity_distance_px;
    inline constexpr float low_hp_cooldown_sec = 15.0f;
    inline constexpr float low_hp_hold_duration_sec = 3.0f;
    inline constexpr int alloy_low_threshold = 5;

    // Level 1 Tutorial Sequence Timing Constants
    inline constexpr float spawn_attack_delay_sec = 1.75f;
    inline constexpr float post_attack_pan_delay_sec = 3.0f;
    inline constexpr float post_pan_network_delay_sec = 5.0f;
    inline constexpr float post_network_spark_delay_sec = 7.0f;
    inline constexpr float network_hint_hold_duration_sec = 3.5f;
    inline constexpr float tower_warning_hold_duration_sec = 3.5f;
    inline constexpr float tower_warning_cooldown_sec = 20.0f;
} // namespace sensor_config

class PlayerContextSensor {
private:
    float m_eval_timer_sec{0.0f};
    float m_level_elapsed_sec{0.0f};
    float m_post_attack_timer_sec{0.0f};
    float m_post_pan_timer_sec{0.0f};
    float m_post_network_timer_sec{0.0f};
    bool m_network_hint_shown{false};
    bool m_spark_hint_shown{false};

public:
    PlayerContextSensor() = default;

    void reset() noexcept {
        m_eval_timer_sec = 0.0f;
        m_level_elapsed_sec = 0.0f;
        m_post_attack_timer_sec = 0.0f;
        m_post_pan_timer_sec = 0.0f;
        m_post_network_timer_sec = 0.0f;
        m_network_hint_shown = false;
        m_spark_hint_shown = false;
    }

    void update(
        float dt,
        PromptOverlay& overlay,
        const Player& player,
        const Network& network,
        const EnemyManager& enemy_manager,
        const Tiles& tiles,
        bool can_build,
        int level_id = 1
    ) noexcept;
};

} // namespace alx

