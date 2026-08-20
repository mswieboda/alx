#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#if ALX_ENABLE_DEV_TOOLS && ALX_ENABLE_TELEMETRY && defined(__APPLE__)
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif // ALX_ENABLE_DEV_TOOLS && ALX_ENABLE_TELEMETRY && defined(__APPLE__)

#include "Debug.h"
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
#include "alx/StartScene.h"
#include "alx/MainScene.h"

namespace {

#if ALX_ENABLE_DEV_TOOLS && ALX_ENABLE_TELEMETRY && defined(__APPLE__)
pid_t g_telemetry_pid = -1;

void stop_inline_telemetry_viewer() {
    if (g_telemetry_pid > 0) {
        kill(-g_telemetry_pid, SIGTERM);
        kill(-g_telemetry_pid, SIGKILL);
        waitpid(g_telemetry_pid, nullptr, WNOHANG);
        g_telemetry_pid = -1;
        std::printf("\033[?25h\033[0m\n");
        std::fflush(stdout);
    }
}

void handle_telemetry_sigint(int) {
    stop_inline_telemetry_viewer();
    std::exit(0);
}

void start_inline_telemetry_viewer() {
    std::signal(SIGINT, handle_telemetry_sigint);
    std::signal(SIGTERM, handle_telemetry_sigint);

    g_telemetry_pid = fork();
    if (g_telemetry_pid == 0) {
        setpgid(0, 0);
        execlp("crystal", "crystal", "run", "toolchain/src/telemetry_viewer.cr", nullptr);
        _exit(0);
    }
}
#endif // ALX_ENABLE_DEV_TOOLS && ALX_ENABLE_TELEMETRY && defined(__APPLE__)

// --- CLI ARGUMENT PARSING HELPERS ---
std::optional<std::string_view> find_cli_arg(int argc, char* argv[], std::string_view name) {
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg.starts_with("--")) {
            std::string_view flag = arg.substr(2);
            if (flag == name && i + 1 < argc) {
                return std::string_view(argv[i + 1]);
            }
            if (flag.starts_with(name) && flag.size() > name.size() && flag[name.size()] == '=') {
                return flag.substr(name.size() + 1);
            }
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
        if constexpr (ALX_ENABLE_DEBUG) {
            Log::error("Invalid integer CLI argument for --" + std::string(name));
        }
        return default_val;
    }
    return result;
}

bool has_cli_flag(int argc, char* argv[], std::string_view name) {
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg.starts_with("--") && arg.substr(2) == name) {
            return true;
        }
    }
    return false;
}

// --- UPDATE --- where game logic updates happens
void frame_updates(GameWindow& window, FrameTime& frame_time, SceneManager& scene_manager) {
    frame_time.update();

    bool did_tick = false;

    while (frame_time.tick()) {
        did_tick = true;

        if (window.is_active()) {
            // Early out on Escape key if allowed
            bool should_quit = scene_manager.m_is_quit;
            if constexpr (Debug::QUIT_ON_ESC) {
                should_quit = should_quit || Input::is_key_just_pressed(KeyCode::Escape);
            }
            if (should_quit) {
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

} // namespace

// --- MAIN --- init window, frame timing management, pixel buffer, scene manager
// game loop - poll events, updates, draw
int main(int argc, char* argv[]) {
    int64_t seed = parse_cli_int64_arg(argc, argv, "seed", Game::CUSTOM_SEED);
    alx::Random::init(seed);

#ifndef ALX_ENABLE_HEADLESS
#  ifdef DEBUG
#    define ALX_ENABLE_HEADLESS 1
#  else
#    define ALX_ENABLE_HEADLESS 0
#  endif
#endif // !defined(ALX_ENABLE_HEADLESS)

    if constexpr (ALX_ENABLE_HEADLESS) {
        if (has_cli_flag(argc, argv, "headless-sim") || has_cli_flag(argc, argv, "headless")) {
            int64_t target_ticks = parse_cli_int64_arg(argc, argv, "ticks", 10000);
            Log::info("=== RUNNING HEADLESS SIMULATION ===");
            Log::info("Target Ticks: " + std::to_string(target_ticks));
            Log::info("Seed: " + std::to_string(seed));

            SceneManager scene_manager;
            auto scene_ptr = std::make_unique<alx::MainScene>();
            auto* raw_scene = scene_ptr.get();
            raw_scene->set_headless(true);
            scene_manager.change_scene(std::move(scene_ptr));

            constexpr float fixed_dt = 1.0f / 60.0f;
            for (int64_t i = 0; i < target_ticks; ++i) {
                scene_manager.update(fixed_dt);
            }

            if (raw_scene) {
                raw_scene->print_headless_summary_report(seed);
            }

            Log::info("=== HEADLESS SIMULATION COMPLETED ===");
            return 0;
        }
    }

    GameWindow game_window(
        Game::TITLE.data(),
        // initial window size
        Game::WIDTH * Game::DEFAULT_WINDOW_SCALE, Game::HEIGHT * Game::DEFAULT_WINDOW_SCALE,
        // game size
        Game::WIDTH, Game::HEIGHT
    );

#if defined(__APPLE__)
    if constexpr (ALX_ENABLE_DEV_TOOLS) {
        if (!has_cli_flag(argc, argv, "window-center")) {
            game_window.move_to_left_edge();
        }
    } else if (has_cli_flag(argc, argv, "window-left")) {
        game_window.move_to_left_edge();
    }

#if ALX_ENABLE_DEV_TOOLS && ALX_ENABLE_TELEMETRY
    if (has_cli_flag(argc, argv, "telemetry")) {
        start_inline_telemetry_viewer();
    }
#endif // ALX_ENABLE_DEV_TOOLS && ALX_ENABLE_TELEMETRY
#endif // defined(__APPLE__)

    FrameTime frame_time(Game::TARGET_FPS);

    if (!Audio::init()) {
        if constexpr (ALX_ENABLE_DEBUG) {
            Log::error("Continuing without audio.");
        }
    }

    Draw::set_palette(Assets::Images::GLOBAL_PALETTE);

    // Setup input routing
    game_window.set_key_callback(Input::keyboard_callback);
    game_window.set_active_callback(Input::window_active_callback);

    // Pixel buffer for drawing
    std::vector<uint32_t> pixel_buffer(Game::WIDTH * Game::HEIGHT, 0x00000000);

    SceneManager scene_manager;

    // Initialize and change to the first scene
    auto start_scene = std::make_unique<alx::StartScene>();

    // TODO: this telemetry report is broken because we have StartScene now
    // auto* raw_scene = main_scene.get();

    scene_manager.change_scene(std::move(start_scene));

    while (game_window.is_running()) {
        Input::update_input_state(game_window.is_active());
        game_window.poll_events();

        frame_updates(game_window, frame_time, scene_manager);

        if (game_window.is_running()) {
            draw(game_window, frame_time, scene_manager, pixel_buffer);
        }
    }

// TODO: this telemetry report is broken because we have StartScene now
// #if ALX_ENABLE_DEV_TOOLS && ALX_ENABLE_TELEMETRY && defined(__APPLE__)
//     stop_inline_telemetry_viewer();
// #endif // ALX_ENABLE_DEV_TOOLS && ALX_ENABLE_TELEMETRY && defined(__APPLE__)

//     if constexpr (ALX_ENABLE_HEADLESS) {
//         if (has_cli_flag(argc, argv, "report")) {
//             if (raw_scene) {
//                 raw_scene->print_headless_summary_report(seed);
//             }
//         }
//     }

    Audio::cleanup();
    return 0;
}
