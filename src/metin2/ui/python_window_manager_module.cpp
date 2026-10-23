#include <metin2/log/log.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include "python_grid_slot_window.h"
#include "python_modules.h"
#include "python_slot_window.h"
#include "python_window.h"
#include "python_window_manager.h"

namespace metin2::inline ui {

    constexpr auto window_converter = &py_handle_converter<widgets::ui_window>;

    auto wnd_mgr_once_ignore_mouse_left_button_up_event(PyObject *self, PyObject *args) -> PyObject * {
        widgets::get_window_manager().once_ignore_mouse_left_button_up_event();
        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_mouse_handler(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handler;
        if (!PyArg_ParseTuple(args, "O", &handler)) {
            return nullptr;
        }

        widgets::get_window_manager().set_mouse_handler(handler);
        Py_RETURN_NONE;
    }

    // wndMgr.Register*(pyHandler, layerName): creates a T, attaches it to the layer and returns its handle.
    template <class T>
    auto wnd_mgr_register_window(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *po;
        char *layer;

        if (!PyArg_ParseTuple(args, "Os", &po, &layer)) {
            return nullptr;
        }

        widgets::ui_window *window = widgets::get_window_manager().register_window(new T(po), layer);
        if (!window) {
            PyErr_Format(PyExc_ValueError, "Unknown window layer %s", layer);
            return nullptr;
        }

        return Py_BuildValue("n", (Py_ssize_t)window);
    }

    auto wnd_mgr_destroy(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        widgets::get_window_manager().destroy_window(win);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_is_focus(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        return Py_BuildValue("i", window == widgets::get_window_manager().get_activate_window());
    }

    auto wnd_mgr_set_focus(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        widgets::get_window_manager().activate_window(win);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_kill_focus(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle = 0;
        if (!PyArg_ParseTuple(args, "|n", &handle)) {
            return nullptr;
        }

        auto *win = (widgets::ui_window *)handle;
        if (win == nullptr || win == widgets::get_window_manager().get_activate_window()) {
            widgets::get_window_manager().deactivate_window();
        }

        Py_RETURN_NONE;
    }

    auto wnd_mgr_lock(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        widgets::get_window_manager().lock_window(win);
        Py_RETURN_NONE;
    }

    // The scripts pass the locked window's handle; the manager only ever has one locked window.
    auto wnd_mgr_unlock(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle = 0;
        if (!PyArg_ParseTuple(args, "|n", &handle)) {
            return nullptr;
        }

        widgets::get_window_manager().unlock_window();
        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        char *name;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &win, &name)) {
            return nullptr;
        }

        win->set_name(name);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_top(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        widgets::get_window_manager().set_top(win);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_show(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        win->show();
        Py_RETURN_NONE;
    }

    auto wnd_mgr_hide(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        win->hide();
        Py_RETURN_NONE;
    }

    auto wnd_mgr_is_show(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        return Py_BuildValue("i", win->is_show());
    }

    auto wnd_mgr_is_rtl(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        return Py_BuildValue("i", win->is_flag(widgets::ui_window::FLAG_RTL));
    }

    auto wnd_mgr_set_screen_size(PyObject *self, PyObject *args) -> PyObject * {
        int width_value, height;
        if (!PyArg_ParseTuple(args, "ii", &width_value, &height)) {
            return nullptr;
        }

        widgets::get_window_manager().set_screen_size(width_value, height);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_parent(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win, *parent_win;
        if (!PyArg_ParseTuple(args, "O&O&", window_converter, &win, window_converter, &parent_win)) {
            return nullptr;
        }

        widgets::get_window_manager().set_parent(win, parent_win);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_pick_always(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        widgets::get_window_manager().set_pick_always(win);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_wnd_size(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int width_value, height;

        if (!PyArg_ParseTuple(args, "O&ii", window_converter, &win, &width_value, &height)) {
            return nullptr;
        }

        win->set_size(width_value, height);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_wnd_position(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int x, y;

        if (!PyArg_ParseTuple(args, "O&ii", window_converter, &win, &x, &y)) {
            return nullptr;
        }

        win->set_position(x, y);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_get_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        return Py_BuildValue("s", win->get_name());
    }

    auto wnd_mgr_get_wnd_width(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        return Py_BuildValue("i", win->width());
    }

    auto wnd_mgr_get_wnd_height(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        return Py_BuildValue("i", win->height());
    }

    auto wnd_mgr_get_wnd_local_position(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        int32_t lx, ly;
        win->get_position(&lx, &ly);

        return Py_BuildValue("ii", lx, ly);
    }

    auto wnd_mgr_get_wnd_global_position(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        rectangle &rect_value = window->rect();
        return Py_BuildValue("ii", rect_value.left, rect_value.top);
    }

    auto wnd_mgr_get_window_rect(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        rectangle &rect_value = win->rect();
        return Py_BuildValue("iiii", rect_value.left, rect_value.top, rect_value.right - rect_value.left, rect_value.bottom - rect_value.top);
    }

    auto wnd_mgr_set_window_horizontal_align(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int align;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &align)) {
            return nullptr;
        }

        win->set_horizontal_align(align);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_window_vertical_align(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int align;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &align)) {
            return nullptr;
        }

        win->set_vertical_align(align);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_is_in(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        return Py_BuildValue("i", win == widgets::get_window_manager().point_window());
    }

    auto wnd_mgr_get_mouse_local_position(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

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
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        widgets::ui_window *picked_win = widgets::get_window_manager().point_window();
        return Py_BuildValue("i", win == picked_win ? 1 : 0);
    }

    auto wnd_mgr_get_child_count(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        return Py_BuildValue("i", win->child_count());
    }

    auto wnd_mgr_add_flag(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        char *flag;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &win, &flag)) {
            return nullptr;
        }

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

        Py_RETURN_NONE;
    }

    auto wnd_mgr_update_rect(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        win->update_rect();
        Py_RETURN_NONE;
    }

    auto wnd_mgr_append_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int index, ix_position, iy_position, ix_cell_size, iy_cell_size;

        if (!PyArg_ParseTuple(args, "O&iiiii", window_converter, &win, &index, &ix_position, &iy_position, &ix_cell_size, &iy_cell_size)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->append_slot(index, ix_position, iy_position, ix_cell_size, iy_cell_size);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_arrange_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int start_index, ix_cell_count, iy_cell_count, ix_cell_size, iy_cell_size, ix_blank, iy_blank;

        if (!PyArg_ParseTuple(args, "O&iiiiiii", window_converter, &win, &start_index, &ix_cell_count, &iy_cell_count, &ix_cell_size, &iy_cell_size, &ix_blank, &iy_blank)) {
            return nullptr;
        }

        if (!win->is_type(widgets::grid_slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a grid slot window");
            return nullptr;
        }

        auto *grid_slot_win = (widgets::grid_slot_window *)win;
        grid_slot_win->arrange_grid_slot(start_index, ix_cell_count, iy_cell_count, ix_cell_size, iy_cell_size, ix_blank, iy_blank);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_slot_base_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        char *file_name;
        float fr, fg, fb, fa;

        if (!PyArg_ParseTuple(args, "O&sffff", window_converter, &win, &file_name, &fr, &fg, &fb, &fa)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot_base_image(file_name, fr, fg, fb, fa);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_cover_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int slot_index;
        char *up_image_name, *over_image_name, *down_image_name, *disable_image_name;
        int left_button_enable, right_button_enable;

        if (!PyArg_ParseTuple(args, "O&issssii", window_converter, &window, &slot_index, &up_image_name, &over_image_name, &down_image_name, &disable_image_name, &left_button_enable, &right_button_enable)) {
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->set_cover_button(slot_index, up_image_name, over_image_name, down_image_name, disable_image_name, left_button_enable, right_button_enable);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_enable_cover_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &slot_index)) {
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->enable_cover_button(slot_index);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_disable_cover_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &slot_index)) {
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->disable_cover_button(slot_index);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_always_render_cover_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int slot_index;
        int always_render = false;

        if (!PyArg_ParseTuple(args, "O&ii", window_converter, &window, &slot_index, &always_render)) {
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->set_always_render_cover_button(slot_index, always_render);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_append_slot_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *up_image_name, *over_image_name, *down_image_name;

        if (!PyArg_ParseTuple(args, "O&sss", window_converter, &window, &up_image_name, &over_image_name, &down_image_name)) {
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->append_slot_button(up_image_name, over_image_name, down_image_name);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_append_requirement_sign_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *image_name;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &image_name)) {
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->append_requirement_sign_image(image_name);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_show_slot_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int slot_number;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &slot_number)) {
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->show_slot_button(slot_number);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_hide_all_slot_button(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->hide_all_slot_button();

        Py_RETURN_NONE;
    }

    auto wnd_mgr_show_requirement_sign(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int slot_number;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &slot_number)) {
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->show_requirement_sign(slot_number);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_hide_requirement_sign(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int slot_number;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &slot_number)) {
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)window;
        slot_win->hide_requirement_sign(slot_number);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_refresh_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->refresh_slot();

        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_use_mode(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int flag;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &flag)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_use_mode(flag);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_usable_item(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int flag;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &flag)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_usable_item(flag);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_clear_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &slot_index)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->clear_slot(slot_index);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_clear_all_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->clear_all_slot();

        Py_RETURN_NONE;
    }

    auto wnd_mgr_has_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &slot_index)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;

        return Py_BuildValue("i", slot_win->has_slot(slot_index));
    }

