#pragma once
#include <MiniFB.h>

namespace Input {
    inline constexpr int MAX_KEYS = 512;

    extern bool current_keys[MAX_KEYS];
    extern bool just_pressed_keys[MAX_KEYS];

    void update_input_state(struct mfb_window *window);
    void keyboard_callback(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool is_pressed);
    void window_active_callback(struct mfb_window *window, bool is_active);
    void force_clear_all_inputs();
    void clear_just_pressed();

    bool is_key_pressed(int key);
    bool is_key_just_pressed(int key);
    bool is_modifier_held(mfb_key_mod modifier_bit);
}
