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

    constexpr auto text_instance_converter = &py_handle_converter<graphic_text_instance>;

    auto grp_text_generate(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance = graphic_text_instance::new_value();
        return Py_BuildValue("n", (Py_ssize_t)text_instance);
    }

    auto grp_text_set_position(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        int ix, iy;

        if (!PyArg_ParseTuple(args, "O&ii", text_instance_converter, &text_instance, &ix, &iy)) {
            return nullptr;
        }

        text_instance->set_position((float)ix, (float)iy);
        Py_RETURN_NONE;
    }

    auto grp_text_set_text(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        char *text;

        if (!PyArg_ParseTuple(args, "O&s", text_instance_converter, &text_instance, &text)) {
            return nullptr;
        }

        text_instance->set_value(text);
        Py_RETURN_NONE;
    }

    auto grp_text_outline(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        int value;

        if (!PyArg_ParseTuple(args, "O&i", text_instance_converter, &text_instance, &value)) {
            return nullptr;
        }

        text_instance->set_outline(value);
        Py_RETURN_NONE;
    }

    auto grp_text_set_font_name(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        char *sz_font_name;

        if (!PyArg_ParseTuple(args, "O&s", text_instance_converter, &text_instance, &sz_font_name)) {
            return nullptr;
        }

        std::string st_font_name = sz_font_name;
        st_font_name += ".fnt";

        resource *resource = get_resource_manager().resource_pointer(st_font_name.c_str());
        text_instance->set_text_pointer(static_cast<graphic_text *>(resource));
        Py_RETURN_NONE;
    }

    auto grp_text_set_font_color(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        uint32_t color;
        float red, green, blue;

        if (2 == PyTuple_Size(args)) {
            if (!PyArg_ParseTuple(args, "O&I", text_instance_converter, &text_instance, &color)) {
                return nullptr;
            }

            text_instance->set_color(color);
        } else if (4 == PyTuple_Size(args)) {
            if (!PyArg_ParseTuple(args, "O&fff", text_instance_converter, &text_instance, &red, &green, &blue)) {
                return nullptr;
            }

            text_instance->set_color(red, green, blue);
        } else {
            PyErr_Format(PyExc_TypeError, "wrong number of arguments (%zd given)", PyTuple_Size(args));
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto grp_set_horizontal_align(PyObject *self, PyObject *args) -> PyObject * {
        graphic_text_instance *text_instance;
        int align;

        if (!PyArg_ParseTuple(args, "O&i", text_instance_converter, &text_instance, &align)) {
            return nullptr;
        }

        text_instance->set_horizonal_align(align);
        Py_RETURN_NONE;
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
        int line_limitation;

        if (!PyArg_ParseTuple(args, "si", &text, &line_limitation)) {
            return nullptr;
        }

        return Py_BuildValue("i", static_cast<int>(split_text_lines(text, line_limitation).size()));
    }

    auto grp_get_spliting_text_line(PyObject *self, PyObject *args) -> PyObject * {
        char *text;
        int line_limitation, getting_line;

        if (!PyArg_ParseTuple(args, "sii", &text, &line_limitation, &getting_line)) {
            return nullptr;
        }

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
