#include "python_ime.h"

#include "abstract_application.h"

#include <metin2/log/log.h>

namespace metin2::inline client::inline app {
    python_ime::python_ime() : cime() {
        event = this;
    }

    python_ime::~python_ime() {
        metin2::log::debug("Clearing the input method state");
    }

    void python_ime::create() {
        initialize();
    }

    void python_ime::move_left() {
        dec_cur_pos();
    }

    void python_ime::move_right() {
        inc_cur_pos();
    }

    void python_ime::move_home() {
        curpos = 0;
    }

    void python_ime::move_end() {
        curpos = lastpos;
    }

    void python_ime::set_cursor_position(int position) {
        set_cur_pos(position);
    }

    void python_ime::delete_value() {
        del_cur_pos();
    }

    void python_ime::on_update() {
        get_abstract_application().run_ime_update();
    }

    void python_ime::on_tab() {
        get_abstract_application().run_ime_tab_event();
    }

    void python_ime::on_return() {
        get_abstract_application().run_ime_return_event();
    }

    void python_ime::on_escape() {
    }

    auto python_ime::on_char_event(char32_t codepoint) -> bool {
        switch (codepoint) {
        case U'\r':
            on_return();
            return true;

        case U'\t':
            if (capture_input == false) {
                return 0;
            }
            on_tab();
            return true;

        case U'\x1b':
            if (capture_input == false)
                return 0;
            on_escape();
            return true;
        }

        return false;
    }
}
