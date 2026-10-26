#pragma once

#include <metin2/core/rectangle.h>
#include <metin2/core/utils.h>
#include <metin2/graphics/grp_expanded_image_instance.h>
#include <metin2/graphics/grp_image_instance.h>
#include <metin2/graphics/grp_mark_instance.h>
#include <metin2/graphics/grp_text_instance.h>
#include <metin2/script/python_header.h>

#include <list>
#include <vector>

namespace metin2::inline ui {
    namespace widgets {
        class ui_window {
          public:
            using window_container = std::list<ui_window *>;

            static auto type() -> uint32_t;
            auto is_type(uint32_t type) -> bool;

            enum horizontal_align {
                HORIZONTAL_ALIGN_LEFT = 0,
                HORIZONTAL_ALIGN_CENTER = 1,
                HORIZONTAL_ALIGN_RIGHT = 2,
            };

            enum vertical_align {
                VERTICAL_ALIGN_TOP = 0,
                VERTICAL_ALIGN_CENTER = 1,
                VERTICAL_ALIGN_BOTTOM = 2,
            };

            enum flags {
                FLAG_MOVABLE = (1 << 0), // movable window
                FLAG_LIMIT = (1 << 1),   // the window stays on screen
                FLAG_DRAGABLE = (1 << 3),
                FLAG_ATTACH = (1 << 4),     // attached to its parent (for drag, e.g. ScriptWindow)
                FLAG_RESTRICT_X = (1 << 5), // no horizontal movement
                FLAG_RESTRICT_Y = (1 << 6), // no vertical movement
                FLAG_NOT_CAPTURE = (1 << 7),
                FLAG_FLOAT = (1 << 8),    // floating window that is reordered
                FLAG_NOT_PICK = (1 << 9), // not picked by the mouse
                FLAG_IGNORE_SIZE = (1 << 10),
                FLAG_RTL = (1 << 11), // Right-to-left
            };

          public:
            ui_window(PyObject *ppy_object);

            virtual ~ui_window();

            void add_child(ui_window *win);

            void clear();
            void destroy_handle();
            void update();
            void render();

            void set_name(const char *name);
            auto get_name() -> const char * { return name_value_.c_str(); }
            virtual void set_size(int32_t width_value, int32_t height);
            auto width() -> int32_t { return width_; }
            auto height() -> int32_t { return height_; }

            void set_horizontal_align(uint32_t align);
            void set_vertical_align(uint32_t align);
            void set_position(int32_t x, int32_t y);
            void get_position(int32_t *plx, int32_t *ply);
            auto position_x(void) const -> int32_t { return x_; }
            auto position_y(void) const -> int32_t { return y_; }
            auto rect() -> rectangle & { return rect_; }
            void mouse_local_position(int32_t &rlx, int32_t &rly);
            auto update_rect() -> int32_t;

            // Moves the children after this window changed from old_width x old_height; windows with an
            // OnScreenResize method are added to scripted instead and place themselves.
            void anchor_children(int32_t old_width, int32_t old_height, std::vector<PyObject *> &scripted);
            // Grows a window that covered the whole screen to the new screen size.
            virtual void fill_screen(int32_t width_value, int32_t height) { set_size(width_value, height); }

            void show();
            void hide();
            auto is_show() -> bool { return show_; }
            auto is_rendering() -> bool;

            auto has_parent() -> bool { return parent_ ? true : false; }
            auto has_child() -> bool { return child_list_.empty() ? false : true; }
            auto child_count() -> int { return child_list_.size(); }

            auto root() -> ui_window *;
            auto parent() -> ui_window *;
            void delete_child(ui_window *win);
            void set_top(ui_window *win);

            auto is_in(int32_t x, int32_t y) -> bool;
            auto is_in() -> bool;
            auto pick_window(int32_t x, int32_t y) -> ui_window *;
            auto pick_top_window(int32_t x, int32_t y) -> ui_window *; // Checks only this level, without descending into the children.

            void remove_reserve_children();

            void add_flag(uint32_t flag) { SET_BIT(flag_, flag); }
            void remove_flag(uint32_t flag) { REMOVE_BIT(flag_, flag); }
            auto is_flag(uint32_t flag) -> bool { return (flag_ & flag) ? true : false; }

            virtual void on_render();
            virtual void on_update();
            virtual void on_change_position() {}

            virtual void on_set_focus();
            virtual void on_kill_focus();

            virtual void on_mouse_drag(int32_t lx, int32_t ly);
            virtual void on_mouse_over_in();
            // The script's on_mouse_wheel(delta); true when it took the wheel.
            auto on_mouse_wheel(int32_t delta) -> bool;
            virtual void on_mouse_over_out();
            virtual void on_mouse_over();
            virtual void on_drop();
            virtual void on_top();
            virtual void on_ime_update();

            virtual void on_move_window(int32_t x, int32_t y);

            auto run_ime_tab_event() -> bool;
            auto run_ime_return_event() -> bool;
            auto run_ime_key_down_event(int ikey) -> bool;

