#pragma once

#include <string_view>

namespace Game {
    // game metadata
    constexpr std::string_view TITLE = "Aetherlux";

    // window / game loop
    constexpr int DEFAULT_WINDOW_SCALE = 2;
    constexpr int TARGET_FPS = 60; // Set your cap here
    constexpr float FRAME_DURATION = 1.0f / TARGET_FPS;

    // tiles / maps
    constexpr int TILE_SIZE = 16;

    // game dimensions
    constexpr int WIDTH = 320;
    constexpr int HEIGHT = 240;
    constexpr int half_screen_width = Game::WIDTH / 2.0f;
    constexpr int half_screen_height = Game::HEIGHT / 2.0f;
    constexpr int qtr_screen_height = Game::HEIGHT / 4.0f;

    // Seed Configuration
    // Set CUSTOM_SEED to a positive integer (e.g. 1337) to force deterministic runs.
    // Set to -1 (or <= 0) for dynamic random seeding on every launch.
    constexpr int64_t CUSTOM_SEED = -1; // e.g. 1337

    // Infrastructure Processing Constants (simulation ticks required per conversion cycle)
    constexpr uint8_t REFINER_PROCESSING_TICKS_REQUIRED = 5;
    constexpr uint8_t REFINER_CONSUMING_WAIT_TICKS = 1;
    constexpr uint8_t LIGHT_SPIRE_TICKS_REQUIRED = 3;
    constexpr uint8_t LIGHT_MANA_TIME_TO_LIFE_TICKS = 20;
}

