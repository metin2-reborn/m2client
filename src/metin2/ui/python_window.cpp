#include "python_window.h"
#include <metin2/core/crc32.h>
#include <metin2/core/random.h>
#include <metin2/core/timer.h>
#include <metin2/core/util.h>
#include <metin2/graphics/grp_base.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include "python_graphic.h"
#include "python_slot_window.h"
#include "python_window_manager.h"

#include <algorithm>
#include <cmath>

namespace metin2::inline ui {

    namespace widgets {

        ui_window::ui_window(PyObject *ppy_object) : x_(0), y_(0), width_(0), height_(0), show_(false), flag_(0), handler_(ppy_object), parent_(nullptr), is_updating_children_(false) {
            horizontal_align_value_ = HORIZONTAL_ALIGN_LEFT;
            vertical_align_value_ = VERTICAL_ALIGN_TOP;
            rect_.bottom = rect_.left = rect_.right = rect_.top = 0;
            if (handler_) {
                handler_has_on_update_ = PyObject_HasAttrString(handler_, "on_update") != 0;
                handler_has_on_render_ = PyObject_HasAttrString(handler_, "on_render") != 0;
            }
        }

        ui_window::~ui_window() = default;

        auto ui_window::type() -> uint32_t {
            static uint32_t type = get_crc32("CWindow", strlen("CWindow"));
            return (type);
        }

        auto ui_window::is_type(uint32_t type) -> bool {
            return on_is_type(type);
        }

        auto ui_window::on_is_type(uint32_t type) -> bool {
            if (ui_window::type() == type)
                return true;

            return false;
        }

        void ui_window::clear() {
            // The children are not deleted here: each Python widget destroys its own native window.
            for (ui_window *child : child_list_)
                child->clear();
            child_list_.clear();

            parent_ = nullptr;
            destroy_handle();
            hide();
        }

        void ui_window::destroy_handle() {
            handler_ = nullptr;
        }

        void ui_window::show() {
            show_ = true;
        }

        void ui_window::hide() {
            show_ = false;
        }

        // is_show() asks "am I shown?", is_rendering() asks "am I being drawn?": a shown window
        // can still have a hidden parent further up the tree.
        auto ui_window::is_rendering() -> bool {
            if (!is_show())
                return false;

            if (!parent_)
                return true;

            return parent_->is_rendering();
        }

        void ui_window::remove_reserve_children() {
            if (reserve_child_list_.empty())
                return;

            window_container::iterator it;
            for (it = reserve_child_list_.begin(); it != reserve_child_list_.end(); ++it) {
                child_list_.remove(*it);
            }
            reserve_child_list_.clear();
        }

        void ui_window::update() {
            if (!is_show())
                return;

            remove_reserve_children();

            on_update();

            is_updating_children_ = true;
            window_container::iterator it;
            for (it = child_list_.begin(); it != child_list_.end();) {
                auto it_next = it;
                ++it_next;
                (*it)->update();
                it = it_next;
            }
            is_updating_children_ = false;
        }

        void ui_window::render() {
            if (!is_show())
                return;

            const bool clip = is_flag(FLAG_CLIP);
            if (clip && !graphic_base::push_clip(rect_)) {
                graphic_base::pop_clip();
                return;
            }

            on_render();

            std::for_each(child_list_.begin(), child_list_.end(), std::mem_fn(&ui_window::render));

            if (clip)
                graphic_base::pop_clip();
        }

        void ui_window::on_update() {
            if (!handler_ || !handler_has_on_update_)
                return;

            if (!is_show())
                return;

            static PyObject *func_name_on_update = PyString_InternFromString("on_update");

            py_call_class_member_func_by_py_string(handler_, func_name_on_update, build_empty_tuple());
        }

        void ui_window::on_render() {
            if (!handler_ || !handler_has_on_render_)
                return;

            if (!is_show())
                return;

            static PyObject *func_name_on_render = PyString_InternFromString("on_render");

            py_call_class_member_func_by_py_string(handler_, func_name_on_render, build_empty_tuple());
        }

        void ui_window::set_name(const char *name) {
            name_value_ = name;
        }

        void ui_window::set_size(int32_t width_value, int32_t height) {
            width_ = width_value;
            height_ = height;

            rect_.right = rect_.left + width_;
            rect_.bottom = rect_.top + height_;
        }

        void ui_window::set_horizontal_align(uint32_t align) {
            horizontal_align_value_ = (horizontal_align)align;
            update_rect();
        }

        void ui_window::set_vertical_align(uint32_t align) {
            vertical_align_value_ = (vertical_align)align;
            update_rect();
        }

        void ui_window::set_position(int32_t x, int32_t y) {
            x_ = x;
            y_ = y;

            update_rect();
        }

        void ui_window::get_position(int32_t *plx, int32_t *ply) {
            *plx = x_;
            *ply = y_;
        }

        auto ui_window::update_rect() -> int32_t {
            rect_.top = y_;
            if (parent_) {
                switch (vertical_align_value_) {
                case VERTICAL_ALIGN_BOTTOM:
                    rect_.top = parent_->height() - rect_.top;
                    break;
                case VERTICAL_ALIGN_CENTER:
                    rect_.top = (parent_->height() - height()) / 2 + rect_.top;
                    break;
                case VERTICAL_ALIGN_TOP:
                    break;
                }
                rect_.top += parent_->rect_.top;
            }
            rect_.bottom = rect_.top + height_;

            rect_.left = x_;
            if (parent_) {
                switch (horizontal_align_value_) {
                case HORIZONTAL_ALIGN_RIGHT:
                    rect_.left = ::abs(parent_->width()) - rect_.left;
                    break;
                case HORIZONTAL_ALIGN_CENTER:
                    rect_.left = parent_->width() / 2 - width() / 2 + rect_.left;
                    break;
                case HORIZONTAL_ALIGN_LEFT:
                    break;
                }
                rect_.left += 0 < parent_->width() ? parent_->rect_.left : parent_->rect_.right + ::abs(parent_->width());
            }
            rect_.right = rect_.left + width_;
            std::for_each(child_list_.begin(), child_list_.end(), std::mem_fn(&ui_window::update_rect));

            on_change_position();

            return 1;
        }

