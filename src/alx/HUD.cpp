#include "alx/HUD.h"
#include <algorithm>
#include <cmath>
#include <string_view>
#include "core/Draw.h"
#include "alx/Layer.h"
#include "alx/TextStyles.h"
#include "alx/Player.h"
#include "Game.h"
#include "Debug.h"
#include "Random.h"

namespace alx::HUD {

void draw(const HUDState& state, const Menu& menu, int screen_width, int screen_height) {
    draw_in_game_bar(state, screen_width, screen_height);

    if (state.paused) {
        draw_pause_menu(menu);
    } else if (state.is_game_over) {
        if (state.game_over_fade_timer > 0.0f) {
            draw_game_over_fade(state.game_over_fade_timer, state.game_over_fade_duration, screen_width, screen_height);
        } else {
            draw_game_over_menu(menu);
        }
    } else if (state.is_victory_screen) {
        draw_victory_menu(menu);
    }
}

void draw_in_game_bar(const HUDState& state, int screen_width, int screen_height) {
    const char* selected_name = fixture_glyph(state.selected_fixture);
    int cost = state.fixture_cost;

    const int bar_w = BAR_WIDTH;
    const int bar_h = BAR_HEIGHT;
    const int bar_x = screen_width / 2 - bar_w / 2;
    const int bar_y = PADDING_VERTICAL;
    const int text_y = bar_y + (bar_h - TextStyles::font.size) / 2;

    // Left HUD Stats: HP and Alloy
    if (state.can_build) {
        // \x03 = Heart icon, \x04 = Gem/Alloy icon
        Draw::text_shadow(
            PADDING_SIDE, text_y,
            Draw::fmt("\x03 %d \x04 %d", state.player_hp, state.player_alloy),
            COLOR_TEXT, COLOR_TEXT_SHADOW, 1, Layer::HUD_Text, &TextStyles::font
        );
    } else {
        // \x03 = Heart icon
        Draw::text_shadow(
            PADDING_SIDE, text_y,
            Draw::fmt("\x03 %d", state.player_hp),
            COLOR_TEXT, COLOR_TEXT_SHADOW, 1, Layer::HUD_Text, &TextStyles::font
        );
    }

    // Center Progress Bar
    std::string_view center_str{};
    float progress = 0.0f;
    uint32_t fill_color = COLOR_BAR_FILL_TWILIGHT;

    if (state.twilight_level <= state.twilight_hold_threshold || state.victory_hold_timer > 0.0f) {
        int remaining_sec = std::clamp(
            static_cast<int>(std::ceil(state.victory_hold_duration - state.victory_hold_timer)),
            0,
            static_cast<int>(state.victory_hold_duration)
        );
        center_str = Draw::fmt("HOLD: %2ds", remaining_sec);
        progress = (state.victory_hold_duration > 0.001f)
            ? std::clamp(state.victory_hold_timer / state.victory_hold_duration, 0.0f, 1.0f)
            : 0.0f;
        fill_color = COLOR_BAR_FILL_HOLD;
    } else {
        int twilight_pct = std::max(0, static_cast<int>(state.twilight_level * 100.0f));
        const char* icon = state.twilight_level >= 0.5f ? "\x08" : "\x0F";
        center_str = Draw::fmt("%s %d%%", icon, twilight_pct);
        progress = (state.twilight_max > 0.001f)
            ? std::clamp(state.twilight_level / state.twilight_max, 0.0f, 1.0f)
            : 0.0f;
        fill_color = COLOR_BAR_FILL_TWILIGHT;
    }

    // 1. Background Track
    Draw::rect_rounded(
        static_cast<float>(bar_x), static_cast<float>(bar_y),
        static_cast<float>(bar_w), static_cast<float>(bar_h),
        BAR_CORNER_RADIUS,
        COLOR_BAR_BG, true, 1, Layer::HUD_BG
    );

    // 2. Dynamic Progress Fill (Inset by border thickness)
    const int inset = BAR_BORDER_THICKNESS;
    const int max_fill_w = bar_w - inset * 2;
    const int fill_h = bar_h - inset * 2;
    const int fill_w = static_cast<int>(std::round(max_fill_w * progress));
    if (fill_w > 0) {
        const float fill_radius = std::min(BAR_CORNER_RADIUS - 1.0f, static_cast<float>(fill_w) / 2.0f);
        Draw::rect_rounded(
            static_cast<float>(bar_x + inset), static_cast<float>(bar_y + inset),
            static_cast<float>(fill_w), static_cast<float>(fill_h),
            std::max(0.0f, fill_radius),
            fill_color, true, 1, Layer::HUD_BG
        );
    }

    // 3. Rounded Border
    Draw::rect_rounded(
        static_cast<float>(bar_x), static_cast<float>(bar_y),
        static_cast<float>(bar_w), static_cast<float>(bar_h),
        BAR_CORNER_RADIUS,
        COLOR_BAR_BORDER, false, BAR_BORDER_THICKNESS, Layer::HUD_BG
    );

    // 4. Centered Overlay Text
    int center_text_w = Draw::text_width(center_str, 1, &TextStyles::font);
    Draw::text_shadow(
        screen_width / 2 - center_text_w / 2, text_y,
        center_str,
        COLOR_TEXT, COLOR_TEXT_SHADOW, 1, Layer::HUD_Text, &TextStyles::font
    );

    // Right HUD Build Selection
    if (state.can_build) {
        std::string_view build_str = Draw::fmt("build: %s (%d)", selected_name, cost);
        int build_width = Draw::text_width(build_str, 1, &TextStyles::font);
        Draw::text_shadow(
            screen_width - PADDING_SIDE - build_width, text_y,
            build_str,
            COLOR_TEXT, COLOR_TEXT_SHADOW, 1, Layer::HUD_Text, &TextStyles::font
        );
    }

    // Bottom Debug Seed Display
    if constexpr (Debug::SHOW_SEED) {
        std::string_view seed_str = Draw::fmt(Random::is_custom_seeded() ? "seed: %u (custom)" : "seed: %u", Random::active_seed());
        int bottom_y = screen_height - TextStyles::font.size - PADDING_VERTICAL;
        Draw::text(
            PADDING_SIDE, bottom_y,
            seed_str,
            COLOR_TEXT, 1, Layer::HUD_Text, &TextStyles::font
        );
    }
}

void draw_game_over_fade(float fade_timer, float duration, int screen_width, int screen_height) {
    float fade_progress = 1.0f - std::clamp(fade_timer / duration, 0.0f, 1.0f);
    uint32_t alpha = static_cast<uint32_t>(fade_progress * 0.5f * 255.0f + 0.5f);
    uint32_t fade_color = alpha << 24;

    Draw::rect(
        0, 0,
        screen_width, screen_height,
        fade_color,
        true,
        1,
        Layer::HUD_Overlay
    );
}

void draw_overlay_menu(
    std::string_view title,
    uint32_t title_color,
    const Menu& menu,
    int screen_width,
    int screen_height,
    bool draw_backdrop
) {
    if (draw_backdrop) {
        Draw::rect(
            0, 0,
            screen_width, screen_height,
            COLOR_OVERLAY_BACKDROP,
            true,
            1,
            Layer::HUD_Overlay
        );
    }

    const int title_scale = TextStyles::scale_big;
    const int title_width = Draw::text_width(title, title_scale, &TextStyles::font);
    const int title_height = Menu::calculate_line_height(TextStyles::font.size, title_scale);

    const float title_x = Game::half_screen_width - (title_width / 2.0f);
    const float title_y = Game::half_screen_height - title_height - Menu::calculate_line_height(TextStyles::font.size, TextStyles::scale);

    Draw::text_shadow(
        title_x,
        title_y,
        title,
        title_color,
        0xFF000000,
        title_scale,
        Layer::HUD_OverlayText,
        &TextStyles::font
    );

    const float menu_start_y = Game::half_screen_height + (Menu::calculate_line_height(TextStyles::font.size, TextStyles::scale) / 2.0f);
    menu.draw({
        .center_x = static_cast<float>(Game::half_screen_width),
        .start_y = menu_start_y,
        .layer = Layer::HUD_OverlayText,
        .color = TextStyles::color,
        .color_shadow = TextStyles::color_shadow,
    });
}

void draw_game_over_menu(const Menu& game_over_menu) {
    draw_overlay_menu("YOU DIED!", COLOR_GAME_OVER_TEXT, game_over_menu, Game::WIDTH, Game::HEIGHT, true);
}

void draw_victory_menu(const Menu& victory_menu) {
    draw_overlay_menu("AETHERLUX RESTORED", COLOR_VICTORY_TEXT, victory_menu, Game::WIDTH, Game::HEIGHT, true);
}

void draw_pause_menu(const Menu& pause_menu) {
    draw_overlay_menu("PAUSED", COLOR_TEXT, pause_menu, Game::WIDTH, Game::HEIGHT, true);
}

} // namespace alx::HUD
