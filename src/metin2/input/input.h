#pragma once

#include "keys.h"

namespace metin2::inline input {

    // Whether a key (DIK_* code) is held: the automation key override, or the real key while the
    // window has the focus.
    auto is_key_down(int dik) -> bool;

    // Keys pressed by the automation harness count as held until it releases them.
    void set_key_override(int dik, bool down);

    // Scroll Lock state, toggled by the presses the game sees (the Windows client read the LED state).
    auto scroll_lock() -> bool;

    // Polls the keyboard once per frame and reports the changes since the last poll.
    class input_keyboard {
      public:
        input_keyboard();
        virtual ~input_keyboard();

        void update_keyboard();
        void reset_keyboard();

        auto is_pressed(int index) -> bool;
        void key_down(int index);
        void key_up(int index);

      protected:
        virtual void on_key_down(int index) = 0;
        virtual void on_key_up(int index) = 0;

      protected:
        static bool pressed_key_[256];
    };
}
