#include <metin2/client/protocol/python_network_stream.h>
#include <metin2/script/python_utils.h>
#include "python_event_manager.h"

namespace metin2::inline client::inline quest {

    auto event_register_event_set(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!py_tuple_get_string(args, 0, &file_name))
            return py_build_exception();

        int event_index = get_python_event_manager().register_event_set(file_name);
        return Py_BuildValue("i", event_index);
    }

    auto event_clear_event_set(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        get_python_event_manager().clear_event_seti(index);
        return py_build_none();
    }

    auto event_set_restricted_count(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        int count;
        if (!py_tuple_get_integer(args, 1, &count))
            return py_build_exception();

        get_python_event_manager().set_restricted_count(index, count);
        return py_build_none();
    }

    auto event_get_event_set_local_y_position(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        return Py_BuildValue("i", get_python_event_manager().event_set_local_y_position(index));
    }

    auto event_add_event_set_local_y_position(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();
        int pos;
        if (!py_tuple_get_integer(args, 1, &pos))
            return py_build_exception();

        get_python_event_manager().add_event_set_local_y_position(index, pos);
        return py_build_none();
    }

    auto event_insert_text_inline(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();
        char *text;
        if (!py_tuple_get_string(args, 1, &text))
            return py_build_exception();
        int x_index;
        if (!py_tuple_get_integer(args, 2, &x_index))
            return py_build_exception();

        get_python_event_manager().insert_text(index, text, x_index);
        return py_build_none();
    }

    auto event_update_event_set(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        int ix;
        if (!py_tuple_get_integer(args, 1, &ix))
            return py_build_exception();

        int iy;
        if (!py_tuple_get_integer(args, 2, &iy))
            return py_build_exception();

        get_python_event_manager().update_event_set(index, ix, -iy);
        return py_build_none();
    }

    auto event_render_event_set(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        get_python_event_manager().render_event_set(index);
        return py_build_none();
    }

    auto event_set_event_set_width(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();
        int width_value;
        if (!py_tuple_get_integer(args, 1, &width_value))
            return py_build_exception();

        get_python_event_manager().set_event_set_width(index, width_value);
        return py_build_none();
    }

    auto event_skip(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        get_python_event_manager().skip(index);
        return py_build_none();
    }

    auto event_is_wait(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        return Py_BuildValue("i", get_python_event_manager().is_wait(index) == true ? 1 : 0);
    }

    auto event_end_event_process(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        get_python_event_manager().end_event_process(index);

        return py_build_none();
    }

    auto event_set_event_handler(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        PyObject *event_handler;
        if (!py_tuple_get_object(args, 1, &event_handler))
            return py_build_exception();

        get_python_event_manager().set_event_handler(index, event_handler);
        return py_build_none();
    }

    auto event_select_answer(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        int answer;
        if (!py_tuple_get_integer(args, 1, &answer))
            return py_build_exception();

        get_python_event_manager().select_answer(index, answer);
        return py_build_none();
    }

    auto event_get_line_count(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        int line_count = get_python_event_manager().line_count(index);
        return Py_BuildValue("i", line_count);
    }

    auto event_set_visible_start_line(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        int start_line;
        if (!py_tuple_get_integer(args, 1, &start_line))
            return py_build_exception();

        get_python_event_manager().set_visible_start_line(index, start_line);
        return py_build_none();
    }

    auto event_get_visible_start_line(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        return Py_BuildValue("i", get_python_event_manager().visible_start_line(index));
    }

    auto event_quest_button_click(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        get_python_network_stream().send_script_button_packet(index);

        return py_build_none();
    }

    auto event_set_interface_window(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *py_handle;
        if (!py_tuple_get_object(args, 0, &py_handle))
            return py_bad_argument();

        python_event_manager &rpem = get_python_event_manager();
        rpem.set_interface_window(py_handle);
        return py_build_none();
    }

    auto event_set_left_time_string(PyObject *self, PyObject *args) -> PyObject * {
        char *text;
        if (!py_tuple_get_string(args, 0, &text))
            return py_build_exception();

        python_event_manager &rpem = get_python_event_manager();
        rpem.set_left_time_string(text);
        return py_build_none();
    }

    auto event_destroy(PyObject *self, PyObject *args) -> PyObject * {
        python_event_manager &rpem = get_python_event_manager();
        rpem.destroy();
        return py_build_none();
    }

    void init_event() {
        static PyMethodDef s_methods[] =
            {
                {"RegisterEventSet", event_register_event_set, METH_VARARGS},
                {"ClearEventSet", event_clear_event_set, METH_VARARGS},

                {"SetRestrictedCount", event_set_restricted_count, METH_VARARGS},

                {"GetEventSetLocalYPosition", event_get_event_set_local_y_position, METH_VARARGS},
                {"AddEventSetLocalYPosition", event_add_event_set_local_y_position, METH_VARARGS},
                {"InsertTextInline", event_insert_text_inline, METH_VARARGS},

                {"UpdateEventSet", event_update_event_set, METH_VARARGS},
                {"RenderEventSet", event_render_event_set, METH_VARARGS},
                {"SetEventSetWidth", event_set_event_set_width, METH_VARARGS},

                {"Skip", event_skip, METH_VARARGS},
                {"IsWait", event_is_wait, METH_VARARGS},
                {"EndEventProcess", event_end_event_process, METH_VARARGS},

                {"SelectAnswer", event_select_answer, METH_VARARGS},
                {"GetLineCount", event_get_line_count, METH_VARARGS},
                {"SetVisibleStartLine", event_set_visible_start_line, METH_VARARGS},
                {"GetVisibleStartLine", event_get_visible_start_line, METH_VARARGS},

                {"SetEventHandler", event_set_event_handler, METH_VARARGS},
                {"SetInterfaceWindow", event_set_interface_window, METH_VARARGS},
                {"SetLeftTimeString", event_set_left_time_string, METH_VARARGS},

                {"QuestButtonClick", event_quest_button_click, METH_VARARGS},
                {"Destroy", event_destroy, METH_VARARGS},
                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = Py_InitModule("event", s_methods);

        PyModule_AddIntConstant(module_value, "BOX_VISIBLE_LINE_COUNT", python_event_manager::BOX_VISIBLE_LINE_COUNT);
        PyModule_AddIntConstant(module_value, "BUTTON_TYPE_NEXT", python_event_manager::BUTTON_TYPE_NEXT);
        PyModule_AddIntConstant(module_value, "BUTTON_TYPE_DONE", python_event_manager::BUTTON_TYPE_DONE);
        PyModule_AddIntConstant(module_value, "BUTTON_TYPE_CANCEL", python_event_manager::BUTTON_TYPE_CANCEL);
    }
}
