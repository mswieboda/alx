#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "Game.h"
#include "core/GameWindow.h"
#include "core/FrameTime.h"
#include "core/Audio.h"
#include "core/Input.h"
#include "core/SceneManager.h"
#include "core/Log.h"
#include "core/Draw.h"
#include "assets/Images.h"
#include "alx/Random.h"
#include "alx/MainScene.h"

// --- CLI ARGUMENT PARSING HELPERS ---
std::optional<std::string_view> find_cli_arg(int argc, char* argv[], std::string_view name) {
    std::string flag_space = "--" + std::string(name);
    std::string flag_eq = flag_space + "=";

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == flag_space && i + 1 < argc) {
            return std::string_view(argv[i + 1]);
        }
        if (arg.starts_with(flag_eq)) {
            return arg.substr(flag_eq.length());
        }
    }
    return std::nullopt;
}

int64_t parse_cli_int64_arg(int argc, char* argv[], std::string_view name, int64_t default_val) {
    auto val_str = find_cli_arg(argc, argv, name);
    if (!val_str.has_value()) {
        return default_val;
    }
    int64_t result = default_val;
    auto [ptr, ec] = std::from_chars(val_str->data(), val_str->data() + val_str->size(), result);
    if (ec != std::errc{}) {
        Log::error("Invalid integer CLI argument for --" + std::string(name));
        return default_val;
    }
    return result;
}

// --- UPDATE --- where game logic updates happens
void frame_updates(GameWindow& window, FrameTime& frame_time, SceneManager& scene_manager) {
    frame_time.update();

    bool did_tick = false;

    while (frame_time.tick()) {
        did_tick = true;

        if (window.is_active()) {
            // Early out on Escape key if allowed
            if (Game::QUIT_ON_ESC && Input::is_key_just_pressed(MFB_KB_KEY_ESCAPE)) {
                window.close();
                break;
            }

            // Actual game logic updates
            scene_manager.update(frame_time.fixed_delta());
        } else {
            // Safe stall if window loses focus
            Input::force_clear_all_inputs();
        }

        frame_time.consume_step();
    }

    // Only wipe out 'just pressed' inputs if we actually ran the game logic this frame!
    if (did_tick) {
        Input::clear_just_pressed();
    }
}

// --- DRAW --- where drawing happens
void draw(GameWindow& window, FrameTime& frame_time, SceneManager& scene_manager, std::vector<uint32_t>& pixel_buffer) {
    float alpha = frame_time.alpha();

    scene_manager.draw(pixel_buffer, alpha);

    window.present(pixel_buffer, Game::WIDTH, Game::HEIGHT);
}

// --- MAIN --- init window, frame timing management, pixel buffer, scene manager
// game loop - poll events, updates, draw
int main(int argc, char* argv[]) {
    int64_t seed = parse_cli_int64_arg(argc, argv, "seed", Game::CUSTOM_SEED);
    alx::Random::init(seed);

    GameWindow game_window(
        Game::TITLE.data(),
        // initial window size
        Game::WIDTH * Game::DEFAULT_WINDOW_SCALE, Game::HEIGHT * Game::DEFAULT_WINDOW_SCALE,
        // game size
        Game::WIDTH, Game::HEIGHT
    );
    FrameTime frame_time(Game::TARGET_FPS);

    if (!Audio::init()) {
        Log::error("Continuing without audio.");
    }

    Draw::set_palette(Assets::Images::GLOBAL_PALETTE);

    // Setup input routing
    mfb_set_keyboard_callback(game_window.raw(), Input::keyboard_callback);
    mfb_set_active_callback(game_window.raw(), Input::window_active_callback);

    // Pixel buffer for drawing
    std::vector<uint32_t> pixel_buffer(Game::WIDTH * Game::HEIGHT, 0x00000000);

    SceneManager scene_manager;

    // Initialize and change to the first scene
    scene_manager.change_scene(std::make_unique<alx::MainScene>());

    while (game_window.is_running()) {
        Input::update_input_state(game_window.raw());
        game_window.poll_events();

        frame_updates(game_window, frame_time, scene_manager);

        if (game_window.is_running()) {
            draw(game_window, frame_time, scene_manager, pixel_buffer);
        }
    }

    Audio::cleanup();
    return 0;
}
