#include "input.h"
#include "keymap.h"

#include <raylib.h>

#include <algorithm>

namespace metin2::inline input {

    namespace {
        bool key_overrides[256] = {};
        bool scroll_lock_on = false;
    }

    auto is_key_down(int dik) -> bool {
        if (dik <= 0 || dik >= 256)
            return false;
        if (key_overrides[dik])
            return true;
        if (!IsWindowReady() || !IsWindowFocused())
            return false;
        const int key = dik_to_raylib(dik);
        return key != 0 && IsKeyDown(key);
    }

    void set_key_override(int dik, bool down) {
        if (dik > 0 && dik < 256)
            key_overrides[dik] = down;
    }

    auto scroll_lock() -> bool {
        return scroll_lock_on;
    }

    bool input_keyboard::pressed_key_[256];

    input_keyboard::input_keyboard() {
        reset_keyboard();
    }

    input_keyboard::~input_keyboard() = default;

    void input_keyboard::reset_keyboard() {
        std::fill(std::begin(pressed_key_), std::end(pressed_key_), false);
    }

    void input_keyboard::update_keyboard() {
        for (int i = 1; i < 256; ++i) {
            if (is_key_down(i)) {
                if (!is_pressed(i))
                    key_down(i);
            } else if (is_pressed(i))
                key_up(i);
        }
    }

    void input_keyboard::key_down(int index) {
        pressed_key_[index] = true;
        if (index == DIK_SCROLL)
            scroll_lock_on = !scroll_lock_on;
        on_key_down(index);
    }

    void input_keyboard::key_up(int index) {
        pressed_key_[index] = false;
        on_key_up(index);
    }

    auto input_keyboard::is_pressed(int index) -> bool {
        return pressed_key_[index];
    }
}
