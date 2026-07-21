#pragma once
#include <string>

namespace Game {
    inline constexpr int WIDTH = 640;
    inline constexpr int HEIGHT = 480;
    inline constexpr std::string_view TITLE = "Mini Game";

    inline constexpr int TARGET_FPS = 60; // Set your cap here
    inline constexpr float FRAME_DURATION = 1.0f / TARGET_FPS;

    // NOTE: disable this in a true released game so ESC doesn't quit so easily
    inline constexpr bool QUIT_ON_ESC = true;

    // You can add other game-wide constants here later
    // e.g., constexpr float GRAVITY = 9.8f;
}
