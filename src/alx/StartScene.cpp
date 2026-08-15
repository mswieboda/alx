#include "alx/StartScene.h"

#include "Game.h"
#include "core/Input.h"
#include "core/Log.h"
#include "core/SceneManager.h"
#include "alx/Action.h"
#include "alx/Layer.h"
#include "alx/MainScene.h"

namespace alx {
  namespace {
    int line_height(uint8_t size, int scale, int line_padding = 0) {
      return size * scale + line_padding * scale;
    }
  }

void StartScene::init(SceneManager& sm) {
    background_color = 0xFF131313; // very dark gray

    // TODO: probably can use the default?
    Draw::set_y_sort_mode(Draw::YSortMode::YPlusHeight);

    m_selected_index = 3;
}

void StartScene::update(SceneManager& sm, float raw_dt) {
  // TODO: temporary ESC before we have menu items
  if (Input::is_key_just_pressed(KeyCode::Escape)) {
    Log::debug_t("[StartScene::update] quit");
    sm.m_is_quit = true;
  }

  if (Action::is_just_pressed(Action::Menu)) {
    Log::debug_t("[StartScene::update] >>> change scene to main");
    auto main_scene = std::make_unique<alx::MainScene>();
    sm.change_scene(std::move(main_scene));
  }
}

void StartScene::draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) {
  const uint32_t text_color = 0xFF00CCCC;
  const uint32_t shadow_color = 0xFF003344;
  const FontData& font = Assets::Fonts::fant_8;
  const int line_h_padding = 4;
  const int half_screen_width = Game::WIDTH / 2.0f;
  const int half_screen_height = Game::HEIGHT / 2.0f;
  const int qtr_screen_height = Game::HEIGHT / 4.0f;
  int text_y = 0;
  int text_scale = 1;
  int build_width = 0;

  // x centered, y centered qtr half screen down
  text_scale = 3;
  std::string_view build_str = Draw::fmt("TWILIGHT");
  build_width = Draw::text_width(build_str, text_scale, &font);
  Draw::text_shadow(
      half_screen_width - build_width / 2.0f, // x
      qtr_screen_height - line_height(font.size, text_scale) / 2.0f, // y
      build_str,
      text_color, shadow_color, text_scale, Layer::HUD_Text, &font
  );

  // x,y centered screen center
  text_scale = 2;
  build_str = Draw::fmt("index: %d", m_selected_index);
  build_width = Draw::text_width(build_str, text_scale, &font);
  text_y = half_screen_height - line_height(font.size, text_scale) / 2.0f;
  Draw::text_shadow(
      half_screen_width - build_width / 2.0f, // x
      text_y,
      build_str,
      text_color, shadow_color, text_scale, Layer::HUD_Text, &font
  );
  text_y += line_height(font.size, text_scale, line_h_padding);

  // x center, y: plus prev text's line height
  text_scale = 1;
  build_str = Draw::fmt("[PRESS ENTER]"); // TODO: press START if gamepad connected
  build_width = Draw::text_width(build_str, text_scale, &font);
  Draw::text_shadow(
      half_screen_width - build_width / 2.0f, // x
      text_y,
      build_str,
      text_color, shadow_color, text_scale, Layer::HUD_Text, &font
  );
  text_y += line_height(font.size, text_scale, line_h_padding);
}

} // namespace alx