    auto wnd_mgr_set_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index, item_index, width_value, height;
        Py_ssize_t image_handle; // A graphic_image *, see item.GetIconImage.
        math::color diffuse_color(1.0f, 1.0f, 1.0f, 1.0f);

        if (!PyArg_ParseTuple(args, "O&iiiin|(ffff)", window_converter, &win, &slot_index, &item_index, &width_value, &height, &image_handle, &diffuse_color.r, &diffuse_color.g, &diffuse_color.b, &diffuse_color.a)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot(slot_index, item_index, width_value, height, (graphic_image *)image_handle, diffuse_color);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_slot_count(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index, count;

        if (!PyArg_ParseTuple(args, "O&ii", window_converter, &win, &slot_index, &count)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot_count(slot_index, count);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_slot_count_new(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index, grade, count;

        if (!PyArg_ParseTuple(args, "O&iii", window_converter, &win, &slot_index, &grade, &count)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot_count_new(slot_index, grade, count);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_slot_cool_time(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;
        float cool_time, elapsed_time = 0.0f;

        if (!PyArg_ParseTuple(args, "O&if|f", window_converter, &win, &slot_index, &cool_time, &elapsed_time)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot_cool_time(slot_index, cool_time, elapsed_time);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_activate_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &slot_index)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->activate_slot(slot_index);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_deactivate_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &slot_index)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->deactivate_slot(slot_index);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_enable_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &slot_index)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->enable_slot(slot_index);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_disable_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &slot_index)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->disable_slot(slot_index);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_show_slot_base_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &slot_index)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->show_slot_base_image(slot_index);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_hide_slot_base_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &slot_index)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->hide_slot_base_image(slot_index);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_slot_style(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int style;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &style)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->set_slot_style(style);

        Py_RETURN_NONE;
    }

