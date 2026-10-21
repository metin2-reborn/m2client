#include "python_window_manager.h"
#include <metin2/core/stl.h>
#include <metin2/core/timer.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include <set>
#include <vector>
#include "python_grid_slot_window.h"
#include "python_slot_window.h"
#include "python_window.h"

namespace metin2::inline ui {

    namespace widgets {
        static PyObject *empty_tuple = nullptr;

        auto build_empty_tuple() -> PyObject * {
            Py_INCREF(empty_tuple);
            return empty_tuple;
        }

        std::set<ui_window *> set_wnd;

        window_manager::window_manager()
            : vres_(0), hres_(0), once_ignore_mouse_left_button_up_event_flag_(false), mouse_handler_(nullptr), attaching_flag_(false), attaching_type_(0), attaching_index_(0), attaching_slot_number_(0), active_window_(nullptr), lock_window_(nullptr), point_window_(nullptr), left_capture_window_(nullptr), right_capture_window_(nullptr), middle_capture_window_(nullptr) {
            root_window_ = new ui_window(nullptr);
            root_window_->set_name("root");
            root_window_->show();

            const char *layer_tbl[] = {"GAME", "UI_BOTTOM", "UI", "TOP_MOST", "CURTAIN"};

            for (uint32_t layer_index = 0; layer_index < sizeof(layer_tbl) / sizeof(layer_tbl[0]); layer_index++) {
                ui_window *layer_value = new layer(nullptr);
                layer_value->set_name(layer_tbl[layer_index]);
                layer_value->show();
                layer_window_map_.insert(layer_container::value_type(layer_tbl[layer_index], layer_value));
                root_window_->add_child(layer_value);
                layer_window_list_.push_back(layer_value);
            }

            active_window_list_.clear();
            lock_window_list_.clear();

            empty_tuple = Py_BuildValue("()");
        }

        window_manager::~window_manager() {
            Py_DECREF(empty_tuple);

            stl_wipe_second(layer_window_map_);
            layer_window_map_.clear();
            layer_window_list_.clear();
            key_capture_window_map_value_.clear();

            delete root_window_;
            root_window_ = nullptr;
        }

        auto window_manager::register_window(ui_window *win, const char *layer) -> ui_window * {
            auto it = layer_window_map_.find(layer);
            if (layer_window_map_.end() == it) {
                metin2::log::error("Unknown window layer {}", layer);
                delete win;
                return nullptr;
            }

            it->second->add_child(win);
            return win;
        }

        void window_manager::destroy() {
            clear_reserve_delete_window_list();
        }

        void window_manager::set_mouse_handler(PyObject *mouse_handler) {
            mouse_handler_ = mouse_handler;
        }

        void window_manager::notify_destroy_window(ui_window *window) {
            if (window == active_window_)
                active_window_ = nullptr;

            if (window == point_window_)
                point_window_ = nullptr;

            if (window == left_capture_window_)
                left_capture_window_ = nullptr;

            if (window == middle_capture_window_)
                middle_capture_window_ = nullptr;

            if (window == right_capture_window_)
                right_capture_window_ = nullptr;

            if (window == lock_window_)
                lock_window_ = nullptr;

            lock_window_list_.remove(window);
            active_window_list_.remove(window);
            pick_always_window_list_.remove(window);

            auto itor = key_capture_window_map_value_.begin();
            for (; itor != key_capture_window_map_value_.end();) {
                if (window == itor->second) {
                    itor = key_capture_window_map_value_.erase(itor);
                } else {
                    ++itor;
                }
            }
        }

        void window_manager::destroy_window(ui_window *win) {
            notify_destroy_window(win);
            if (win->has_parent()) {
                ui_window *parent_win = win->parent();
                parent_win->delete_child(win);
            }
            win->clear();
            reserve_delete_window_list_.push_back(win);
        }

        auto window_manager::is_dragging() -> bool {
            int32_t ldx = abs(mouse_x_ - picked_x_);
            int32_t ldy = abs(mouse_y_ - picked_y_);
            if (ldx + ldy < 10)
                return false;

            return true;
        }