        // The scripts place windows at absolute positions computed from the screen size, so a window
        // follows the edge its centre is nearest to: the left or top third stays, the middle third moves
        // by half the change (stays centred), the right or bottom third by all of it. A child covering
        // the whole old size grows instead and anchors its own children.
        static auto anchor_offset(int32_t position, int32_t size, int32_t old_extent, int32_t delta) -> int32_t {
            const int32_t center = position + size / 2;
            if (center * 3 < old_extent)
                return 0;
            if (center * 3 > old_extent * 2)
                return delta;
            return delta / 2;
        }

        void ui_window::anchor_children(int32_t old_width, int32_t old_height, std::vector<PyObject *> &scripted) {
            const int32_t dx = width_ - old_width;
            const int32_t dy = height_ - old_height;
            for (ui_window *child : child_list_) {
                if (child->handler_ && PyObject_HasAttrString(child->handler_, "on_screen_resize")) {
                    Py_INCREF(child->handler_);
                    scripted.push_back(child->handler_);
                    continue;
                }

                if (child->width_ == old_width && child->height_ == old_height) {
                    child->fill_screen(child->width_ + dx, child->height_ + dy);
                    child->anchor_children(old_width, old_height, scripted);
                    continue;
                }

                if (child->horizontal_align_value_ == HORIZONTAL_ALIGN_LEFT)
                    child->x_ += anchor_offset(child->x_, child->width_, old_width, dx);
                if (child->vertical_align_value_ == VERTICAL_ALIGN_TOP)
                    child->y_ += anchor_offset(child->y_, child->height_, old_height, dy);
            }
        }

        void ui_window::mouse_local_position(int32_t &rlx, int32_t &rly) {
            get_window_manager().mouse_position(rlx, rly);
            rlx = rlx - rect_.left;
            rly = rly - rect_.top;
        }

        void ui_window::add_child(ui_window *win) {
            child_list_.push_back(win);
            win->parent_ = this;
        }

        auto ui_window::root() -> ui_window * {
            if (parent_)
                if (parent_->is_window())
                    return parent_->root();

            return this;
        }

        auto ui_window::parent() -> ui_window * {
            return parent_;
        }

        void ui_window::delete_child(ui_window *win) {
            if (is_updating_children_) {
                reserve_child_list_.push_back(win);
            } else {
                child_list_.remove(win);
            }
        }

        void ui_window::set_top(ui_window *win) {
            if (!win->is_flag(ui_window::FLAG_FLOAT))
                return;

            auto itor = std::find(child_list_.begin(), child_list_.end(), win);
            if (child_list_.end() != itor) {
                child_list_.push_back(*itor);
                child_list_.erase(itor);

                win->on_top();
            } else {
                metin2::log::warn("Child window to bring to the top not found");
            }
        }

        void ui_window::on_mouse_drag(int32_t lx, int32_t ly) {
            py_call_class_member_func(handler_, "on_mouse_drag", Py_BuildValue("(ii)", lx, ly));
        }

        void ui_window::on_resize() {
            if (handler_ && PyObject_HasAttrString(handler_, "on_resize"))
                py_call_class_member_func(handler_, "on_resize", Py_BuildValue("(ii)", width_, height_));
        }

        void ui_window::on_move_window(int32_t lx, int32_t ly) {
            py_call_class_member_func(handler_, "on_move_window", Py_BuildValue("(ii)", lx, ly));
        }

        void ui_window::on_set_focus() {
            py_call_class_member_func(handler_, "on_set_focus", build_empty_tuple());
        }

        void ui_window::on_kill_focus() {
            py_call_class_member_func(handler_, "on_kill_focus", build_empty_tuple());
        }

        void ui_window::on_mouse_over_in() {
            py_call_class_member_func(handler_, "on_mouse_over_in", build_empty_tuple());
        }

        auto ui_window::on_mouse_wheel(int32_t delta) -> bool {
            if (!handler_ || !PyObject_HasAttrString(handler_, "on_mouse_wheel")) {
                return false;
            }
            bool taken = false;
            if (!py_call_class_member_func(handler_, "on_mouse_wheel", Py_BuildValue("(i)", delta), &taken)) {
                return false;
            }
            return taken;
        }

        void ui_window::on_mouse_over_out() {
            py_call_class_member_func(handler_, "on_mouse_over_out", build_empty_tuple());
        }

        void ui_window::on_mouse_over() {
        }

        void ui_window::on_drop() {
            py_call_class_member_func(handler_, "on_drop", build_empty_tuple());
        }

        void ui_window::on_top() {
            py_call_class_member_func(handler_, "on_top", build_empty_tuple());
        }

        void ui_window::on_ime_update() {
            py_call_class_member_func(handler_, "on_ime_update", build_empty_tuple());
        }

        auto ui_window::run_ime_tab_event() -> bool {
            if (!is_rendering())
                return false;

            if (on_ime_tab_event())
                return true;

            window_container::reverse_iterator itor;
            for (itor = child_list_.rbegin(); itor != child_list_.rend(); ++itor) {
                ui_window *window = *itor;

                if (window->run_ime_tab_event())
                    return true;
            }

            return false;
        }

        auto ui_window::run_ime_return_event() -> bool {
            if (!is_rendering())
                return false;

            if (on_ime_return_event())
                return true;

            window_container::reverse_iterator itor;
            for (itor = child_list_.rbegin(); itor != child_list_.rend(); ++itor) {
                ui_window *window = *itor;

                if (window->run_ime_return_event())
                    return true;
            }

            return false;
        }

        auto ui_window::run_ime_key_down_event(int ikey) -> bool {
            if (!is_rendering())
                return false;

            if (on_ime_key_down_event(ikey))
                return true;

            window_container::reverse_iterator itor;
            for (itor = child_list_.rbegin(); itor != child_list_.rend(); ++itor) {
                ui_window *window = *itor;

                if (window->run_ime_key_down_event(ikey))
                    return true;
            }

            return false;
        }

        auto ui_window::run_key_down_event(int ikey) -> ui_window * {
            if (on_key_down(ikey))
                return this;

            window_container::reverse_iterator itor;
            for (itor = child_list_.rbegin(); itor != child_list_.rend(); ++itor) {
                ui_window *window = *itor;

                if (window->is_show()) {
                    ui_window *processed_window = window->run_key_down_event(ikey);
                    if (nullptr != processed_window) {
                        return processed_window;
                    }
                }
            }

            return nullptr;
        }

