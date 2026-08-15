#include "alx/StartScene.h"

#include <array>
#include <string_view>

#include "Game.h"
#include "core/Input.h"
#include "core/SceneManager.h"
#include "alx/Action.h"
#include "alx/Layer.h"
#include "alx/MainScene.h"
#include "alx/TextStyles.h"

namespace alx {
  namespace {
    constexpr std::array<std::string_view, static_cast<size_t>(MenuItem::Count)> menu_items = {
        "Start", "Options", "Quit"
    };

    int line_height(uint8_t size, int scale, int text_line_height = 0) {
        int line_h_padding = text_line_height - size;
        return size * scale + line_h_padding * scale;
    }

    void draw_menu_item(std::string_view text, int scale, int text_y) {
        int text_width = Draw::text_width(text, scale, &TextStyles::font);
        Draw::text_shadow(
            Game::half_screen_width - text_width / 2.0f, // x
            text_y, // y
            text,
            TextStyles::color, TextStyles::color_shadow, // colors
            scale, Layer::HUD_Text, &TextStyles::font // font scale, font
        );
    }
  }

void StartScene::init(SceneManager& sm) {
    background_color = 0xFF131313; // very dark gray

    // TODO: probably can use the default?
    // Draw::set_y_sort_mode(Draw::YSortMode::YPlusHeight);

    m_selected_index = 3;
}

void StartScene::update(SceneManager& sm, float raw_dt) {
    // TODO: temporary ESC before we have menu items
    if (Input::is_key_just_pressed(KeyCode::Escape)) {
        sm.m_is_quit = true;
    }

    if (Action::is_just_pressed(Action::Menu)) {
        auto main_scene = std::make_unique<alx::MainScene>();
        sm.change_scene(std::move(main_scene));
    }
}

void StartScene::draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) {
    int text_y = 0;

    // x centered, y centered qtr half screen down
    int title_scale = TextStyles::scale_big;
    int title_width = Draw::text_width(Game::TITLE, title_scale, &TextStyles::font);
    Draw::text_shadow(
        Game::half_screen_width - title_width / 2.0f, // x
        Game::qtr_screen_height - line_height(TextStyles::font.size, title_scale) / 2.0f, // y
        Game::TITLE,
        TextStyles::color, TextStyles::color_shadow, // colors
        title_scale, Layer::HUD_Text, &TextStyles::font // scale, z-index, font
    );

    // x,y centered screen center
    text_y = Game::half_screen_height - line_height(TextStyles::font.size, TextStyles::scale) / 2.0f;
    for (size_t i = 0; i < static_cast<size_t>(MenuItem::Count); i++) {
        std::string_view text = menu_items[i];
        draw_menu_item(text, TextStyles::scale_med, text_y);
        text_y += line_height(TextStyles::font.size, TextStyles::scale_med, TextStyles::menu_item_text_line_height);
    }
}

} // namespace alx
