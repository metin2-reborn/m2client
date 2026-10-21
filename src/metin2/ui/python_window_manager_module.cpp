#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include "python_grid_slot_window.h"
#include "python_modules.h"
#include "python_slot_window.h"
#include "python_window.h"
#include "python_window_manager.h"

namespace metin2::inline ui {

    auto py_tuple_get_window(PyObject *args, int pos, widgets::ui_window **ret_window) -> bool {
        intptr_t handle;
        if (!py_tuple_get_integer(args, pos, &handle))
            return false;
        if (!handle)
            return false;

        *ret_window = (widgets::ui_window *)handle;
        return true;
    }

    auto wnd_mgr_once_ignore_mouse_left_button_up_event(PyObject *self, PyObject *args) -> PyObject * {
        widgets::get_window_manager().once_ignore_mouse_left_button_up_event();
        return py_build_none();
    }

    auto wnd_mgr_set_mouse_handler(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handler;
        if (!py_tuple_get_object(args, 0, &handler))
            return py_build_exception();

        widgets::get_window_manager().set_mouse_handler(handler);
        return py_build_none();
    }

    // wndMgr.Register*(pyHandler, layerName): creates a T, attaches it to the layer and returns its handle.
    template <class T>
    auto wnd_mgr_register_window(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *po;
        if (!py_tuple_get_object(args, 0, &po))
            return py_build_exception();
        char *layer;
        if (!py_tuple_get_string(args, 1, &layer))
            return py_build_exception();

        widgets::ui_window *window = widgets::get_window_manager().register_window(new T(po), layer);
        if (!window)
            return py_build_exception();

        return Py_BuildValue("n", (Py_ssize_t)window);
    }

    auto wnd_mgr_destroy(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        widgets::get_window_manager().destroy_window(win);
        return py_build_none();
    }

    auto wnd_mgr_is_focus(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        return Py_BuildValue("i", window == widgets::get_window_manager().get_activate_window());
    }

    auto wnd_mgr_set_focus(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        widgets::get_window_manager().activate_window(win);
        return py_build_none();
    }

    auto wnd_mgr_kill_focus(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (py_tuple_get_window(args, 0, &win)) {
            if (win == widgets::get_window_manager().get_activate_window()) {
                widgets::get_window_manager().deactivate_window();
            }
        } else {
            widgets::get_window_manager().deactivate_window();
        }

        return py_build_none();
    }

    auto wnd_mgr_lock(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        widgets::get_window_manager().lock_window(win);
        return py_build_none();
    }

    auto wnd_mgr_unlock(PyObject *self, PyObject *args) -> PyObject * {
        widgets::get_window_manager().unlock_window();
        return py_build_none();
    }

    auto wnd_mgr_set_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();
        char *name;
        if (!py_tuple_get_string(args, 1, &name))
            return py_build_exception();

