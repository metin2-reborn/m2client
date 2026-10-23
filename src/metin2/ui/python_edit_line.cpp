#include "python_edit_line.h"

#include <metin2/core/timer.h>
#include <metin2/core/utf8.h>
#include <metin2/input/input.h>
#include <metin2/input/keys.h>
#include <metin2/window/main_window.h>
#include "python_graphic.h"
#include "python_window_manager.h"

#include <algorithm>

namespace metin2::inline ui::widgets {

    namespace {
        constexpr uint32_t SELECTION_COLOR = 0x663366cc;
        constexpr uint32_t CARET_COLOR = 0xffffffff;
        constexpr uint32_t BLINK_PERIOD_MS = 1000;

        auto shift_down() -> bool {
            return input::is_key_down(DIK_LSHIFT) || input::is_key_down(DIK_RSHIFT);
        }

        auto ctrl_down() -> bool {
            return input::is_key_down(DIK_LCONTROL) || input::is_key_down(DIK_RCONTROL);
        }
    }

    edit_line::edit_line(PyObject *handler) : text_line(handler) {
    }

    void edit_line::set_max_bytes(int bytes) {
        buffer_.set_max_bytes(static_cast<std::size_t>(std::max(0, bytes)));
    }

    void edit_line::set_max_visible_bytes(int bytes) {
        buffer_.set_max_visible_bytes(static_cast<std::size_t>(std::max(0, bytes)));
    }

    void edit_line::set_number_mode(bool on) {
        buffer_.set_number_mode(on);
    }

    void edit_line::set_secret(bool on) {
        text_line::set_secret(on);
        buffer_.set_secret(on);
        caret_moved();
    }

    void edit_line::set_multi_line(bool on) {
        text_line::set_multi_line(on);
        multi_line_ = on;
        scroll_x_ = 0.0f;
    }

    void edit_line::set_caret(int visible_index) {
        if (visible_index < 0)
            buffer_.move_end(false);
        else
            buffer_.set_caret(static_cast<std::size_t>(visible_index), false);
        caret_moved();
    }

    auto edit_line::caret() -> int {
        return static_cast<int>(buffer_.caret());
    }

    void edit_line::select_all() {
        buffer_.select_all();
        caret_moved();
    }

    auto edit_line::insert_text(const char *utf8) -> bool {
        if (!buffer_.insert_text(utf8::to_u32(utf8)))
            return false;
        text_changed();
        return true;
    }

    auto edit_line::erase_backward() -> bool {
        if (!buffer_.erase_backward())
            return false;
        text_changed();
        return true;
    }

    auto edit_line::on_char(char32_t codepoint) -> bool {
        if (!focused_)
            return false;
        if (buffer_.insert_char(codepoint))
            text_changed();
        return true;
    }

    auto edit_line::on_ime_key_down_event(int vk) -> bool {
        const bool shift = shift_down();
        const bool ctrl = ctrl_down();
        switch (vk) {
        case VK_LEFT:
            buffer_.move_left(shift, ctrl);
            caret_moved();
            return true;
        case VK_RIGHT:
            buffer_.move_right(shift, ctrl);
            caret_moved();
            return true;
        case VK_HOME:
            buffer_.move_home(shift);
            caret_moved();
            return true;
        case VK_END:
            buffer_.move_end(shift);
            caret_moved();
            return true;
        case VK_BACK:
            erase_backward();
            return true;
        case VK_DELETE:
            if (buffer_.erase_forward())
                text_changed();
            return true;
        case VK_RETURN:
            get_window_manager().run_ime_return_event();
            return true;
        case VK_TAB:
            get_window_manager().run_ime_tab_event();
            return true;
        case 'A':
            if (!ctrl)
                break;
            select_all();
            return true;
        case 'C':
            if (!ctrl)
                break;
            if (buffer_.has_selection())
                window::set_clipboard_text(utf8::from_u32(buffer_.copy()));
            return true;
        case 'X':
            if (!ctrl)
                break;
            if (const std::u32string cut = buffer_.cut(); !cut.empty()) {
                window::set_clipboard_text(utf8::from_u32(cut));
                text_changed();
            }
            return true;
        case 'V':
            if (!ctrl)
                break;
            insert_text(window::clipboard_text().c_str());
            return true;
        default:
            break;
        }
        return ui_window::on_ime_key_down_event(vk);
    }

    void edit_line::on_set_text(const char *text) {
        buffer_.set_text(utf8::to_u32(text));
        text_line::on_set_text(text);
        caret_moved();
    }

    void edit_line::text_changed() {
        text_line::on_set_text(buffer_.utf8().c_str());
        caret_moved();
        ui_window::on_ime_update();
    }

    void edit_line::caret_moved() {
        scroll_to_caret();
        blink_start_ = el_timer_get_m_sec();
    }