        auto window_manager::is_attaching() -> bool {
            return attaching_flag_;
        }

        auto window_manager::attaching_index() -> uint32_t {
            return attaching_index_;
        }

        auto window_manager::attaching_slot_number() -> uint32_t {
            return attaching_slot_number_;
        }

        void window_manager::attaching_icon_size(uint8_t *width_value, uint8_t *height) {
            *width_value = attaching_icon_width_;
            *height = attaching_icon_height_;
        }

        void window_manager::attach_icon(uint32_t type, uint32_t index, uint32_t slot_number, uint8_t width_value, uint8_t height) {
            attaching_flag_ = true;
            attaching_type_ = type;
            attaching_index_ = index;
            attaching_slot_number_ = slot_number;

            attaching_icon_width_ = width_value;
            attaching_icon_height_ = height;
        }

        void window_manager::set_attaching_flag(bool flag) {
            attaching_flag_ = flag;
        }

        void window_manager::deattach_icon() {
            set_attaching_flag(false);
            if (mouse_handler_)
                py_call_class_member_func(mouse_handler_, "DeattachObject", build_empty_tuple());
        }

        void window_manager::set_parent(ui_window *window, ui_window *parent_window) {
            if (!window) {
                assert(!"CWindowManager::SetParent - There is no self window!");
                return;
            }
            if (!parent_window) {
                assert(!"There is no parent window");
                return;
            }

            if (window->has_parent()) {
                ui_window *old_parent_window = window->parent();

                if (parent_window == old_parent_window)
                    return;

                old_parent_window->delete_child(window);
            }

            parent_window->add_child(window);
        }

        void window_manager::set_pick_always(ui_window *window) {
            pick_always_window_list_.push_back(window);
        }

        void window_manager::once_ignore_mouse_left_button_up_event() {
            once_ignore_mouse_left_button_up_event_flag_ = true;
            ignore_end_time_ = el_timer_get_m_sec() + 500;
        }

        void window_manager::lock_window(ui_window *win) {
            if (active_window_)
                active_window_->on_kill_focus();

            // Remove it from the locked window list if it is already there.
            lock_window_list_.remove(win);

            if (lock_window_) {
                if (lock_window_ == win)
                    return;

                lock_window_list_.push_back(lock_window_);
            }

            lock_window_ = win;
        }

        void window_manager::unlock_window() {
            if (lock_window_) {
                if (lock_window_list_.empty()) {
                    lock_window_ = nullptr;
                    if (active_window_)
                        active_window_->on_set_focus();
                } else {
                    lock_window_ = lock_window_list_.back();
                    lock_window_list_.pop_back();
                }
            }
        }

        void window_manager::activate_window(ui_window *win) {
            active_window_list_.remove(win);

            if (win == active_window_)
                return;

            if (active_window_) {
                active_window_list_.push_back(active_window_);
                active_window_->on_kill_focus();
            }

            active_window_ = win;

            if (active_window_) {
                active_window_->on_set_focus();
            }
        }

        void window_manager::deactivate_window() {
            if (active_window_) {
                if (active_window_list_.empty()) {
                    active_window_->on_kill_focus();
                    active_window_ = nullptr;
                } else {
                    active_window_->on_kill_focus();

                    active_window_ = active_window_list_.back();
                    active_window_list_.pop_back();

                    active_window_->on_set_focus();
                }
            }
        }

        void window_manager::set_top(ui_window *win) {
            if (!win->has_parent())
                return;

            ui_window *parent_window = win->parent();
            parent_window->set_top(win);

            // This also stops a capture left over from dragging an item out of the inventory from making
            // the window buttons need two clicks.
        }

        void window_manager::set_top_ui_window() {
            if (lock_window_)
                return;

            // A picked window of the game layer is always brought to the top.
            auto itor = layer_window_map_.find("UI");
            if (itor == layer_window_map_.end())
                return;
            ui_window *game_layer = itor->second;
            ui_window *top_window = game_layer->pick_top_window(mouse_x_, mouse_y_);
            if (top_window) {
                set_top(top_window);
            }
        }

