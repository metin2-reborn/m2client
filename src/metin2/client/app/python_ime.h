#pragma once

#include <metin2/input/ime.h>

namespace metin2::inline client::inline app {
    class python_ime : public ime_event_sink, public cime {
      public:
        python_ime();

        ~python_ime() override;

        void move_left();
        void move_right();
        void move_home();
        void move_end();
        void set_cursor_position(int position);
        void delete_value();

        void create();

      protected:
        virtual void on_tab();
        virtual void on_return();
        virtual void on_escape();

        auto on_char_event(char32_t codepoint) -> bool override;
        void on_update() override;
    };

    inline auto get_python_ime() -> python_ime & {
        static python_ime instance;

        return instance;
    }
}
