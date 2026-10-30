#include <metin2/client/protocol/python_network_stream.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include "python_event_manager.h"

namespace metin2::inline client::inline quest {

    auto event_register_event_set(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        int event_index = get_python_event_manager().register_event_set(file_name);
        return Py_BuildValue("i", event_index);
    }

    auto event_clear_event_set(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        get_python_event_manager().clear_event_seti(index);
        Py_RETURN_NONE;
    }

    auto event_set_restricted_count(PyObject *self, PyObject *args) -> PyObject * {
        int index, count;
        if (!PyArg_ParseTuple(args, "ii", &index, &count)) {
            return nullptr;
        }

        get_python_event_manager().set_restricted_count(index, count);
        Py_RETURN_NONE;
    }

    auto event_get_event_set_local_y_position(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_event_manager().event_set_local_y_position(index));
    }

    auto event_add_event_set_local_y_position(PyObject *self, PyObject *args) -> PyObject * {
        int index, pos;
        if (!PyArg_ParseTuple(args, "ii", &index, &pos)) {
            return nullptr;
        }

        get_python_event_manager().add_event_set_local_y_position(index, pos);
        Py_RETURN_NONE;
    }

    auto event_insert_text_inline(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        char *text;
        int x_index;

        if (!PyArg_ParseTuple(args, "isi", &index, &text, &x_index)) {
            return nullptr;
        }

        get_python_event_manager().insert_text(index, text, x_index);
        Py_RETURN_NONE;
    }

    auto event_update_event_set(PyObject *self, PyObject *args) -> PyObject * {
        int index, ix, iy;
        if (!PyArg_ParseTuple(args, "iii", &index, &ix, &iy)) {
            return nullptr;
        }

        get_python_event_manager().update_event_set(index, ix, -iy);
        Py_RETURN_NONE;
    }

    auto event_render_event_set(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        get_python_event_manager().render_event_set(index);
        Py_RETURN_NONE;
    }

    auto event_set_event_set_width(PyObject *self, PyObject *args) -> PyObject * {
        int index, width_value;
        if (!PyArg_ParseTuple(args, "ii", &index, &width_value)) {
            return nullptr;
        }

        get_python_event_manager().set_event_set_width(index, width_value);
        Py_RETURN_NONE;
    }

    auto event_skip(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        get_python_event_manager().skip(index);
        Py_RETURN_NONE;
    }

    auto event_is_wait(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_event_manager().is_wait(index) == true ? 1 : 0);
    }

    auto event_end_event_process(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        get_python_event_manager().end_event_process(index);

        Py_RETURN_NONE;
    }

    auto event_set_event_handler(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        PyObject *event_handler;

        if (!PyArg_ParseTuple(args, "iO", &index, &event_handler)) {
            return nullptr;
        }

        get_python_event_manager().set_event_handler(index, event_handler);
        Py_RETURN_NONE;
    }

    auto event_select_answer(PyObject *self, PyObject *args) -> PyObject * {
        int index, answer;
        if (!PyArg_ParseTuple(args, "ii", &index, &answer)) {
            return nullptr;
        }

        get_python_event_manager().select_answer(index, answer);
        Py_RETURN_NONE;
    }

    auto event_get_line_count(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        int line_count = get_python_event_manager().line_count(index);
        return Py_BuildValue("i", line_count);
    }

    auto event_set_visible_start_line(PyObject *self, PyObject *args) -> PyObject * {
        int index, start_line;
        if (!PyArg_ParseTuple(args, "ii", &index, &start_line)) {
            return nullptr;
        }

        get_python_event_manager().set_visible_start_line(index, start_line);
        Py_RETURN_NONE;
    }

    auto event_get_visible_start_line(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_event_manager().visible_start_line(index));
    }

    auto event_finish(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        get_python_event_manager().finish(index);
        Py_RETURN_NONE;
    }

    auto event_set_visible_line_count(PyObject *self, PyObject *args) -> PyObject * {
        int index, line_count;
        if (!PyArg_ParseTuple(args, "ii", &index, &line_count)) {
            return nullptr;
        }

        get_python_event_manager().set_visible_line_count(index, line_count);
        Py_RETURN_NONE;
    }

    auto event_quest_button_click(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        get_python_network_stream().send_script_button_packet(index);

        Py_RETURN_NONE;
    }

    auto event_set_interface_window(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *py_handle;
        if (!PyArg_ParseTuple(args, "O", &py_handle)) {
            return nullptr;
        }

        python_event_manager &rpem = get_python_event_manager();
        rpem.set_interface_window(py_handle);
        Py_RETURN_NONE;
    }

    auto event_set_left_time_string(PyObject *self, PyObject *args) -> PyObject * {
        char *text;
        if (!PyArg_ParseTuple(args, "s", &text)) {
            return nullptr;
        }

        python_event_manager &rpem = get_python_event_manager();
        rpem.set_left_time_string(text);
        Py_RETURN_NONE;
    }

    auto event_destroy(PyObject *self, PyObject *args) -> PyObject * {
        python_event_manager &rpem = get_python_event_manager();
        rpem.destroy();
        Py_RETURN_NONE;
    }

    void init_quest_events_module() {
        static PyMethodDef s_methods[] =
            {
                {"register_event_set", event_register_event_set, METH_VARARGS},
                {"clear_event_set", event_clear_event_set, METH_VARARGS},

                {"set_restricted_count", event_set_restricted_count, METH_VARARGS},

                {"get_event_set_local_y_position", event_get_event_set_local_y_position, METH_VARARGS},
                {"add_event_set_local_y_position", event_add_event_set_local_y_position, METH_VARARGS},
                {"insert_text_inline", event_insert_text_inline, METH_VARARGS},

                {"update_event_set", event_update_event_set, METH_VARARGS},
                {"render_event_set", event_render_event_set, METH_VARARGS},
                {"set_event_set_width", event_set_event_set_width, METH_VARARGS},

                {"skip", event_skip, METH_VARARGS},
                {"finish", event_finish, METH_VARARGS},
                {"is_wait", event_is_wait, METH_VARARGS},
                {"end_event_process", event_end_event_process, METH_VARARGS},

                {"select_answer", event_select_answer, METH_VARARGS},
                {"get_line_count", event_get_line_count, METH_VARARGS},
                {"set_visible_start_line", event_set_visible_start_line, METH_VARARGS},
                {"get_visible_start_line", event_get_visible_start_line, METH_VARARGS},
                {"set_visible_line_count", event_set_visible_line_count, METH_VARARGS},

                {"set_event_handler", event_set_event_handler, METH_VARARGS},
                {"set_interface_window", event_set_interface_window, METH_VARARGS},
                {"set_left_time_string", event_set_left_time_string, METH_VARARGS},

                {"quest_button_click", event_quest_button_click, METH_VARARGS},
                {"destroy", event_destroy, METH_VARARGS},
                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module = init_python_module("metin2.quest.events", s_methods);

        PyModule_AddIntConstant(module, "BOX_VISIBLE_LINE_COUNT", python_event_manager::BOX_VISIBLE_LINE_COUNT);
        PyModule_AddIntConstant(module, "BUTTON_TYPE_NEXT", python_event_manager::BUTTON_TYPE_NEXT);
        PyModule_AddIntConstant(module, "BUTTON_TYPE_DONE", python_event_manager::BUTTON_TYPE_DONE);
        PyModule_AddIntConstant(module, "BUTTON_TYPE_CANCEL", python_event_manager::BUTTON_TYPE_CANCEL);
    }
}
