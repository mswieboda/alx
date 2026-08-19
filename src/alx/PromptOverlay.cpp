#include "alx/PromptOverlay.h"
#include <algorithm>
#include <cmath>
#include "alx/Action.h"
#include "alx/TextStyles.h"
#include "core/Draw.h"
#include "core/Input.h"

namespace alx {

uint32_t PromptOverlay::border_color_for_type(PromptType type) const noexcept {
    switch (type) {
        case PromptType::info:
            return prompt_style::color_border_tier0;
        case PromptType::warning:
            return prompt_style::color_border_tier1_elav;
        case PromptType::alert:
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

size_t PromptOverlay::format_tokens(std::string_view input, char* out_buf, size_t max_out_len) noexcept {
    if (out_buf == nullptr || max_out_len == 0) return 0;

    const bool is_gamepad = ::Input::is_gamepad_connected();
    size_t out_idx = 0;

    auto append_str = [&](std::string_view s) {
        for (char c : s) {
            if (out_idx + 1 < max_out_len) {
                out_buf[out_idx++] = c;
            }
        }
    };

    auto resolve_action_label = [&](std::string_view token) -> std::string_view {
        if (is_gamepad) {
            if (token == "ATTACK") return "[A]";
            if (token == "PLACE") return "[X]";
            if (token == "CANCEL" || token == "DEMOLISH") return "[B]";
            if (token == "CYCLE") return "[Y]";
            if (token == "PAN") return "[L]";
            if (token == "SPARK") return "[ZR]";
            if (token == "BUILD_MODE") return "[R]";
            if (token == "FOUNDATION") return "[A]";
            if (token == "MENU") return "[START]";
        } else {
            if (token == "ATTACK") return "[J]";
            if (token == "PLACE") return "[U]";
            if (token == "CANCEL" || token == "DEMOLISH") return "[K]";
            if (token == "CYCLE") return "[I]";
            if (token == "PAN") return "[Tab]";
            if (token == "SPARK") return "[;]";
            if (token == "BUILD_MODE") return "[L]";
            if (token == "FOUNDATION") return "[Space]";
            if (token == "MENU") return "[Enter]";
        }
        return {};
    };

    size_t i = 0;
    while (i < input.size() && out_idx + 1 < max_out_len) {
        if (input[i] == '{') {
            const size_t close_pos = input.find('}', i + 1);
            if (close_pos != std::string_view::npos) {
                const std::string_view token = input.substr(i + 1, close_pos - (i + 1));
                const std::string_view label = resolve_action_label(token);
                if (!label.empty()) {
                    append_str(label);
                    i = close_pos + 1;
                    continue;
                }
            }
        }
        out_buf[out_idx++] = input[i++];
    }

    out_buf[out_idx] = '\0';
    return out_idx;
}

bool PromptOverlay::show(
    std::string_view text,
    PromptType type,
    PromptId id,
    float hold_duration,
    bool is_sticky
) {
    char formatted_buf[PromptMessage::max_text_length]{};
    format_tokens(text, formatted_buf, PromptMessage::max_text_length);
    const std::string_view resolved_text{formatted_buf};

    if (m_state == PromptState::inactive) {
        if (id != PromptId::none) {
            m_seen_history.set(static_cast<size_t>(id));
        }
        m_current.set_text(resolved_text);
        m_current.type = type;
        m_current.id = id;
        m_current.hold_duration_sec = hold_duration;
        m_current.is_sticky = is_sticky;

        m_state = PromptState::fade_in;
        m_state_timer_sec = 0.0f;
        m_alpha = 0.0f;
        m_slide_offset_y = prompt_style::slide_distance_px;
        return true;
    }

    // Preemption for critical threats over lower-priority active prompts
    if (type == PromptType::alert && m_current.type != PromptType::alert) {
        if (m_queue_count < max_queued_prompts) {
            for (size_t i = m_queue_count; i > 0; --i) {
                m_queue[i] = m_queue[i - 1];
            }
            m_queue[0] = m_current;
            ++m_queue_count;
        }
        if (id != PromptId::none) {
            m_seen_history.set(static_cast<size_t>(id));
        }
        m_current.set_text(resolved_text);
        m_current.type = type;
        m_current.id = id;
        m_current.hold_duration_sec = hold_duration;
        m_current.is_sticky = is_sticky;

        m_state = PromptState::fade_in;
        m_state_timer_sec = 0.0f;
        m_alpha = 0.0f;
        m_slide_offset_y = prompt_style::slide_distance_px;
        return true;
    }

    // Standard queuing
    if (m_queue_count < max_queued_prompts) {
        if (id != PromptId::none) {
            m_seen_history.set(static_cast<size_t>(id));
        }
        PromptMessage msg{
            .id = id,
            .type = type,
            .hold_duration_sec = hold_duration,
            .dismiss_on_action = true,
            .is_sticky = is_sticky,
        };
        msg.set_text(resolved_text);
        m_queue[m_queue_count] = msg;
        ++m_queue_count;
        return true;
    }

    return false;
}

bool PromptOverlay::try_show_cooldown(
    std::string_view text,
    PromptType type,
    PromptId id,
    float hold_duration,
    bool is_sticky,
    float cooldown_sec
) {
    if (id != PromptId::none) {
        const size_t idx = static_cast<size_t>(id);
        if (m_cooldown_timers[idx] > 0.0f) {
            return false;
        }
    }
    const bool shown = show(text, type, id, hold_duration, is_sticky);
    if (shown && id != PromptId::none) {
        m_cooldown_timers[static_cast<size_t>(id)] = cooldown_sec;
    }
    return shown;
}

bool PromptOverlay::try_show_once(
    std::string_view text,
    PromptType type,
    PromptId id,
    float hold_duration,
    bool is_sticky
) {
    if (id != PromptId::none) {
        const size_t idx = static_cast<size_t>(id);
        if (m_seen_history.test(idx)) {
            return false;
        }
    }
    return show(text, type, id, hold_duration, is_sticky);
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

void PromptOverlay::reset_room_cooldowns() {
    m_cooldown_timers.fill(0.0f);
}

void PromptOverlay::reset_history() {
    m_seen_history.reset();
}

void PromptOverlay::reset_all() {
    reset();
    reset_room_cooldowns();
    reset_history();
}

bool PromptOverlay::has_seen(PromptId id) const noexcept {
    if (id == PromptId::none) return false;
    return m_seen_history.test(static_cast<size_t>(id));
}

bool PromptOverlay::is_on_cooldown(PromptId id) const noexcept {
    if (id == PromptId::none) return false;
    return m_cooldown_timers[static_cast<size_t>(id)] > 0.0f;
}

float PromptOverlay::cooldown_remaining(PromptId id) const noexcept {
    if (id == PromptId::none) return 0.0f;
    return m_cooldown_timers[static_cast<size_t>(id)];
}

void PromptOverlay::update(float dt) noexcept {
    // 1. Update cooldown timers
    for (float& cd : m_cooldown_timers) {
        if (cd > 0.0f) {
            cd = std::max(0.0f, cd - dt);
        }
    }

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
            if (!m_current.is_sticky && m_state_timer_sec >= m_current.hold_duration_sec) {
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
    if (m_state == PromptState::inactive || m_alpha <= 0.005f || m_current.text().empty()) {
        return;
    }

    const int text_w = Draw::text_width(m_current.text(), 1, &TextStyles::font);
    const int box_w = text_w + (prompt_style::padding_x_px * 2);
    const int box_h = prompt_style::box_height_px;

    const float box_x = static_cast<float>(prompt_style::margin_left_px);
    const float base_y = static_cast<float>(screen_height - box_h - prompt_style::margin_bottom_px);
    const float box_y = base_y + m_slide_offset_y;

    const uint32_t bg_color = apply_alpha(prompt_style::color_bg, m_alpha);
    const uint32_t border_color = apply_alpha(border_color_for_type(m_current.type), m_alpha);
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
        m_current.text(),
        text_color,
        1,
        Layer::HUD_OverlayText,
        &TextStyles::font
    );
}

} // namespace alx
