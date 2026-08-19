#include "alx/TwilightOverlay.h"
#include <algorithm>
#include <cmath>
#include "alx/Camera.h"
#include "core/Draw.h"
#include "alx/Layer.h"
#include "Game.h"

namespace alx {

void TwilightOverlay::init(int width, int height) {
    const size_t target_size = static_cast<size_t>(width * height);
    if (m_pixel_buffer.size() != target_size) {
        m_pixel_buffer.resize(target_size);
    }
}

void TwilightOverlay::update(float dt) noexcept {
    if (m_vignette_timer > 0.0f) {
        m_vignette_timer = std::max(0.0f, m_vignette_timer - dt);
    }
}

void TwilightOverlay::trigger_vignette_surge(float duration) noexcept {
    m_vignette_timer = duration;
}

void TwilightOverlay::draw_vignette_surge() const {
    if (m_vignette_timer <= 0.0f) return;

    const float progress = std::clamp(1.0f - (m_vignette_timer / VIGNETTE_DURATION), 0.0f, 1.0f);
    constexpr float PI = 3.14159265358979323846f;
    const float pulse_t = std::sin(progress * PI);
    const float current_intensity = pulse_t * VIGNETTE_PEAK_INTENSITY;

    Draw::vignette(current_intensity, VIGNETTE_COLOR, VIGNETTE_INNER_RADIUS, VIGNETTE_OUTER_RADIUS, Layer::WorldOverlay);
}

void TwilightOverlay::draw(
    float twilight_level,
    const Camera& camera,
    float player_center_x,
    float player_center_y,
    float wand_radius
) const {
    if (twilight_level <= 0.0f) return;

    const int w = Game::WIDTH;
    const int h = Game::HEIGHT;

    const float max_alpha_float = twilight_level * 255.0f;
    const uint8_t base_alpha = static_cast<uint8_t>(max_alpha_float);
    const uint32_t twilight_color = (static_cast<uint32_t>(base_alpha) << 24) | TWILIGHT_RGB;

    std::fill(m_pixel_buffer.begin(), m_pixel_buffer.end(), twilight_color);

    const int player_screen_x = camera.to_screen_x(player_center_x);
    const int player_screen_y = camera.to_screen_y(player_center_y);
    const int radius = static_cast<int>(std::round(wand_radius));
    const int radius_sq = radius * radius;
    const float inv_radius_sq = (radius_sq > 0) ? (1.0f / static_cast<float>(radius_sq)) : 1.0f;

    const int min_x = std::clamp(player_screen_x - radius, 0, w);
    const int max_x = std::clamp(player_screen_x + radius + 1, 0, w);
    const int min_y = std::clamp(player_screen_y - radius, 0, h);
    const int max_y = std::clamp(player_screen_y + radius + 1, 0, h);

    for (int y = min_y; y < max_y; ++y) {
        const int dy = y - player_screen_y;
        const int dy_sq = dy * dy;
        const int row_offset = y * w;

        for (int x = min_x; x < max_x; ++x) {
            const int dx = x - player_screen_x;
            const int dist_sq = dx * dx + dy_sq;

            if (dist_sq < radius_sq) {
                const float factor = static_cast<float>(dist_sq) * inv_radius_sq;
                const uint8_t alpha_byte = static_cast<uint8_t>(max_alpha_float * factor);
                const int idx = row_offset + x;

                m_pixel_buffer[idx] = (static_cast<uint32_t>(alpha_byte) << 24) | TWILIGHT_RGB;
            }
        }
    }

    Draw::blend_pixels(
        0, 0,
        m_pixel_buffer.data(),
        static_cast<uint32_t>(m_pixel_buffer.size() * sizeof(uint32_t)),
        w, h,
        Layer::WorldOverlay
    );
}

} // namespace alx
