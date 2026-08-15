#include "alx/Menu.h"

#include "alx/Action.h"
#include "core/Draw.h"

namespace alx {

void Menu::set_items(std::span<const std::string_view> items) noexcept {
    m_items = items;
    if (m_selected_index >= static_cast<int>(m_items.size())) {
        m_selected_index = 0;
    }
}

void Menu::set_selected_index(int idx) noexcept {
    if (m_items.empty()) {
        m_selected_index = 0;
        return;
    }
    const int count = static_cast<int>(m_items.size());
    if (idx < 0) {
        m_selected_index = 0;
    } else if (idx >= count) {
        m_selected_index = count - 1;
    } else {
        m_selected_index = idx;
    }
}

bool Menu::update_navigation() {
    if (m_items.empty()) {
        return false;
    }
    const int count = static_cast<int>(m_items.size());
    if (Action::is_just_pressed(Action::MoveUp)) {
        m_selected_index = (m_selected_index == 0) ? (count - 1) : (m_selected_index - 1);
        return true;
    }
    if (Action::is_just_pressed(Action::MoveDown)) {
        m_selected_index = (m_selected_index + 1 >= count) ? 0 : (m_selected_index + 1);
        return true;
    }
    return false;
}

bool Menu::is_confirmed() const {
    return Action::is_just_pressed(Action::Menu) || Action::is_just_pressed(Action::ActionBtn);
}

int Menu::calculate_line_height(uint8_t font_size, int scale, int text_line_height) noexcept {
    if (text_line_height <= 0) {
        return font_size * scale;
    }
    return text_line_height * scale;
}

void Menu::draw(const MenuConfig& config) const {
    if (m_items.empty()) {
        return;
    }

    float current_y = config.start_y;
    const size_t count = m_items.size();
    const FontData* font = config.font != nullptr ? config.font : &TextStyles::font;

    for (size_t idx = 0; idx < count; ++idx) {
        const std::string_view text = m_items[idx];
        const bool is_selected = (static_cast<int>(idx) == m_selected_index);
        const int scale = is_selected ? config.scale_selected : config.scale_unselected;

        const int text_width = Draw::text_width(text, scale, font);
        const float x = config.center_x - (text_width / 2.0f);

        Draw::text_shadow(
            x,
            current_y,
            text,
            config.color,
            config.color_shadow,
            scale,
            config.layer,
            font
        );

        current_y += calculate_line_height(font->size, scale, config.item_line_height);
    }
}

} // namespace alx
