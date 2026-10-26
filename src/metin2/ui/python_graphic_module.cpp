#include "python_graphic.h"

#include <metin2/config/state_dir.h>
#include <metin2/core/time_utils.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/text_bar.h>
#include <metin2/log/log.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>

#include <cstdlib>
#include <ctime>
#include <string>

namespace metin2::inline ui {
    auto grp_create_text_bar(PyObject *self, PyObject *args) -> PyObject * {
        int width_value, height_value;
        if (!PyArg_ParseTuple(args, "ii", &width_value, &height_value)) {
            return nullptr;
        }

        auto *new_text_bar = new text_bar(12, false);
        if (!new_text_bar->create(width_value, height_value)) {
            delete new_text_bar;

            return Py_BuildValue("i", NULL);
        }

        return Py_BuildValue("n", (Py_ssize_t)new_text_bar);
    }

    auto grp_create_big_text_bar(PyObject *self, PyObject *args) -> PyObject * {
        int width_value, height_value, font_size;
        if (!PyArg_ParseTuple(args, "iii", &width_value, &height_value, &font_size)) {
            return nullptr;
        }

        auto *text_bar_value = new text_bar(font_size, true);
        if (!text_bar_value->create(width_value, height_value)) {
            delete text_bar_value;

            return Py_BuildValue("i", NULL);
        }

        return Py_BuildValue("n", (Py_ssize_t)text_bar_value);
    }

    auto grp_destroy_text_bar(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle;
        if (!PyArg_ParseTuple(args, "n", &handle)) {
            return nullptr;
        }

        auto *text_bar_value = (text_bar *)handle;

        delete text_bar_value;

        Py_RETURN_NONE;
    }

    auto grp_render_text_bar(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle;
        int x, y;

        if (!PyArg_ParseTuple(args, "nii", &handle, &x, &y)) {
            return nullptr;
        }

        auto *text_bar_value = (text_bar *)handle;
        if (text_bar_value) {
            text_bar_value->render(x, y);
        }

        Py_RETURN_NONE;
    }

    auto grp_text_bar_set_text_color(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle;
        int r, g, b;

        if (!PyArg_ParseTuple(args, "niii", &handle, &r, &g, &b)) {
            return nullptr;
        }

        auto *text_bar_value = (text_bar *)handle;
        if (text_bar_value) {
            text_bar_value->set_text_color(r, g, b);
        }

        Py_RETURN_NONE;
    }

    auto grp_text_bar_get_text_extent(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle;
        char *text;

        if (!PyArg_ParseTuple(args, "ns", &handle, &text)) {
            return nullptr;
        }

        int width_value = 0;
        int height_value = 0;

        auto *text_bar_value = (text_bar *)handle;
        if (text_bar_value) {
            text_bar_value->text_extent(text, &width_value, &height_value);
        }

        return Py_BuildValue("ii", width_value, height_value);
    }

    auto grp_text_bar_text_out(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle;
        int x, y;
        char *text;

        if (!PyArg_ParseTuple(args, "niis", &handle, &x, &y, &text)) {
            return nullptr;
        }

        auto *text_bar_value = (text_bar *)handle;
        if (text_bar_value) {
            text_bar_value->text_out(x, y, text);
        }

        Py_RETURN_NONE;
    }

    auto grp_clear_text_bar(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle;
        if (!PyArg_ParseTuple(args, "n", &handle)) {
            return nullptr;
        }

        auto *text_bar_value = (text_bar *)handle;
        if (text_bar_value) {
            text_bar_value->clear_bar();
        }

        Py_RETURN_NONE;
    }

    auto grp_set_text_bar_clip_rect(PyObject *self, PyObject *args) -> PyObject * {
        Py_ssize_t handle;
        int isx, isy;
        int iex, iey;

        if (!PyArg_ParseTuple(args, "niiii", &handle, &isx, &isy, &iex, &iey)) {
            return nullptr;
        }

        auto *text_bar_value = (text_bar *)handle;
        if (text_bar_value) {
            rectangle rect_value;
            rect_value.left = isx;
            rect_value.top = isy;
            rect_value.right = iex;
            rect_value.bottom = iey;

            text_bar_value->set_clip_rect(rect_value);
        }

        Py_RETURN_NONE;
    }

    auto grp_init_screen_effect(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().init_screen_effect();
        Py_RETURN_NONE;
    }

    auto grp_clear_depth_buffer(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().clear_depth_buffer();
        Py_RETURN_NONE;
    }