        auto ui_window::run_key_up_event(int ikey) -> bool {
            if (on_key_up(ikey))
                return true;

            window_container::reverse_iterator itor;
            for (itor = child_list_.rbegin(); itor != child_list_.rend(); ++itor) {
                ui_window *window = *itor;

                if (window->is_show())
                    if (window->run_key_up_event(ikey))
                        return true;
            }

            return false;
        }

        auto ui_window::run_press_escape_key_event() -> bool {
            window_container::reverse_iterator itor;
            for (itor = child_list_.rbegin(); itor != child_list_.rend(); ++itor) {
                ui_window *window = *itor;

                if (window->is_show())
                    if (window->run_press_escape_key_event())
                        return true;
            }

            if (on_press_escape_key())
                return true;

            return false;
        }

        auto ui_window::run_press_exit_key_event() -> bool {
            window_container::reverse_iterator itor;
            for (itor = child_list_.rbegin(); itor != child_list_.rend(); ++itor) {
                ui_window *window = *itor;

                if (window->run_press_exit_key_event())
                    return true;

                if (window->is_show())
                    if (window->on_press_exit_key())
                        return true;
            }

            return false;
        }

        auto ui_window::on_mouse_left_button_down() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_mouse_left_button_down", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_mouse_left_button_up() -> bool {
            py_call_class_member_func(handler_, "on_mouse_left_button_up", build_empty_tuple());
            return true; // Button up is the exception and always returns true.
        }

        auto ui_window::on_mouse_left_button_double_click() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_mouse_left_button_double_click", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_mouse_right_button_down() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_mouse_right_button_down", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_mouse_right_button_up() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_mouse_right_button_up", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_mouse_right_button_double_click() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_mouse_right_button_double_click", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_mouse_middle_button_down() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_mouse_middle_button_down", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_mouse_middle_button_up() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_mouse_middle_button_up", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_ime_tab_event() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_ime_tab", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_ime_return_event() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_ime_return", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_ime_key_down_event(int ikey) -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_ime_key_down", Py_BuildValue("(i)", ikey), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_key_down(int ikey) -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_key_down", Py_BuildValue("(i)", ikey), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_key_up(int ikey) -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_key_up", Py_BuildValue("(i)", ikey), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_press_escape_key() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_press_escape_key", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::on_press_exit_key() -> bool {
            long value;
            if (py_call_class_member_func(handler_, "on_press_exit_key", build_empty_tuple(), &value))
                if (0 != value)
                    return true;

            return false;
        }

        auto ui_window::is_within(ui_window *ancestor) -> bool {
            for (ui_window *window = this; window; window = window->parent_)
                if (window == ancestor)
                    return true;
            return false;
        }

        auto ui_window::is_in(int32_t x, int32_t y) -> bool {
            if (x >= rect_.left && x <= rect_.right)
                if (y >= rect_.top && y <= rect_.bottom)
                    return !hit_mask_ || hit_mask_->hit(x - rect_.left, y - rect_.top, width_, height_);

            return false;
        }

        auto ui_window::set_hit_mask(const char *name) -> bool {
            if (!name || !*name) {
                hit_mask_.reset();
                return true;
            }
            auto mask = std::make_unique<hit_mask>();
            if (!mask->load(name))
                return false;
            hit_mask_ = std::move(mask);
            return true;
        }

        auto ui_window::is_in() -> bool {
            int32_t lx, ly;
            widgets::get_window_manager().mouse_position(lx, ly);

            return is_in(lx, ly);
        }

        auto ui_window::pick_window(int32_t x, int32_t y) -> ui_window * {
            auto ritor = child_list_.rbegin();
            for (; ritor != child_list_.rend(); ++ritor) {
                ui_window *win = *ritor;
                if (win->is_show()) {
                    if (!win->is_flag(ui_window::FLAG_IGNORE_SIZE)) {
                        if (!win->is_in(x, y)) {
                            if (0 <= win->width()) {
                                continue;
                            }
                        }
                    }

                    ui_window *result = win->pick_window(x, y);
                    if (result)
                        return result;
                }
            }

            if (is_flag(ui_window::FLAG_NOT_PICK))
                return nullptr;

            return (this);
        }

        auto ui_window::pick_top_window(int32_t x, int32_t y) -> ui_window * {
            auto ritor = child_list_.rbegin();
            for (; ritor != child_list_.rend(); ++ritor) {
                ui_window *win = *ritor;
                if (win->is_show())
                    if (win->is_in(x, y))
                        if (!win->is_flag(ui_window::FLAG_NOT_PICK))
                            return win;
            }

            return nullptr;
        }

        box::box(PyObject *ppy_object) : ui_window(ppy_object), color_value_(0xff000000) {
        }
        box::~box() = default;

        void box::set_color(uint32_t color) {
            color_value_ = color;
        }

        void box::on_render() {
            get_python_graphic().set_diffuse_color(color_value_);
            get_python_graphic().render_box2d(rect_.left, rect_.top, rect_.right, rect_.bottom);
        }

        bar::bar(PyObject *ppy_object) : ui_window(ppy_object), color_value_(0xff000000) {
        }
        bar::~bar() = default;

        void bar::set_color(uint32_t color) {
            color_value_ = color;
        }

        void bar::on_render() {
            get_python_graphic().set_diffuse_color(color_value_);
            get_python_graphic().render_bar2d(rect_.left, rect_.top, rect_.right, rect_.bottom);
        }

        line::line(PyObject *ppy_object) : ui_window(ppy_object), color_value_(0xff000000) {
        }
        line::~line() = default;

        void line::set_color(uint32_t color) {
            color_value_ = color;
        }

        void line::on_render() {
            python_graphic &rkpy_graphic = get_python_graphic();
            rkpy_graphic.set_diffuse_color(color_value_);
            rkpy_graphic.render_line2d(rect_.left, rect_.top, rect_.right, rect_.bottom);
        }

        auto bar3d::type() -> uint32_t {
            static uint32_t type = get_crc32("CBar3D", strlen("CBar3D"));
            return (type);
        }

