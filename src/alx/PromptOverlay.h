#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <string_view>
#include "alx/Layer.h"

namespace alx {

enum class PromptType : uint8_t {
    info = 0, // Tier 0: Guidance, discovery, first-time control hints
    warning,  // Tier 1: Low alloy, unlinked pipes, stalled flow
    alert    // Tier 2: Spire attacked, Twilight surge incoming
};

enum class PromptState : uint8_t {
    inactive = 0,
    fade_in,
    active_hold,
    fade_out
};

enum class PromptId : uint16_t {
    none = 0,
    // Onboarding / First-time hints (Run-once)
    mine_alloy_hint,
    place_pipe_hint,
    cycle_fixture_hint,
    camera_pan_hint,
    mana_spark_hint,

    // Network & Automation Logistics (Room Cooldowns)
    pipe_unlinked_warning,
    refiner_active_info,
    spire_linked_info,
    seep_depleted_info,

    // Economy & Resources (Room Cooldowns)
    low_alloy_warning,
    storage_full_info,

    // Twilight Threats & Combat (Alerts / Fast Preemption)
    spire_attacked_alert,
    surge_incoming_alert,
    tower_emerged_alert,
    player_low_hp_alert,
    room_purified_info,

    Count
};

struct PromptMessage {
    static constexpr size_t max_text_length = 64;

    std::array<char, max_text_length> text_buf{};
    size_t text_len{0};
    PromptId id{PromptId::none};
    PromptType type{PromptType::info};
    float hold_duration_sec{3.0f};
    bool dismiss_on_action{true};
    bool is_sticky{false};

    [[nodiscard]] std::string_view text() const noexcept {
        return std::string_view(text_buf.data(), text_len);
    }

    void set_text(std::string_view str) noexcept {
        const size_t len = std::min(str.size(), max_text_length - 1);
        if (len > 0) {
            std::copy_n(str.data(), len, text_buf.data());
        }
        text_buf[len] = '\0';
        text_len = len;
    }
};

namespace prompt_style {
    inline constexpr int box_height_px = 16;
    inline constexpr float corner_radius = 3.0f;
    inline constexpr int border_thickness_px = 1;
    inline constexpr int padding_x_px = 8;
    inline constexpr int margin_left_px = 8;
    inline constexpr int margin_bottom_px = 8;
    inline constexpr float slide_distance_px = 4.0f;
    inline constexpr float dismiss_slide_distance_px = 9.0f;

    inline constexpr float fade_in_duration_sec = 0.33f;
    inline constexpr float fade_out_duration_sec = 0.69f;
    inline constexpr float default_hold_duration_sec = 3.00f;
    inline constexpr float prompt_repeat_cooldown_sec = 10.00f;

    // Palette definitions (Strictly no amber/gold)
    inline constexpr uint32_t color_bg = 0xD80F131D;               // 85% alpha dark slate
    inline constexpr uint32_t color_border_tier0 = 0xFFA8C0D8;      // Resonant silver / slate
    inline constexpr uint32_t color_border_tier1_elav = 0xFF9D68EE; // Electric lavender (default)
    inline constexpr uint32_t color_border_tier1_hvtl = 0xFF38E2D8; // High-voltage teal (alternative)
    inline constexpr uint32_t color_border_tier2 = 0xFFD82850;      // Crimson quartz
    inline constexpr uint32_t color_text = 0xFFFFFFFF;              // Crisp white
} // namespace prompt_style

class PromptOverlay {
private:
    static constexpr size_t max_queued_prompts = 8;
    static constexpr size_t prompt_id_count = static_cast<size_t>(PromptId::Count);

    PromptMessage m_current{};
    std::array<PromptMessage, max_queued_prompts> m_queue{};
    size_t m_queue_count{0};
    PromptState m_state{PromptState::inactive};
    float m_state_timer_sec{0.0f};
    float m_alpha{0.0f};
    float m_slide_offset_y{0.0f};

    // Hybrid History & Cooldown Tracking
    std::bitset<prompt_id_count> m_seen_history{};
    std::array<float, prompt_id_count> m_cooldown_timers{};

    [[nodiscard]] uint32_t border_color_for_type(PromptType type) const noexcept;
    [[nodiscard]] static uint32_t apply_alpha(uint32_t argb, float alpha) noexcept;
    static size_t format_tokens(std::string_view input, char* out_buf, size_t max_out_len) noexcept;

public:
    PromptOverlay() = default;

    bool show(
        std::string_view text,
        PromptType type = PromptType::info,
        PromptId id = PromptId::none,
        float hold_duration = prompt_style::default_hold_duration_sec,
        bool is_sticky = false
    );

    bool try_show_cooldown(
        std::string_view text,
        PromptType type = PromptType::info,
        PromptId id = PromptId::none,
        float hold_duration = prompt_style::default_hold_duration_sec,
        bool is_sticky = false,
        float cooldown_sec = prompt_style::prompt_repeat_cooldown_sec
    );

    bool try_show_once(
        std::string_view text,
        PromptType type = PromptType::info,
        PromptId id = PromptId::none,
        float hold_duration = prompt_style::default_hold_duration_sec,
        bool is_sticky = false
    );

    void dismiss();
    void dismiss_if_matching(PromptId id);
    void reset();                     // Resets active state & queue (leaves history & cooldowns intact)
    void reset_room_cooldowns();       // Resets per-room cooldown timers
    void reset_history();              // Clears run-scoped seen-once history
    void reset_all();                  // Full reset (state, queue, cooldowns, history)

    void update(float dt) noexcept;
    void draw(int screen_width, int screen_height) const;

    [[nodiscard]] bool is_active() const noexcept { return m_state != PromptState::inactive; }
    [[nodiscard]] PromptState state() const noexcept { return m_state; }
    [[nodiscard]] PromptId current_id() const noexcept { return m_current.id; }
    [[nodiscard]] bool has_seen(PromptId id) const noexcept;
    [[nodiscard]] bool is_on_cooldown(PromptId id) const noexcept;
    [[nodiscard]] float cooldown_remaining(PromptId id) const noexcept;
};

} // namespace alx

