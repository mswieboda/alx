#pragma once

#include <cstdint>
#include <vector>

namespace alx {

class Camera;

class TwilightOverlay {
private:
    std::vector<uint32_t> m_pixel_buffer;
    float m_vignette_timer{0.0f};

    static constexpr float VIGNETTE_DURATION = 1.0f;
    static constexpr float VIGNETTE_PEAK_INTENSITY = 0.33f;
    static constexpr float VIGNETTE_INNER_RADIUS = 0.90f;
    static constexpr float VIGNETTE_OUTER_RADIUS = 1.05f;
    static constexpr uint32_t VIGNETTE_COLOR = 0x00CC44FF;
    static constexpr uint32_t TWILIGHT_RGB = 0x00130C1A;

public:
    void init(int width, int height);
    void update(float dt) noexcept;
    void trigger_vignette_surge(float duration = VIGNETTE_DURATION) noexcept;

    void draw(
        float twilight_level,
        const Camera& camera,
        float player_center_x,
        float player_center_y,
        float wand_radius
    );

    void draw_vignette_surge() const;
};

} // namespace alx
