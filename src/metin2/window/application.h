#pragma once

#include "main_window.h"

namespace metin2::inline window {

    // The main loop: each pass turns the window and input state into events, hands them to the
    // handlers below and then runs one frame (process()), until request_quit() is called.
    class application : public main_window {
      public:
        application();
        ~application() override;

        void run();

      protected:
        virtual auto process() -> bool = 0;

        virtual void on_close_request();
        virtual void on_activate(bool active) {}
        virtual void on_resize(bool minimized) {}
        virtual void on_mouse_button(mouse_button button, bool down, point position) {}
        virtual void on_mouse_wheel(int delta) {}
        virtual void on_ime_key_down(int vk) {}
        virtual void on_char(char32_t codepoint) {}

      private:
        void dispatch(const event &e);
    };
}
