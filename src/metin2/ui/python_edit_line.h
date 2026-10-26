#pragma once

#include <metin2/graphics/grp_image_instance.h>
#include "python_window.h"
#include "text_edit_buffer.h"

namespace metin2::inline ui::widgets {

    // A single-line (or, for the whisper dialog, wrapping) text box: typing, caret, selection with
    // Shift and the mouse, word jumps with Ctrl, clipboard with Ctrl+C/X/V. Python learns about
    // changes through the usual callbacks (on_ime_update, on_ime_return, on_ime_tab,
    // on_press_escape_key); keys the line does not use still reach on_ime_key_down. An optional
    // background image (a nine patch stretches to the box) is drawn behind the text.
    class edit_line : public text_line {
      public:
        explicit edit_line(PyObject *handler);

        ~edit_line() override = default;

        void set_max_bytes(int bytes);
        void set_max_visible_bytes(int bytes);
        void set_number_mode(bool on);
        void set_secret(bool on) override;
        void set_multi_line(bool on) override;
        auto set_background(const char *file_name) -> bool;
        void set_size(int32_t width_value, int32_t height) override;

        void set_caret(int visible_index); // -1 puts the caret at the end
        auto caret() -> int;
        void select_all();
        auto insert_text(const char *utf8) -> bool;
        auto erase_backward() -> bool;

        auto on_char(char32_t codepoint) -> bool override;
        auto on_ime_key_down_event(int vk) -> bool override;

      protected:
        void on_set_text(const char *text) override;
        void on_update() override;
        void on_render() override;
        void on_change_position() override;
        void on_set_focus() override;
        void on_kill_focus() override;
        auto on_mouse_left_button_down() -> bool override;
        auto on_mouse_left_button_up() -> bool override;
        auto on_mouse_left_button_double_click() -> bool override;

      private:
        void text_changed();
        void caret_moved();
        void scroll_to_caret();
        auto mouse_visible_index() -> std::size_t;
        void render_bar(float left, float top, float right, float bottom, uint32_t color);

        text_edit_buffer buffer_;
        graphic_image_instance background_;
        float scroll_x_ = 0.0f;
        bool focused_ = false;
        bool selecting_ = false;
        bool multi_line_ = false;
        uint32_t blink_start_ = 0;
    };
}
