#pragma once

#include <random>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace alx {

class Random {
private:
    static uint32_t s_active_seed;
    static bool s_is_custom;
    static std::mt19937 s_engine;
    static bool s_initialized;

public:
    // Initialize or re-seed the RNG engine.
    // If custom_seed <= 0, dynamic seed is generated via std::random_device.
    // If custom_seed > 0, it uses custom_seed directly.
    static void init(int64_t custom_seed = -1);

    static uint32_t active_seed();
    static bool is_custom_seeded();

    // Range-based integer random: [min, max] inclusive
    static int get_int(int min, int max);

    // Range-based float random: [min, max]
    static float get_float(float min, float max);

    // Returns true with given probability [0.0f, 1.0f]
    static bool chance(float probability);

    // Shuffles any std::vector using the central RNG engine
    template<typename T>
    static void shuffle(std::vector<T>& container) {
        ensure_initialized();
        std::shuffle(container.begin(), container.end(), s_engine);
    }

    // Access raw engine reference if needed by std algorithms
    static std::mt19937& engine();

private:
    static void ensure_initialized();
};

} // namespace alx
