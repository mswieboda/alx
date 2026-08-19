#pragma once

#include <cstdint>
#include <string_view>
#include "alx/Fixture.h"
#include "alx/Menu.h"
#include "alx/TwilightMomentumTracker.h"

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
    bool is_victory_screen{false};
    TwilightMomentumState momentum{};
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
inline constexpr uint32_t COLOR_OVERLAY_BACKDROP = 0x80000000;
inline constexpr int PADDING_SIDE = 6;
inline constexpr int PADDING_VERTICAL = 4;

// Twilight Momentum Barometer Styling & Layout Constants
inline constexpr int momentum_offset_y = 2;
inline constexpr uint32_t color_momentum_light_flash = 0xFFFFFFFF;
inline constexpr uint32_t color_momentum_light_vivid = 0xFF00E5FF;
inline constexpr uint32_t color_momentum_light_muted = 0xFF40A8C0;
inline constexpr uint32_t color_momentum_equilibrium = 0xFF4A6B82;
inline constexpr uint32_t color_momentum_tw_muted    = 0xFF7A4B9E;
inline constexpr uint32_t color_momentum_tw_vivid    = 0xFF9B30FF;
inline constexpr uint32_t color_momentum_tw_flash    = 0xFFD154FF;

void draw(const HUDState& state, const Menu& menu, int screen_width, int screen_height);
void draw_in_game_bar(const HUDState& state, int screen_width, int screen_height);
void draw_momentum_barometer(const HUDState& state, int screen_width, int screen_height);
void draw_game_over_fade(float fade_timer, float duration, int screen_width, int screen_height);
void draw_overlay_menu(
    std::string_view title,
    uint32_t title_color,
    const Menu& menu,
    int screen_width = Game::WIDTH,
    int screen_height = Game::HEIGHT,
    bool draw_backdrop = true
);
void draw_game_over_menu(const Menu& game_over_menu);
void draw_victory_menu(const Menu& victory_menu);
void draw_pause_menu(const Menu& pause_menu);

} // namespace HUD

} // namespace alx
