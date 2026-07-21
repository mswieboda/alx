#include "Input.h"
#include <cstring>
#include <MiniFB.h>
#include <algorithm>

namespace Input {
    bool current_keys[MAX_KEYS] = { false };
    bool just_pressed_keys[MAX_KEYS] = { false };

    static mfb_key_mod current_mods = static_cast<mfb_key_mod>(0);

    void update_input_state(struct mfb_window *window) {
        if (window && !mfb_is_window_active(window)) {
            force_clear_all_inputs();
            return;
        }
    }

    void clear_just_pressed() {
        std::fill(std::begin(just_pressed_keys), std::end(just_pressed_keys), false);
    }

    void keyboard_callback(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool is_pressed) {
        // Cast to int to perform a valid range check against MAX_KEYS safely
        const int key_code = static_cast<int>(key);

        current_mods = mod;

        if (key_code >= 0 && key_code < MAX_KEYS) {
            // THE CRITICAL FIX: Detect the exact 0 -> 1 rising edge right when the OS fires it!
            if (is_pressed && !current_keys[key_code]) {
                just_pressed_keys[key_code] = true;
            }

            current_keys[key_code] = is_pressed;
        }
    }

    void window_active_callback(struct mfb_window *window, bool is_active) {
        force_clear_all_inputs();
    }

    void force_clear_all_inputs() {
        std::fill(std::begin(current_keys), std::end(current_keys), false);
        std::fill(std::begin(just_pressed_keys), std::end(just_pressed_keys), false);

        current_mods = static_cast<mfb_key_mod>(0);
    }

    bool is_key_pressed(int key) {
        if (key < 0 || key >= MAX_KEYS) return false;

        return current_keys[key];
    }

    bool is_key_just_pressed(int key) {
        if (key < 0 || key >= MAX_KEYS) return false;

        return just_pressed_keys[key];
    }

    bool is_modifier_held(mfb_key_mod modifier_bit) {
        return (current_mods & modifier_bit) != 0;
    }
}
