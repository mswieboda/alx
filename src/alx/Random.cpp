#include "Random.h"
#include <chrono>

namespace alx {

uint32_t Random::s_active_seed = 0;
bool Random::s_is_custom = false;
std::mt19937 Random::s_engine;
bool Random::s_initialized = false;

void Random::init(int64_t custom_seed) {
    if (custom_seed > 0) {
        s_active_seed = static_cast<uint32_t>(custom_seed);
        s_is_custom = true;
    } else {
        std::random_device rd;
        uint32_t entropy = rd();
        if (entropy == 0) {
            entropy = static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        }
        s_active_seed = entropy;
        s_is_custom = false;
    }

    s_engine.seed(s_active_seed);
    s_initialized = true;
}

void Random::ensure_initialized() {
    if (!s_initialized) {
        init(-1);
    }
}

uint32_t Random::active_seed() {
    ensure_initialized();
    return s_active_seed;
}

bool Random::is_custom_seeded() {
    ensure_initialized();
    return s_is_custom;
}

int Random::get_int(int min, int max) {
    ensure_initialized();
    if (min >= max) return min;
    std::uniform_int_distribution<int> dist(min, max);
    return dist(s_engine);
}

float Random::get_float(float min, float max) {
    ensure_initialized();
    if (min >= max) return min;
    std::uniform_real_distribution<float> dist(min, max);
    return dist(s_engine);
}

bool Random::chance(float probability) {
    ensure_initialized();
    if (probability <= 0.0f) return false;
    if (probability >= 1.0f) return true;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(s_engine) < probability;
}

std::mt19937& Random::engine() {
    ensure_initialized();
    return s_engine;
}

} // namespace alx
