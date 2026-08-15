#pragma once
#include "core/Scene.h"

namespace alx {

class StartScene : public Scene {
private:
    // --- CONSTANTS ---
    // static constexpr float TWILIGHT_MAX = 0.99f;

    int m_selected_index = 0;

public:
    void init(SceneManager& sm) override;
    void update(SceneManager& sm, float dt) override;
    void draw_screen(std::vector<uint32_t>& pixel_buffer, float alpha) override;
};

} // namespace alx
