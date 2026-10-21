#include "python_graphic.h"

#include <metin2/config/state_dir.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/text_bar.h>
#include <metin2/log/log.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_utils.h>

#include <cstdlib>
#include <ctime>
#include <string>
#include <sys/stat.h>

namespace metin2::inline ui {
    auto grp_create_text_bar(PyObject *self, PyObject *args) -> PyObject * {
        int width_value, height_value;
        if (!py_tuple_get_integer(args, 0, &width_value) ||
            !py_tuple_get_integer(args, 1, &height_value)) {
            return py_build_exception();
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
        if (!py_tuple_get_integer(args, 0, &width_value) ||
            !py_tuple_get_integer(args, 1, &height_value) ||
            !py_tuple_get_integer(args, 2, &font_size)) {
            return py_build_exception();
        }

        auto *text_bar_value = new text_bar(font_size, true);
        if (!text_bar_value->create(width_value, height_value)) {
            delete text_bar_value;

            return Py_BuildValue("i", NULL);
        }

        return Py_BuildValue("n", (Py_ssize_t)text_bar_value);
    }

    auto grp_destroy_text_bar(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle)) {
            return py_build_exception();
        }

        auto *text_bar_value = (text_bar *)handle;

        delete text_bar_value;

        return py_build_none();
    }

    auto grp_render_text_bar(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle)) {
            return py_build_exception();
        }

        int x, y;
        if (!py_tuple_get_integer(args, 1, &x) || !py_tuple_get_integer(args, 2, &y)) {
            return py_build_exception();
        }

        auto *text_bar_value = (text_bar *)handle;
        if (text_bar_value) {
            text_bar_value->render(x, y);
        }

        return py_build_none();
    }

    auto grp_text_bar_set_text_color(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle)) {
            return py_build_exception();
        }

        int r, g, b;
        if (!py_tuple_get_integer(args, 1, &r) || !py_tuple_get_integer(args, 2, &g) || !py_tuple_get_integer(args, 3, &b)) {
            return py_build_exception();
        }

        auto *text_bar_value = (text_bar *)handle;
        if (text_bar_value) {
            text_bar_value->set_text_color(r, g, b);
        }

        return py_build_none();
    }

    auto grp_text_bar_get_text_extent(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle)) {
            return py_build_exception();
        }

        char *text;
        if (!py_tuple_get_string(args, 1, &text)) {
            return py_build_exception();
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
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle)) {
            return py_build_exception();
        }

        int x, y;
        if (!py_tuple_get_integer(args, 1, &x) || !py_tuple_get_integer(args, 2, &y)) {
            return py_build_exception();
        }

        char *text;
        if (!py_tuple_get_string(args, 3, &text)) {
            return py_build_exception();
        }

        auto *text_bar_value = (text_bar *)handle;
        if (text_bar_value) {
            text_bar_value->text_out(x, y, text);
        }

        return py_build_none();
    }

    auto grp_clear_text_bar(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle)) {
            return py_build_exception();
        }

        auto *text_bar_value = (text_bar *)handle;
        if (text_bar_value) {
            text_bar_value->clear_bar();
        }

        return py_build_none();
    }

    auto grp_set_text_bar_clip_rect(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle)) {
            return py_build_exception();
        }

        int isx, isy;
        if (!py_tuple_get_integer(args, 1, &isx) || !py_tuple_get_integer(args, 2, &isy)) {
            return py_build_exception();
        }

        int iex, iey;
        if (!py_tuple_get_integer(args, 3, &iex) || !py_tuple_get_integer(args, 4, &iey)) {
            return py_build_exception();
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

        return py_build_none();
    }

    auto grp_init_screen_effect(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().init_screen_effect();
        return py_build_none();
    }

    auto grp_clear_depth_buffer(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().clear_depth_buffer();
        return py_build_none();
    }

    auto grp_push_state(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().push_state();
        return py_build_none();
    }

    auto grp_pop_state(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().pop_state();
        return py_build_none();
    }

    auto grp_set_perspective(PyObject *self, PyObject *args) -> PyObject * {
        float fov;
        if (!py_tuple_get_float(args, 0, &fov)) {
            return py_build_exception();
        }

        float aspect;
        if (!py_tuple_get_float(args, 1, &aspect)) {
            return py_build_exception();
        }

        float z_near, z_far;
        if (!py_tuple_get_float(args, 2, &z_near) || !py_tuple_get_float(args, 3, &z_far)) {
            return py_build_exception();
        }

        get_python_graphic().set_perspective(fov, aspect, z_near, z_far);
        return py_build_none();
    }

    auto grp_generate_color(PyObject *self, PyObject *args) -> PyObject * {
        float r, g, b, a;
        if (!py_tuple_get_float(args, 0, &r) ||
            !py_tuple_get_float(args, 1, &g) ||
            !py_tuple_get_float(args, 2, &b) ||
            !py_tuple_get_float(args, 3, &a)) {
            return py_build_exception();
        }

        return Py_BuildValue("i", get_python_graphic().generate_color(r, g, b, a));
    }

    auto grp_set_color(PyObject *self, PyObject *args) -> PyObject * {
        int color;
        if (!py_tuple_get_integer(args, 0, &color)) {
            return py_build_exception();
        }

        get_python_graphic().set_diffuse_color(uint32_t(color));
        return py_build_none();
    }

    auto grp_set_cursor_position(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!py_tuple_get_integer(args, 0, &x) || !py_tuple_get_integer(args, 1, &y)) {
            return py_build_exception();
        }

        get_python_graphic().set_cursor_position(x, y);
        return py_build_none();
    }

    auto grp_render_line(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!py_tuple_get_integer(args, 0, &x) || !py_tuple_get_integer(args, 1, &y)) {
            return py_build_exception();
        }

        int width, height;
        if (!py_tuple_get_integer(args, 2, &width) || !py_tuple_get_integer(args, 3, &height)) {
            return py_build_exception();
        }

        get_python_graphic().render_line2d((float)x, (float)y, (float)x + width, (float)y + height);
        return py_build_none();
    }

    auto grp_render_round_box(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!py_tuple_get_integer(args, 0, &x) || !py_tuple_get_integer(args, 1, &y)) {
            return py_build_exception();
        }

        int width, height;
        if (!py_tuple_get_integer(args, 2, &width) || !py_tuple_get_integer(args, 3, &height)) {
            return py_build_exception();
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

        return py_build_none();
    }

    auto grp_render_box(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!py_tuple_get_integer(args, 0, &x) || !py_tuple_get_integer(args, 1, &y)) {
            return py_build_exception();
        }

        int width, height;
        if (!py_tuple_get_integer(args, 2, &width) || !py_tuple_get_integer(args, 3, &height)) {
            return py_build_exception();
        }

        get_python_graphic().render_box2d((float)x, (float)y, (float)x + width, (float)y + height);
        return py_build_none();
    }

    auto grp_render_bar(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!py_tuple_get_integer(args, 0, &x) || !py_tuple_get_integer(args, 1, &y)) {
            return py_build_exception();
        }

        int width, height;
        if (!py_tuple_get_integer(args, 2, &width) || !py_tuple_get_integer(args, 3, &height)) {
            return py_build_exception();
        }

        get_python_graphic().render_bar2d((float)x, (float)y, (float)x + width, (float)y + height);
        return py_build_none();
    }

    auto grp_render_gradation_bar(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!py_tuple_get_integer(args, 0, &x) || !py_tuple_get_integer(args, 1, &y)) {
            return py_bad_argument();
        }

        int width, height;
        if (!py_tuple_get_integer(args, 2, &width) || !py_tuple_get_integer(args, 3, &height)) {
            return py_bad_argument();
        }

        int start_color, end_color;
        if (!py_tuple_get_integer(args, 4, &start_color) || !py_tuple_get_integer(args, 5, &end_color)) {
            return py_bad_argument();
        }

        get_python_graphic().render_gradation_bar2d((float)x, (float)y, (float)x + width, (float)y + height, start_color, end_color);
        return py_build_none();
    }

    namespace {
        // "MMDD_hhmmss.jpg", appended to the directory.
        auto make_screen_shot_path(const std::string &directory) -> std::string {
            const time_t ct = time(nullptr);
            struct tm tm_now;
            localtime_r(&ct, &tm_now);

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
        if (!py_tuple_get_string(args, 0, &base_path)) {
            return py_build_exception();
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
        return py_build_none();
    }

    auto grp_set_game_render_state(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().set_game_render_state();
        return py_build_none();
    }

    auto grp_set_viewport(PyObject *self, PyObject *args) -> PyObject * {
        float fx, fy;
        if (!py_tuple_get_float(args, 0, &fx) || !py_tuple_get_float(args, 1, &fy)) {
            return py_build_exception();
        }

        float f_width, f_height;
        if (!py_tuple_get_float(args, 2, &f_width) || !py_tuple_get_float(args, 3, &f_height)) {
            return py_build_exception();
        }

        uint32_t u_width, u_height;

        get_python_graphic().back_buffer_size(&u_width, &u_height);
        get_python_graphic().set_viewport(fx * u_width, fy * u_height, f_width * u_width, f_height * u_height);
        return py_build_none();
    }

    auto grp_restore_viewport(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().restore_viewport();
        return py_build_none();
    }

    auto grp_set_omni_light(PyObject *self, PyObject *args) -> PyObject * {
        get_python_graphic().set_omni_light();
        return py_build_none();
    }

    void initgrp() {
        static PyMethodDef methods[] = {
            {"InitScreenEffect", grp_init_screen_effect, METH_VARARGS},
            {"ClearDepthBuffer", grp_clear_depth_buffer, METH_VARARGS},
            {"GenerateColor", grp_generate_color, METH_VARARGS},
            {"PopState", grp_pop_state, METH_VARARGS},
            {"PushState", grp_push_state, METH_VARARGS},
            {"SetPerspective", grp_set_perspective, METH_VARARGS},
            {"SetColor", grp_set_color, METH_VARARGS},
            {"SetCursorPosition", grp_set_cursor_position, METH_VARARGS},
            {"RenderLine", grp_render_line, METH_VARARGS},
            {"RenderBox", grp_render_box, METH_VARARGS},
            {"RenderRoundBox", grp_render_round_box, METH_VARARGS},
            {"RenderBar", grp_render_bar, METH_VARARGS},
            {"RenderGradationBar", grp_render_gradation_bar, METH_VARARGS},
            {"SaveScreenShot", grp_save_screen_shot, METH_VARARGS},
            {"SaveScreenShotToPath", grp_save_screen_shot_to_path, METH_VARARGS},
            {"SetInterfaceRenderState", grp_set_interface_render_state, METH_VARARGS},
            {"SetGameRenderState", grp_set_game_render_state, METH_VARARGS},
            {"SetViewport", grp_set_viewport, METH_VARARGS},
            {"RestoreViewport", grp_restore_viewport, METH_VARARGS},
            {"SetOmniLight", grp_set_omni_light, METH_VARARGS},

            {"CreateTextBar", grp_create_text_bar, METH_VARARGS},
            {"CreateBigTextBar", grp_create_big_text_bar, METH_VARARGS},
            {"DestroyTextBar", grp_destroy_text_bar, METH_VARARGS},
            {"RenderTextBar", grp_render_text_bar, METH_VARARGS},
            {"TextBarTextOut", grp_text_bar_text_out, METH_VARARGS},
            {"TextBarSetTextColor", grp_text_bar_set_text_color, METH_VARARGS},
            {"TextBarGetTextExtent", grp_text_bar_get_text_extent, METH_VARARGS},
            {"ClearTextBar", grp_clear_text_bar, METH_VARARGS},
            {"SetTextBarClipRect", grp_set_text_bar_clip_rect, METH_VARARGS},

            {nullptr, nullptr, 0, nullptr},
        };

        Py_InitModule("grp", methods);
    }
}
