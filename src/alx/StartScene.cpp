#include "alx/StartScene.h"

#include <array>
#include <string_view>

#include "Debug.h"
#include "Game.h"
#include "core/Log.h"
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
    background_color = 0xFF090909; // very dark gray
}

void StartScene::update(SceneManager& sm, float raw_dt) {
    // TODO: keeping ESC in here, not sure if i want to kill that later
    // but it's WAYYY faster for testing, maybe in debug only?
    if (Debug::QUIT_ON_ESC && Input::is_key_just_pressed(KeyCode::Escape)) {
        sm.m_is_quit = true;
    } else if (Action::is_just_pressed(Action::Menu)) {
        if (m_selected_item == MenuItem::Start) {
            // TODO: this needs to do the menu item we selected
            auto main_scene = std::make_unique<alx::MainScene>();
            sm.change_scene(std::move(main_scene));
        } else if (m_selected_item == MenuItem::Quit) {
            sm.m_is_quit = true;
        }
    } else if (Action::is_just_pressed(Action::MoveUp)) {
        auto idx = static_cast<uint8_t>(m_selected_item);
        idx = idx == 0 ? static_cast<uint8_t>(MenuItem::Count) - 1 : idx - 1;
        m_selected_item = static_cast<MenuItem>(idx);
    } else if (Action::is_just_pressed(Action::MoveDown)) {
        auto idx = static_cast<uint8_t>(m_selected_item);
        idx = idx + 1 >= static_cast<uint8_t>(MenuItem::Count) ? 0 : idx + 1;
        m_selected_item = static_cast<MenuItem>(idx);
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
    for (size_t idx = 0; idx < static_cast<size_t>(MenuItem::Count); idx++) {
        std::string_view text = menu_items[idx];

        auto selected_idx = static_cast<uint8_t>(m_selected_item);
        bool is_selected = selected_idx == idx;
        int scale = is_selected ? TextStyles::scale_med : TextStyles::scale;

        draw_menu_item(text, scale, text_y);

        text_y += line_height(TextStyles::font.size, scale, TextStyles::menu_item_text_line_height);
    }
}

} // namespace alx