            auto run_key_down_event(int ikey) -> ui_window *;
            auto run_key_up_event(int ikey) -> bool;
            auto run_press_escape_key_event() -> bool;
            auto run_press_exit_key_event() -> bool;

            virtual auto on_ime_tab_event() -> bool;
            virtual auto on_ime_return_event() -> bool;
            virtual auto on_ime_key_down_event(int ikey) -> bool;
            // A typed character for the focused window; only edit lines take it.
            virtual auto on_char(char32_t codepoint) -> bool { return false; }

            // Whether `ancestor` is this window or one of its parents.
            auto is_within(ui_window *ancestor) -> bool;

            virtual auto on_mouse_left_button_down() -> bool;
            virtual auto on_mouse_left_button_up() -> bool;
            virtual auto on_mouse_left_button_double_click() -> bool;
            virtual auto on_mouse_right_button_down() -> bool;
            virtual auto on_mouse_right_button_up() -> bool;
            virtual auto on_mouse_right_button_double_click() -> bool;
            virtual auto on_mouse_middle_button_down() -> bool;
            virtual auto on_mouse_middle_button_up() -> bool;

            virtual auto on_key_down(int ikey) -> bool;
            virtual auto on_key_up(int ikey) -> bool;
            virtual auto on_press_escape_key() -> bool;
            virtual auto on_press_exit_key() -> bool;

            virtual void set_color(uint32_t color) {}
            virtual auto on_is_type(uint32_t type) -> bool;

            virtual auto is_window() -> bool { return true; }

          protected:
            std::string name_value_;

            horizontal_align horizontal_align_value_;
            vertical_align vertical_align_value_;
            int32_t x_, y_;          // relative position
            int32_t width_, height_; // size
            rectangle rect_;         // global rectangle

            bool movable_;
            bool show_;

            uint32_t flag_;

            PyObject *handler_;
            // Whether the handler defines on_update / on_render. Most widgets do not, and a failed
            // attribute lookup per window per frame (with its AttributeError) is what the per-frame
            // calls would otherwise cost.
            bool handler_has_on_update_ = false;
            bool handler_has_on_render_ = false;

            ui_window *parent_;
            window_container child_list_;

            bool is_updating_children_;
            window_container reserve_child_list_;
        };

        class layer : public ui_window {
          public:
            layer(PyObject *ppy_object) : ui_window(ppy_object) {}

            ~layer() override = default;

            auto is_window() -> bool override { return false; }
        };

        class box : public ui_window {
          public:
            box(PyObject *ppy_object);

            ~box() override;

            void set_color(uint32_t color) override;

          protected:
            void on_render() override;

          protected:
            uint32_t color_value_;
        };

        class bar : public ui_window {
          public:
            bar(PyObject *ppy_object);

            ~bar() override;

            void set_color(uint32_t color) override;

          protected:
            void on_render() override;

          protected:
            uint32_t color_value_;
        };

        class line : public ui_window {
          public:
            line(PyObject *ppy_object);

            ~line() override;

            void set_color(uint32_t color) override;

          protected:
            void on_render() override;

          protected:
            uint32_t color_value_;
        };

        class bar3d : public ui_window {
          public:
            static auto type() -> uint32_t;

          public:
            bar3d(PyObject *ppy_object);

            ~bar3d() override;

            using ui_window::set_color;
            void set_color(uint32_t left, uint32_t right, uint32_t center);

          protected:
            void on_render() override;

          protected:
            uint32_t left_color_;
            uint32_t right_color_;
            uint32_t center_color_;
        };

        class text_line : public ui_window {
          public:
            text_line(PyObject *ppy_object);

            ~text_line() override;

            void set_horizontal_align(int type);
            void set_vertical_align(int type);
            virtual void set_secret(bool flag);
            void set_outline(bool flag);
            virtual void set_multi_line(bool flag);
            void set_font_name(const char *font_name);
            void set_font_color(uint32_t color);
            void set_limit_width(float width_value);

            void set_text(const char *text);
            auto get_text() -> const char *;

            void text_size(int *width_value, int *height);

          protected:
            void on_update() override;
            void on_render() override;
            void on_change_position() override;

            virtual void on_set_text(const char *text);

          protected:
            graphic_text_instance text_instance_;
        };

        class number_line : public ui_window {
          public:
            number_line(PyObject *ppy_object);
            number_line(ui_window *parent);

            ~number_line() override;

            void set_path(const char *path);
            void set_horizontal_align(int type);
            void set_number(const char *number);

          protected:
            void clear_number();
            void on_render() override;
            void on_change_position() override;

          protected:
            std::string path_;
            std::string number_;
            std::vector<graphic_image_instance *> image_instance_vector_value_;

            int horizontal_align_value_;
            uint32_t width_summary_;
        };

        class image_box : public ui_window {
          public:
            image_box(PyObject *ppy_object);

            ~image_box() override;

            auto load_image(const char *file_name) -> bool;
            void set_diffuse_color(float fr, float fg, float fb, float fa);