        win->set_name(name);
        return py_build_none();
    }

    auto wnd_mgr_set_top(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        widgets::get_window_manager().set_top(win);
        return py_build_none();
    }

    auto wnd_mgr_show(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        win->show();
        return py_build_none();
    }

    auto wnd_mgr_hide(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        win->hide();
        return py_build_none();
    }

    auto wnd_mgr_is_show(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        return Py_BuildValue("i", win->is_show());
    }

    auto wnd_mgr_is_rtl(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        return Py_BuildValue("i", win->is_flag(widgets::ui_window::FLAG_RTL));
    }

    auto wnd_mgr_set_screen_size(PyObject *self, PyObject *args) -> PyObject * {
        int width_value;
        if (!py_tuple_get_integer(args, 0, &width_value))
            return py_build_exception();
        int height;
        if (!py_tuple_get_integer(args, 1, &height))
            return py_build_exception();

        widgets::get_window_manager().set_screen_size(width_value, height);
        return py_build_none();
    }

    auto wnd_mgr_set_parent(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();
        widgets::ui_window *parent_win;
        if (!py_tuple_get_window(args, 1, &parent_win))
            return py_build_exception();

        widgets::get_window_manager().set_parent(win, parent_win);
        return py_build_none();
    }

    auto wnd_mgr_set_pick_always(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        widgets::get_window_manager().set_pick_always(win);
        return py_build_none();
    }

    auto wnd_mgr_set_wnd_size(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();
        int width_value;
        if (!py_tuple_get_integer(args, 1, &width_value))
            return py_build_exception();
        int height;
        if (!py_tuple_get_integer(args, 2, &height))
            return py_build_exception();

        win->set_size(width_value, height);
        return py_build_none();
    }

    auto wnd_mgr_set_wnd_position(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();
        int x;
        if (!py_tuple_get_integer(args, 1, &x))
            return py_build_exception();
        int y;
        if (!py_tuple_get_integer(args, 2, &y))
            return py_build_exception();

        win->set_position(x, y);
        return py_build_none();
    }

    auto wnd_mgr_get_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        return Py_BuildValue("s", win->get_name());
    }

    auto wnd_mgr_get_wnd_width(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        return Py_BuildValue("i", win->width());
    }

    auto wnd_mgr_get_wnd_height(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        return Py_BuildValue("i", win->height());
    }

    auto wnd_mgr_get_wnd_local_position(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int32_t lx, ly;
        win->get_position(&lx, &ly);

        return Py_BuildValue("ii", lx, ly);
    }

    auto wnd_mgr_get_wnd_global_position(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        rectangle &rect_value = window->rect();
        return Py_BuildValue("ii", rect_value.left, rect_value.top);
    }

    auto wnd_mgr_get_window_rect(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        rectangle &rect_value = win->rect();
        return Py_BuildValue("iiii", rect_value.left, rect_value.top, rect_value.right - rect_value.left, rect_value.bottom - rect_value.top);
    }

    auto wnd_mgr_set_window_horizontal_align(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();
        int align;
        if (!py_tuple_get_integer(args, 1, &align))
            return py_build_exception();

        win->set_horizontal_align(align);

        return py_build_none();
    }

    auto wnd_mgr_set_window_vertical_align(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();
        int align;
        if (!py_tuple_get_integer(args, 1, &align))
            return py_build_exception();

        win->set_vertical_align(align);

        return py_build_none();
    }

    auto wnd_mgr_is_in(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        return Py_BuildValue("i", win == widgets::get_window_manager().point_window());
    }

    auto wnd_mgr_get_mouse_local_position(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int32_t lx, ly;
        win->mouse_local_position(lx, ly);
        return Py_BuildValue("ii", lx, ly);
    }

    auto wnd_mgr_get_hyperlink(PyObject *self, PyObject *args) -> PyObject * {
        char ret_buf[1024];
        int ret_len = graphic_text_instance::hyperlink_get_text(ret_buf, sizeof(ret_buf) - 1);
        ret_buf[ret_len] = '\0';

        return Py_BuildValue("s#", ret_buf, ret_len);
    }

    auto wnd_mgr_get_screen_width(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", widgets::get_window_manager().screen_width());
    }

    auto wnd_mgr_get_screen_height(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", widgets::get_window_manager().screen_height());
    }

    auto wnd_mgr_get_mouse_position(PyObject *self, PyObject *args) -> PyObject * {
        int32_t lx, ly;
        widgets::get_window_manager().mouse_position(lx, ly);
        return Py_BuildValue("ii", lx, ly);
    }

    auto wnd_mgr_is_picked_window(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        widgets::ui_window *picked_win = widgets::get_window_manager().point_window();
        return Py_BuildValue("i", win == picked_win ? 1 : 0);
    }

    auto wnd_mgr_get_child_count(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        return Py_BuildValue("i", win->child_count());
    }

    auto wnd_mgr_add_flag(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        char *flag;
        if (!py_tuple_get_string(args, 1, &flag))
            return py_build_exception();

        if (flag && *flag) {
            if (!strcasecmp(flag, "movable"))
                win->add_flag(widgets::ui_window::FLAG_MOVABLE);
            else if (!strcasecmp(flag, "limit"))
                win->add_flag(widgets::ui_window::FLAG_LIMIT);
            else if (!strcasecmp(flag, "dragable"))
                win->add_flag(widgets::ui_window::FLAG_DRAGABLE);
            else if (!strcasecmp(flag, "attach"))
                win->add_flag(widgets::ui_window::FLAG_ATTACH);
            else if (!strcasecmp(flag, "restrict_x"))
                win->add_flag(widgets::ui_window::FLAG_RESTRICT_X);
            else if (!strcasecmp(flag, "restrict_y"))
                win->add_flag(widgets::ui_window::FLAG_RESTRICT_Y);
            else if (!strcasecmp(flag, "float"))
                win->add_flag(widgets::ui_window::FLAG_FLOAT);
            else if (!strcasecmp(flag, "not_pick"))
                win->add_flag(widgets::ui_window::FLAG_NOT_PICK);
            else if (!strcasecmp(flag, "ignore_size"))
                win->add_flag(widgets::ui_window::FLAG_IGNORE_SIZE);
            else if (!strcasecmp(flag, "rtl"))
                win->add_flag(widgets::ui_window::FLAG_RTL);
            else if (!strcasecmp(flag, "ltr"))
                win->remove_flag(widgets::ui_window::FLAG_RTL);
            else
                metin2::log::error("Unknown window flag {}", flag);
        }

        return py_build_none();
    }

    auto wnd_mgr_update_rect(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        win->update_rect();
        return py_build_none();
    }

    auto wnd_mgr_append_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int index;
        if (!py_tuple_get_integer(args, 1, &index))
            return py_build_exception();

        int ix_position;
        if (!py_tuple_get_integer(args, 2, &ix_position))
            return py_build_exception();

        int iy_position;
        if (!py_tuple_get_integer(args, 3, &iy_position))
            return py_build_exception();

        int ix_cell_size;
        if (!py_tuple_get_integer(args, 4, &ix_cell_size))
            return py_build_exception();

        int iy_cell_size;
        if (!py_tuple_get_integer(args, 5, &iy_cell_size))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->append_slot(index, ix_position, iy_position, ix_cell_size, iy_cell_size);

        return py_build_none();
    }

    auto wnd_mgr_arrange_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int start_index;
        if (!py_tuple_get_integer(args, 1, &start_index))
            return py_build_exception();

        int ix_cell_count;
        if (!py_tuple_get_integer(args, 2, &ix_cell_count))
            return py_build_exception();

        int iy_cell_count;
        if (!py_tuple_get_integer(args, 3, &iy_cell_count))
            return py_build_exception();

        int ix_cell_size;
        if (!py_tuple_get_integer(args, 4, &ix_cell_size))
            return py_build_exception();

        int iy_cell_size;
        if (!py_tuple_get_integer(args, 5, &iy_cell_size))
            return py_build_exception();

        int ix_blank;
        if (!py_tuple_get_integer(args, 6, &ix_blank))
            return py_build_exception();

        int iy_blank;
        if (!py_tuple_get_integer(args, 7, &iy_blank))
            return py_build_exception();

        if (!win->is_type(widgets::grid_slot_window::type())) {
            metin2::log::error("Window is not a grid window");
            return py_build_exception();
        }

        auto *grid_slot_win = (widgets::grid_slot_window *)win;
        grid_slot_win->arrange_grid_slot(start_index, ix_cell_count, iy_cell_count, ix_cell_size, iy_cell_size, ix_blank, iy_blank);

        return py_build_none();
    }

    auto wnd_mgr_set_slot_base_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_build_exception();

        float fr;
        if (!py_tuple_get_float(args, 2, &fr))
            return py_build_exception();
        float fg;
        if (!py_tuple_get_float(args, 3, &fg))
            return py_build_exception();
        float fb;
        if (!py_tuple_get_float(args, 4, &fb))
            return py_build_exception();
        float fa;
        if (!py_tuple_get_float(args, 5, &fa))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type())) {
            metin2::log::error("Window is not a slot window");
            return py_build_exception();
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot_base_image(file_name, fr, fg, fb, fa);

        return py_build_none();
    }

    auto wnd_mgr_set_cover_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        char *up_image_name;
        if (!py_tuple_get_string(args, 2, &up_image_name))
            return py_build_exception();
        char *over_image_name;
        if (!py_tuple_get_string(args, 3, &over_image_name))
            return py_build_exception();
        char *down_image_name;
        if (!py_tuple_get_string(args, 4, &down_image_name))
            return py_build_exception();
        char *disable_image_name;
        if (!py_tuple_get_string(args, 5, &disable_image_name))
            return py_build_exception();

        int left_button_enable;
        if (!py_tuple_get_integer(args, 6, &left_button_enable))
            return py_build_exception();
        int right_button_enable;
        if (!py_tuple_get_integer(args, 7, &right_button_enable))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->set_cover_button(slot_index, up_image_name, over_image_name, down_image_name, disable_image_name, left_button_enable, right_button_enable);

        return py_build_none();
    }

    auto wnd_mgr_enable_cover_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->enable_cover_button(slot_index);

        return py_build_none();
    }

    auto wnd_mgr_disable_cover_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->disable_cover_button(slot_index);

        return py_build_none();
    }

    auto wnd_mgr_set_always_render_cover_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        int slot_index;
        bool always_render = false;

        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!py_tuple_get_boolean(args, 2, &always_render))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->set_always_render_cover_button(slot_index, always_render);

        return py_build_none();
    }

    auto wnd_mgr_append_slot_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        char *up_image_name;
        if (!py_tuple_get_string(args, 1, &up_image_name))
            return py_build_exception();
        char *over_image_name;
        if (!py_tuple_get_string(args, 2, &over_image_name))
            return py_build_exception();
        char *down_image_name;
        if (!py_tuple_get_string(args, 3, &down_image_name))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->append_slot_button(up_image_name, over_image_name, down_image_name);

        return py_build_none();
    }

    auto wnd_mgr_append_requirement_sign_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        char *image_name;
        if (!py_tuple_get_string(args, 1, &image_name))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->append_requirement_sign_image(image_name);

        return py_build_none();
    }

    auto wnd_mgr_show_slot_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        int slot_number;
        if (!py_tuple_get_integer(args, 1, &slot_number))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->show_slot_button(slot_number);

        return py_build_none();
    }

    auto wnd_mgr_hide_all_slot_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->hide_all_slot_button();

        return py_build_none();
    }

    auto wnd_mgr_show_requirement_sign(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        int slot_number;
        if (!py_tuple_get_integer(args, 1, &slot_number))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->show_requirement_sign(slot_number);

        return py_build_none();
    }

    auto wnd_mgr_hide_requirement_sign(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        int slot_number;
        if (!py_tuple_get_integer(args, 1, &slot_number))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->hide_requirement_sign(slot_number);

        return py_build_none();
    }

    auto wnd_mgr_refresh_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->refresh_slot();

        return py_build_none();
    }

    auto wnd_mgr_set_use_mode(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();
        int flag;
        if (!py_tuple_get_integer(args, 1, &flag))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_use_mode(flag);

        return py_build_none();
    }

    auto wnd_mgr_set_usable_item(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();
        int flag;
        if (!py_tuple_get_integer(args, 1, &flag))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_usable_item(flag);

        return py_build_none();
    }

    auto wnd_mgr_clear_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->clear_slot(slot_index);

        return py_build_none();
    }

    auto wnd_mgr_clear_all_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->clear_all_slot();

        return py_build_none();
    }

    auto wnd_mgr_has_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;

        return Py_BuildValue("i", slot_win->has_slot(slot_index));
    }

    auto wnd_mgr_set_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        int item_index;
        if (!py_tuple_get_integer(args, 2, &item_index))
            return py_build_exception();

        int width_value;
        if (!py_tuple_get_integer(args, 3, &width_value))
            return py_build_exception();

        int height;
        if (!py_tuple_get_integer(args, 4, &height))
            return py_build_exception();

        intptr_t image_handle; // A graphic_image *, see item.GetIconImage.
        if (!py_tuple_get_integer(args, 5, &image_handle))
            return py_build_exception();

        math::color diffuse_color;
        PyObject *tuple;
        if (!py_tuple_get_object(args, 6, &tuple)) {
            diffuse_color = math::color(1.0, 1.0, 1.0, 1.0);
        } else {
            if (PyTuple_Size(tuple) != 4)
                return py_build_exception();
            if (!py_tuple_get_float(tuple, 0, &diffuse_color.r))
                return py_build_exception();
            if (!py_tuple_get_float(tuple, 1, &diffuse_color.g))
                return py_build_exception();
            if (!py_tuple_get_float(tuple, 2, &diffuse_color.b))
                return py_build_exception();
            if (!py_tuple_get_float(tuple, 3, &diffuse_color.a))
                return py_build_exception();
        }

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot(slot_index, item_index, width_value, height, (graphic_image *)image_handle, diffuse_color);

        return py_build_none();
    }

    auto wnd_mgr_set_slot_count(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        int count;
        if (!py_tuple_get_integer(args, 2, &count))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot_count(slot_index, count);

        return py_build_none();
    }

    auto wnd_mgr_set_slot_count_new(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        int grade;
        if (!py_tuple_get_integer(args, 2, &grade))
            return py_build_exception();

        int count;
        if (!py_tuple_get_integer(args, 3, &count))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot_count_new(slot_index, grade, count);

        return py_build_none();
    }

    auto wnd_mgr_set_slot_cool_time(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        float cool_time;
        if (!py_tuple_get_float(args, 2, &cool_time))
            return py_build_exception();

        float elapsed_time = 0.0f;
        py_tuple_get_float(args, 3, &elapsed_time);

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot_cool_time(slot_index, cool_time, elapsed_time);

        return py_build_none();
    }

    auto wnd_mgr_activate_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->activate_slot(slot_index);
        return py_build_none();
    }

    auto wnd_mgr_deactivate_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->deactivate_slot(slot_index);
        return py_build_none();
    }

    auto wnd_mgr_enable_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->enable_slot(slot_index);
        return py_build_none();
    }

    auto wnd_mgr_disable_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->disable_slot(slot_index);
        return py_build_none();
    }

    auto wnd_mgr_show_slot_base_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->show_slot_base_image(slot_index);
        return py_build_none();
    }

    auto wnd_mgr_hide_slot_base_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->hide_slot_base_image(slot_index);
        return py_build_none();
    }

    auto wnd_mgr_set_slot_style(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        int style;
        if (!py_tuple_get_integer(args, 1, &style))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot_style(style);

        return py_build_none();
    }

    auto wnd_mgr_get_slot_count(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        return Py_BuildValue("i", slot_win->slot_count());
    }

    auto wnd_mgr_lock_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();
        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->lock_slot(slot_index);
        return py_build_none();
    }

    auto wnd_mgr_unlock_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!py_tuple_get_window(args, 0, &win))
            return py_build_exception();
        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (!win->is_type(widgets::slot_window::type()))
            return py_build_exception();

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->unlock_slot(slot_index);
        return py_build_none();
    }

    auto wnd_bar_set_color(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        int color;
        if (!py_tuple_get_integer(args, 1, &color))
            return py_build_exception();

        if (window->is_type(widgets::bar3d::type())) {
            int left_color = color;

            int right_color;
            if (!py_tuple_get_integer(args, 2, &right_color))
                return py_build_exception();
            int center_color;
            if (!py_tuple_get_integer(args, 3, &center_color))
                return py_build_exception();

            ((widgets::bar3d *)window)->set_color(left_color, right_color, center_color);
        } else {
            ((widgets::ui_window *)window)->set_color(color);
        }
        return py_build_none();
    }

    auto wnd_mgr_attach_icon(PyObject *self, PyObject *args) -> PyObject * {
        int type;
        if (!py_tuple_get_integer(args, 0, &type))
            return py_build_exception();
        int index;
        if (!py_tuple_get_integer(args, 1, &index))
            return py_build_exception();
        int slot_number;
        if (!py_tuple_get_integer(args, 2, &slot_number))
            return py_build_exception();
        int width_value;
        if (!py_tuple_get_integer(args, 3, &width_value))
            return py_build_exception();
        int height;
        if (!py_tuple_get_integer(args, 4, &height))
            return py_build_exception();

        widgets::get_window_manager().attach_icon(type, index, slot_number, width_value, height);
        return py_build_none();
    }

    auto wnd_mgr_set_attaching_flag(PyObject *self, PyObject *args) -> PyObject * {
        bool flag;
        if (!py_tuple_get_boolean(args, 0, &flag))
            return py_build_exception();

        widgets::get_window_manager().set_attaching_flag(flag);
        return py_build_none();
    }

    auto wnd_text_set_max(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        int max_value;
        if (!py_tuple_get_integer(args, 1, &max_value))
            return py_build_exception();

        ((widgets::text_line *)window)->set_max(max_value);
        return py_build_none();
    }
    auto wnd_text_set_horizontal_align(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        int type;
        if (!py_tuple_get_integer(args, 1, &type))
            return py_build_exception();

        ((widgets::text_line *)window)->set_horizontal_align(type);
        return py_build_none();
    }
    auto wnd_text_set_vertical_align(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        int type;
        if (!py_tuple_get_integer(args, 1, &type))
            return py_build_exception();

        ((widgets::text_line *)window)->set_vertical_align(type);
        return py_build_none();
    }
    auto wnd_text_set_secret(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        int flag;
        if (!py_tuple_get_integer(args, 1, &flag))
            return py_build_exception();

        ((widgets::text_line *)window)->set_secret(flag);
        return py_build_none();
    }
    auto wnd_text_set_outline(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        int flag;
        if (!py_tuple_get_integer(args, 1, &flag))
            return py_build_exception();

        ((widgets::text_line *)window)->set_outline(flag);
        return py_build_none();
    }
    auto wnd_text_set_feather(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        int flag;
        if (!py_tuple_get_integer(args, 1, &flag))
            return py_build_exception();

        ((widgets::text_line *)window)->set_feather(flag);
        return py_build_none();
    }
    auto wnd_text_set_multi_line(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        int flag;
        if (!py_tuple_get_integer(args, 1, &flag))
            return py_build_exception();

        ((widgets::text_line *)window)->set_multi_line(flag);
        return py_build_none();
    }
    auto wnd_text_set_font_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *font_name;
        if (!py_tuple_get_string(args, 1, &font_name))
            return py_build_exception();

        ((widgets::text_line *)window)->set_font_name(font_name);
        return py_build_none();
    }
    auto wnd_text_set_font_color(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        if (2 == PyTuple_Size(args)) {
            int color;
            if (!py_tuple_get_integer(args, 1, &color))
                return py_build_exception();
            ((widgets::text_line *)window)->set_font_color(color);
        } else if (4 == PyTuple_Size(args)) {
            float fr;
            if (!py_tuple_get_float(args, 1, &fr))
                return py_build_exception();
            float fg;
            if (!py_tuple_get_float(args, 2, &fg))
                return py_build_exception();
            float fb;
            if (!py_tuple_get_float(args, 3, &fb))
                return py_build_exception();
            float fa = 1.0f;

            uint8_t argb[4] =
                {
                    (uint8_t)(255.0f * fb),
                    (uint8_t)(255.0f * fg),
                    (uint8_t)(255.0f * fr),
                    (uint8_t)(255.0f * fa)};
            ((widgets::text_line *)window)->set_font_color(*((uint32_t *)argb));
        } else {
            return py_build_exception();
        }

        return py_build_none();
    }
    auto wnd_text_set_limit_width(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        float width_value;
        if (!py_tuple_get_float(args, 1, &width_value))
            return py_build_exception();

        ((widgets::text_line *)window)->set_limit_width(width_value);
        return py_build_none();
    }
    auto wnd_text_set_text(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *text;
        if (!py_tuple_get_string(args, 1, &text))
            return py_build_exception();

        ((widgets::text_line *)window)->set_text(text);
        return py_build_none();
    }
    auto wnd_text_get_text(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        return Py_BuildValue("s", ((widgets::text_line *)window)->get_text());
    }

    auto wnd_text_get_text_size(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        int width_value;
        int height;

        auto *text_line = (widgets::text_line *)window;
        text_line->text_size(&width_value, &height);

        return Py_BuildValue("(ii)", width_value, height);
    }

    auto wnd_text_show_cursor(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        ((widgets::text_line *)window)->show_cursor();
        return py_build_none();
    }
    auto wnd_text_hide_cursor(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        ((widgets::text_line *)window)->hide_cursor();
        return py_build_none();
    }
    auto wnd_text_get_cursor_position(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        return Py_BuildValue("i", ((widgets::text_line *)window)->cursor_position());
    }

    auto wnd_number_set_number(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *number;
        if (!py_tuple_get_string(args, 1, &number))
            return py_build_exception();

        ((widgets::number_line *)window)->set_number(number);

        return py_build_none();
    }

    auto wnd_number_set_number_horizontal_align_center(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        ((widgets::number_line *)window)->set_horizontal_align(widgets::ui_window::HORIZONTAL_ALIGN_CENTER);

        return py_build_none();
    }

    auto wnd_number_set_number_horizontal_align_right(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        ((widgets::number_line *)window)->set_horizontal_align(widgets::ui_window::HORIZONTAL_ALIGN_RIGHT);

        return py_build_none();
    }

    auto wnd_number_set_path(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *path;
        if (!py_tuple_get_string(args, 1, &path))
            return py_build_exception();

        ((widgets::number_line *)window)->set_path(path);

        return py_build_none();
    }

    auto wnd_mark_box_set_image_filename(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_build_exception();

        ((widgets::mark_box *)window)->load_image(file_name);
        return py_build_none();
    }

    auto wnd_mark_box_load(PyObject *self, PyObject *args) -> PyObject * {
        // Does nothing; kept because the scripts call it.
        return py_build_none();
    }

    auto wnd_mark_box_set_index(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        int index;
        if (!py_tuple_get_integer(args, 1, &index))
            return py_build_exception();

        ((widgets::mark_box *)window)->set_index(index);
        return py_build_none();
    }

    auto wnd_mark_box_set_scale(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        float scale;
        if (!py_tuple_get_float(args, 1, &scale))
            return py_build_exception();

        ((widgets::mark_box *)window)->set_scale(scale);
        return py_build_none();
    }

    auto wnd_mark_box_set_diffuse_color(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        float fr;
        if (!py_tuple_get_float(args, 1, &fr))
            return py_build_exception();
        float fg;
        if (!py_tuple_get_float(args, 2, &fg))
            return py_build_exception();
        float fb;
        if (!py_tuple_get_float(args, 3, &fb))
            return py_build_exception();
        float fa;
        if (!py_tuple_get_float(args, 4, &fa))
            return py_build_exception();

        ((widgets::mark_box *)window)->set_diffuse_color(fr, fg, fb, fa);

        return py_build_none();
    }

    auto wnd_image_load_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_build_exception();

        if (!((widgets::image_box *)window)->load_image(file_name))
            return py_build_exception("Failed to load image (filename: %s)", file_name);

        return py_build_none();
    }

    auto wnd_image_set_diffuse_color(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        float fr;
        if (!py_tuple_get_float(args, 1, &fr))
            return py_build_exception();
        float fg;
        if (!py_tuple_get_float(args, 2, &fg))
            return py_build_exception();
        float fb;
        if (!py_tuple_get_float(args, 3, &fb))
            return py_build_exception();
        float fa;
        if (!py_tuple_get_float(args, 4, &fa))
            return py_build_exception();

        ((widgets::image_box *)window)->set_diffuse_color(fr, fg, fb, fa);

        return py_build_none();
    }

    auto wnd_image_get_width(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        return Py_BuildValue("i", ((widgets::image_box *)window)->width());
    }

    auto wnd_image_get_height(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        return Py_BuildValue("i", ((widgets::image_box *)window)->height());
    }

    auto wnd_image_set_scale(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        float fx;
        if (!py_tuple_get_float(args, 1, &fx))
            return py_build_exception();
        float fy;
        if (!py_tuple_get_float(args, 2, &fy))
            return py_build_exception();

        ((widgets::expanded_image_box *)window)->set_scale(fx, fy);

        return py_build_none();
    }
    auto wnd_image_set_origin(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        float fx;
        if (!py_tuple_get_float(args, 1, &fx))
            return py_build_exception();
        float fy;
        if (!py_tuple_get_float(args, 2, &fy))
            return py_build_exception();

        ((widgets::expanded_image_box *)window)->set_origin(fx, fy);

        return py_build_none();
    }
    auto wnd_image_set_rotation(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        float rotation;
        if (!py_tuple_get_float(args, 1, &rotation))
            return py_build_exception();

        ((widgets::expanded_image_box *)window)->set_rotation(rotation);

        return py_build_none();
    }
    auto wnd_image_set_rendering_rect(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        float left;
        if (!py_tuple_get_float(args, 1, &left))
            return py_build_exception();
        float top;
        if (!py_tuple_get_float(args, 2, &top))
            return py_build_exception();
        float right;
        if (!py_tuple_get_float(args, 3, &right))
            return py_build_exception();
        float bottom;
        if (!py_tuple_get_float(args, 4, &bottom))
            return py_build_exception();

        if (window->is_type(widgets::expanded_image_box::type()))
            ((widgets::expanded_image_box *)window)->set_rendering_rect(left, top, right, bottom);
        else if (window->is_type(widgets::ani_image_box::type()))
            ((widgets::ani_image_box *)window)->set_rendering_rect(left, top, right, bottom);

        return py_build_none();
    }
    auto wnd_image_set_rendering_mode(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        int mode;
        if (!py_tuple_get_integer(args, 1, &mode))
            return py_build_exception();

        if (window->is_type(widgets::expanded_image_box::type()))
            ((widgets::expanded_image_box *)window)->set_rendering_mode(mode);

        return py_build_none();
    }
    auto wnd_image_set_delay(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        float delay;
        if (!py_tuple_get_float(args, 1, &delay))
            return py_build_exception();

        ((widgets::ani_image_box *)window)->set_delay(delay);

        return py_build_none();
    }
    auto wnd_image_append_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_build_exception();

        ((widgets::ani_image_box *)window)->append_image(file_name);

        return py_build_none();
    }

    auto wnd_button_set_up_visual(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_build_exception();

        ((widgets::button *)window)->set_up_visual(file_name);

        return py_build_none();
    }
    auto wnd_button_set_over_visual(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_build_exception();

        ((widgets::button *)window)->set_over_visual(file_name);

        return py_build_none();
    }
    auto wnd_button_set_down_visual(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_build_exception();

        ((widgets::button *)window)->set_down_visual(file_name);

        return py_build_none();
    }
    auto wnd_button_set_disable_visual(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_build_exception();

        ((widgets::button *)window)->set_disable_visual(file_name);

        return py_build_none();
    }
    auto wnd_button_get_up_visual_file_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        return Py_BuildValue("s", ((widgets::button *)window)->up_visual_file_name());
    }
    auto wnd_button_get_over_visual_file_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        return Py_BuildValue("s", ((widgets::button *)window)->over_visual_file_name());
    }
    auto wnd_button_get_down_visual_file_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        return Py_BuildValue("s", ((widgets::button *)window)->down_visual_file_name());
    }
    auto wnd_button_flash(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        ((widgets::button *)window)->flash();

        return py_build_none();
    }
    auto wnd_button_enable(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        ((widgets::button *)window)->enable();

        return py_build_none();
    }
    auto wnd_button_disable(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        ((widgets::button *)window)->disable();

        return py_build_none();
    }
    auto wnd_button_down(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        ((widgets::button *)window)->down();

        return py_build_none();
    }
    auto wnd_button_set_up(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        ((widgets::button *)window)->set_up();

        return py_build_none();
    }
    auto wnd_button_set_restrict_movement_area(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();
        int ix;
        if (!py_tuple_get_integer(args, 1, &ix))
            return py_build_exception();
        int iy;
        if (!py_tuple_get_integer(args, 2, &iy))
            return py_build_exception();
        int iwidth;
        if (!py_tuple_get_integer(args, 3, &iwidth))
            return py_build_exception();
        int iheight;
        if (!py_tuple_get_integer(args, 4, &iheight))
            return py_build_exception();

        ((widgets::drag_button *)window)->set_restrict_movement_area(ix, iy, iwidth, iheight);

        return py_build_none();
    }

    auto wnd_button_is_down(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!py_tuple_get_window(args, 0, &window))
            return py_build_exception();

        return Py_BuildValue("i", ((widgets::button *)window)->is_pressed());
    }

    extern bool outline_box_enable;

    auto wnd_mgr_set_outline_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag))
            return py_build_exception();

        outline_box_enable = flag;

        return py_build_none();
    }

    void initwnd_mgr() {
        static PyMethodDef s_methods[] =
            {
                // WindowManager
                {"SetMouseHandler", wnd_mgr_set_mouse_handler, METH_VARARGS},
                {"SetScreenSize", wnd_mgr_set_screen_size, METH_VARARGS},
                {"GetScreenWidth", wnd_mgr_get_screen_width, METH_VARARGS},
                {"GetScreenHeight", wnd_mgr_get_screen_height, METH_VARARGS},
                {"AttachIcon", wnd_mgr_attach_icon, METH_VARARGS},
                {"SetAttachingFlag", wnd_mgr_set_attaching_flag, METH_VARARGS},
                {"GetHyperlink", wnd_mgr_get_hyperlink, METH_VARARGS},
                {"OnceIgnoreMouseLeftButtonUpEvent", wnd_mgr_once_ignore_mouse_left_button_up_event, METH_VARARGS},

                // Window
                {"Register", wnd_mgr_register_window<widgets::ui_window>, METH_VARARGS},
                {"RegisterSlotWindow", wnd_mgr_register_window<widgets::slot_window>, METH_VARARGS},
                {"RegisterGridSlotWindow", wnd_mgr_register_window<widgets::grid_slot_window>, METH_VARARGS},
                {"RegisterTextLine", wnd_mgr_register_window<widgets::text_line>, METH_VARARGS},
                {"RegisterMarkBox", wnd_mgr_register_window<widgets::mark_box>, METH_VARARGS},
                {"RegisterImageBox", wnd_mgr_register_window<widgets::image_box>, METH_VARARGS},
                {"RegisterExpandedImageBox", wnd_mgr_register_window<widgets::expanded_image_box>, METH_VARARGS},
                {"RegisterAniImageBox", wnd_mgr_register_window<widgets::ani_image_box>, METH_VARARGS},
                {"RegisterButton", wnd_mgr_register_window<widgets::button>, METH_VARARGS},
                {"RegisterRadioButton", wnd_mgr_register_window<widgets::radio_button>, METH_VARARGS},
                {"RegisterToggleButton", wnd_mgr_register_window<widgets::toggle_button>, METH_VARARGS},
                {"RegisterDragButton", wnd_mgr_register_window<widgets::drag_button>, METH_VARARGS},
                {"RegisterBox", wnd_mgr_register_window<widgets::box>, METH_VARARGS},
                {"RegisterBar", wnd_mgr_register_window<widgets::bar>, METH_VARARGS},
                {"RegisterLine", wnd_mgr_register_window<widgets::line>, METH_VARARGS},
                {"RegisterBar3D", wnd_mgr_register_window<widgets::bar3d>, METH_VARARGS},
                {"RegisterNumberLine", wnd_mgr_register_window<widgets::number_line>, METH_VARARGS},
                {"Destroy", wnd_mgr_destroy, METH_VARARGS},
                {"AddFlag", wnd_mgr_add_flag, METH_VARARGS},
                {"IsRTL", wnd_mgr_is_rtl, METH_VARARGS},

                // Base Window
                {"SetName", wnd_mgr_set_name, METH_VARARGS},
                {"GetName", wnd_mgr_get_name, METH_VARARGS},

                {"SetTop", wnd_mgr_set_top, METH_VARARGS},
                {"Show", wnd_mgr_show, METH_VARARGS},
                {"Hide", wnd_mgr_hide, METH_VARARGS},
                {"IsShow", wnd_mgr_is_show, METH_VARARGS},
                {"SetParent", wnd_mgr_set_parent, METH_VARARGS},
                {"SetPickAlways", wnd_mgr_set_pick_always, METH_VARARGS},

                {"IsFocus", wnd_mgr_is_focus, METH_VARARGS},
                {"SetFocus", wnd_mgr_set_focus, METH_VARARGS},
                {"KillFocus", wnd_mgr_kill_focus, METH_VARARGS},
                {"Lock", wnd_mgr_lock, METH_VARARGS},
                {"Unlock", wnd_mgr_unlock, METH_VARARGS},

                {"SetWindowSize", wnd_mgr_set_wnd_size, METH_VARARGS},
                {"SetWindowPosition", wnd_mgr_set_wnd_position, METH_VARARGS},
                {"GetWindowWidth", wnd_mgr_get_wnd_width, METH_VARARGS},
                {"GetWindowHeight", wnd_mgr_get_wnd_height, METH_VARARGS},
                {"GetWindowLocalPosition", wnd_mgr_get_wnd_local_position, METH_VARARGS},
                {"GetWindowGlobalPosition", wnd_mgr_get_wnd_global_position, METH_VARARGS},
                {"GetWindowRect", wnd_mgr_get_window_rect, METH_VARARGS},
                {"SetWindowHorizontalAlign", wnd_mgr_set_window_horizontal_align, METH_VARARGS},
                {"SetWindowVerticalAlign", wnd_mgr_set_window_vertical_align, METH_VARARGS},

                {"GetChildCount", wnd_mgr_get_child_count, METH_VARARGS},

                {"IsPickedWindow", wnd_mgr_is_picked_window, METH_VARARGS},
                {"IsIn", wnd_mgr_is_in, METH_VARARGS},
                {"GetMouseLocalPosition", wnd_mgr_get_mouse_local_position, METH_VARARGS},
                {"GetMousePosition", wnd_mgr_get_mouse_position, METH_VARARGS},

                {"UpdateRect", wnd_mgr_update_rect, METH_VARARGS},

                // Slot Window
                {"AppendSlot", wnd_mgr_append_slot, METH_VARARGS},
                {"ArrangeSlot", wnd_mgr_arrange_slot, METH_VARARGS},
                {"ClearSlot", wnd_mgr_clear_slot, METH_VARARGS},
                {"ClearAllSlot", wnd_mgr_clear_all_slot, METH_VARARGS},
                {"HasSlot", wnd_mgr_has_slot, METH_VARARGS},
                {"SetSlot", wnd_mgr_set_slot, METH_VARARGS},
                {"SetSlotCount", wnd_mgr_set_slot_count, METH_VARARGS},
                {"SetSlotCountNew", wnd_mgr_set_slot_count_new, METH_VARARGS},
                {"SetSlotCoolTime", wnd_mgr_set_slot_cool_time, METH_VARARGS},
                {"ActivateSlot", wnd_mgr_activate_slot, METH_VARARGS},
                {"DeactivateSlot", wnd_mgr_deactivate_slot, METH_VARARGS},
                {"EnableSlot", wnd_mgr_enable_slot, METH_VARARGS},
                {"DisableSlot", wnd_mgr_disable_slot, METH_VARARGS},
                {"ShowSlotBaseImage", wnd_mgr_show_slot_base_image, METH_VARARGS},
                {"HideSlotBaseImage", wnd_mgr_hide_slot_base_image, METH_VARARGS},
                {"SetSlotStyle", wnd_mgr_set_slot_style, METH_VARARGS},
                {"SetSlotBaseImage", wnd_mgr_set_slot_base_image, METH_VARARGS},

                {"SetCoverButton", wnd_mgr_set_cover_button, METH_VARARGS},
                {"EnableCoverButton", wnd_mgr_enable_cover_button, METH_VARARGS},
                {"DisableCoverButton", wnd_mgr_disable_cover_button, METH_VARARGS},
                {"SetAlwaysRenderCoverButton", wnd_mgr_set_always_render_cover_button, METH_VARARGS},

                {"AppendSlotButton", wnd_mgr_append_slot_button, METH_VARARGS},
                {"AppendRequirementSignImage", wnd_mgr_append_requirement_sign_image, METH_VARARGS},
                {"ShowSlotButton", wnd_mgr_show_slot_button, METH_VARARGS},
                {"HideAllSlotButton", wnd_mgr_hide_all_slot_button, METH_VARARGS},
                {"ShowRequirementSign", wnd_mgr_show_requirement_sign, METH_VARARGS},
                {"HideRequirementSign", wnd_mgr_hide_requirement_sign, METH_VARARGS},
                {"RefreshSlot", wnd_mgr_refresh_slot, METH_VARARGS},
                {"SetUseMode", wnd_mgr_set_use_mode, METH_VARARGS},
                {"SetUsableItem", wnd_mgr_set_usable_item, METH_VARARGS},

                {"GetSlotCount", wnd_mgr_get_slot_count, METH_VARARGS},
                {"LockSlot", wnd_mgr_lock_slot, METH_VARARGS},
                {"UnlockSlot", wnd_mgr_unlock_slot, METH_VARARGS},

                // Bar
                {"SetColor", wnd_bar_set_color, METH_VARARGS},

                // TextLine
                {"SetMax", wnd_text_set_max, METH_VARARGS},
                {"SetHorizontalAlign", wnd_text_set_horizontal_align, METH_VARARGS},
                {"SetVerticalAlign", wnd_text_set_vertical_align, METH_VARARGS},
                {"SetSecret", wnd_text_set_secret, METH_VARARGS},
                {"SetOutline", wnd_text_set_outline, METH_VARARGS},
                {"SetFeather", wnd_text_set_feather, METH_VARARGS},
                {"SetMultiLine", wnd_text_set_multi_line, METH_VARARGS},
                {"SetText", wnd_text_set_text, METH_VARARGS},
                {"SetFontName", wnd_text_set_font_name, METH_VARARGS},
                {"SetFontColor", wnd_text_set_font_color, METH_VARARGS},
                {"SetLimitWidth", wnd_text_set_limit_width, METH_VARARGS},
                {"GetText", wnd_text_get_text, METH_VARARGS},
                {"GetTextSize", wnd_text_get_text_size, METH_VARARGS},
                {"ShowCursor", wnd_text_show_cursor, METH_VARARGS},
                {"HideCursor", wnd_text_hide_cursor, METH_VARARGS},
                {"GetCursorPosition", wnd_text_get_cursor_position, METH_VARARGS},
                // NumberLine
                {"SetNumber", wnd_number_set_number, METH_VARARGS},
                {"SetNumberHorizontalAlignCenter", wnd_number_set_number_horizontal_align_center, METH_VARARGS},
                {"SetNumberHorizontalAlignRight", wnd_number_set_number_horizontal_align_right, METH_VARARGS},
                {"SetPath", wnd_number_set_path, METH_VARARGS},

                // ImageBox
                {"MarkBox_SetImageFilename", wnd_mark_box_set_image_filename, METH_VARARGS},
                {"MarkBox_Load", wnd_mark_box_load, METH_VARARGS},
                {"MarkBox_SetIndex", wnd_mark_box_set_index, METH_VARARGS},
                {"MarkBox_SetScale", wnd_mark_box_set_scale, METH_VARARGS},
                {"MarkBox_SetDiffuseColor", wnd_mark_box_set_diffuse_color, METH_VARARGS},

                // ImageBox
                {"LoadImage", wnd_image_load_image, METH_VARARGS},
                {"SetDiffuseColor", wnd_image_set_diffuse_color, METH_VARARGS},
                {"GetWidth", wnd_image_get_width, METH_VARARGS},
                {"GetHeight", wnd_image_get_height, METH_VARARGS},
                // ExpandedImageBox
                {"SetScale", wnd_image_set_scale, METH_VARARGS},
                {"SetOrigin", wnd_image_set_origin, METH_VARARGS},
                {"SetRotation", wnd_image_set_rotation, METH_VARARGS},
                {"SetRenderingRect", wnd_image_set_rendering_rect, METH_VARARGS},
                {"SetRenderingMode", wnd_image_set_rendering_mode, METH_VARARGS},
                // AniImageBox
                {"SetDelay", wnd_image_set_delay, METH_VARARGS},
                {"AppendImage", wnd_image_append_image, METH_VARARGS},

                // Button
                {"SetUpVisual", wnd_button_set_up_visual, METH_VARARGS},
                {"SetOverVisual", wnd_button_set_over_visual, METH_VARARGS},
                {"SetDownVisual", wnd_button_set_down_visual, METH_VARARGS},
                {"SetDisableVisual", wnd_button_set_disable_visual, METH_VARARGS},
                {"GetUpVisualFileName", wnd_button_get_up_visual_file_name, METH_VARARGS},
                {"GetOverVisualFileName", wnd_button_get_over_visual_file_name, METH_VARARGS},
                {"GetDownVisualFileName", wnd_button_get_down_visual_file_name, METH_VARARGS},
                {"Flash", wnd_button_flash, METH_VARARGS},
                {"Enable", wnd_button_enable, METH_VARARGS},
                {"Disable", wnd_button_disable, METH_VARARGS},
                {"Down", wnd_button_down, METH_VARARGS},
                {"SetUp", wnd_button_set_up, METH_VARARGS},
                {"IsDown", wnd_button_is_down, METH_VARARGS},

                // DragButton
                {"SetRestrictMovementArea", wnd_button_set_restrict_movement_area, METH_VARARGS},

                // For Debug
                {"SetOutlineFlag", wnd_mgr_set_outline_flag, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = Py_InitModule("wndMgr", s_methods);

        PyModule_AddIntConstant(module_value, "SLOT_STYLE_NONE", widgets::SLOT_STYLE_NONE);
        PyModule_AddIntConstant(module_value, "SLOT_STYLE_PICK_UP", widgets::SLOT_STYLE_PICK_UP);
        PyModule_AddIntConstant(module_value, "SLOT_STYLE_SELECT", widgets::SLOT_STYLE_SELECT);

        PyModule_AddIntConstant(module_value, "TEXT_HORIZONTAL_ALIGN_LEFT", graphic_text_instance::HORIZONTAL_ALIGN_LEFT);
        PyModule_AddIntConstant(module_value, "TEXT_HORIZONTAL_ALIGN_RIGHT", graphic_text_instance::HORIZONTAL_ALIGN_RIGHT);
        PyModule_AddIntConstant(module_value, "TEXT_HORIZONTAL_ALIGN_CENTER", graphic_text_instance::HORIZONTAL_ALIGN_CENTER);
        PyModule_AddIntConstant(module_value, "TEXT_VERTICAL_ALIGN_TOP", graphic_text_instance::VERTICAL_ALIGN_TOP);
        PyModule_AddIntConstant(module_value, "TEXT_VERTICAL_ALIGN_BOTTOM", graphic_text_instance::VERTICAL_ALIGN_BOTTOM);
        PyModule_AddIntConstant(module_value, "TEXT_VERTICAL_ALIGN_CENTER", graphic_text_instance::VERTICAL_ALIGN_CENTER);

        PyModule_AddIntConstant(module_value, "HORIZONTAL_ALIGN_LEFT", widgets::ui_window::HORIZONTAL_ALIGN_LEFT);
        PyModule_AddIntConstant(module_value, "HORIZONTAL_ALIGN_CENTER", widgets::ui_window::HORIZONTAL_ALIGN_CENTER);
        PyModule_AddIntConstant(module_value, "HORIZONTAL_ALIGN_RIGHT", widgets::ui_window::HORIZONTAL_ALIGN_RIGHT);
        PyModule_AddIntConstant(module_value, "VERTICAL_ALIGN_TOP", widgets::ui_window::VERTICAL_ALIGN_TOP);
        PyModule_AddIntConstant(module_value, "VERTICAL_ALIGN_CENTER", widgets::ui_window::VERTICAL_ALIGN_CENTER);
        PyModule_AddIntConstant(module_value, "VERTICAL_ALIGN_BOTTOM", widgets::ui_window::VERTICAL_ALIGN_BOTTOM);

        PyModule_AddIntConstant(module_value, "RENDERING_MODE_MODULATE", graphic_expanded_image_instance::RENDERING_MODE_MODULATE);
    }
}