    auto grp_push_state(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().push_state();
        Py_RETURN_NONE;
    }

    auto grp_pop_state(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().pop_state();
        Py_RETURN_NONE;
    }

    auto grp_set_perspective(PyObject *self, PyObject *args) -> PyObject * {
        float fov, aspect;
        float z_near, z_far;

        if (!PyArg_ParseTuple(args, "ffff", &fov, &aspect, &z_near, &z_far)) {
            return nullptr;
        }

        get_python_graphic().set_perspective(fov, aspect, z_near, z_far);
        Py_RETURN_NONE;
    }

    auto grp_generate_color(PyObject *self, PyObject *args) -> PyObject * {
        float r, g, b, a;
        if (!PyArg_ParseTuple(args, "ffff", &r, &g, &b, &a)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_graphic().generate_color(r, g, b, a));
    }

    auto grp_set_color(PyObject *self, PyObject *args) -> PyObject * {
        uint32_t color;
        if (!PyArg_ParseTuple(args, "I", &color)) {
            return nullptr;
        }

        get_python_graphic().set_diffuse_color(color);
        Py_RETURN_NONE;
    }

    auto grp_set_cursor_position(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!PyArg_ParseTuple(args, "ii", &x, &y)) {
            return nullptr;
        }

        get_python_graphic().set_cursor_position(x, y);
        Py_RETURN_NONE;
    }

    auto grp_render_line(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        int width, height;

        if (!PyArg_ParseTuple(args, "iiii", &x, &y, &width, &height)) {
            return nullptr;
        }

        get_python_graphic().render_line2d((float)x, (float)y, (float)x + width, (float)y + height);
        Py_RETURN_NONE;
    }

    auto grp_render_round_box(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        int width, height;

        if (!PyArg_ParseTuple(args, "iiii", &x, &y, &width, &height)) {
            return nullptr;
        }

        auto f_x = float(x);
        auto f_y = float(y);
        auto f_width = float(width);
        auto f_height = float(height);

        get_python_graphic().render_line2d(f_x + 2.0f, f_y, f_x + 2.0f + (f_width - 3.0f), f_y);
        get_python_graphic().render_line2d(f_x + 2.0f, f_y + f_height, f_x + 2.0f + (f_width - 3.0f), f_y + f_height);
        get_python_graphic().render_line2d(f_x, f_y + 2.0f, f_x, f_y + 2.0f + f_height - 4.0f);
        get_python_graphic().render_line2d(f_x + f_width, f_y + 1.0f, f_x + f_width, f_y + 1.0f + f_height - 3.0f);
        get_python_graphic().render_line2d(f_x, f_y + 2.0f, f_x + 2.0f, f_y);
        get_python_graphic().render_line2d(f_x, f_y + f_height - 2.0f, f_x + 2.0f, f_y + f_height);
        get_python_graphic().render_line2d(f_x + f_width - 2.0f, f_y, f_x + f_width, f_y + 2.0f);
        get_python_graphic().render_line2d(f_x + f_width - 2.0f, f_y + f_height, f_x + f_width, f_y + f_height - 2.0f);

        Py_RETURN_NONE;
    }

    auto grp_render_box(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        int width, height;

        if (!PyArg_ParseTuple(args, "iiii", &x, &y, &width, &height)) {
            return nullptr;
        }

        get_python_graphic().render_box2d((float)x, (float)y, (float)x + width, (float)y + height);
        Py_RETURN_NONE;
    }

    auto grp_render_bar(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        int width, height;

        if (!PyArg_ParseTuple(args, "iiii", &x, &y, &width, &height)) {
            return nullptr;
        }

        get_python_graphic().render_bar2d((float)x, (float)y, (float)x + width, (float)y + height);
        Py_RETURN_NONE;
    }

    auto grp_render_gradation_bar(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        int width, height;
        uint32_t start_color, end_color;

        if (!PyArg_ParseTuple(args, "iiiiII", &x, &y, &width, &height, &start_color, &end_color)) {
            return nullptr;
        }

        get_python_graphic().render_gradation_bar2d((float)x, (float)y, (float)x + width, (float)y + height, start_color, end_color);
        Py_RETURN_NONE;
    }

    namespace {
        // "MMDD_hhmmss.jpg", appended to the directory.
        auto make_screen_shot_path(const std::string &directory) -> std::string {
            const struct tm tm_now = local_time(time(nullptr));

            char name[32];

            snprintf(name, sizeof(name), "%02d%02d_%02d%02d%02d.jpg",
                tm_now.tm_mon + 1, tm_now.tm_mday,
                tm_now.tm_hour, tm_now.tm_min,
                tm_now.tm_sec);

            return directory + name;
        }
    }

    auto grp_save_screen_shot_to_path(PyObject *self, PyObject *args) -> PyObject * {
        char *base_path;
        if (!PyArg_ParseTuple(args, "s", &base_path)) {
            return nullptr;
        }

        const auto path = make_screen_shot_path(base_path);
        const auto result = get_python_graphic().save_screen_shot(path.c_str());

        return Py_BuildValue("(is)", result, path.c_str());
    }

    auto grp_save_screen_shot(PyObject *self, PyObject *args) -> PyObject * {
        const auto directory = metin2::state::path("screenshots", true) + "/";
        const auto path = make_screen_shot_path(directory);
        const auto result = get_python_graphic().save_screen_shot(path.c_str());

        return Py_BuildValue("(is)", result, path.c_str());
    }

    auto grp_set_interface_render_state(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().set_interface_render_state();
        Py_RETURN_NONE;
    }

    auto grp_set_game_render_state(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().set_game_render_state();
        Py_RETURN_NONE;
    }

    auto grp_set_viewport(PyObject *self, PyObject *args) -> PyObject * {
        float fx, fy;
        float f_width, f_height;

        if (!PyArg_ParseTuple(args, "ffff", &fx, &fy, &f_width, &f_height)) {
            return nullptr;
        }

        uint32_t u_width, u_height;

        get_python_graphic().back_buffer_size(&u_width, &u_height);
        get_python_graphic().set_viewport(fx * u_width, fy * u_height, f_width * u_width, f_height * u_height);
        Py_RETURN_NONE;
    }

    auto grp_restore_viewport(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().restore_viewport();
        Py_RETURN_NONE;
    }

    auto grp_set_omni_light(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().set_omni_light();
        Py_RETURN_NONE;
    }

    void init_graphics_module() {
        static PyMethodDef methods[] = {
            {"init_screen_effect", grp_init_screen_effect, METH_VARARGS},
            {"clear_depth_buffer", grp_clear_depth_buffer, METH_VARARGS},
            {"generate_color", grp_generate_color, METH_VARARGS},
            {"pop_state", grp_pop_state, METH_VARARGS},
            {"push_state", grp_push_state, METH_VARARGS},
            {"set_perspective", grp_set_perspective, METH_VARARGS},
            {"set_color", grp_set_color, METH_VARARGS},
            {"set_cursor_position", grp_set_cursor_position, METH_VARARGS},
            {"render_line", grp_render_line, METH_VARARGS},
            {"render_box", grp_render_box, METH_VARARGS},
            {"render_round_box", grp_render_round_box, METH_VARARGS},
            {"render_bar", grp_render_bar, METH_VARARGS},
            {"render_gradation_bar", grp_render_gradation_bar, METH_VARARGS},
            {"save_screen_shot", grp_save_screen_shot, METH_VARARGS},
            {"save_screen_shot_to_path", grp_save_screen_shot_to_path, METH_VARARGS},
            {"set_interface_render_state", grp_set_interface_render_state, METH_VARARGS},
            {"set_game_render_state", grp_set_game_render_state, METH_VARARGS},
            {"set_viewport", grp_set_viewport, METH_VARARGS},
            {"restore_viewport", grp_restore_viewport, METH_VARARGS},
            {"set_omni_light", grp_set_omni_light, METH_VARARGS},

            {"create_text_bar", grp_create_text_bar, METH_VARARGS},
            {"create_big_text_bar", grp_create_big_text_bar, METH_VARARGS},
            {"destroy_text_bar", grp_destroy_text_bar, METH_VARARGS},
            {"render_text_bar", grp_render_text_bar, METH_VARARGS},
            {"text_bar_text_out", grp_text_bar_text_out, METH_VARARGS},
            {"text_bar_set_text_color", grp_text_bar_set_text_color, METH_VARARGS},
            {"text_bar_get_text_extent", grp_text_bar_get_text_extent, METH_VARARGS},
            {"clear_text_bar", grp_clear_text_bar, METH_VARARGS},
            {"set_text_bar_clip_rect", grp_set_text_bar_clip_rect, METH_VARARGS},

            {nullptr, nullptr, 0, nullptr},
        };

        init_python_module("metin2.graphics", methods);
    }
}
