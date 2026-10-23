#pragma once
#include <cstdint>
#include <list>
#include <map>
#include <metin2/script/python_header.h>
#include <string>

namespace metin2::inline ui {

    namespace widgets {
        class ui_window;

        class window_manager {
          public:
            using layer_container = std::map<std::string, ui_window *>;
            using window_container = std::list<ui_window *>;
            using key_capture_window_map = std::map<int, ui_window *>;

          public:
            window_manager();
            ~window_manager();

            void destroy();

            auto aspect() -> float;
            void set_screen_size(int32_t width_value, int32_t height);
            void set_resolution(int hres, int vres);
            // The window was resized: resizes the layers, moves the windows with the screen edges and
            // calls OnScreenResize(width, height) on the windows that define it.
            void resize_screen(int32_t width_value, int32_t height);

            void get_resolution(int32_t &rx, int32_t &ry) {
                rx = hres_;
                ry = vres_;
            }

            void set_mouse_handler(PyObject *mouse_handler);
            auto screen_width() -> int32_t { return width_; }
            auto screen_height() -> int32_t { return height_; }
            void mouse_position(int32_t &rx, int32_t &ry);
            auto is_dragging() -> bool;

            auto get_lock_window() -> ui_window * { return lock_window_; }
            auto point_window() -> ui_window * { return point_window_; }
            auto is_focus() -> bool { return (active_window_ || lock_window_); }
            auto is_focus_window(ui_window *window) -> bool { return window == active_window_; }

            void set_parent(ui_window *window, ui_window *parent_window);
            void set_pick_always(ui_window *window);

            // Adds a new window to a layer and takes ownership; nullptr (window deleted) for an unknown layer.
            auto register_window(ui_window *win, const char *layer) -> ui_window *;

            void destroy_window(ui_window *win);
            void notify_destroy_window(ui_window *window);

            auto is_attaching() -> bool;
            auto attaching_index() -> uint32_t;
            auto attaching_slot_number() -> uint32_t;
            void attaching_icon_size(uint8_t *width_value, uint8_t *height);
            void attach_icon(uint32_t type, uint32_t index, uint32_t slot_number, uint8_t width_value, uint8_t height);
            void deattach_icon();
            void set_attaching_flag(bool flag);

            void once_ignore_mouse_left_button_up_event();
            void lock_window(ui_window *win);
            void unlock_window();

            void activate_window(ui_window *win);
            void deactivate_window();
            auto get_activate_window() -> ui_window *;
            void set_top(ui_window *win);
            void set_top_ui_window();

            void update();
            void render();

            void run_mouse_move(int32_t x, int32_t y);
            void run_mouse_left_button_down(int32_t x, int32_t y);
            void run_mouse_left_button_up(int32_t x, int32_t y);
            void run_mouse_left_button_double_click(int32_t x, int32_t y);
            void run_mouse_right_button_down(int32_t x, int32_t y);
            void run_mouse_right_button_up(int32_t x, int32_t y);
            void run_mouse_middle_button_down(int32_t x, int32_t y);
            void run_mouse_middle_button_up(int32_t x, int32_t y);

            // The window that receives typed text: the active window while it renders and, under a
            // lock window, lies inside it; nullptr otherwise.
            auto edit_target() -> ui_window *;
            void run_char(char32_t codepoint);
            void run_ime_tab_event();
            void run_ime_return_event();
            void run_ime_key_down(int vkey);

            void run_key_down(int vkey);
            void run_key_up(int vkey);
            void run_press_escape_key();
            void run_press_exit_key();

          private:
            void set_mouse_position(int32_t x, int32_t y);
            auto pick_window_impl(int32_t x, int32_t y) -> ui_window *;

            void clear_reserve_delete_window_list();

          private:
            int32_t width_;
            int32_t height_;

            int vres_;
            int hres_;

            int32_t mouse_x_, mouse_y_;
            int32_t drag_x_, drag_y_;
            int32_t picked_x_, picked_y_;

            bool once_ignore_mouse_left_button_up_event_flag_;
            uint32_t ignore_end_time_ = 0;

            PyObject *mouse_handler_;
            bool attaching_flag_;
            uint32_t attaching_type_;
            uint32_t attaching_index_;
            uint32_t attaching_slot_number_;
            uint8_t attaching_icon_width_;
            uint8_t attaching_icon_height_;

            ui_window *active_window_;
            window_container active_window_list_;
            ui_window *lock_window_;
            window_container lock_window_list_;
            ui_window *point_window_;
            ui_window *left_capture_window_;
            ui_window *right_capture_window_;
            ui_window *middle_capture_window_;
            key_capture_window_map key_capture_window_map_value_;
            window_container reserve_delete_window_list_;
            window_container pick_always_window_list_;

            ui_window *root_window_;
            window_container layer_window_list_;
            layer_container layer_window_map_;
        };

        inline auto get_window_manager() -> window_manager & {
            static window_manager instance;
            return instance;
        }

        auto build_empty_tuple() -> PyObject *;
    };
}
