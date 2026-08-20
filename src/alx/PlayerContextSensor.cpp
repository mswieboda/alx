#include "alx/PlayerContextSensor.h"
#include <cmath>
#include "alx/AlloyItem.h"
#include "alx/EnemyManager.h"
#include "alx/Fixture.h"
#include "alx/Network.h"
#include "alx/Player.h"
#include "alx/PromptOverlay.h"
#include "alx/Tiles.h"
#include "alx/WorldStructure.h"

namespace alx {

void PlayerContextSensor::update(
    float dt,
    PromptOverlay& overlay,
    const Player& player,
    const Network& network,
    const EnemyManager& enemy_manager,
    const Tiles& tiles,
    bool can_build,
    int level_id
) noexcept {
    // 1. Level 1 Sequential Onboarding State Machine
    if (level_id == 1) {
        m_level_elapsed_sec += dt;

        // Step 1: Sword Attack Verb Hint (0.75s delay from spawn)
        if (m_level_elapsed_sec >= sensor_config::spawn_attack_delay_sec && !overlay.has_seen(PromptId::sword_attack_hint)) {
            overlay.try_show_once(
                "Press {ATTACK} to strike",
                PromptType::info,
                PromptId::sword_attack_hint,
                0.0f,
                true
            );
        }

        // Step 2: Camera Pan Hint (3.0s delay after attack is seen/dismissed)
        if (overlay.has_seen(PromptId::sword_attack_hint) && !overlay.has_seen(PromptId::camera_pan_hint)) {
            m_post_attack_timer_sec += dt;
            if (m_post_attack_timer_sec >= sensor_config::post_attack_pan_delay_sec) {
                overlay.try_show_once(
                    "Hold {PAN} to scout ahead",
                    PromptType::info,
                    PromptId::camera_pan_hint,
                    0.0f,
                    true
                );
            }
        }

        // Step 3: Network Protection Goal Statement (3.0s delay after camera pan is seen/dismissed)
        if (overlay.has_seen(PromptId::camera_pan_hint) && !m_network_hint_shown) {
            m_post_pan_timer_sec += dt;
            if (m_post_pan_timer_sec >= sensor_config::post_pan_network_delay_sec) {
                if (overlay.try_show_once(
                    "Protect the Mana Network...",
                    PromptType::info,
                    PromptId::protect_network_hint,
                    sensor_config::network_hint_hold_duration_sec,
                    false
                )) {
                    overlay.show(
                        "to reduce the \x08 Twilight!",
                        PromptType::info,
                        PromptId::none,
                        sensor_config::network_hint_hold_duration_sec,
                        false
                    );
                }
                m_network_hint_shown = true;
            }
        }

        // Step 4: Mana Spark Hint (5.0s delay after network protection prompts finish)
        if (m_network_hint_shown && !overlay.has_seen(PromptId::mana_spark_hint)) {
            if (!overlay.is_active()) {
                m_post_network_timer_sec += dt;
            }
            if (m_post_network_timer_sec >= sensor_config::post_network_spark_delay_sec) {
                overlay.try_show_once(
                    "Hold {SPARK} to fire spark",
                    PromptType::info,
                    PromptId::mana_spark_hint,
                    0.0f,
                    true
                );
                m_spark_hint_shown = true;
            }
        }
    }

    // 2. Throttled Proximity & State Evaluation
    m_eval_timer_sec += dt;
    if (m_eval_timer_sec < sensor_config::update_interval_sec) {
        return;
    }
    m_eval_timer_sec = 0.0f;

    const float px = player.center_x(1.0f);
    const float py = player.center_y(1.0f);

    // Critical Health Alert (Tier 2)
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

    const int tile_sz = tiles.tile_size();
    const int net_w = network.width();
    const int net_h = network.height();

    // Guard infrastructure proximity hints so they only trigger after Step 3 ("Protect the Mana Network" sequence) has dismissed
    const bool network_goal_dismissed = (level_id != 1) ||
        (m_network_hint_shown && overlay.current_id() != PromptId::protect_network_hint && !overlay.is_active());

    // Refiner Proximity Hint
    if (network_goal_dismissed && !overlay.has_seen(PromptId::refiner_info)) {
        for (int ty = 0; ty < net_h; ++ty) {
            for (int tx = 0; tx < net_w; ++tx) {
                const Fixture& fix = network.fixture(tx, ty);
                if (fix.type == FixtureType::Refiner && fix.is_root()) {
                    const float fx = (static_cast<float>(tx) + 1.0f) * static_cast<float>(tile_sz);
                    const float fy = (static_cast<float>(ty) + 1.0f) * static_cast<float>(tile_sz);
                    const float dx = fx - px;
                    const float dy = fy - py;
                    if ((dx * dx + dy * dy) <= sensor_config::proximity_distance_sq) {
                        // const char* icon = fixture_glyph(fix.type)
                        // std::string_view msg = Draw::fmt("Refiner: purifies Dark Mana", icon);
                        overlay.try_show_once(
                            "Refiner: purifies Dark Mana",
                            PromptType::info,
                            PromptId::refiner_info,
                            4.0f,
                            false
                        );
                        break;
                    }
                }
            }
        }
    }

    // Spire Proximity Hint (2-part sequenced hint)
    if (network_goal_dismissed && !overlay.has_seen(PromptId::spire_info)) {
        for (int ty = 0; ty < net_h; ++ty) {
            for (int tx = 0; tx < net_w; ++tx) {
                const Fixture& fix = network.fixture(tx, ty);
                if (fix.type == FixtureType::Spire && fix.is_root()) {
                    const float fx = (static_cast<float>(tx) + 1.0f) * static_cast<float>(tile_sz);
                    const float fy = (static_cast<float>(ty) + 1.0f) * static_cast<float>(tile_sz);
                    const float dx = fx - px;
                    const float dy = fy - py;
                    if ((dx * dx + dy * dy) <= sensor_config::proximity_distance_sq) {
                        // const char* icon = fixture_glyph(fix.type)
                        // std::string_view msg = Draw::fmt("Spire: burns Light Mana, clears \x08", icon);
                        if (overlay.try_show_once(
                            "Spire: creates \x0F clearing \x08",
                            PromptType::info,
                            PromptId::spire_info,
                            3.0f,
                            false
                        ))
                        break;
                    }
                }
            }
        }
    }

    // Dark Tower Proximity Warning (Sticky until player strikes the tower)
    for (const auto& structure : enemy_manager.structures()) {
        if (structure.type == StructureType::DarkTower && structure.hp > 0) {
            const float dx = structure.center_x() - px;
            const float dy = structure.center_y() - py;
            if ((dx * dx + dy * dy) <= sensor_config::proximity_distance_sq) {
                overlay.try_show_cooldown(
                    "Strike Tower with {ATTACK}",
                    PromptType::warning,
                    PromptId::dark_tower_hint,
                    0.0f,
                    true,
                    sensor_config::tower_warning_cooldown_sec
                );
                break;
            }
        }
    }

    // Building & Logistics Contextual Hints (Level 2+)
    if (can_build) {
        // Proximity to unlinked Seeps (encourage pipe placement)
        if (!overlay.has_seen(PromptId::place_pipe_hint)) {
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
                                "Hold {PLACE} to build Pipe",
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
                            "Pickup Alloy to build",
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

