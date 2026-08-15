#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "Game.h"
#include "alx/Layer.h"
#include "alx/TextStyles.h"
#include "assets/Fonts.h"

namespace alx {

struct MenuConfig {
    float center_x = Game::half_screen_width;
    float start_y = Game::half_screen_height;
    int layer = Layer::HUD_Text;
    uint32_t color = TextStyles::color;
    uint32_t color_shadow = TextStyles::color_shadow;
    int scale_unselected = TextStyles::scale;
    int scale_selected = TextStyles::scale_med;
    int item_line_height = TextStyles::menu_item_text_line_height;
    const FontData* font = &TextStyles::font;
};

class Menu {
private:
    std::span<const std::string_view> m_items{};
    int m_selected_index{0};

public:
    constexpr Menu() = default;
    constexpr explicit Menu(std::span<const std::string_view> items, int initial_selection = 0) noexcept
        : m_items(items), m_selected_index(initial_selection) {}

    void set_items(std::span<const std::string_view> items) noexcept;
    bool update_navigation();
    [[nodiscard]] bool is_confirmed() const;

    [[nodiscard]] int selected_index() const noexcept { return m_selected_index; }
    void set_selected_index(int idx) noexcept;

    template <typename EnumT>
    [[nodiscard]] EnumT selected_item() const noexcept {
        return static_cast<EnumT>(m_selected_index);
    }

    template <typename EnumT>
    void set_selected_item(EnumT item) noexcept {
        set_selected_index(static_cast<int>(item));
    }

    void draw(const MenuConfig& config) const;

    [[nodiscard]] static int calculate_line_height(uint8_t font_size, int scale, int text_line_height = 0) noexcept;
};

} // namespace alx
