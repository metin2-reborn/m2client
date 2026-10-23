#include <metin2/client/characters/python_character_manager.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <metin2/ui/python_graphic.h>
#include "python_text_tail.h"

namespace metin2::inline client::inline social {

    auto text_tail_clear(PyObject *self, PyObject *args) -> PyObject * {
        get_python_text_tail().clear();
        Py_RETURN_NONE;
    }

    auto text_tail_update_all_text_tail(PyObject *self, PyObject *args) -> PyObject * {
        get_python_text_tail().update_all_text_tail();
        Py_RETURN_NONE;
    }

    auto text_tail_update_showing_text_tail(PyObject *self, PyObject *args) -> PyObject * {
        get_python_text_tail().update_showing_text_tail();
        Py_RETURN_NONE;
    }

    auto text_tail_render(PyObject *self, PyObject *args) -> PyObject * {
        get_python_text_tail().render();
        Py_RETURN_NONE;
    }

    auto text_tail_get_position(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!PyArg_ParseTuple(args, "i", &virtual_id)) {
            return nullptr;
        }

        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        bool is_data = get_python_text_tail().text_tail_position(virtual_id, &x, &y, &z);
        if (!is_data) {
            python_character_manager &chr_mgr = get_python_character_manager();
            instance_base *inst_main = chr_mgr.main_instance_ptr();
            if (inst_main) {
                const math::vector3 &position = inst_main->graphic_thing_instance_ref().get_position();
                get_python_graphic().project_position(position.x, position.y, position.z, &x, &y);
            }
        }

        return Py_BuildValue("fff", x, y, z);
    }

    auto text_tail_is_chat(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!PyArg_ParseTuple(args, "i", &virtual_id)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_text_tail().is_chat_text_tail(virtual_id));
    }

    auto text_tail_register_info_tail(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        char *text;

        if (!PyArg_ParseTuple(args, "is", &virtual_id, &text)) {
            return nullptr;
        }

        get_python_text_tail().register_info_tail(virtual_id, text);

        Py_RETURN_NONE;
    }

    auto text_tail_show_character_text_tail(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!PyArg_ParseTuple(args, "i", &virtual_id)) {
            return nullptr;
        }

        get_python_text_tail().show_character_text_tail(virtual_id);
        Py_RETURN_NONE;
    }

    auto text_tail_show_item_text_tail(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!PyArg_ParseTuple(args, "i", &virtual_id)) {
            return nullptr;
        }

        get_python_text_tail().show_item_text_tail(virtual_id);
        Py_RETURN_NONE;
    }

    auto text_tail_arrange_text_tail(PyObject *self, PyObject *args) -> PyObject * {
        get_python_text_tail().arrange_text_tail();
        Py_RETURN_NONE;
    }

    auto text_tail_hide_all_text_tail(PyObject *self, PyObject *args) -> PyObject * {
        get_python_text_tail().hide_all_text_tail();
        Py_RETURN_NONE;
    }

    auto text_tail_show_all_text_tail(PyObject *self, PyObject *args) -> PyObject * {
        get_python_text_tail().show_all_text_tail();
        Py_RETURN_NONE;
    }

    auto text_tail_pick(PyObject *self, PyObject *args) -> PyObject * {
        int ix, iy;
        if (!PyArg_ParseTuple(args, "ii", &ix, &iy)) {
            return nullptr;
        }

        int value = get_python_text_tail().pick(ix, iy);
        return Py_BuildValue("i", value);
    }

    auto text_tail_select_item_name(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!PyArg_ParseTuple(args, "i", &virtual_id)) {
            return nullptr;
        }

        get_python_text_tail().select_item_name(virtual_id);
        Py_RETURN_NONE;
    }

    auto text_tail_enable_pk_title(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        get_python_text_tail().enable_pk_title(flag);
        Py_RETURN_NONE;
    }

    void init_social_text_tail_module() {
        static PyMethodDef s_methods[] =
            {
                {"clear", text_tail_clear, METH_VARARGS},

                {"update_all_text_tail", text_tail_update_all_text_tail, METH_VARARGS},
                {"update_showing_text_tail", text_tail_update_showing_text_tail, METH_VARARGS},
                {"render", text_tail_render, METH_VARARGS},

                {"show_character_text_tail", text_tail_show_character_text_tail, METH_VARARGS},
                {"show_item_text_tail", text_tail_show_item_text_tail, METH_VARARGS},

                {"get_position", text_tail_get_position, METH_VARARGS},
                {"is_chat", text_tail_is_chat, METH_VARARGS},

                {"arrange_text_tail", text_tail_arrange_text_tail, METH_VARARGS},
                {"hide_all_text_tail", text_tail_hide_all_text_tail, METH_VARARGS},
                {"show_all_text_tail", text_tail_show_all_text_tail, METH_VARARGS},

                {"pick", text_tail_pick, METH_VARARGS},
                {"select_item_name", text_tail_select_item_name, METH_VARARGS},

                {"enable_pk_title", text_tail_enable_pk_title, METH_VARARGS},

                {"register_info_tail", text_tail_register_info_tail, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        init_python_module("metin2.social.text_tail", s_methods);
    }
}