        bar3d::bar3d(PyObject *ppy_object) : ui_window(ppy_object) {
            left_color_ = math::color(0.2f, 0.2f, 0.2f, 1.0f);
            right_color_ = math::color(0.7f, 0.7f, 0.7f, 1.0f);
            center_color_ = math::color(0.0f, 0.0f, 0.0f, 1.0f);
        }
        bar3d::~bar3d() = default;

        void bar3d::set_color(uint32_t left, uint32_t right, uint32_t center) {
            left_color_ = left;
            right_color_ = right;
            center_color_ = center;
        }

        void bar3d::on_render() {
            python_graphic &rkpy_graphic = get_python_graphic();

            rkpy_graphic.set_diffuse_color(center_color_);
            rkpy_graphic.render_bar2d(rect_.left, rect_.top, rect_.right, rect_.bottom);

            rkpy_graphic.set_diffuse_color(left_color_);
            rkpy_graphic.render_line2d(rect_.left, rect_.top, rect_.right, rect_.top);
            rkpy_graphic.render_line2d(rect_.left, rect_.top, rect_.left, rect_.bottom);

            rkpy_graphic.set_diffuse_color(right_color_);
            rkpy_graphic.render_line2d(rect_.left, rect_.bottom, rect_.right, rect_.bottom);
            rkpy_graphic.render_line2d(rect_.right, rect_.top, rect_.right, rect_.bottom);
        }

        text_line::text_line(PyObject *ppy_object) : ui_window(ppy_object) {
            text_instance_.set_color(0.78f, 0.78f, 0.78f);
            text_instance_.set_horizonal_align(graphic_text_instance::HORIZONTAL_ALIGN_LEFT);
            text_instance_.set_vertical_align(graphic_text_instance::VERTICAL_ALIGN_TOP);
        }
        text_line::~text_line() {
            text_instance_.destroy();
        }

        void text_line::set_horizontal_align(int type) {
            text_instance_.set_horizonal_align(type);
            text_line::on_change_position();
        }
        void text_line::set_vertical_align(int type) {
            text_instance_.set_vertical_align(type);
            text_line::on_change_position();
        }
        void text_line::set_secret(bool flag) {
            text_instance_.set_secret(flag ? true : false);
        }
        void text_line::set_outline(bool flag) {
            text_instance_.set_outline(flag ? true : false);
        }
        void text_line::set_multi_line(bool flag) {
            text_instance_.set_multi_line(flag ? true : false);
            text_line::on_change_position();
        }
        void text_line::set_font(const font_spec &font) {
            resource *res = get_resource_manager().type_resource_pointer(graphic_text::resource_name(font).c_str());
            text_instance_.set_text_pointer(static_cast<graphic_text *>(res));
        }
        void text_line::set_font_color(uint32_t color) {
            text_instance_.set_color(color);
        }
        void text_line::set_limit_width(float width_value) {
            text_instance_.set_limit_width(width_value);
        }
        void text_line::set_size(int32_t width_value, int32_t height) {
            ui_window::set_size(width_value, height);
            text_line::on_change_position();
        }
        void text_line::set_text(const char *text) {
            on_set_text(text);
        }
        void text_line::text_size(int *width_value, int *height) {
            text_instance_.text_size(width_value, height);
        }
        auto text_line::get_text() -> const char * {
            return text_instance_.value_string_reference().c_str();
        }

        void text_line::on_set_text(const char *text) {
            text_instance_.set_value(text);
            text_instance_.update();
        }

        void text_line::on_update() {
            if (is_show())
                text_instance_.update();
        }
        void text_line::on_render() {
            if (is_show())
                text_instance_.render();
        }

        // Multi-line text in a sized window is aligned within the window (its rows wrap at the limit
        // width); otherwise the alignment is around the window's top left corner.
        void text_line::on_change_position() {
            float x = static_cast<float>(rect_.left);
            float y = static_cast<float>(rect_.top);
            if (text_instance_.is_multi_line() && width_ > 0 && height_ > 0) {
                const int h_align = text_instance_.get_horizontal_align();
                if (h_align == graphic_text_instance::HORIZONTAL_ALIGN_CENTER)
                    x += static_cast<float>(width_) / 2.0f;
                else if (h_align == graphic_text_instance::HORIZONTAL_ALIGN_RIGHT)
                    x += static_cast<float>(width_);
                const int v_align = text_instance_.get_vertical_align();
                if (v_align == graphic_text_instance::VERTICAL_ALIGN_CENTER)
                    y += static_cast<float>(height_) / 2.0f;
                else if (v_align == graphic_text_instance::VERTICAL_ALIGN_BOTTOM)
                    y += static_cast<float>(height_);
            }
            text_instance_.set_position(x, y);
        }

        number_line::number_line(PyObject *ppy_object) : ui_window(ppy_object) {
            path_ = "ui:game/taskbar/";
            horizontal_align_value_ = HORIZONTAL_ALIGN_LEFT;
            width_summary_ = 0;
        }
        number_line::number_line(ui_window *parent) : ui_window(nullptr) {
            path_ = "ui:game/taskbar/";
            horizontal_align_value_ = HORIZONTAL_ALIGN_LEFT;
            width_summary_ = 0;

            parent_ = parent;
        }
        number_line::~number_line() {
            clear_number();
        }

        void number_line::set_path(const char *path) {
            path_ = path;
        }
        void number_line::set_horizontal_align(int type) {
            horizontal_align_value_ = type;
        }
        void number_line::set_number(const char *number) {
            if (0 == number_.compare(number))
                return;

            clear_number();

            number_ = number;

            for (uint32_t i = 0; i < number_.size(); ++i) {
                char char_value = number_[i];
                std::string image_file_name;

                if (':' == char_value) {
                    image_file_name = path_ + "colon.toml";
                } else if ('?' == char_value) {
                    image_file_name = path_ + "questionmark.toml";
                } else if ('/' == char_value) {
                    image_file_name = path_ + "slash.toml";
                } else if ('%' == char_value) {
                    image_file_name = path_ + "percent.toml";
                } else if ('+' == char_value) {
                    image_file_name = path_ + "plus.toml";
                } else if ('m' == char_value) {
                    image_file_name = path_ + "m.toml";
                } else if ('g' == char_value) {
                    image_file_name = path_ + "g.toml";
                } else if ('p' == char_value) {
                    image_file_name = path_ + "p.toml";
                } else if (char_value >= '0' && char_value <= '9') {
                    image_file_name = path_;
                    image_file_name += char_value;
                    image_file_name += ".toml";
                } else
                    continue;

                if (!get_resource_manager().is_file_exist(image_file_name.c_str()))
                    continue;

                auto *image = (graphic_image *)get_resource_manager().resource_pointer(image_file_name.c_str());

                graphic_image_instance *instance = graphic_image_instance::new_value();
                instance->set_image_pointer(image);
                image_instance_vector_value_.push_back(instance);

                width_summary_ += instance->width();
            }
        }

