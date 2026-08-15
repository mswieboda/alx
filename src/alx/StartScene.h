#pragma once
#include "core/Scene.h"

namespace alx {

enum class MenuItem : uint8_t { Start, Options, Quit, Count };

class StartScene : public Scene {
private:
    MenuItem m_selected_item = MenuItem::Start;

public:
    void init(SceneManager& sm) override;
    void update(SceneManager& sm, float dt) override;
    void draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) override;
};

} // namespace alx