        auto window_manager::get_activate_window() -> ui_window * {
            return active_window_;
        }

        void window_manager::set_resolution(int hres, int vres) {
            if (hres <= 0 || vres <= 0)
                return;

            hres_ = hres;
            vres_ = vres;
        }

        auto window_manager::aspect() -> float {
            return (hres_) / float(vres_);
        }

        void window_manager::set_screen_size(int32_t width_value, int32_t height) {
            width_ = width_value;
            height_ = height;

            for (auto itor = layer_window_map_.begin(); itor != layer_window_map_.end(); ++itor) {
                itor->second->set_size(width_value, height);
            }
        }

        void window_manager::resize_screen(int32_t width_value, int32_t height) {
            if (width_value <= 0 || height <= 0 || (width_value == width_ && height == height_))
                return;

            const int32_t old_width = width_;
            const int32_t old_height = height_;
            set_screen_size(width_value, height);
            set_resolution(width_value, height);
            if (old_width <= 0 || old_height <= 0)
                return;

            std::vector<PyObject *> scripted;
            for (ui_window *layer : layer_window_list_) {
                layer->anchor_children(old_width, old_height, scripted);
                layer->update_rect();
            }

            // Called after the walk: a script may create or destroy windows.
            for (PyObject *handler : scripted) {
                py_call_class_member_func(handler, "OnScreenResize", Py_BuildValue("(ii)", width_value, height));
                Py_DECREF(handler);
            }
        }

        void window_manager::clear_reserve_delete_window_list() {
            for (auto itor = reserve_delete_window_list_.begin(); itor != reserve_delete_window_list_.end(); ++itor) {
                ui_window *win = *itor;
                delete win;
            }
            reserve_delete_window_list_.clear();
        }

        void window_manager::update() {
            clear_reserve_delete_window_list();

            root_window_->update();
        }

        void window_manager::render() {
            root_window_->render();
        }

        auto window_manager::pick_window_impl(int32_t x, int32_t y) -> ui_window * {
            if (lock_window_) {
                return lock_window_->pick_window(x, y);
            }

            for (auto itor = pick_always_window_list_.begin(); itor != pick_always_window_list_.end(); ++itor) {
                ui_window *window = *itor;
                if (window->is_rendering())
                    if (window->is_in(x, y))
                        return window;
            }

            for (auto ritor = layer_window_list_.rbegin(); ritor != layer_window_list_.rend(); ++ritor) {
                ui_window *layer = *ritor;
                ui_window *picked_window = layer->pick_window(x, y);

                if (picked_window != layer)
                    return picked_window;
            }

            return nullptr;
        }

        void window_manager::set_mouse_position(int32_t x, int32_t y) {
            if (hres_ == 0)
                return;

            if (vres_ == 0)
                return;

            mouse_x_ = width_ * x / hres_;
            mouse_y_ = height_ * y / vres_;
        }

        void window_manager::mouse_position(int32_t &rx, int32_t &ry) {
            rx = mouse_x_;
            ry = mouse_y_;
        }