        void number_line::clear_number() {
            for (graphic_image_instance *instance : image_instance_vector_value_)
                graphic_image_instance::delete_value(instance);
            image_instance_vector_value_.clear();
            width_summary_ = 0;
            number_ = "";
        }

        void number_line::on_render() {
            for (uint32_t i = 0; i < image_instance_vector_value_.size(); ++i) {
                image_instance_vector_value_[i]->render();
            }
        }

        void number_line::on_change_position() {
            int ix = x_;
            int iy = y_;

            if (parent_) {
                ix = rect_.left;
                iy = rect_.top;
            }

            switch (horizontal_align_value_) {
            case HORIZONTAL_ALIGN_LEFT:
                break;
            case HORIZONTAL_ALIGN_CENTER:
                ix -= int(width_summary_) / 2;
                break;
            case HORIZONTAL_ALIGN_RIGHT:
                ix -= int(width_summary_);
                break;
            }

            for (uint32_t i = 0; i < image_instance_vector_value_.size(); ++i) {
                image_instance_vector_value_[i]->set_position(ix, iy);
                ix += image_instance_vector_value_[i]->width();
            }
        }

        image_box::image_box(PyObject *ppy_object) : ui_window(ppy_object) {
            image_instance_ = nullptr;
        }
        image_box::~image_box() {
            on_destroy_instance();
        }

        void image_box::on_create_instance() {
            on_destroy_instance();

            image_instance_ = graphic_image_instance::new_value();
        }
        void image_box::on_destroy_instance() {
            if (image_instance_) {
                graphic_image_instance::delete_value(image_instance_);
                image_instance_ = nullptr;
            }
        }

        auto image_box::load_image(const char *file_name) -> bool {
            if (!file_name[0])
                return false;

            on_create_instance();

            resource *resource = get_resource_manager().resource_pointer(file_name);
            if (!resource)
                return false;
            if (!resource->is_type(graphic_image::type()))
                return false;

            image_instance_->set_image_pointer(static_cast<graphic_image *>(resource));
            if (image_instance_->is_empty())
                return false;

            set_size(image_instance_->width(), image_instance_->height());
            update_rect();

            return true;
        }

        void image_box::set_size(int32_t width_value, int32_t height_value) {
            ui_window::set_size(width_value, height_value);
            if (image_instance_)
                image_instance_->set_size(width_value, height_value);
        }

        void image_box::set_diffuse_color(float fr, float fg, float fb, float fa) {
            if (!image_instance_)
                return;

            image_instance_->set_diffuse_color(fr, fg, fb, fa);
        }

        auto image_box::width() -> int {
            if (!image_instance_)
                return 0;

            return image_instance_->width();
        }

        auto image_box::height() -> int {
            if (!image_instance_)
                return 0;

            return image_instance_->height();
        }

        void image_box::on_update() {
        }
        void image_box::on_render() {
            if (!image_instance_)
                return;

            if (is_show())
                image_instance_->render();
        }
        void image_box::on_change_position() {
            if (!image_instance_)
                return;

            image_instance_->set_position(rect_.left, rect_.top);
        }

        // A window that shows a guild mark.
        mark_box::mark_box(PyObject *ppy_object) : ui_window(ppy_object) {
            mark_instance_ = nullptr;
        }

        mark_box::~mark_box() {
            on_destroy_instance();
        }

        void mark_box::on_create_instance() {
            on_destroy_instance();
            mark_instance_ = graphic_mark_instance::new_value();
        }

        void mark_box::on_destroy_instance() {
            if (mark_instance_) {
                graphic_mark_instance::delete_value(mark_instance_);
                mark_instance_ = nullptr;
            }
        }

        void mark_box::load_image(const char *filename) {
            on_create_instance();

            mark_instance_->set_image_file_name(filename);
            mark_instance_->load();
            set_size(mark_instance_->width(), mark_instance_->height());

            update_rect();
        }

        void mark_box::set_scale(float scale) {
            if (!mark_instance_)
                return;

            mark_instance_->set_scale(scale);
        }

        void mark_box::set_index(uint32_t index) {
            if (!mark_instance_)
                return;

            mark_instance_->set_index(index);
        }

        void mark_box::set_diffuse_color(float fr, float fg, float fb, float fa) {
            if (!mark_instance_)
                return;

            mark_instance_->set_diffuse_color(fr, fg, fb, fa);
        }

        void mark_box::on_update() {
        }

        void mark_box::on_render() {
            if (!mark_instance_)
                return;

            if (is_show())
                mark_instance_->render();
        }

        void mark_box::on_change_position() {
            if (!mark_instance_)
                return;

            mark_instance_->set_position(rect_.left, rect_.top);
        }

        auto expanded_image_box::type() -> uint32_t {
            static uint32_t type = get_crc32("CExpandedImageBox", strlen("CExpandedImageBox"));
            return (type);
        }

        auto expanded_image_box::on_is_type(uint32_t type) -> bool {
            return expanded_image_box::type() == type || image_box::on_is_type(type);
        }

        expanded_image_box::expanded_image_box(PyObject *ppy_object) : image_box(ppy_object) {
        }
        expanded_image_box::~expanded_image_box() {
            on_destroy_instance();
        }

        void expanded_image_box::on_create_instance() {
            on_destroy_instance();

            image_instance_ = graphic_expanded_image_instance::new_value();
        }
        void expanded_image_box::on_destroy_instance() {
            if (image_instance_) {
                graphic_expanded_image_instance::delete_value((graphic_expanded_image_instance *)image_instance_);
                image_instance_ = nullptr;
            }
        }

