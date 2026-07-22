#pragma once
#include <vector>
#include <cstdint>
#include <MiniFB.h>

namespace alx {

class Player {
private:
    float m_x;
    float m_y;
    float m_speed; // Pixels per second
    int m_width;
    int m_height;
    uint32_t m_color;

public:
    Player(float startX, float startY) :
        m_x(startX),
        m_y(startY),
        m_speed(128.0f),
        m_width(32),
        m_height(48),
        m_color(0xFFFF00FF) // Magenta placeholder box
    {}

    void update(float dt) {
        float dx = 0.0f;
        float dy = 0.0f;

        if (Input::is_key_pressed(MFB_KB_KEY_W) || Input::is_key_pressed(MFB_KB_KEY_UP))    dy -= 1.0f;
        if (Input::is_key_pressed(MFB_KB_KEY_S) || Input::is_key_pressed(MFB_KB_KEY_DOWN))  dy += 1.0f;
        if (Input::is_key_pressed(MFB_KB_KEY_A) || Input::is_key_pressed(MFB_KB_KEY_LEFT))  dx -= 1.0f;
        if (Input::is_key_pressed(MFB_KB_KEY_D) || Input::is_key_pressed(MFB_KB_KEY_RIGHT)) dx += 1.0f;

        // Normalize diagonal movement so player doesn't move faster diagonally
        if (dx != 0.0f && dy != 0.0f) {
            float invLength = 0.7071f; // ~1 / sqrt(2)
            dx *= invLength;
            dy *= invLength;
        }

        // Apply movement scaled by delta time
        m_x += dx * m_speed * dt;
        m_y += dy * m_speed * dt;
    }

    void draw(std::vector<uint32_t>& screen_buffer, float alpha = 1.0f) const {
        // Draw the player box
        Draw::rect(
            static_cast<int>(m_x), // x
            static_cast<int>(m_y), // y
            m_width, // width
            m_height, // height
            m_color, // color
            true, // filled
            1, // thickness (unused if filled)
            1 // z-index (TODO: make a new m_z_index for this or a constant PLAYER_Z_INDEX etc)
        );
    }

    // Getters for future collision checks (Phase 1.4)
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    
    void setPosition(float x, float y) { m_x = x; m_y = y; }
};

} // namespace alx
