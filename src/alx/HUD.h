#pragma once

#include <cstdint>
#include <string_view>
#include "alx/Fixture.h"
#include "alx/Menu.h"

namespace alx {

struct HUDState {
    int player_hp{0};
    int player_alloy{0};
    FixtureType selected_fixture{FixtureType::None};
    int fixture_cost{0};
    float twilight_level{0.0f};
    float victory_hold_timer{0.0f};
    float victory_hold_duration{15.0f};
    float twilight_hold_threshold{0.0f};
    float twilight_max{0.99f};
    bool can_build{false};
    bool paused{false};
    bool victory{false};
    bool is_game_over{false};
    float game_over_fade_timer{0.0f};
    float game_over_fade_duration{0.5f};
};

namespace HUD {

// Visual Styling & Layout Constants
inline constexpr int BAR_WIDTH = 120;
inline constexpr int BAR_HEIGHT = 16;
inline constexpr float BAR_CORNER_RADIUS = 3.0f;
inline constexpr int BAR_BORDER_THICKNESS = 2;
inline constexpr uint32_t COLOR_BAR_BG = 0xCC00141C;
inline constexpr uint32_t COLOR_BAR_BORDER = 0xFF004455;
inline constexpr uint32_t COLOR_BAR_FILL_HOLD = 0xDD33FFFF;
inline constexpr uint32_t COLOR_BAR_FILL_TWILIGHT = 0xCC662288;
inline constexpr uint32_t COLOR_TEXT = 0xFF00CCCC;
inline constexpr uint32_t COLOR_TEXT_SHADOW = 0xFF003344;
inline constexpr uint32_t COLOR_VICTORY_TEXT = 0xFF00FF88;
inline constexpr uint32_t COLOR_GAME_OVER_TEXT = 0xFF66001C;
inline constexpr uint32_t COLOR_PAUSE_TEXT = 0xFFFFCC00;
inline constexpr int PADDING_SIDE = 6;
inline constexpr int PADDING_VERTICAL = 4;

void draw(const HUDState& state, const Menu& game_over_menu, int screen_width, int screen_height);
void draw_in_game_bar(const HUDState& state, int screen_width, int screen_height);
void draw_game_over_fade(float fade_timer, float duration, int screen_width, int screen_height);
void draw_game_over_menu(const Menu& game_over_menu);
void draw_victory_and_pause_overlays(bool victory, bool paused, int screen_width, int screen_height);

} // namespace HUD

} // namespace alx
