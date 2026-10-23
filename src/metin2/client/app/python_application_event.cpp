#include "python_application.h"

#include <metin2/client/background/python_background.h>
#include <metin2/graphics/camera.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline app {
    void python_application::on_camera_update() {
        if (get_python_background().is_map_ready()) {
            auto *camera = get_camera_manager().current_camera();
            if (camera) {
                camera->update();
            }
        }
    }

    void python_application::on_ui_update() {
        widgets::get_window_manager().update();
    }

    void python_application::on_ui_render() {
        widgets::get_window_manager().render();
    }

    void python_application::on_mouse_middle_button_down(int x, int y) {
        auto *camera = get_camera_manager().current_camera();
        if (camera) {
            camera->begin_drag(x, y);
        }

        if (!get_python_background().is_map_ready()) {
            return;
        }

        set_cursor_num(CAMERA_ROTATE);
        if (CURSOR_MODE_HARDWARE == get_cursor_mode()) {
            set_cursor_visible(false, true);
        }
    }

    void python_application::on_mouse_middle_button_up(int x, int y) {
        auto *camera = get_camera_manager().current_camera();
        if (camera) {
            camera->end_drag();
        }

        if (!get_python_background().is_map_ready()) {
            return;
        }

        set_cursor_num(NORMAL);
        if (CURSOR_MODE_HARDWARE == get_cursor_mode()) {
            set_cursor_visible(true);
        }
    }

    void python_application::on_mouse_wheel(int len) {
        auto *camera = get_camera_manager().current_camera();
        if (camera) {
            camera->wheel(len);
        }
    }

    void python_application::on_mouse_move(int x, int y) {
        auto *camera = get_camera_manager().current_camera();
        if (camera) {
            int drag_x = 0;
            int drag_y = 0;

            if (get_python_background().is_map_ready() && camera->drag(x, y, drag_x, drag_y)) {
                x = drag_x;
                y = drag_y;

                window::set_mouse_position({drag_x, drag_y});
            }
        }

        const auto client = size();

        auto &window_manager = widgets::get_window_manager();

        window_manager.set_resolution(client.x, client.y);
        window_manager.run_mouse_move(x, y);
    }

    void python_application::on_mouse_left_button_down(int x, int y) {
        widgets::get_window_manager().run_mouse_left_button_down(x, y);
    }

    void python_application::on_mouse_left_button_up(int x, int y) {
        widgets::get_window_manager().run_mouse_left_button_up(x, y);
    }

    void python_application::on_mouse_left_button_double_click(int x, int y) {
        auto &window_manager = widgets::get_window_manager();

        window_manager.run_mouse_left_button_down(x, y);
        window_manager.run_mouse_left_button_double_click(x, y);
    }

    void python_application::on_mouse_right_button_down(int x, int y) {
        widgets::get_window_manager().run_mouse_right_button_down(x, y);
    }

    void python_application::on_mouse_right_button_up(int x, int y) {
        widgets::get_window_manager().run_mouse_right_button_up(x, y);
    }

    void python_application::on_key_down(int index) {
        auto &window_manager = widgets::get_window_manager();

        if (index == DIK_ESCAPE) {
            window_manager.run_press_escape_key();
        }

        window_manager.run_key_down(index);
    }

    void python_application::on_key_up(int index) {
        widgets::get_window_manager().run_key_up(index);
    }

    void python_application::on_ime_key_down(int index) {
        widgets::get_window_manager().run_ime_key_down(index);
    }

    void python_application::run_press_exit_key() {
        widgets::get_window_manager().run_press_exit_key();
    }

    void python_application::on_mouse_update() {
        int32_t lx, ly;

        widgets::get_window_manager().mouse_position(lx, ly);

        py_call_class_member_func(mouse_handler_, "update", Py_BuildValue("(ii)", lx, ly));
    }

    void python_application::on_mouse_render() {
        py_call_class_member_func(mouse_handler_, "render", Py_BuildValue("()"));
    }
}
