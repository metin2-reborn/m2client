#include <cstdint>
#include <metin2/core/utf8.h>

#include <metin2/graphics/grp_text.h>
#include <metin2/graphics/grp_text_instance.h>
#include <metin2/graphics/resource.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <string>
#include <string_view>
#include <vector>
#include "python_modules.h"

namespace metin2::inline ui {

    auto py_tuple_get_text_instance(PyObject *args, int pos, graphic_text_instance **text_instance) -> bool {
        intptr_t handle;
        if (!py_tuple_get_integer(args, pos, &handle))
            return false;

        if (!handle)
            return false;

        *text_instance = (graphic_text_instance *)handle;

        return true;
    }

    auto grp_text_generate(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance = graphic_text_instance::new_value();
        return Py_BuildValue("n", (Py_ssize_t)text_instance);
    }

    auto grp_text_set_position(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        if (!py_tuple_get_text_instance(args, 0, &text_instance))
            return py_build_exception();

        int ix;
        if (!py_tuple_get_integer(args, 1, &ix))
            return py_build_exception();
        int iy;
        if (!py_tuple_get_integer(args, 2, &iy))
            return py_build_exception();

        text_instance->set_position((float)ix, (float)iy);
        return py_build_none();
    }

    auto grp_text_set_text(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        if (!py_tuple_get_text_instance(args, 0, &text_instance))
            return py_build_exception();

        char *text;
        if (!py_tuple_get_string(args, 1, &text))
            return py_build_exception();

        text_instance->set_value(text);
        return py_build_none();
    }

    auto grp_text_outline(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        if (!py_tuple_get_text_instance(args, 0, &text_instance))
            return py_build_exception();

        bool value;
        if (!py_tuple_get_boolean(args, 1, &value))
            return py_build_exception();

        text_instance->set_outline(value);
        return py_build_none();
    }

    auto grp_text_set_font_name(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        if (!py_tuple_get_text_instance(args, 0, &text_instance))
            return py_build_exception();

        char *sz_font_name;
        if (!py_tuple_get_string(args, 1, &sz_font_name))
            return py_build_exception();

        std::string st_font_name = sz_font_name;
        st_font_name += ".fnt";

        resource *resource = get_resource_manager().resource_pointer(st_font_name.c_str());
        text_instance->set_text_pointer(static_cast<graphic_text *>(resource));
        return py_build_none();
    }

    auto grp_text_set_font_color(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        if (!py_tuple_get_text_instance(args, 0, &text_instance))
            return py_build_exception();

        if (2 == PyTuple_Size(args)) {
            int color;
            if (!py_tuple_get_integer(args, 1, &color))
                return py_build_exception();
            text_instance->set_color(uint32_t(color));
        } else if (4 == PyTuple_Size(args)) {
            float red;
            if (!py_tuple_get_float(args, 1, &red))
                return py_build_exception();
            float green;
            if (!py_tuple_get_float(args, 2, &green))
                return py_build_exception();
            float blue;
            if (!py_tuple_get_float(args, 3, &blue))
                return py_build_exception();
            text_instance->set_color(red, green, blue);
        } else {
            return py_build_exception();
        }

        return py_build_none();
    }

    auto grp_set_horizontal_align(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        if (!py_tuple_get_text_instance(args, 0, &text_instance))
            return py_build_exception();

        int align;
        if (!py_tuple_get_integer(args, 1, &align))
            return py_build_exception();

        text_instance->set_horizonal_align(align);
        return py_build_none();
    }

    namespace {
        // Splits UTF-8 text into lines of at most lineColumns columns; '|' forces a break.
        auto split_text_lines(std::string_view text, int line_columns) -> std::vector<std::string> {
            std::vector<std::string> lines;
            std::string line;
            int columns = 0;

            for (std::size_t pos = 0; pos < text.size();) {
                if (text[pos] == '|') {
                    ++pos;
                    // A '|' right after an automatic line break does not add an empty line.
                    if (columns > 0)
                        lines.push_back(std::move(line));
                    line.clear();
                    columns = 0;
                    continue;
                }

                std::size_t start = pos;
                columns += utf8::columns(utf8::next(text, pos));
                line.append(text.substr(start, pos - start));

                if (columns >= line_columns) {
                    lines.push_back(std::move(line));
                    line.clear();
                    columns = 0;
                }
            }

            if (columns > 0)
                lines.push_back(std::move(line));
            return lines;
        }
    }

    auto grp_get_spliting_text_line_count(PyObject *self, PyObject *args) -> PyObject * {
        char *text;
        if (!py_tuple_get_string(args, 0, &text))
            return py_build_exception();

        int line_limitation;
        if (!py_tuple_get_integer(args, 1, &line_limitation))
            return py_build_exception();

        return Py_BuildValue("i", static_cast<int>(split_text_lines(text, line_limitation).size()));
    }

    auto grp_get_spliting_text_line(PyObject *self, PyObject *args) -> PyObject * {
        char *text;
        if (!py_tuple_get_string(args, 0, &text))
            return py_build_exception();

        int line_limitation;
        if (!py_tuple_get_integer(args, 1, &line_limitation))
            return py_build_exception();

        int getting_line;
        if (!py_tuple_get_integer(args, 2, &getting_line))
            return py_build_exception();

        std::vector<std::string> lines = split_text_lines(text, line_limitation);
        const char *line = getting_line >= 0 && getting_line < static_cast<int>(lines.size()) ? lines[getting_line].c_str() : "";
        return Py_BuildValue("s", line);
    }

    void init_graphics_text_module() {
        static PyMethodDef s_methods[] =
            {
                {"Generate", grp_text_generate, METH_VARARGS},
                {"SetFontName", grp_text_set_font_name, METH_VARARGS},
                {"SetFontColor", grp_text_set_font_color, METH_VARARGS},
                {"SetOutline", grp_text_outline, METH_VARARGS},
                {"SetText", grp_text_set_text, METH_VARARGS},
                {"SetPosition", grp_text_set_position, METH_VARARGS},
                {"SetHorizontalAlign", grp_set_horizontal_align, METH_VARARGS},
                {"GetSplitingTextLineCount", grp_get_spliting_text_line_count, METH_VARARGS},
                {"GetSplitingTextLine", grp_get_spliting_text_line, METH_VARARGS},
                {nullptr, nullptr, 0, nullptr},
        };

        init_python_module("metin2.graphics.text", s_methods);
    }
}
