#pragma once

#include "core/KeyCodes.h"

namespace Input {
    inline constexpr int MAX_KEYS = static_cast<int>(KeyCode::Count);

    extern bool current_keys[MAX_KEYS];
    extern bool just_pressed_keys[MAX_KEYS];

    void init();
    void cleanup();
    void update_input_state(bool is_window_active);
    void keyboard_callback(KeyCode key, KeyModifier mod, bool is_pressed);
    void window_active_callback(bool is_active);
    void force_clear_all_inputs();
    void clear_just_pressed();

    bool is_key_pressed(KeyCode key);
    bool is_key_pressed(int key);
    bool is_key_just_pressed(KeyCode key);
    bool is_key_just_pressed(int key);
    bool is_modifier_held(KeyModifier modifier);

    // Gamepad API
    bool is_gamepad_connected();
    bool is_gamepad_button_pressed(int button);
    bool is_gamepad_button_just_pressed(int button);
    bool is_gamepad_stick_dir_pressed(int dir);
    bool is_gamepad_stick_dir_just_pressed(int dir);
}
