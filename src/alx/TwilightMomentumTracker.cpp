#include "alx/TwilightMomentumTracker.h"
#include <algorithm>
#include <cmath>

namespace alx {

void TwilightMomentumTracker::reset(float /*initial_twilight*/) noexcept {
    for (size_t i = 0; i < twilight_momentum::bucket_count; ++i) {
        m_buckets[i] = TimeBucket{};
    }
    m_head = 0;
    m_bucket_count = 0;
    m_accum_dt = 0.0f;
    m_accum_delta = 0.0f;
    m_state = TwilightMomentumState{};
}

MomentumTier TwilightMomentumTracker::evaluate_tier(float delta) const noexcept {
    if (delta <= -twilight_momentum::threshold_heavy) {
        return MomentumTier::HeavyLight;
    }
    if (delta <= -twilight_momentum::threshold_moderate) {
        return MomentumTier::ModerateLight;
    }
    if (delta <= -twilight_momentum::threshold_slight) {
        return MomentumTier::SlightLight;
    }
    if (delta < twilight_momentum::threshold_slight) {
        return MomentumTier::Equilibrium;
    }
    if (delta < twilight_momentum::threshold_moderate) {
        return MomentumTier::SlightTwilight;
    }
    if (delta < twilight_momentum::threshold_heavy) {
        return MomentumTier::ModerateTwilight;
    }
    return MomentumTier::HeavyTwilight;
}

float TwilightMomentumTracker::get_pulse_frequency(MomentumTier tier) const noexcept {
    switch (tier) {
        case MomentumTier::HeavyLight:
        case MomentumTier::HeavyTwilight:
            return twilight_momentum::pulse_freq_heavy;
        case MomentumTier::ModerateLight:
        case MomentumTier::ModerateTwilight:
            return twilight_momentum::pulse_freq_moderate;
        case MomentumTier::SlightLight:
        case MomentumTier::SlightTwilight:
            return twilight_momentum::pulse_freq_slight;
        case MomentumTier::Equilibrium:
        default:
            return twilight_momentum::pulse_freq_equilibrium;
    }
}

void TwilightMomentumTracker::update(float dt, float current_twilight, float prev_twilight) noexcept {
    const float frame_delta = current_twilight - prev_twilight;
    m_accum_dt += dt;
    m_accum_delta += frame_delta;

    // Commit bucket on interval
    if (m_accum_dt >= twilight_momentum::bucket_duration_sec) {
        m_buckets[m_head] = TimeBucket{ .dt = m_accum_dt, .delta = m_accum_delta };
        m_head = (m_head + 1) % twilight_momentum::bucket_count;
        if (m_bucket_count < twilight_momentum::bucket_count) {
            ++m_bucket_count;
        }
        m_accum_dt = 0.0f;
        m_accum_delta = 0.0f;
    }

    // Sum history window + active accumulator
    float total_dt = m_accum_dt;
    float total_delta = m_accum_delta;
    for (size_t i = 0; i < m_bucket_count; ++i) {
        total_dt += m_buckets[i].dt;
        total_delta += m_buckets[i].delta;
    }

    if (total_dt > 0.001f) {
        const float rate_per_sec = total_delta / total_dt;
        m_state.rolling_delta = rate_per_sec * twilight_momentum::rolling_window_sec * 100.0f;
    } else {
        m_state.rolling_delta = 0.0f;
    }

    const MomentumTier new_tier = evaluate_tier(m_state.rolling_delta);

    auto is_light_tier = [](MomentumTier t) noexcept {
        return t == MomentumTier::HeavyLight || t == MomentumTier::ModerateLight || t == MomentumTier::SlightLight;
    };
    auto is_tw_tier = [](MomentumTier t) noexcept {
        return t == MomentumTier::HeavyTwilight || t == MomentumTier::ModerateTwilight || t == MomentumTier::SlightTwilight;
    };

    const bool was_light = is_light_tier(m_state.current_tier);
    const bool is_light = is_light_tier(new_tier);
    const bool was_tw = is_tw_tier(m_state.current_tier);
    const bool is_tw = is_tw_tier(new_tier);

    // Trigger snap-flash on direction inversion across equilibrium
    if ((was_light && is_tw) || (was_tw && is_light)) {
        m_state.flash_timer = twilight_momentum::flash_duration_sec;
        m_state.flash_is_light = is_light;
    }

    // Trigger micro-kick on tier change
    if (new_tier != m_state.current_tier) {
        m_state.previous_tier = m_state.current_tier;
        m_state.current_tier = new_tier;
        m_state.kick_timer = twilight_momentum::kick_duration_sec;
        if (is_light) {
            m_state.kick_offset_x = -twilight_momentum::kick_offset_px;
        } else if (is_tw) {
            m_state.kick_offset_x = twilight_momentum::kick_offset_px;
        } else {
            m_state.kick_offset_x = 0.0f;
        }
    }

    // Update kick decay
    if (m_state.kick_timer > 0.0f) {
        m_state.kick_timer = std::max(0.0f, m_state.kick_timer - dt);
        if (m_state.kick_timer <= 0.0f) {
            m_state.kick_offset_x = 0.0f;
        }
    }

    // Update flash decay
    if (m_state.flash_timer > 0.0f) {
        m_state.flash_timer = std::max(0.0f, m_state.flash_timer - dt);
    }

    // Advance pulse oscillator
    const float freq = get_pulse_frequency(m_state.current_tier);
    constexpr float two_pi = 6.28318530717958647692f;
    m_state.pulse_phase = std::fmod(m_state.pulse_phase + (dt * freq * two_pi), two_pi);
}

} // namespace alx