        void expanded_image_box::set_scale(float fx, float fy) {
            if (!image_instance_)
                return;

            ((graphic_expanded_image_instance *)image_instance_)->set_scale(fx, fy);
            // With a fit mode the window is the box and the scale only mirrors the image.
            if (fit_mode_ == graphic_expanded_image_instance::FIT_NONE)
                ui_window::set_size(int32_t(float(width()) * fx), int32_t(float(height()) * fy));
        }
        void expanded_image_box::fill_screen(int32_t width_value, int32_t height_value) {
            // width()/height() are the image's own size, so this stretches the image to exactly the screen.
            if (image_instance_ && width() > 0 && height() > 0) {
                auto *instance = static_cast<graphic_expanded_image_instance *>(image_instance_);
                instance->set_scale(float(width_value) / float(width()), float(height_value) / float(height()));
            }
            ui_window::set_size(width_value, height_value);
        }
        void expanded_image_box::set_origin(float fx, float fy) {
            if (!image_instance_)
                return;

            ((graphic_expanded_image_instance *)image_instance_)->set_origin(fx, fy);
        }
        void expanded_image_box::set_rotation(float rotation) {
            if (!image_instance_)
                return;

            ((graphic_expanded_image_instance *)image_instance_)->set_rotation(rotation);
        }
        void expanded_image_box::set_rendering_rect(float left, float top, float right, float bottom) {
            if (!image_instance_)
                return;

            ((graphic_expanded_image_instance *)image_instance_)->set_rendering_rect(left, top, right, bottom);
        }

        void expanded_image_box::set_rendering_mode(int mode) {
            if (image_instance_)
                static_cast<graphic_expanded_image_instance *>(image_instance_)->set_rendering_mode(mode);
        }

        void expanded_image_box::set_fit(int mode) {
            fit_mode_ = mode;
            apply_fit();
        }

        void expanded_image_box::set_size(int32_t width_value, int32_t height_value) {
            image_box::set_size(width_value, height_value);
            apply_fit();
        }

        auto expanded_image_box::set_shader(const char *shader_name, const char *mask_name) -> bool {
            shader_.clear();
            mask_.clear();
            if (shader_name[0]) {
                resource *shader = get_resource_manager().resource_pointer(shader_name);
                if (!shader || !shader->is_type(graphic_pixel_shader::type()))
                    return false;
                shader_.set_pointer(shader);
                if (shader_->is_empty()) {
                    shader_.clear();
                    return false;
                }
                if (mask_name[0]) {
                    resource *mask = get_resource_manager().resource_pointer(mask_name);
                    if (!mask || !mask->is_type(graphic_image::type())) {
                        shader_.clear();
                        return false;
                    }
                    mask_.set_pointer(mask);
                }
            }
            apply_fit();
            return true;
        }

        void expanded_image_box::apply_fit() {
            if (!image_instance_)
                return;
            auto *instance = static_cast<graphic_expanded_image_instance *>(image_instance_);
            instance->set_fit(fit_mode_, float(width_), float(height_));
            instance->set_shader(shader_.pointer(), mask_.pointer());
        }

        void expanded_image_box::on_update() {
        }
        void expanded_image_box::on_render() {
            if (!image_instance_)
                return;

            if (is_show())
                image_instance_->render();
        }

        auto expanded_image_box::image_fit(float (&out)[8]) -> bool {
            if (!image_instance_ || image_instance_->is_empty()) {
                return false;
            }
            const auto image_width = float(image_instance_->width());
            const auto image_height = float(image_instance_->height());
            using instance = graphic_expanded_image_instance;
            const auto mode = static_cast<instance::fit_mode>(fit_mode_);
            if (mode == instance::FIT_NONE || mode == instance::FIT_TILE) {
                out[0] = 0.0f;
                out[1] = 0.0f;
                out[2] = float(width_);
                out[3] = float(height_);
                out[4] = 0.0f;
                out[5] = 0.0f;
                out[6] = 1.0f;
                out[7] = 1.0f;
                return true;
            }
            const instance::fit_rect box = instance::fit(mode, image_width, image_height, float(width_), float(height_));
            out[0] = box.x;
            out[1] = box.y;
            out[2] = box.width;
            out[3] = box.height;
            out[4] = box.u0;
            out[5] = box.v0;
            out[6] = box.u1;
            out[7] = box.v1;
            return true;
        }

        particle_box::particle_box(PyObject *ppy_object) : ui_window(ppy_object) {
            add_flag(FLAG_NOT_PICK);
        }

        particle_box::~particle_box() = default;

        auto particle_box::load(const char *file_name) -> bool {
            resource *resource = get_resource_manager().resource_pointer(file_name);
            if (!resource || !resource->is_type(graphic_particles::type())) {
                return false;
            }
            particles_ = resource;
            if (particles_->is_empty()) {
                particles_.clear();
                return false;
            }
            emitter_.set_settings(particles_->settings());
            emitter_.set_image(particles_->image());
            update_box();
            return true;
        }

        void particle_box::reset() {
            emitter_.reset();
        }

        void particle_box::set_size(int32_t width_value, int32_t height_value) {
            ui_window::set_size(width_value, height_value);
            update_box();
        }

        void particle_box::update_box() {
            emitter_.set_box(float(rect_.left), float(rect_.top), float(width_), float(height_));
        }

        void particle_box::on_update() {
            if (!particles_.is_null()) {
                emitter_.update(get_timer().elapsed_second());
            }
        }

        void particle_box::on_render() {
            if (!particles_.is_null() && is_show()) {
                emitter_.render();
            }
        }

        void particle_box::on_change_position() {
            update_box();
        }

        auto ani_image_box::type() -> uint32_t {
            static uint32_t type = get_crc32("CAniImageBox", strlen("CAniImageBox"));
            return (type);
        }

        auto ani_image_box::on_is_type(uint32_t type) -> bool {
            return ani_image_box::type() == type || ui_window::on_is_type(type);
        }

        ani_image_box::ani_image_box(PyObject *ppy_object)
            : ui_window(ppy_object),
              frame_time_(0.0f),
              delay_(4),
              bycur_index_(0) {
            image_vector_.clear();
        }
        ani_image_box::~ani_image_box() {
            for_each(image_vector_.begin(), image_vector_.end(), graphic_expanded_image_instance::delete_expanded_image_instance);
        }

