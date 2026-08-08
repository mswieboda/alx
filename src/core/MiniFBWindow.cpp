#include <MiniFB.h>
#include "core/MiniFBWindow.h"
#include "core/KeyCodes.h"
#include <algorithm>
#include <cstddef>

static KeyCode map_mfb_key_to_key_code(mfb_key key) {
    switch (key) {
        case MFB_KB_KEY_A: return KeyCode::A;
        case MFB_KB_KEY_B: return KeyCode::B;
        case MFB_KB_KEY_C: return KeyCode::C;
        case MFB_KB_KEY_D: return KeyCode::D;
        case MFB_KB_KEY_E: return KeyCode::E;
        case MFB_KB_KEY_F: return KeyCode::F;
        case MFB_KB_KEY_G: return KeyCode::G;
        case MFB_KB_KEY_H: return KeyCode::H;
        case MFB_KB_KEY_I: return KeyCode::I;
        case MFB_KB_KEY_J: return KeyCode::J;
        case MFB_KB_KEY_K: return KeyCode::K;
        case MFB_KB_KEY_L: return KeyCode::L;
        case MFB_KB_KEY_M: return KeyCode::M;
        case MFB_KB_KEY_N: return KeyCode::N;
        case MFB_KB_KEY_O: return KeyCode::O;
        case MFB_KB_KEY_P: return KeyCode::P;
        case MFB_KB_KEY_Q: return KeyCode::Q;
        case MFB_KB_KEY_R: return KeyCode::R;
        case MFB_KB_KEY_S: return KeyCode::S;
        case MFB_KB_KEY_T: return KeyCode::T;
        case MFB_KB_KEY_U: return KeyCode::U;
        case MFB_KB_KEY_V: return KeyCode::V;
        case MFB_KB_KEY_W: return KeyCode::W;
        case MFB_KB_KEY_X: return KeyCode::X;
        case MFB_KB_KEY_Y: return KeyCode::Y;
        case MFB_KB_KEY_Z: return KeyCode::Z;

        case MFB_KB_KEY_0: return KeyCode::Key0;
        case MFB_KB_KEY_1: return KeyCode::Key1;
        case MFB_KB_KEY_2: return KeyCode::Key2;
        case MFB_KB_KEY_3: return KeyCode::Key3;
        case MFB_KB_KEY_4: return KeyCode::Key4;
        case MFB_KB_KEY_5: return KeyCode::Key5;
        case MFB_KB_KEY_6: return KeyCode::Key6;
        case MFB_KB_KEY_7: return KeyCode::Key7;
        case MFB_KB_KEY_8: return KeyCode::Key8;
        case MFB_KB_KEY_9: return KeyCode::Key9;

        case MFB_KB_KEY_F1: return KeyCode::F1;
        case MFB_KB_KEY_F2: return KeyCode::F2;
        case MFB_KB_KEY_F3: return KeyCode::F3;
        case MFB_KB_KEY_F4: return KeyCode::F4;
        case MFB_KB_KEY_F5: return KeyCode::F5;
        case MFB_KB_KEY_F6: return KeyCode::F6;
        case MFB_KB_KEY_F7: return KeyCode::F7;
        case MFB_KB_KEY_F8: return KeyCode::F8;
        case MFB_KB_KEY_F9: return KeyCode::F9;
        case MFB_KB_KEY_F10: return KeyCode::F10;
        case MFB_KB_KEY_F11: return KeyCode::F11;
        case MFB_KB_KEY_F12: return KeyCode::F12;

        case MFB_KB_KEY_ESCAPE: return KeyCode::Escape;
        case MFB_KB_KEY_ENTER: return KeyCode::Enter;
        case MFB_KB_KEY_TAB: return KeyCode::Tab;
        case MFB_KB_KEY_SPACE: return KeyCode::Space;
        case MFB_KB_KEY_BACKSPACE: return KeyCode::Backspace;
        case MFB_KB_KEY_INSERT: return KeyCode::Insert;
        case MFB_KB_KEY_DELETE: return KeyCode::Delete;
        case MFB_KB_KEY_RIGHT: return KeyCode::Right;
        case MFB_KB_KEY_LEFT: return KeyCode::Left;
        case MFB_KB_KEY_DOWN: return KeyCode::Down;
        case MFB_KB_KEY_UP: return KeyCode::Up;
        case MFB_KB_KEY_PAGE_UP: return KeyCode::PageUp;
        case MFB_KB_KEY_PAGE_DOWN: return KeyCode::PageDown;
        case MFB_KB_KEY_HOME: return KeyCode::Home;
        case MFB_KB_KEY_END: return KeyCode::End;
        case MFB_KB_KEY_CAPS_LOCK: return KeyCode::CapsLock;
        case MFB_KB_KEY_SCROLL_LOCK: return KeyCode::ScrollLock;
        case MFB_KB_KEY_NUM_LOCK: return KeyCode::NumLock;
        case MFB_KB_KEY_PRINT_SCREEN: return KeyCode::PrintScreen;
        case MFB_KB_KEY_PAUSE: return KeyCode::Pause;

        case MFB_KB_KEY_KP_0: return KeyCode::KP0;
        case MFB_KB_KEY_KP_1: return KeyCode::KP1;
        case MFB_KB_KEY_KP_2: return KeyCode::KP2;
        case MFB_KB_KEY_KP_3: return KeyCode::KP3;
        case MFB_KB_KEY_KP_4: return KeyCode::KP4;
        case MFB_KB_KEY_KP_5: return KeyCode::KP5;
        case MFB_KB_KEY_KP_6: return KeyCode::KP6;
        case MFB_KB_KEY_KP_7: return KeyCode::KP7;
        case MFB_KB_KEY_KP_8: return KeyCode::KP8;
        case MFB_KB_KEY_KP_9: return KeyCode::KP9;
        case MFB_KB_KEY_KP_DECIMAL: return KeyCode::KPDecimal;
        case MFB_KB_KEY_KP_DIVIDE: return KeyCode::KPDivide;
        case MFB_KB_KEY_KP_MULTIPLY: return KeyCode::KPMultiply;
        case MFB_KB_KEY_KP_SUBTRACT: return KeyCode::KPSubtract;
        case MFB_KB_KEY_KP_ADD: return KeyCode::KPAdd;
        case MFB_KB_KEY_KP_ENTER: return KeyCode::KPEnter;
        case MFB_KB_KEY_KP_EQUAL: return KeyCode::KPEqual;

        case MFB_KB_KEY_LEFT_SHIFT: return KeyCode::LeftShift;
        case MFB_KB_KEY_LEFT_CONTROL: return KeyCode::LeftControl;
        case MFB_KB_KEY_LEFT_ALT: return KeyCode::LeftAlt;
        case MFB_KB_KEY_LEFT_SUPER: return KeyCode::LeftSuper;
        case MFB_KB_KEY_RIGHT_SHIFT: return KeyCode::RightShift;
        case MFB_KB_KEY_RIGHT_CONTROL: return KeyCode::RightControl;
        case MFB_KB_KEY_RIGHT_ALT: return KeyCode::RightAlt;
        case MFB_KB_KEY_RIGHT_SUPER: return KeyCode::RightSuper;

        case MFB_KB_KEY_MENU: return KeyCode::Menu;
        case MFB_KB_KEY_LEFT_BRACKET: return KeyCode::LeftBracket;
        case MFB_KB_KEY_BACKSLASH: return KeyCode::Backslash;
        case MFB_KB_KEY_RIGHT_BRACKET: return KeyCode::RightBracket;
        case MFB_KB_KEY_GRAVE_ACCENT: return KeyCode::GraveAccent;
        case MFB_KB_KEY_EQUAL: return KeyCode::Equal;
        case MFB_KB_KEY_MINUS: return KeyCode::Minus;
        case MFB_KB_KEY_APOSTROPHE: return KeyCode::Apostrophe;
        case MFB_KB_KEY_COMMA: return KeyCode::Comma;
        case MFB_KB_KEY_PERIOD: return KeyCode::Period;
        case MFB_KB_KEY_SLASH: return KeyCode::Slash;
        case MFB_KB_KEY_SEMICOLON: return KeyCode::Semicolon;

        default: return KeyCode::Unknown;
    }
}

