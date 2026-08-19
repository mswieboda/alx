#include "alx/PlayerContextSensor.h"
#include <cmath>
#include "alx/AlloyItem.h"
#include "alx/EnemyManager.h"
#include "alx/Fixture.h"
#include "alx/Network.h"
#include "alx/Player.h"
#include "alx/PromptOverlay.h"
#include "alx/Tiles.h"

namespace alx {

void PlayerContextSensor::update(
    float dt,
    PromptOverlay& overlay,
    const Player& player,
    const Network& network,
    const EnemyManager& enemy_manager,
    const Tiles& tiles,
    bool can_build
) noexcept {
    m_eval_timer_sec += dt;
    if (m_eval_timer_sec < sensor_config::update_interval_sec) {
        return;
    }
    m_eval_timer_sec = 0.0f;

    const float px = player.center_x(1.0f);
    const float py = player.center_y(1.0f);

    // 1. Critical Health Alert (Tier 2)
    if (player.state.hp == 1 && !player.state.defeated) {
        overlay.try_show_cooldown(
            "WARNING: Low health!",
            PromptType::alert,
            PromptId::player_low_hp_alert,
            sensor_config::low_hp_hold_duration_sec,
            false,
            sensor_config::low_hp_cooldown_sec
        );
    }

    // 2. Progression-Gated Building & Logistics Contextual Hints
    if (can_build) {
        // Proximity to unlinked Seeps (encourage pipe placement)
        if (!overlay.has_seen(PromptId::place_pipe_hint)) {
            const int tile_sz = tiles.tile_size();
            const int net_w = network.width();
            const int net_h = network.height();

            for (int ty = 0; ty < net_h; ++ty) {
                for (int tx = 0; tx < net_w; ++tx) {
                    const Fixture& fix = network.fixture(tx, ty);
                    if (fix.type == FixtureType::Seep && fix.is_root() && fix.flow_out_mask == 0) {
                        const float fx = (static_cast<float>(tx) + 1.5f) * static_cast<float>(tile_sz);
                        const float fy = (static_cast<float>(ty) + 1.0f) * static_cast<float>(tile_sz);
                        const float dx = fx - px;
                        const float dy = fy - py;
                        if ((dx * dx + dy * dy) <= sensor_config::proximity_distance_sq) {
                            overlay.try_show_once(
                                "Hold {PLACE} Lay Pipe",
                                PromptType::info,
                                PromptId::place_pipe_hint,
                                0.0f,
                                true
                            );
                            break;
                        }
                    }
                }
            }
        }

        // Proximity to Alloy items when player has low alloy (< 5)
        if (player.cursed_alloy() < sensor_config::alloy_low_threshold && !overlay.has_seen(PromptId::mine_alloy_hint)) {
            for (const auto& item : enemy_manager.alloy_items()) {
                if (item.active) {
                    const float dx = item.center_x() - px;
                    const float dy = item.center_y() - py;
                    if ((dx * dx + dy * dy) <= sensor_config::proximity_distance_sq) {
                        overlay.try_show_once(
                            "Pickup Alloy To Build",
                            PromptType::info,
                            PromptId::mine_alloy_hint,
                            0.0f,
                            true
                        );
                        break;
                    }
                }
            }
        }
    }
}

} // namespace alx