        void ani_image_box::set_delay(int delay) {
            delay_ = delay;
        }
        void ani_image_box::append_image(const char *file_name) {
            resource *resource = get_resource_manager().resource_pointer(file_name);
            if (!resource || !resource->is_type(graphic_image::type()))
                return;

            graphic_expanded_image_instance *image_instance = graphic_expanded_image_instance::new_value();

            image_instance->set_image_pointer(static_cast<graphic_image *>(resource));
            if (image_instance->is_empty()) {
                graphic_expanded_image_instance::delete_value(image_instance);
                return;
            }

            image_vector_.push_back(image_instance);

            bycur_index_ = static_cast<uint8_t>(metin2::random::number(0, static_cast<int>(image_vector_.size()) - 1));
        }

        void ani_image_box::set_rendering_rect(float left, float top, float right, float bottom) {
            for (graphic_expanded_image_instance *image : image_vector_)
                image->set_rendering_rect(left, top, right, bottom);
        }

        void ani_image_box::set_rendering_mode(int mode) {
            for (graphic_expanded_image_instance *image : image_vector_)
                image->set_rendering_mode(mode);
        }

        void ani_image_box::reset_frame() {
            bycur_index_ = 0;
        }

        void ani_image_box::on_update() {
            frame_time_ += get_timer().elapsed_second();
            const float frame_delay = static_cast<float>(delay_) / 60.0f;
            if (frame_time_ < frame_delay) {
                return;
            }

            // Keep the remainder so the animation runs at its rate whatever the frame time.
            frame_time_ = std::fmod(frame_time_ - frame_delay, std::max(frame_delay, 1e-3f));

            ++bycur_index_;
            if (bycur_index_ >= image_vector_.size()) {
                bycur_index_ = 0;

                on_end_frame();
            }
        }
        void ani_image_box::on_render() {
            if (bycur_index_ < image_vector_.size()) {
                graphic_expanded_image_instance *image = image_vector_[bycur_index_];
                image->render();
            }
        }

        void ani_image_box::on_change_position() {
            for (graphic_expanded_image_instance *image : image_vector_)
                image->set_position(rect_.left, rect_.top);
        }

        void ani_image_box::on_end_frame() {
            py_call_class_member_func(handler_, "on_end_frame", build_empty_tuple());
        }

        button::button(PyObject *ppy_object)
            : ui_window(ppy_object), enable_(true), is_pressed_(false), is_flash_(false), pcur_visual_(nullptr) {
            ui_window::add_flag(ui_window::FLAG_NOT_CAPTURE);
        }
        button::~button() = default;

        auto button::set_up_visual(const char *file_name) -> bool {
            resource *resource = get_resource_manager().resource_pointer(file_name);
            if (!resource || !resource->is_type(graphic_image::type()))
                return false;

            up_visual_.set_image_pointer(static_cast<graphic_image *>(resource));
            if (up_visual_.is_empty())
                return false;

            if (!up_visual_.is_nine_patch())
                set_size(up_visual_.width(), up_visual_.height());
            else
                up_visual_.set_size(width_, height_);
            set_current_visual(&up_visual_);

            return true;
        }
        auto button::set_over_visual(const char *file_name) -> bool {
            resource *resource = get_resource_manager().resource_pointer(file_name);
            if (!resource || !resource->is_type(graphic_image::type()))
                return false;

            over_visual_.set_image_pointer(static_cast<graphic_image *>(resource));
            if (over_visual_.is_empty())
                return false;

            if (!over_visual_.is_nine_patch())
                set_size(over_visual_.width(), over_visual_.height());
            else
                over_visual_.set_size(width_, height_);

            return true;
        }
        auto button::set_down_visual(const char *file_name) -> bool {
            resource *resource = get_resource_manager().resource_pointer(file_name);
            if (!resource || !resource->is_type(graphic_image::type()))
                return false;

            down_visual_.set_image_pointer(static_cast<graphic_image *>(resource));
            if (down_visual_.is_empty())
                return false;

            if (!down_visual_.is_nine_patch())
                set_size(down_visual_.width(), down_visual_.height());
            else
                down_visual_.set_size(width_, height_);

            return true;
        }
        auto button::set_disable_visual(const char *file_name) -> bool {
            resource *resource = get_resource_manager().resource_pointer(file_name);
            if (!resource || !resource->is_type(graphic_image::type()))
                return false;

            disable_visual_.set_image_pointer(static_cast<graphic_image *>(resource));
            if (disable_visual_.is_empty())
                return false;

            if (!disable_visual_.is_nine_patch())
                set_size(disable_visual_.width(), disable_visual_.height());
            else
                disable_visual_.set_size(width_, height_);

            return true;
        }

        void button::set_size(int32_t width_value, int32_t height_value) {
            ui_window::set_size(width_value, height_value);
            for (graphic_image_instance *visual : {&up_visual_, &over_visual_, &down_visual_, &disable_visual_})
                visual->set_size(width_value, height_value);
        }

        auto button::up_visual_file_name() -> const char * {
            return up_visual_.graphic_image_pointer()->file_name();
        }
        auto button::over_visual_file_name() -> const char * {
            return over_visual_.graphic_image_pointer()->file_name();
        }
        auto button::down_visual_file_name() -> const char * {
            return down_visual_.graphic_image_pointer()->file_name();
        }

        void button::flash() {
            is_flash_ = true;
        }

        void button::enable() {
            set_up();
            enable_ = true;
        }

        void button::disable() {
            enable_ = false;
            if (!disable_visual_.is_empty())
                set_current_visual(&disable_visual_);
        }

        void button::set_up() {
            set_current_visual(&up_visual_);
            is_pressed_ = false;
        }
        void button::up() {
            if (is_in())
                set_current_visual(&over_visual_);
            else
                set_current_visual(&up_visual_);

            py_call_class_member_func(handler_, "call_event", build_empty_tuple());
        }
        void button::over() {
            set_current_visual(&over_visual_);
        }
        void button::down() {
            is_pressed_ = true;
            set_current_visual(&down_visual_);
            py_call_class_member_func(handler_, "down_event", build_empty_tuple());
        }

