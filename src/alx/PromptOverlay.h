#pragma once

#include <array>
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
    mine_alloy_hint,
    place_pipe_hint,
    link_spire_hint,
    low_alloy_warning,
    pipe_unlinked_warning,
    spire_attacked_alert,
    surge_incoming_alert,
    mana_spark_hint
};

struct PromptMessage {
    std::string_view text{};
    PromptId id{PromptId::none};
    PromptType type{PromptType::info};
    float hold_duration_sec{3.0f};
    bool dismiss_on_action{true};
    bool is_sticky{false};
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
    static constexpr size_t max_queued_prompts = 4;

    PromptMessage m_current{};
    std::array<PromptMessage, max_queued_prompts> m_queue{};
    size_t m_queue_count{0};
    PromptState m_state{PromptState::inactive};
    float m_state_timer_sec{0.0f};
    float m_alpha{0.0f};
    float m_slide_offset_y{0.0f};

    [[nodiscard]] uint32_t border_color_for_type(PromptType type) const noexcept;
    [[nodiscard]] static uint32_t apply_alpha(uint32_t argb, float alpha) noexcept;

public:
    PromptOverlay() = default;

    void show(
        std::string_view text,
        PromptType type = PromptType::info,
        PromptId id = PromptId::none,
        float hold_duration = prompt_style::default_hold_duration_sec,
        bool is_sticky = false
    );

    void dismiss();
    void dismiss_if_matching(PromptId id);
    void reset();

    void update(float dt) noexcept;
    void draw(int screen_width, int screen_height) const;

    [[nodiscard]] bool is_active() const noexcept { return m_state != PromptState::inactive; }
    [[nodiscard]] PromptState state() const noexcept { return m_state; }
    [[nodiscard]] PromptId current_id() const noexcept { return m_current.id; }
};

} // namespace alx