    void edit_line::scroll_to_caret() {
        if (multi_line_ || width_ <= 0) {
            scroll_x_ = 0.0f;
            return;
        }
        text_instance_.update();
        const float caret_x = text_instance_.character_x(buffer_.caret());
        const float visible = static_cast<float>(width_ - 2);
        if (caret_x - scroll_x_ > visible)
            scroll_x_ = caret_x - visible;
        if (caret_x < scroll_x_)
            scroll_x_ = caret_x;
        int text_width = 0, text_height = 0;
        text_instance_.text_size(&text_width, &text_height);
        scroll_x_ = std::clamp(scroll_x_, 0.0f, std::max(0.0f, static_cast<float>(text_width) - visible));
    }

    void edit_line::on_change_position() {
        text_line::on_change_position();
        if (!multi_line_ && width_ > 0)
            text_instance_.set_limit_width(static_cast<float>(width_));
    }

    void edit_line::on_set_focus() {
        focused_ = true;
        buffer_.move_end(false);
        caret_moved();
        text_line::on_set_focus();
    }

    void edit_line::on_kill_focus() {
        focused_ = false;
        selecting_ = false;
        buffer_.clear_selection();
        text_line::on_kill_focus();
    }

    auto edit_line::mouse_visible_index() -> std::size_t {
        int32_t lx = 0, ly = 0;
        mouse_local_position(lx, ly);
        float origin_x = 0.0f, origin_y = 0.0f;
        text_instance_.aligned_origin(&origin_x, &origin_y);
        const float x = static_cast<float>(lx) + scroll_x_ - (origin_x - static_cast<float>(rect_.left));
        const float y = static_cast<float>(ly) - (origin_y - static_cast<float>(rect_.top));
        return text_instance_.character_index_at(x, y);
    }

    auto edit_line::on_mouse_left_button_down() -> bool {
        if (text_line::on_mouse_left_button_down())
            return true;
        get_window_manager().activate_window(this);
        buffer_.set_caret(mouse_visible_index(), shift_down());
        selecting_ = true;
        caret_moved();
        return true;
    }

    auto edit_line::on_mouse_left_button_up() -> bool {
        selecting_ = false;
        return text_line::on_mouse_left_button_up();
    }

    auto edit_line::on_mouse_left_button_double_click() -> bool {
        if (text_line::on_mouse_left_button_double_click())
            return true;
        buffer_.select_word_at(mouse_visible_index());
        selecting_ = false;
        caret_moved();
        return true;
    }

    void edit_line::on_update() {
        text_line::on_update();
        if (selecting_ && focused_) {
            buffer_.set_caret(mouse_visible_index(), true);
            scroll_to_caret();
        }
    }

    void edit_line::render_bar(float left, float top, float right, float bottom, uint32_t color) {
        get_python_graphic().set_diffuse_color(color);
        get_python_graphic().render_bar2d(left, top, right, bottom);
    }

    void edit_line::on_render() {
        if (!is_show())
            return;

        float origin_x = 0.0f, origin_y = 0.0f;
        text_instance_.aligned_origin(&origin_x, &origin_y);
        origin_x -= scroll_x_;
        int text_width = 0, text_height = 0;
        text_instance_.text_size(&text_width, &text_height);
        const auto line_height = static_cast<float>(text_height);
        const float clip_left = static_cast<float>(rect_.left);
        const float clip_right = width_ > 0 ? static_cast<float>(rect_.right) : origin_x + static_cast<float>(text_width) + 1.0f;

        if (focused_ && buffer_.has_selection()) {
            const auto [begin, end] = buffer_.selection();
            if (multi_line_) {
                for (std::size_t i = begin; i < end; ++i) {
                    float x = 0.0f, y = 0.0f, next_x = 0.0f, next_y = 0.0f;
                    text_instance_.character_position(i, &x, &y);
                    text_instance_.character_position(i + 1, &next_x, &next_y);
                    const float right = next_y == y ? next_x : static_cast<float>(text_width);
                    render_bar(origin_x + x, origin_y + y, origin_x + right, origin_y + y + line_height, SELECTION_COLOR);
                }
            } else {
                const float left = std::max(clip_left, origin_x + text_instance_.character_x(begin));
                const float right = std::min(clip_right, origin_x + text_instance_.character_x(end));
                if (right > left)
                    render_bar(left, origin_y, right, origin_y + line_height, SELECTION_COLOR);
            }
        }

        text_instance_.set_scroll_x(scroll_x_);
        text_instance_.render();

        if (focused_ && (el_timer_get_m_sec() - blink_start_) % BLINK_PERIOD_MS < BLINK_PERIOD_MS / 2) {
            float x = 0.0f, y = 0.0f;
            text_instance_.character_position(buffer_.caret(), &x, &y);
            const float caret_x = origin_x + x;
            if (caret_x >= clip_left - 1.0f && caret_x <= clip_right)
                render_bar(caret_x, origin_y + y, caret_x + 1.0f, origin_y + y + line_height, CARET_COLOR);
        }
    }
}