        void button::on_update() {
        }
        void button::on_render() {
            if (!is_show())
                return;

            if (pcur_visual_) {
                if (is_flash_)
                    if (!is_in())
                        if (int(el_timer_get_m_sec() / 500) % 2) {
                            return;
                        }

                pcur_visual_->render();
            }

            py_call_class_member_func(handler_, "on_render", build_empty_tuple());
        }
        void button::on_change_position() {
            if (pcur_visual_)
                pcur_visual_->set_position(rect_.left, rect_.top);
        }

        auto button::on_mouse_left_button_down() -> bool {
            if (!is_enable())
                return true;

            is_pressed_ = true;
            down();

            return true;
        }
        auto button::on_mouse_left_button_double_click() -> bool {
            if (!is_enable())
                return true;

            on_mouse_left_button_down();

            return true;
        }
        auto button::on_mouse_left_button_up() -> bool {
            if (!is_enable())
                return true;
            if (!is_pressed())
                return true;

            is_pressed_ = false;
            up();

            return true;
        }
        void button::on_mouse_over_in() {
            if (!is_enable())
                return;

            over();
            py_call_class_member_func(handler_, "show_tool_tip", build_empty_tuple());
        }
        void button::on_mouse_over_out() {
            if (!is_enable())
                return;

            set_up();
            py_call_class_member_func(handler_, "hide_tool_tip", build_empty_tuple());
        }

        void button::set_current_visual(graphic_image_instance *visual) {
            pcur_visual_ = visual;
            pcur_visual_->set_position(rect_.left, rect_.top);
        }

        auto button::is_enable() -> bool {
            return enable_;
        }

        auto button::is_pressed() -> bool {
            return is_pressed_;
        }

        radio_button::radio_button(PyObject *ppy_object) : button(ppy_object) {
        }
        radio_button::~radio_button() = default;

        auto radio_button::on_mouse_left_button_down() -> bool {
            if (!is_enable())
                return true;

            if (!is_pressed_) {
                down();
                py_call_class_member_func(handler_, "call_event", build_empty_tuple());
            }

            return true;
        }
        auto radio_button::on_mouse_left_button_up() -> bool {
            return true;
        }
        void radio_button::on_mouse_over_in() {
            if (!is_enable())
                return;

            if (!is_pressed_) {
                set_current_visual(&over_visual_);
            }

            py_call_class_member_func(handler_, "show_tool_tip", build_empty_tuple());
        }
        void radio_button::on_mouse_over_out() {
            if (!is_enable())
                return;

            if (!is_pressed_) {
                set_current_visual(&up_visual_);
            }

            py_call_class_member_func(handler_, "hide_tool_tip", build_empty_tuple());
        }

        toggle_button::toggle_button(PyObject *ppy_object) : button(ppy_object) {
        }
        toggle_button::~toggle_button() = default;

        auto toggle_button::on_mouse_left_button_down() -> bool {
            if (!is_enable())
                return true;

            if (is_pressed_) {
                set_up();
                if (is_in())
                    set_current_visual(&over_visual_);
                else
                    set_current_visual(&up_visual_);
                py_call_class_member_func(handler_, "on_toggle_up", build_empty_tuple());
            } else {
                down();
                py_call_class_member_func(handler_, "on_toggle_down", build_empty_tuple());
            }

            return true;
        }
        auto toggle_button::on_mouse_left_button_up() -> bool {
            return true;
        }

        void toggle_button::on_mouse_over_in() {
            if (!is_enable())
                return;

            if (!is_pressed_) {
                set_current_visual(&over_visual_);
            }

            py_call_class_member_func(handler_, "show_tool_tip", build_empty_tuple());
        }
        void toggle_button::on_mouse_over_out() {
            if (!is_enable())
                return;

            if (!is_pressed_) {
                set_current_visual(&up_visual_);
            }

            py_call_class_member_func(handler_, "hide_tool_tip", build_empty_tuple());
        }

        drag_button::drag_button(PyObject *ppy_object) : button(ppy_object) {
            ui_window::remove_flag(ui_window::FLAG_NOT_CAPTURE);
            restrict_area_.left = 0;
            restrict_area_.top = 0;
            restrict_area_.right = get_window_manager().screen_width();
            restrict_area_.bottom = get_window_manager().screen_height();
        }
        drag_button::~drag_button() = default;

        void drag_button::set_restrict_movement_area(int ix, int iy, int iwidth, int iheight) {
            restrict_to_screen_ = false;
            restrict_area_.left = ix;
            restrict_area_.top = iy;
            restrict_area_.right = ix + iwidth;
            restrict_area_.bottom = iy + iheight;
        }

        void drag_button::on_change_position() {
            if (restrict_to_screen_) {
                restrict_area_.right = get_window_manager().screen_width();
                restrict_area_.bottom = get_window_manager().screen_height();
            }
            x_ = std::max(x_, restrict_area_.left);
            y_ = std::max(y_, restrict_area_.top);
            x_ = std::min(x_, std::max(0, restrict_area_.right - width_));
            y_ = std::min(y_, std::max(0, restrict_area_.bottom - height_));

            rect_.left = x_;
            rect_.top = y_;

            if (parent_) {
                const rectangle &rect_value = parent_->rect();
                rect_.left += rect_value.left;
                rect_.top += rect_value.top;
            }

            rect_.right = rect_.left + width_;
            rect_.bottom = rect_.top + height_;

            std::for_each(child_list_.begin(), child_list_.end(), std::mem_fn(&ui_window::update_rect));

            if (pcur_visual_)
                pcur_visual_->set_position(rect_.left, rect_.top);

            if (is_pressed())
                py_call_class_member_func(handler_, "on_move", build_empty_tuple());
        }

        void drag_button::on_mouse_over_in() {
            if (!is_enable())

                return;

            button::on_mouse_over_in();
            py_call_class_member_func(handler_, "on_mouse_over_in", build_empty_tuple());
        }

        void drag_button::on_mouse_over_out() {
            if (!is_enable())
                return;

            button::on_mouse_over_out();
            py_call_class_member_func(handler_, "on_mouse_over_out", build_empty_tuple());
        }
    };
}
