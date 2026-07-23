#pragma once
#include <string>

namespace Game {
    inline constexpr int WIDTH = 640;
    inline constexpr int HEIGHT = 480;
    inline constexpr std::string_view TITLE = "Aetherlux";

    inline constexpr int TARGET_FPS = 60; // Set your cap here
    inline constexpr float FRAME_DURATION = 1.0f / TARGET_FPS;

    // NOTE: disable this in a true released game so ESC doesn't quit so easily
    inline constexpr bool QUIT_ON_ESC = true;

    // Infrastructure Processing Constants (simulation ticks required per conversion cycle)
    inline constexpr uint8_t REFINER_TICKS_REQUIRED = 3;
    inline constexpr uint8_t LIGHT_SPIRE_TICKS_REQUIRED = 3;
    inline constexpr uint8_t LIGHT_MANA_TIME_TO_LIFE_TICKS = 9;
    inline constexpr uint8_t SEEP_OVERPRESSURE_THRESHOLD_TICKS = 5;
}
