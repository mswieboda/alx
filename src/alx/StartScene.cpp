#include "alx/StartScene.h"

#include <array>
#include <string_view>

#include "Debug.h"
#include "Game.h"
#include "core/Draw.h"
#include "core/Log.h"
#include "core/Input.h"
#include "core/SceneManager.h"
#include "alx/Action.h"
#include "alx/Layer.h"
#include "alx/MainScene.h"
#include "alx/TextStyles.h"

namespace alx {

void StartScene::init(SceneManager& sm) {
    background_color = 0xFF090909; // very dark gray
}

void StartScene::update(SceneManager& sm, float raw_dt) {
    // TODO: keeping ESC in here, not sure if i want to kill that later
    // but it's WAYYY faster for testing, maybe in debug only?
    if (Debug::QUIT_ON_ESC && Input::is_key_just_pressed(KeyCode::Escape)) {
        sm.m_is_quit = true;
        return;
    }

    m_menu.update_navigation();

    if (m_menu.is_confirmed()) {
        switch (m_menu.selected_item<MenuItem>()) {
            case MenuItem::Start: {
                auto main_scene = std::make_unique<alx::MainScene>();
                sm.change_scene(std::move(main_scene));
                break;
            }
            case MenuItem::Options:
                // TODO: Options menu/scene implementation
                break;
            case MenuItem::Quit:
                sm.m_is_quit = true;
                break;
            case MenuItem::Count:
                break;
        }
    }
}

void StartScene::draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) {
    // x centered, y centered qtr half screen down
    int title_scale = TextStyles::scale_big;
    int title_width = Draw::text_width(Game::TITLE, title_scale, &TextStyles::font);
    int title_line_height = Menu::calculate_line_height(TextStyles::font.size, title_scale);
    Draw::text_shadow(
        Game::half_screen_width - title_width / 2.0f, // x
        Game::qtr_screen_height - title_line_height / 2.0f, // y
        Game::TITLE,
        TextStyles::color, TextStyles::color_shadow, // colors
        title_scale, Layer::HUD_Text, &TextStyles::font // scale, z-index, font
    );

    // x,y centered screen center
    float menu_start_y = Game::half_screen_height - Menu::calculate_line_height(TextStyles::font.size, TextStyles::scale) / 2.0f;
    m_menu.draw({
        .center_x = static_cast<float>(Game::half_screen_width),
        .start_y = menu_start_y,
        .layer = Layer::HUD_Text,
    });
}

} // namespace alx
