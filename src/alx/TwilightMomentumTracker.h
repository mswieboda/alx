#pragma once

#include <cstdint>
#include <cstddef>

namespace alx {

namespace twilight_momentum {

    // Rolling window configuration
    inline constexpr float rolling_window_sec = 15.0f;
    inline constexpr float bucket_duration_sec = 0.25f;
    inline constexpr size_t bucket_count = 60; // 60 * 0.25s = 15.0s

    // Rate thresholds (% change projected per rolling window)
    inline constexpr float threshold_slight = 3.0f;    // |delta| >= 3.0% (below this is Equilibrium)
    inline constexpr float threshold_moderate = 5.0f;  // |delta| >= 5.0%
    inline constexpr float threshold_heavy = 15.0f;    // |delta| >= 15.0%
    inline constexpr float min_sampling_window_sec = 3.0f; // Warmup floor for delta calculation

    // Pulse beat frequencies (Hz)
    inline constexpr float pulse_freq_equilibrium = 0.5f;
    inline constexpr float pulse_freq_slight = 1.0f;
    inline constexpr float pulse_freq_moderate = 2.0f;
    inline constexpr float pulse_freq_heavy = 4.0f;

    // Transition & kick timers (seconds)
    inline constexpr float kick_duration_sec = 0.06f; // ~3-4 frames
    inline constexpr float kick_offset_px = 2.0f;
    inline constexpr float flash_duration_sec = 0.05f; // ~3 frames

} // namespace twilight_momentum

enum class MomentumTier : uint8_t {
    HeavyLight,
    ModerateLight,
    SlightLight,
    Equilibrium,
    SlightTwilight,
    ModerateTwilight,
    HeavyTwilight
};

struct TwilightMomentumState {
    float rolling_delta{0.0f}; // Projected % change per rolling 15s window
    MomentumTier current_tier{MomentumTier::Equilibrium};
    MomentumTier previous_tier{MomentumTier::Equilibrium};

    // Animation & Feedback timers
    float pulse_phase{0.0f};       // Oscillator for sine breathing / luminance
    float kick_offset_x{0.0f};     // Micro-nudge pixel offset
    float kick_timer{0.0f};        // Remaining kick duration
    float flash_timer{0.0f};       // Remaining snap-flash duration
    bool flash_is_light{false};    // Direction of snap flash
};

class TwilightMomentumTracker {
private:
    struct TimeBucket {
        float dt{0.0f};
        float delta{0.0f};
    };

    TimeBucket m_buckets[twilight_momentum::bucket_count]{};
    size_t m_head{0};
    size_t m_bucket_count{0};
    float m_accum_dt{0.0f};
    float m_accum_delta{0.0f};

    TwilightMomentumState m_state{};

    [[nodiscard]] MomentumTier evaluate_tier(float delta) const noexcept;
    [[nodiscard]] float get_pulse_frequency(MomentumTier tier) const noexcept;

public:
    void reset(float initial_twilight) noexcept;
    void update(float dt, float current_twilight, float prev_twilight) noexcept;

    [[nodiscard]] const TwilightMomentumState& state() const noexcept { return m_state; }
    [[nodiscard]] float rolling_delta() const noexcept { return m_state.rolling_delta; }
    [[nodiscard]] MomentumTier current_tier() const noexcept { return m_state.current_tier; }
};

} // namespace alx