        void window_manager::run_mouse_move(int32_t x, int32_t y) {
            if (is_attaching()) {
                if (x > width_)
                    x = width_;
                if (y > height_)
                    y = height_;
            }

            set_mouse_position(x, y);
            ui_window *point_window = pick_window_impl(mouse_x_, mouse_y_);

            if (left_capture_window_) {
                ui_window *win = left_capture_window_;

                if (win->is_flag(ui_window::FLAG_MOVABLE)) {
                    int32_t x = mouse_x_ - drag_x_;
                    int32_t y = mouse_y_ - drag_y_;
                    if (win->has_parent()) {
                        x -= win->parent()->rect().left;
                        y -= win->parent()->rect().top;
                    }

                    int32_t lx, ly;
                    win->get_position(&lx, &ly);
                    if (win->is_flag(ui_window::FLAG_RESTRICT_X)) {
                        x = lx;
                    }
                    if (win->is_flag(ui_window::FLAG_RESTRICT_Y)) {
                        y = ly;
                    }

                    if (win->is_flag(ui_window::FLAG_LIMIT)) {
                        rectangle limit_rect = {0, 0, 0, 0};

                        limit_rect.right = width_ - limit_rect.right;
                        limit_rect.bottom = height_ - limit_rect.bottom;

                        if (x < limit_rect.left)
                            x = limit_rect.left;
                        else if (x + win->width() >= limit_rect.right)
                            x = limit_rect.right - win->width();

                        if (y < limit_rect.top)
                            y = limit_rect.top;
                        else if (y + win->height() >= limit_rect.bottom)
                            y = limit_rect.bottom - win->height();
                    }

                    win->set_position(x, y);
                    win->on_move_window(x, y);
                } else if (left_capture_window_->is_flag(ui_window::FLAG_DRAGABLE)) {
                    int32_t x = mouse_x_ - drag_x_;
                    int32_t y = mouse_y_ - drag_y_;
                    left_capture_window_->on_mouse_drag(x, y);
                }
            }

            if (point_window_ != point_window) {
                if (point_window_)
                    point_window_->on_mouse_over_out();

                point_window_ = point_window;

                if (point_window_)
                    point_window_->on_mouse_over_in();
            }

            if (point_window_) {
                point_window_->on_mouse_over();
            }
        }

        void window_manager::run_mouse_left_button_down(int32_t x, int32_t y) {
            set_top_ui_window();

            set_mouse_position(x, y);
            ui_window *win = point_window();

            if (!win)
                return;

            if (win->is_flag(ui_window::FLAG_ATTACH))
                win = win->root();

            if (!win->is_flag(ui_window::FLAG_NOT_CAPTURE))
                left_capture_window_ = win;

            drag_x_ = mouse_x_ - win->rect().left;
            drag_y_ = mouse_y_ - win->rect().top;
            picked_x_ = mouse_x_;
            picked_y_ = mouse_y_;

            win->on_mouse_left_button_down();
        }

        void window_manager::run_mouse_left_button_up(int32_t x, int32_t y) {
            if (once_ignore_mouse_left_button_up_event_flag_) {
                once_ignore_mouse_left_button_up_event_flag_ = false;

                if (el_timer_get_m_sec() < ignore_end_time_) {
                    return;
                }
            }

            set_mouse_position(x, y);

            if (left_capture_window_) {
                if (left_capture_window_->on_mouse_left_button_up()) {
                    // left_capture_window_ can be null here.
                    left_capture_window_ = nullptr;
                    return;
                }
            }

            ui_window *win = point_window();
            if (win)
                win->on_mouse_left_button_up();

            left_capture_window_ = nullptr;
        }

        void window_manager::run_mouse_left_button_double_click(int32_t x, int32_t y) {
            set_mouse_position(x, y);

            ui_window *win = point_window();
            if (!win)
                return;

            win->on_mouse_left_button_double_click();
        }

        void window_manager::run_mouse_right_button_down(int32_t x, int32_t y) {
            set_top_ui_window();

            set_mouse_position(x, y);
            ui_window *win = point_window();
            if (!win)
                return;

            if (win->is_flag(ui_window::FLAG_ATTACH))
                win = win->root();

            if (!win->is_flag(ui_window::FLAG_NOT_CAPTURE))
                right_capture_window_ = win;

            win->on_mouse_right_button_down();
        }

        void window_manager::run_mouse_right_button_up(int32_t x, int32_t y) {
            if (right_capture_window_) {
                if (right_capture_window_->on_mouse_right_button_up()) {
                    right_capture_window_ = nullptr;
                    return;
                }
            }

            ui_window *win = point_window();
            if (win)
                win->on_mouse_right_button_up();

            right_capture_window_ = nullptr;
            deattach_icon();
        }

        void window_manager::run_mouse_middle_button_down(int32_t x, int32_t y) {
            set_mouse_position(x, y);

            ui_window *win = point_window();
            if (!win)
                return;

            middle_capture_window_ = win;

            win->on_mouse_middle_button_down();
        }

