#pragma once
#include <cstdint>

namespace alx {

enum class ParticleType : uint8_t {
    Spark = 0,
    LightEmber,
    ManaPulseStraight,
    ManaPulseCurved
};

struct Particle {
    // World and screen positions
    float x{0.0f};
    float y{0.0f};
    float render_x{0.0f};
    float render_y{0.0f};

    // Kinematic physics (Route A)
    float vx{0.0f};
    float vy{0.0f};

    // Parametric pathing (Route B)
    float start_x{0.0f};
    float start_y{0.0f};
    float target_x{0.0f};
    float target_y{0.0f};
    float control_x{0.0f};
    float control_y{0.0f};

    // Timers & custom modulation
    float life{0.0f};
    float max_life{1.0f};
    float param_a{0.0f};

    // Visual attributes
    uint32_t color{0xFFFFFFFF};

    // Grid coordinates for pipe network queries
    uint16_t tile_x{0};
    uint16_t tile_y{0};

    // Size & type attributes
    uint8_t size{1};
    ParticleType type{ParticleType::Spark};
    bool active{false};

    [[nodiscard]] inline bool is_alive() const {
        return active && life > 0.0f;
    }

    [[nodiscard]] inline float progress() const {
        return (max_life > 0.0f) ? (1.0f - (life / max_life)) : 1.0f;
    }
};

} // namespace alx
