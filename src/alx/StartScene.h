#pragma once
#include <array>
#include <string_view>
#include "core/Scene.h"
#include "alx/Menu.h"

namespace alx {

enum class MenuItem : uint8_t { Start, Options, Quit, Count };

class StartScene : public Scene {
private:
    static constexpr std::array<std::string_view, static_cast<size_t>(MenuItem::Count)> MENU_ITEMS = {
        "Start", "Options", "Quit"
    };

    Menu m_menu{MENU_ITEMS};

public:
    void init(SceneManager& sm) override;
    void update(SceneManager& sm, float dt) override;
    void draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) override;
};

} // namespace alx