static KeyModifier map_mfb_mod_to_key_mod(mfb_key_mod mod) {
    KeyModifier result = KeyModifier::None;
    if (mod & MFB_KB_MOD_SHIFT) result |= KeyModifier::Shift;
    if (mod & MFB_KB_MOD_CONTROL) result |= KeyModifier::Control;
    if (mod & MFB_KB_MOD_ALT) result |= KeyModifier::Alt;
    if (mod & MFB_KB_MOD_SUPER) result |= KeyModifier::Super;
    return result;
}

MiniFBWindow::MiniFBWindow(const char* title, unsigned int width, unsigned int height, int min_width, int min_height)
    : m_running(true),
      m_presentation_pixel_buffer(width * height) {
    m_window = mfb_open_ex(title, width, height, MFB_WF_RESIZABLE);
    if (!m_window) {
        m_running = false;
    } else {
        mfb_set_user_data(m_window, this);
    }
}

MiniFBWindow::~MiniFBWindow() {
    if (m_window) {
        mfb_close(m_window);
        m_window = nullptr;
    }
}

bool MiniFBWindow::is_running() const {
    return m_running;
}

void MiniFBWindow::close() {
    m_running = false;
}

bool MiniFBWindow::is_active() const {
    return m_window ? mfb_is_window_active(m_window) : false;
}