            auto width() -> int;
            auto height() -> int;

            // A nine patch is drawn at the window size.
            void set_size(int32_t width_value, int32_t height) override;

          protected:
            virtual void on_create_instance();
            virtual void on_destroy_instance();

            void on_update() override;
            void on_render() override;
            void on_change_position() override;

          protected:
            graphic_image_instance *image_instance_;
        };

        class mark_box : public ui_window {
          public:
            mark_box(PyObject *ppy_object);

            ~mark_box() override;

            void load_image(const char *filename);
            void set_diffuse_color(float fr, float fg, float fb, float fa);
            void set_index(uint32_t index);
            void set_scale(float scale);

          protected:
            virtual void on_create_instance();
            virtual void on_destroy_instance();

            void on_update() override;
            void on_render() override;
            void on_change_position() override;

          protected:
            graphic_mark_instance *mark_instance_;
        };

        class expanded_image_box : public image_box {
          public:
            static auto type() -> uint32_t;

          public:
            expanded_image_box(PyObject *ppy_object);

            ~expanded_image_box() override;

            void set_scale(float fx, float fy);
            void set_origin(float fx, float fy);
            void set_rotation(float rotation);
            void set_rendering_rect(float left, float top, float right, float bottom);
            void set_rendering_mode(int mode);
            // A screen background: the image is scaled to the new size.
            void fill_screen(int32_t width_value, int32_t height) override;

          protected:
            void on_create_instance() override;
            void on_destroy_instance() override;

            void on_update() override;
            void on_render() override;

            auto on_is_type(uint32_t type) -> bool override;
        };

        class ani_image_box : public ui_window {
          public:
            static auto type() -> uint32_t;

          public:
            ani_image_box(PyObject *ppy_object);

            ~ani_image_box() override;

            void set_delay(int delay);
            void append_image(const char *file_name);
            void set_rendering_rect(float left, float top, float right, float bottom);
            void set_rendering_mode(int mode);

            void reset_frame();

          protected:
            void on_update() override;
            void on_render() override;
            void on_change_position() override;
            virtual void on_end_frame();

            auto on_is_type(uint32_t type) -> bool override;

          protected:
            float frame_time_; // seconds the current image has been shown
            uint8_t delay_;    // frames per image, at 60 fps
            uint8_t bycur_index_;
            std::vector<graphic_expanded_image_instance *> image_vector_;
        };

        class button : public ui_window {
          public:
            button(PyObject *ppy_object);

            ~button() override;

            // A sub image visual sizes the button; a nine patch visual is drawn at the button size,
            // which the script sets.
            auto set_up_visual(const char *file_name) -> bool;
            auto set_over_visual(const char *file_name) -> bool;
            auto set_down_visual(const char *file_name) -> bool;
            auto set_disable_visual(const char *file_name) -> bool;

            void set_size(int32_t width_value, int32_t height) override;

            auto up_visual_file_name() -> const char *;
            auto over_visual_file_name() -> const char *;
            auto down_visual_file_name() -> const char *;

            void flash();
            void enable();
            void disable();

            void set_up();
            void up();
            void over();
            void down();

            auto is_pressed() -> bool;

          protected:
            void on_update() override;
            void on_render() override;
            void on_change_position() override;

            auto on_mouse_left_button_down() -> bool override;
            auto on_mouse_left_button_double_click() -> bool override;
            auto on_mouse_left_button_up() -> bool override;
            void on_mouse_over_in() override;
            void on_mouse_over_out() override;

            auto is_enable() -> bool;

            void set_current_visual(graphic_image_instance *visual);

          protected:
            bool enable_;
            bool is_pressed_;
            bool is_flash_;
            graphic_image_instance *pcur_visual_;
            graphic_image_instance up_visual_;
            graphic_image_instance over_visual_;
            graphic_image_instance down_visual_;
            graphic_image_instance disable_visual_;
        };

        class radio_button : public button {
          public:
            radio_button(PyObject *ppy_object);

            ~radio_button() override;

          protected:
            auto on_mouse_left_button_down() -> bool override;
            auto on_mouse_left_button_up() -> bool override;
            void on_mouse_over_in() override;
            void on_mouse_over_out() override;
        };

        class toggle_button : public button {
          public:
            toggle_button(PyObject *ppy_object);

            ~toggle_button() override;

          protected:
            auto on_mouse_left_button_down() -> bool override;
            auto on_mouse_left_button_up() -> bool override;
            void on_mouse_over_in() override;
            void on_mouse_over_out() override;
        };

        class drag_button : public button {
          public:
            drag_button(PyObject *ppy_object);

            ~drag_button() override;

            void set_restrict_movement_area(int ix, int iy, int iwidth, int iheight);

          protected:
            void on_change_position() override;
            void on_mouse_over_in() override;
            void on_mouse_over_out() override;

          protected:
            rectangle restrict_area_;
            // Without an explicit area the button is kept on the screen as it is now, not as it was
            // when the button was made (the window can be resized).
            bool restrict_to_screen_ = true;
        };
    };
}