    auto wnd_mgr_get_slot_count(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &win)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        return Py_BuildValue("i", slot_win->slot_count());
    }

    auto wnd_mgr_lock_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &slot_index)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->lock_slot(slot_index);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_unlock_slot(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *win;
        int slot_index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &win, &slot_index)) {
            return nullptr;
        }

        if (!win->is_type(widgets::slot_window::type())) {
            PyErr_SetString(PyExc_TypeError, "Window is not a slot window");
            return nullptr;
        }

        auto *slot_win = (widgets::slot_window *)win;
        slot_win->unlock_slot(slot_index);
        Py_RETURN_NONE;
    }

    auto wnd_bar_set_color(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        uint32_t color, right_color = 0, center_color = 0;

        if (!PyArg_ParseTuple(args, "O&I|II", window_converter, &window, &color, &right_color, &center_color)) {
            return nullptr;
        }

        if (window->is_type(widgets::bar3d::type())) {
            if (PyTuple_Size(args) != 4) {
                PyErr_SetString(PyExc_TypeError, "A 3D bar takes three colors");
                return nullptr;
            }

            ((widgets::bar3d *)window)->set_color(color, right_color, center_color);
        } else {
            window->set_color(color);
        }
        Py_RETURN_NONE;
    }

    auto wnd_mgr_attach_icon(PyObject *self, PyObject *args) -> PyObject * {
        int type, index, slot_number, width_value, height;
        if (!PyArg_ParseTuple(args, "iiiii", &type, &index, &slot_number, &width_value, &height)) {
            return nullptr;
        }

        widgets::get_window_manager().attach_icon(type, index, slot_number, width_value, height);
        Py_RETURN_NONE;
    }

    auto wnd_mgr_set_attaching_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        widgets::get_window_manager().set_attaching_flag(flag);
        Py_RETURN_NONE;
    }

    auto wnd_text_set_max(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int max_value;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &max_value)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->set_max(max_value);
        Py_RETURN_NONE;
    }
    auto wnd_text_set_horizontal_align(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int type;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &type)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->set_horizontal_align(type);
        Py_RETURN_NONE;
    }
    auto wnd_text_set_vertical_align(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int type;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &type)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->set_vertical_align(type);
        Py_RETURN_NONE;
    }
    auto wnd_text_set_secret(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int flag;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &flag)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->set_secret(flag);
        Py_RETURN_NONE;
    }
    auto wnd_text_set_outline(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int flag;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &flag)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->set_outline(flag);
        Py_RETURN_NONE;
    }
    auto wnd_text_set_feather(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int flag;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &flag)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->set_feather(flag);
        Py_RETURN_NONE;
    }
    auto wnd_text_set_multi_line(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int flag;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &flag)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->set_multi_line(flag);
        Py_RETURN_NONE;
    }
    auto wnd_text_set_font_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *font_name;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &font_name)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->set_font_name(font_name);
        Py_RETURN_NONE;
    }
    auto wnd_text_set_font_color(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        uint32_t color;
        float fr, fg, fb;

        if (2 == PyTuple_Size(args)) {
            if (!PyArg_ParseTuple(args, "O&I", window_converter, &window, &color)) {
                return nullptr;
            }

            ((widgets::text_line *)window)->set_font_color(color);
        } else if (4 == PyTuple_Size(args)) {
            if (!PyArg_ParseTuple(args, "O&fff", window_converter, &window, &fr, &fg, &fb)) {
                return nullptr;
            }

            float fa = 1.0f;

            uint8_t argb[4] =
                {
                    (uint8_t)(255.0f * fb),
                    (uint8_t)(255.0f * fg),
                    (uint8_t)(255.0f * fr),
                    (uint8_t)(255.0f * fa)};
            ((widgets::text_line *)window)->set_font_color(*((uint32_t *)argb));
        } else {
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }

        Py_RETURN_NONE;
    }
    auto wnd_text_set_limit_width(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        float width_value;

        if (!PyArg_ParseTuple(args, "O&f", window_converter, &window, &width_value)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->set_limit_width(width_value);
        Py_RETURN_NONE;
    }
    auto wnd_text_set_text(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *text;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &text)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->set_text(text);
        Py_RETURN_NONE;
    }
    auto wnd_text_get_text(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        return Py_BuildValue("s", ((widgets::text_line *)window)->get_text());
    }

    auto wnd_text_get_text_size(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        int width_value;
        int height;

        auto *text_line = (widgets::text_line *)window;
        text_line->text_size(&width_value, &height);

        return Py_BuildValue("(ii)", width_value, height);
    }

    auto wnd_text_show_cursor(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->show_cursor();
        Py_RETURN_NONE;
    }
    auto wnd_text_hide_cursor(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        ((widgets::text_line *)window)->hide_cursor();
        Py_RETURN_NONE;
    }
    auto wnd_text_get_cursor_position(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        return Py_BuildValue("i", ((widgets::text_line *)window)->cursor_position());
    }

    auto wnd_number_set_number(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *number;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &number)) {
            return nullptr;
        }

        ((widgets::number_line *)window)->set_number(number);

        Py_RETURN_NONE;
    }

    auto wnd_number_set_number_horizontal_align_center(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        ((widgets::number_line *)window)->set_horizontal_align(widgets::ui_window::HORIZONTAL_ALIGN_CENTER);

        Py_RETURN_NONE;
    }

    auto wnd_number_set_number_horizontal_align_right(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        ((widgets::number_line *)window)->set_horizontal_align(widgets::ui_window::HORIZONTAL_ALIGN_RIGHT);

        Py_RETURN_NONE;
    }

    auto wnd_number_set_path(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *path;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &path)) {
            return nullptr;
        }

        ((widgets::number_line *)window)->set_path(path);

        Py_RETURN_NONE;
    }

    auto wnd_mark_box_set_image_filename(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *file_name;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &file_name)) {
            return nullptr;
        }

        ((widgets::mark_box *)window)->load_image(file_name);
        Py_RETURN_NONE;
    }

    auto wnd_mark_box_load(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle;
        if (!PyArg_ParseTuple(args, "n", &handle)) {
            return nullptr;
        }

        // Does nothing; kept because the scripts call it.
        Py_RETURN_NONE;
    }

    auto wnd_mark_box_set_index(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int index;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &index)) {
            return nullptr;
        }

        ((widgets::mark_box *)window)->set_index(index);
        Py_RETURN_NONE;
    }

    auto wnd_mark_box_set_scale(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        float scale;

        if (!PyArg_ParseTuple(args, "O&f", window_converter, &window, &scale)) {
            return nullptr;
        }

        ((widgets::mark_box *)window)->set_scale(scale);
        Py_RETURN_NONE;
    }

    auto wnd_mark_box_set_diffuse_color(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        float fr, fg, fb, fa;

        if (!PyArg_ParseTuple(args, "O&ffff", window_converter, &window, &fr, &fg, &fb, &fa)) {
            return nullptr;
        }

        ((widgets::mark_box *)window)->set_diffuse_color(fr, fg, fb, fa);

        Py_RETURN_NONE;
    }

    auto wnd_image_load_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *file_name;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &file_name)) {
            return nullptr;
        }

        if (!((widgets::image_box *)window)->load_image(file_name)) {
            PyErr_Format(PyExc_IOError, "Cannot load image %s", file_name);
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto wnd_image_set_diffuse_color(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        float fr, fg, fb, fa;

        if (!PyArg_ParseTuple(args, "O&ffff", window_converter, &window, &fr, &fg, &fb, &fa)) {
            return nullptr;
        }

        ((widgets::image_box *)window)->set_diffuse_color(fr, fg, fb, fa);

        Py_RETURN_NONE;
    }

    auto wnd_image_get_width(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        return Py_BuildValue("i", ((widgets::image_box *)window)->width());
    }

    auto wnd_image_get_height(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        return Py_BuildValue("i", ((widgets::image_box *)window)->height());
    }

    auto wnd_image_set_scale(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        float fx, fy;

        if (!PyArg_ParseTuple(args, "O&ff", window_converter, &window, &fx, &fy)) {
            return nullptr;
        }

        ((widgets::expanded_image_box *)window)->set_scale(fx, fy);

        Py_RETURN_NONE;
    }
    auto wnd_image_set_origin(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        float fx, fy;

        if (!PyArg_ParseTuple(args, "O&ff", window_converter, &window, &fx, &fy)) {
            return nullptr;
        }

        ((widgets::expanded_image_box *)window)->set_origin(fx, fy);

        Py_RETURN_NONE;
    }
    auto wnd_image_set_rotation(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        float rotation;

        if (!PyArg_ParseTuple(args, "O&f", window_converter, &window, &rotation)) {
            return nullptr;
        }

        ((widgets::expanded_image_box *)window)->set_rotation(rotation);

        Py_RETURN_NONE;
    }
    auto wnd_image_set_rendering_rect(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        float left, top, right, bottom;

        if (!PyArg_ParseTuple(args, "O&ffff", window_converter, &window, &left, &top, &right, &bottom)) {
            return nullptr;
        }

        if (window->is_type(widgets::expanded_image_box::type()))
            ((widgets::expanded_image_box *)window)->set_rendering_rect(left, top, right, bottom);
        else if (window->is_type(widgets::ani_image_box::type()))
            ((widgets::ani_image_box *)window)->set_rendering_rect(left, top, right, bottom);

        Py_RETURN_NONE;
    }
    auto wnd_image_set_rendering_mode(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int mode;

        if (!PyArg_ParseTuple(args, "O&i", window_converter, &window, &mode)) {
            return nullptr;
        }

        if (window->is_type(widgets::expanded_image_box::type()))
            ((widgets::expanded_image_box *)window)->set_rendering_mode(mode);

        Py_RETURN_NONE;
    }
    auto wnd_image_set_delay(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        float delay;

        if (!PyArg_ParseTuple(args, "O&f", window_converter, &window, &delay)) {
            return nullptr;
        }

        ((widgets::ani_image_box *)window)->set_delay(delay);

        Py_RETURN_NONE;
    }
    auto wnd_image_append_image(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *file_name;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &file_name)) {
            return nullptr;
        }

        ((widgets::ani_image_box *)window)->append_image(file_name);

        Py_RETURN_NONE;
    }

    auto wnd_button_set_up_visual(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *file_name;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &file_name)) {
            return nullptr;
        }

        ((widgets::button *)window)->set_up_visual(file_name);

        Py_RETURN_NONE;
    }
    auto wnd_button_set_over_visual(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *file_name;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &file_name)) {
            return nullptr;
        }

        ((widgets::button *)window)->set_over_visual(file_name);

        Py_RETURN_NONE;
    }
    auto wnd_button_set_down_visual(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *file_name;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &file_name)) {
            return nullptr;
        }

        ((widgets::button *)window)->set_down_visual(file_name);

        Py_RETURN_NONE;
    }
    auto wnd_button_set_disable_visual(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        char *file_name;

        if (!PyArg_ParseTuple(args, "O&s", window_converter, &window, &file_name)) {
            return nullptr;
        }

        ((widgets::button *)window)->set_disable_visual(file_name);

        Py_RETURN_NONE;
    }
    auto wnd_button_get_up_visual_file_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        return Py_BuildValue("s", ((widgets::button *)window)->up_visual_file_name());
    }
    auto wnd_button_get_over_visual_file_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        return Py_BuildValue("s", ((widgets::button *)window)->over_visual_file_name());
    }
    auto wnd_button_get_down_visual_file_name(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        return Py_BuildValue("s", ((widgets::button *)window)->down_visual_file_name());
    }
    auto wnd_button_flash(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        ((widgets::button *)window)->flash();

        Py_RETURN_NONE;
    }
    auto wnd_button_enable(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        ((widgets::button *)window)->enable();

        Py_RETURN_NONE;
    }
    auto wnd_button_disable(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        ((widgets::button *)window)->disable();

        Py_RETURN_NONE;
    }
    auto wnd_button_down(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        ((widgets::button *)window)->down();

        Py_RETURN_NONE;
    }
    auto wnd_button_set_up(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        ((widgets::button *)window)->set_up();

        Py_RETURN_NONE;
    }
    auto wnd_button_set_restrict_movement_area(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        int ix, iy, iwidth, iheight;

        if (!PyArg_ParseTuple(args, "O&iiii", window_converter, &window, &ix, &iy, &iwidth, &iheight)) {
            return nullptr;
        }

        ((widgets::drag_button *)window)->set_restrict_movement_area(ix, iy, iwidth, iheight);

        Py_RETURN_NONE;
    }

    auto wnd_button_is_down(PyObject *self, PyObject *args) -> PyObject * {
        widgets::ui_window *window;
        if (!PyArg_ParseTuple(args, "O&", window_converter, &window)) {
            return nullptr;
        }

        return Py_BuildValue("i", ((widgets::button *)window)->is_pressed());
    }

    extern bool outline_box_enable;

    auto wnd_mgr_set_outline_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        outline_box_enable = flag;

        Py_RETURN_NONE;
    }

    void init_widgets_module() {
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

        PyObject *module = init_python_module("metin2.widgets", s_methods);

        PyModule_AddIntConstant(module, "SLOT_STYLE_NONE", widgets::SLOT_STYLE_NONE);
        PyModule_AddIntConstant(module, "SLOT_STYLE_PICK_UP", widgets::SLOT_STYLE_PICK_UP);
        PyModule_AddIntConstant(module, "SLOT_STYLE_SELECT", widgets::SLOT_STYLE_SELECT);

        PyModule_AddIntConstant(module, "TEXT_HORIZONTAL_ALIGN_LEFT", graphic_text_instance::HORIZONTAL_ALIGN_LEFT);
        PyModule_AddIntConstant(module, "TEXT_HORIZONTAL_ALIGN_RIGHT", graphic_text_instance::HORIZONTAL_ALIGN_RIGHT);
        PyModule_AddIntConstant(module, "TEXT_HORIZONTAL_ALIGN_CENTER", graphic_text_instance::HORIZONTAL_ALIGN_CENTER);
        PyModule_AddIntConstant(module, "TEXT_VERTICAL_ALIGN_TOP", graphic_text_instance::VERTICAL_ALIGN_TOP);
        PyModule_AddIntConstant(module, "TEXT_VERTICAL_ALIGN_BOTTOM", graphic_text_instance::VERTICAL_ALIGN_BOTTOM);
        PyModule_AddIntConstant(module, "TEXT_VERTICAL_ALIGN_CENTER", graphic_text_instance::VERTICAL_ALIGN_CENTER);

        PyModule_AddIntConstant(module, "HORIZONTAL_ALIGN_LEFT", widgets::ui_window::HORIZONTAL_ALIGN_LEFT);
        PyModule_AddIntConstant(module, "HORIZONTAL_ALIGN_CENTER", widgets::ui_window::HORIZONTAL_ALIGN_CENTER);
        PyModule_AddIntConstant(module, "HORIZONTAL_ALIGN_RIGHT", widgets::ui_window::HORIZONTAL_ALIGN_RIGHT);
        PyModule_AddIntConstant(module, "VERTICAL_ALIGN_TOP", widgets::ui_window::VERTICAL_ALIGN_TOP);
        PyModule_AddIntConstant(module, "VERTICAL_ALIGN_CENTER", widgets::ui_window::VERTICAL_ALIGN_CENTER);
        PyModule_AddIntConstant(module, "VERTICAL_ALIGN_BOTTOM", widgets::ui_window::VERTICAL_ALIGN_BOTTOM);

        PyModule_AddIntConstant(module, "RENDERING_MODE_MODULATE", graphic_expanded_image_instance::RENDERING_MODE_MODULATE);
    }
}
