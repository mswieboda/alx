#include "alx/PromptOverlay.h"
#include <algorithm>
#include <cmath>
#include "core/Draw.h"
#include "alx/TextStyles.h"

namespace alx {

uint32_t PromptOverlay::border_color_for_severity(PromptSeverity severity) const noexcept {
    switch (severity) {
        case PromptSeverity::info_tutorial:
            return prompt_style::color_border_tier0;
        case PromptSeverity::resource_warning:
            return prompt_style::color_border_tier1_elav;
        case PromptSeverity::critical_threat:
            return prompt_style::color_border_tier2;
    }
    return prompt_style::color_border_tier0;
}

uint32_t PromptOverlay::apply_alpha(uint32_t argb, float alpha) noexcept {
    const float clamped_a = std::clamp(alpha, 0.0f, 1.0f);
    const uint32_t original_a = (argb >> 24) & 0xFF;
    const uint32_t scaled_a = static_cast<uint32_t>(static_cast<float>(original_a) * clamped_a);
    return (scaled_a << 24) | (argb & 0x00FFFFFF);
}

void PromptOverlay::show(
    std::string_view text,
    PromptSeverity severity,
    PromptId id,
    float hold_duration
) {
    if (m_state == PromptState::inactive) {
        m_current.text = text;
        m_current.severity = severity;
        m_current.id = id;
        m_current.hold_duration_sec = hold_duration;

        m_state = PromptState::fade_in;
        m_state_timer_sec = 0.0f;
        m_alpha = 0.0f;
        m_slide_offset_y = prompt_style::slide_distance_px;
        return;
    }

    // Preemption for critical threats over lower-priority active prompts
    if (severity == PromptSeverity::critical_threat && m_current.severity != PromptSeverity::critical_threat) {
        if (m_queue_count < max_queued_prompts) {
            for (size_t i = m_queue_count; i > 0; --i) {
                m_queue[i] = m_queue[i - 1];
            }
            m_queue[0] = m_current;
            ++m_queue_count;
        }
        m_current.text = text;
        m_current.severity = severity;
        m_current.id = id;
        m_current.hold_duration_sec = hold_duration;

        m_state = PromptState::fade_in;
        m_state_timer_sec = 0.0f;
        m_alpha = 0.0f;
        m_slide_offset_y = prompt_style::slide_distance_px;
        return;
    }

    // Standard queuing
    if (m_queue_count < max_queued_prompts) {
        m_queue[m_queue_count] = PromptMessage{
            .text = text,
            .id = id,
            .severity = severity,
            .hold_duration_sec = hold_duration,
            .dismiss_on_action = true,
        };
        ++m_queue_count;
    }
}

void PromptOverlay::dismiss() {
    if (m_state == PromptState::fade_in || m_state == PromptState::active_hold) {
        m_state = PromptState::fade_out;
        m_state_timer_sec = 0.0f;
    }
}

void PromptOverlay::dismiss_if_matching(PromptId id) {
    if (m_current.id == id && id != PromptId::none) {
        dismiss();
    }
}

void PromptOverlay::reset() {
    m_state = PromptState::inactive;
    m_state_timer_sec = 0.0f;
    m_alpha = 0.0f;
    m_slide_offset_y = 0.0f;
    m_current = {};
    m_queue_count = 0;
}

void PromptOverlay::update(float dt) noexcept {
    if (m_state == PromptState::inactive) return;

    m_state_timer_sec += dt;

    switch (m_state) {
        case PromptState::fade_in: {
            const float progress = std::clamp(
                m_state_timer_sec / prompt_style::fade_in_duration_sec,
                0.0f,
                1.0f
            );
            // Quadratic ease-out: progress * (2.0f - progress)
            const float ease = progress * (2.0f - progress);
            m_alpha = ease;
            m_slide_offset_y = (1.0f - ease) * prompt_style::slide_distance_px;

            if (m_state_timer_sec >= prompt_style::fade_in_duration_sec) {
                m_state = PromptState::active_hold;
                m_state_timer_sec = 0.0f;
                m_alpha = 1.0f;
                m_slide_offset_y = 0.0f;
            }
            break;
        }
        case PromptState::active_hold: {
            m_alpha = 1.0f;
            m_slide_offset_y = 0.0f;
            if (m_state_timer_sec >= m_current.hold_duration_sec) {
                m_state = PromptState::fade_out;
                m_state_timer_sec = 0.0f;
            }
            break;
        }
        case PromptState::fade_out: {
            const float progress = std::clamp(
                m_state_timer_sec / prompt_style::fade_out_duration_sec,
                0.0f,
                1.0f
            );
            m_alpha = 1.0f - progress;
            m_slide_offset_y = -progress * prompt_style::dismiss_slide_distance_px;

            if (m_state_timer_sec >= prompt_style::fade_out_duration_sec) {
                if (m_queue_count > 0) {
                    m_current = m_queue[0];
                    for (size_t i = 1; i < m_queue_count; ++i) {
                        m_queue[i - 1] = m_queue[i];
                    }
                    --m_queue_count;
                    m_state = PromptState::fade_in;
                    m_state_timer_sec = 0.0f;
                    m_alpha = 0.0f;
                    m_slide_offset_y = prompt_style::slide_distance_px;
                } else {
                    m_state = PromptState::inactive;
                    m_state_timer_sec = 0.0f;
                    m_alpha = 0.0f;
                    m_slide_offset_y = 0.0f;
                    m_current = {};
                }
            }
            break;
        }
        case PromptState::inactive:
            break;
    }
}

void PromptOverlay::draw(int screen_width, int screen_height) const {
    if (m_state == PromptState::inactive || m_alpha <= 0.005f || m_current.text.empty()) {
        return;
    }

    const int text_w = Draw::text_width(m_current.text, 1, &TextStyles::font);
    const int box_w = text_w + (prompt_style::padding_x_px * 2);
    const int box_h = prompt_style::box_height_px;

    const float box_x = static_cast<float>(prompt_style::margin_left_px);
    const float base_y = static_cast<float>(screen_height - box_h - prompt_style::margin_bottom_px);
    const float box_y = base_y + m_slide_offset_y;

    const uint32_t bg_color = apply_alpha(prompt_style::color_bg, m_alpha);
    const uint32_t border_color = apply_alpha(border_color_for_severity(m_current.severity), m_alpha);
    const uint32_t text_color = apply_alpha(prompt_style::color_text, m_alpha);

    // 1. Solid translucent rounded background box
    Draw::rect_rounded(
        box_x,
        box_y,
        static_cast<float>(box_w),
        static_cast<float>(box_h),
        prompt_style::corner_radius,
        bg_color,
        true,
        1,
        Layer::HUD_Overlay
    );

    // 2. 1px rounded outer border
    Draw::rect_rounded(
        box_x,
        box_y,
        static_cast<float>(box_w),
        static_cast<float>(box_h),
        prompt_style::corner_radius,
        border_color,
        false,
        prompt_style::border_thickness_px,
        Layer::HUD_Overlay
    );

    // 3. Crisp single-line font rendered inside padding bounds (vertically centered)
    const float text_x = box_x + static_cast<float>(prompt_style::padding_x_px);
    const float text_y = box_y + static_cast<float>(box_h - TextStyles::font.size) / 2.0f;
    Draw::text(
        text_x,
        text_y,
        m_current.text,
        text_color,
        1,
        Layer::HUD_OverlayText,
        &TextStyles::font
    );
}

} // namespace alx