void MiniFBWindow::poll_events() {
    if (!m_running || !m_window) return;
    if (mfb_update_events(m_window) < 0) {
        m_running = false;
    }
}

int MiniFBWindow::width() const {
    return m_window ? mfb_get_window_width(m_window) : 0;
}

int MiniFBWindow::height() const {
    return m_window ? mfb_get_window_height(m_window) : 0;
}

void MiniFBWindow::dispatch_key_callback(KeyCode key, KeyModifier mod, bool is_pressed) {
    if (m_key_callback) {
        m_key_callback(key, mod, is_pressed);
    }
}

void MiniFBWindow::dispatch_active_callback(bool is_active) {
    if (m_active_callback) {
        m_active_callback(is_active);
    }
}

static void mfb_key_stub(struct mfb_window* window, mfb_key key, mfb_key_mod mod, bool is_pressed) {
    auto* self = static_cast<MiniFBWindow*>(mfb_get_user_data(window));
    if (self) {
        KeyCode key_code = map_mfb_key_to_key_code(key);
        KeyModifier key_mod = map_mfb_mod_to_key_mod(mod);
        self->dispatch_key_callback(key_code, key_mod, is_pressed);
    }
}

static void mfb_active_stub(struct mfb_window* window, bool is_active) {
    auto* self = static_cast<MiniFBWindow*>(mfb_get_user_data(window));
    if (self) {
        self->dispatch_active_callback(is_active);
    }
}

void MiniFBWindow::set_key_callback(KeyCallback cb) {
    m_key_callback = cb;
    if (m_window) {
        mfb_set_keyboard_callback(m_window, mfb_key_stub);
    }
}

void MiniFBWindow::set_active_callback(ActiveCallback cb) {
    m_active_callback = cb;
    if (m_window) {
        mfb_set_active_callback(m_window, mfb_active_stub);
    }
}

