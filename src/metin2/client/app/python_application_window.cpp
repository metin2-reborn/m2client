#include "python_application.h"
#include "python_system.h"

#include <metin2/graphics/camera.h>
#include <metin2/ui/python_graphic.h>
#include <metin2/ui/python_window_manager.h>

#include <algorithm>
#include <cstdlib>

namespace metin2::inline client::inline app {
    void python_application::on_activate(bool active) {
        is_activate_wnd_ = active;

        if (is_activate_wnd_) {
            get_sound_manager().restore_volume();
        } else {
            get_sound_manager().save_volume();
        }
    }

    void python_application::on_char(char32_t codepoint) {
        widgets::get_window_manager().run_char(codepoint);
    }

    // A release only counts when the press happened inside the window (Windows' mouse capture).
    void python_application::on_mouse_button(mouse_button button, bool down, point position) {
        const int double_click_time = 300;
        const int double_click_box = 5;
        static point down_position;

        if (down) {
            ++mouse_capture_count_;
        } else {
            if (mouse_capture_count_ == 0)
                return;
            --mouse_capture_count_;
        }

        // The interface works in logical pixels.
        position.x /= get_python_graphic().interface_scale();
        position.y /= get_python_graphic().interface_scale();

        switch (button) {
        case mouse_button::left:
            if (!down) {
                l_button_up_time_ = el_timer_get_m_sec();
                on_mouse_left_button_up(position.x, position.y);
                break;
            }

            if (el_timer_get_m_sec() - l_button_down_time_ < double_click_time &&
                abs(position.x - down_position.x) < double_click_box &&
                abs(position.y - down_position.y) < double_click_box) {
                l_button_down_time_ = 0;
                on_mouse_left_button_double_click(position.x, position.y);
            } else {
                l_button_down_time_ = el_timer_get_m_sec();
                on_mouse_left_button_down(position.x, position.y);
            }
            down_position = position;
            break;

        case mouse_button::middle:
            if (down)
                widgets::get_window_manager().run_mouse_middle_button_down(position.x, position.y);
            else
                widgets::get_window_manager().run_mouse_middle_button_up(position.x, position.y);
            break;

        case mouse_button::right:
            if (down)
                on_mouse_right_button_down(position.x, position.y);
            else
                on_mouse_right_button_up(position.x, position.y);
            break;
        }
    }

    void python_application::on_resize(bool minimized) {
        is_minimized_wnd_ = minimized;
        if (!minimized) {
            const point client = size();

            grp_device_.resize_back_buffer(client.x, client.y);
            apply_interface_scale();
        }
    }

    // [interface] ui_scale, or when it is 0 a factor that keeps a 1024x768 layout usable: 2 from
    // 1600 window pixels of height, 3 from 2400.
    void python_application::apply_interface_scale() {
        const point client = size();
        int scale = get_python_system().ui_scale();
        if (scale <= 0)
            scale = client.y >= 2400 ? 3 : (client.y >= 1600 ? 2 : 1);
        scale = std::clamp(scale, 1, 3);
        get_python_graphic().set_interface_scale(scale);
        widgets::get_window_manager().resize_screen(client.x / scale, client.y / scale);
    }

    auto python_application::interface_width() const -> int {
        return size().x / get_python_graphic().interface_scale();
    }

    auto python_application::interface_height() const -> int {
        return size().y / get_python_graphic().interface_scale();
    }

    void python_application::on_close_request() {
        run_press_exit_key();
    }
}
