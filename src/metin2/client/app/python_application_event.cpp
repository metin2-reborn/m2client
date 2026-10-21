#include <metin2/graphics/camera.h>
#include <metin2/script/python_utils.h>
#include "python_application.h"

namespace metin2::inline client::inline app {

    void python_application::on_camera_update() {
        if (get_python_background().is_map_ready()) {
            camera *camera_mgr = get_camera_manager().current_camera();
            if (camera_mgr)
                camera_mgr->update();
        }
    }

    void python_application::on_ui_update() {
        widgets::window_manager &ui_mgr = widgets::get_window_manager();
        ui_mgr.update();
    }

    void python_application::on_ui_render() {
        widgets::window_manager &ui_mgr = widgets::get_window_manager();
        ui_mgr.render();
    }

    void python_application::on_mouse_middle_button_down(int x, int y) {
        camera_manager &cmr_mgr = get_camera_manager();
        camera *cmr_cur = cmr_mgr.current_camera();
        if (cmr_cur)
            cmr_cur->begin_drag(x, y);

        if (!get_python_background().is_map_ready())
            return;

        set_cursor_num(CAMERA_ROTATE);
        if (CURSOR_MODE_HARDWARE == get_cursor_mode())
            set_cursor_visible(false, true);
    }

    void python_application::on_mouse_middle_button_up(int x, int y) {
        camera_manager &cmr_mgr = get_camera_manager();
        camera *cmr_cur = cmr_mgr.current_camera();
        if (cmr_cur)
            cmr_cur->end_drag();

        if (!get_python_background().is_map_ready())
            return;

        set_cursor_num(NORMAL);
        if (CURSOR_MODE_HARDWARE == get_cursor_mode())
            set_cursor_visible(true);
    }

    void python_application::on_mouse_wheel(int len) {
        camera_manager &cmr_mgr = get_camera_manager();
        camera *cmr_cur = cmr_mgr.current_camera();
        if (cmr_cur)
            cmr_cur->wheel(len);
    }

    void python_application::on_mouse_move(int x, int y) {
        camera_manager &cmr_mgr = get_camera_manager();
        camera *cmr_cur = cmr_mgr.current_camera();

        if (cmr_cur) {
            int drag_x = 0;
            int drag_y = 0;
            if (get_python_background().is_map_ready() && cmr_cur->drag(x, y, drag_x, drag_y)) {
                x = drag_x;
                y = drag_y;
                window::set_mouse_position({drag_x, drag_y});
            }
        }

        const window::point client = size();

        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.set_resolution(client.x, client.y);

        wnd_mgr.run_mouse_move(x, y);
    }

    void python_application::on_mouse_left_button_down(int x, int y) {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_mouse_left_button_down(x, y);
    }

    void python_application::on_mouse_left_button_up(int x, int y) {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_mouse_left_button_up(x, y);
    }

    void python_application::on_mouse_left_button_double_click(int x, int y) {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_mouse_left_button_down(x, y);
        wnd_mgr.run_mouse_left_button_double_click(x, y);
    }

    void python_application::on_mouse_right_button_down(int x, int y) {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_mouse_right_button_down(x, y);
    }

    void python_application::on_mouse_right_button_up(int x, int y) {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_mouse_right_button_up(x, y);
    }

    void python_application::on_key_down(int index) {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();

        if (DIK_ESCAPE == index) {
            wnd_mgr.run_press_escape_key();
        }

        wnd_mgr.run_key_down(index);
    }

    void python_application::on_key_up(int index) {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_key_up(index);
    }

    void python_application::run_ime_update() {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_ime_update();
    }
    void python_application::run_ime_tab_event() {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_ime_tab_event();
    }
    void python_application::run_ime_return_event() {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_ime_return_event();
    }
    void python_application::on_ime_key_down(int index) {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_ime_key_down(index);
    }

    void python_application::run_press_exit_key() {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        wnd_mgr.run_press_exit_key();
    }

    void python_application::on_mouse_update() {

        widgets::window_manager &wnd_mgr = widgets::get_window_manager();
        int32_t lx, ly;
        wnd_mgr.mouse_position(lx, ly);
        py_call_class_member_func(mouse_handler_, "Update", Py_BuildValue("(ii)", lx, ly));
    }

    void python_application::on_mouse_render() {

        py_call_class_member_func(mouse_handler_, "Render", Py_BuildValue("()"));
    }
}