void MiniFBWindow::present(const std::vector<uint32_t>& buffer, int target_w, int target_h) {
    if (!m_running || !m_window) return;

    int window_w = this->width();
    int window_h = this->height();

    if (window_w <= 0 || window_h <= 0) return;

    size_t required_size = static_cast<size_t>(window_w * window_h);
    if (m_presentation_pixel_buffer.size() < required_size) {
        m_presentation_pixel_buffer.resize(required_size);
    }

    std::fill(m_presentation_pixel_buffer.begin(), m_presentation_pixel_buffer.end(), 0xFF000000);

    int scale_x = window_w / target_w;
    int scale_y = window_h / target_h;
    int scale = std::min(scale_x, scale_y);

    if (scale >= 1) {
        int scaled_w = target_w * scale;
        int scaled_h = target_h * scale;
        int offset_x = (window_w - scaled_w) / 2;
        int offset_y = (window_h - scaled_h) / 2;

        for (int ly = 0; ly < target_h; ++ly) {
            int py_start = offset_y + (ly * scale);

            for (int lx = 0; lx < target_w; ++lx) {
                uint32_t pixel = buffer[ly * target_w + lx];
                int px_start = offset_x + (lx * scale);

                for (int sy = 0; sy < scale; ++sy) {
                    int target_y = py_start + sy;
                    uint32_t* row_ptr = &m_presentation_pixel_buffer[target_y * window_w + px_start];

                    for (int sx = 0; sx < scale; ++sx) {
                        row_ptr[sx] = pixel;
                    }
                }
            }
        }
    } else {
        int start_lx = (target_w - window_w) / 2;
        int start_ly = (target_h - window_h) / 2;

        for (int wy = 0; wy < window_h; ++wy) {
            int ly = start_ly + wy;
            if (ly < 0 || ly >= target_h) continue;

            uint32_t* dest_row = &m_presentation_pixel_buffer[wy * window_w];
            const uint32_t* src_row = &buffer[ly * target_w];

            for (int wx = 0; wx < window_w; ++wx) {
                int lx = start_lx + wx;
                if (lx >= 0 && lx < target_w) {
                    dest_row[wx] = src_row[lx];
                }
            }
        }
    }

    if (mfb_update_ex(m_window, m_presentation_pixel_buffer.data(), window_w, window_h) < 0) {
        m_running = false;
    }
}

#if defined(__APPLE__)
#include <objc/message.h>
#include <objc/runtime.h>
#include <CoreGraphics/CoreGraphics.h>

namespace {
    struct MfbInternalWindowData {
        void* specific;
    };
}
#endif

void MiniFBWindow::move_to_left_edge() {
#if defined(__APPLE__)
    if (!m_window) return;

    auto* window_data = reinterpret_cast<MfbInternalWindowData*>(m_window);
    if (!window_data || !window_data->specific) return;

    id ns_window = *reinterpret_cast<id*>(window_data->specific);
    if (!ns_window) return;

    Class ns_screen_class = reinterpret_cast<Class>(objc_getClass("NSScreen"));
    if (!ns_screen_class) return;

    SEL sel_main_screen = sel_registerName("mainScreen");
    SEL sel_visible_frame = sel_registerName("visibleFrame");
    SEL sel_frame = sel_registerName("frame");
    SEL sel_set_frame_origin = sel_registerName("setFrameOrigin:");

    id main_screen = reinterpret_cast<id(*)(Class, SEL)>(objc_msgSend)(ns_screen_class, sel_main_screen);
    if (!main_screen) return;

    typedef CGRect (*FrameSendFunc)(id, SEL);
    FrameSendFunc frame_send = reinterpret_cast<FrameSendFunc>(objc_msgSend);

    CGRect visible_frame = frame_send(main_screen, sel_visible_frame);
    CGRect window_frame = frame_send(ns_window, sel_frame);

    CGPoint new_origin;
    new_origin.x = visible_frame.origin.x;
    new_origin.y = visible_frame.origin.y + visible_frame.size.height - window_frame.size.height;

    typedef void (*SetOriginFunc)(id, SEL, CGPoint);
    reinterpret_cast<SetOriginFunc>(objc_msgSend)(ns_window, sel_set_frame_origin, new_origin);
#endif
}
