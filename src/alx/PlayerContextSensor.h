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
} // namespace sensor_config



class PlayerContextSensor {
private:
    float m_eval_timer_sec{0.0f};

public:
    PlayerContextSensor() = default;

    void reset() noexcept { m_eval_timer_sec = 0.0f; }

    void update(
        float dt,
        PromptOverlay& overlay,
        const Player& player,
        const Network& network,
        const EnemyManager& enemy_manager,
        const Tiles& tiles,
        bool can_build
    ) noexcept;
};

} // namespace alx