        void window_manager::run_mouse_middle_button_up(int32_t x, int32_t y) {
            set_mouse_position(x, y);

            if (middle_capture_window_) {
                if (middle_capture_window_->on_mouse_middle_button_up()) {
                    middle_capture_window_ = nullptr;
                    return;
                }
            }

            ui_window *win = point_window();
            if (!win)
                return;

            win->on_mouse_middle_button_up();
            middle_capture_window_ = nullptr;
        }

        void window_manager::run_ime_update() {
            if (lock_window_) {
                lock_window_->on_ime_update();
                return;
            }

            if (!active_window_)
                return;
            if (!active_window_->is_rendering())
                return;

            active_window_->on_ime_update();
        }

        void window_manager::run_ime_tab_event() {
            if (lock_window_) {
                lock_window_->on_ime_tab_event();
                return;
            }
            if (active_window_)
                if (active_window_->is_rendering()) {
                    if (active_window_->on_ime_tab_event())
                        return;
                }

            if (!root_window_->run_ime_tab_event()) {
                if (!active_window_list_.empty()) {
                    ui_window *window = *(active_window_list_.begin());
                    activate_window(window);

                    ui_window *parent_window = window;
                    ui_window *current_window = window->parent();

                    uint32_t max_loop_count = 20;
                    for (uint32_t i = 0; i < max_loop_count; ++i) {
                        if (!parent_window)
                            break;

                        if (parent_window == layer_window_map_["GAME"]) {
                            set_top(current_window);
                            break;
                        }

                        current_window = parent_window;
                        parent_window = current_window->parent();
                    }
                }
            }
        }

        void window_manager::run_ime_return_event() {
            if (lock_window_) {
                lock_window_->on_ime_return_event();
                return;
            }
            if (active_window_)
                if (active_window_->is_rendering()) {
                    if (active_window_->on_ime_return_event())
                        return;
                }

            root_window_->run_ime_return_event();
        }

        void window_manager::run_ime_key_down(int vkey) {
            if (lock_window_) {
                lock_window_->on_key_down(vkey);
                return;
            }
            if (active_window_)
                if (active_window_->is_rendering()) {
                    if (active_window_->on_ime_key_down_event(vkey))
                        return;
                }

            // Sent only to the active edit line, not to every window.
        }

        void window_manager::run_key_down(int vkey) {
            if (lock_window_) {
                lock_window_->on_key_down(vkey);
                key_capture_window_map_value_.insert(std::make_pair(vkey, lock_window_));
                return;
            }
            if (active_window_)
                if (active_window_->is_show()) {
                    if (active_window_->on_key_down(vkey)) {
                        key_capture_window_map_value_.insert(std::make_pair(vkey, active_window_));
                        return;
                    }
                }

            ui_window *key_capture_window = root_window_->run_key_down_event(vkey);
            if (key_capture_window)
                if (reserve_delete_window_list_.end() == std::find(reserve_delete_window_list_.begin(), reserve_delete_window_list_.end(), key_capture_window)) {
                    key_capture_window_map_value_.insert(key_capture_window_map::value_type(vkey, key_capture_window));
                }
        }

        void window_manager::run_key_up(int vkey) {
            auto itor = key_capture_window_map_value_.find(vkey);
            if (key_capture_window_map_value_.end() != itor) {
                ui_window *key_capture_window = itor->second;
                if (key_capture_window)
                    key_capture_window->on_key_up(vkey);

                key_capture_window_map_value_.erase(itor);
                return;
            }

            if (lock_window_) {
                lock_window_->on_key_up(vkey);
                return;
            }

            if (active_window_) {
                if (active_window_->on_key_up(vkey))
                    return;
            }

            root_window_->run_key_up_event(vkey);
        }

        void window_manager::run_press_escape_key() {
            if (lock_window_) {
                lock_window_->on_press_escape_key();
                return;
            }

            root_window_->run_press_escape_key_event();
        }

        void window_manager::run_press_exit_key() {
            if (lock_window_) {
                lock_window_->on_press_exit_key();
                return;
            }
            if (active_window_) {
                if (active_window_->on_press_exit_key())
                    return;
            }

            root_window_->run_press_exit_key_event();
        }
    }
}
