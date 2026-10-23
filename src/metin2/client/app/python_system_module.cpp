#include "python_system.h"

#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline app {
    auto system_get_width(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().width());
    }

    auto system_get_height(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().height());
    }

    auto system_set_interface_handler(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handler;
        if (!PyArg_ParseTuple(args, "O", &handler)) {
            return nullptr;
        }

        get_python_system().set_interface_handler(handler);
        Py_RETURN_NONE;
    }

    auto system_destroy_interface_handler(PyObject *self, PyObject *args) -> PyObject * {
        get_python_system().destroy_interface_handler();
        Py_RETURN_NONE;
    }

    auto system_get_music_volume(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("f", get_python_system().music_volume());
    }

    auto system_get_sound_volume(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().sound_volume());
    }

    auto system_set_music_volume(PyObject *self, PyObject *args) -> PyObject * {
        float volume;
        if (!PyArg_ParseTuple(args, "f", &volume)) {
            return nullptr;
        }

        get_python_system().set_music_volume(volume);
        Py_RETURN_NONE;
    }

    auto system_set_sound_volumef(PyObject *self, PyObject *args) -> PyObject * {
        float volume;
        if (!PyArg_ParseTuple(args, "f", &volume)) {
            return nullptr;
        }

        get_python_system().set_sound_volumef(volume);
        Py_RETURN_NONE;
    }

    auto system_is_software_cursor(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().is_software_cursor());
    }

    auto system_set_view_chat_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        get_python_system().set_view_chat_flag(flag);
        Py_RETURN_NONE;
    }

    auto system_is_view_chat(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().is_view_chat());
    }

    auto system_set_always_show_name_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        get_python_system().set_always_show_name_flag(flag);
        Py_RETURN_NONE;
    }

    auto system_set_show_damage_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        get_python_system().set_show_damage_flag(flag);
        Py_RETURN_NONE;
    }

    auto system_set_show_sales_text_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        get_python_system().set_show_sales_text_flag(flag);
        Py_RETURN_NONE;
    }

    auto system_is_always_show_name(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().is_always_show_name());
    }

    auto system_is_show_damage(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().is_show_damage());
    }

    auto system_is_show_sales_text(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().is_show_sales_text());
    }

    auto system_get_shadow_level(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().shadow_level());
    }

    auto system_set_shadow_level(PyObject *self, PyObject *args) -> PyObject * {
        int level;
        if (!PyArg_ParseTuple(args, "i", &level)) {
            return nullptr;
        }

        if (level > 0) {
            get_python_system().set_shadow_level(level);
        }

        Py_RETURN_NONE;
    }

    void init_settings_module() {
        static PyMethodDef methods[] = {
            {"get_width", system_get_width, METH_VARARGS},
            {"get_height", system_get_height, METH_VARARGS},

            {"set_interface_handler", system_set_interface_handler, METH_VARARGS},
            {"destroy_interface_handler", system_destroy_interface_handler, METH_VARARGS},

            {"get_music_volume", system_get_music_volume, METH_VARARGS},
            {"get_sound_volume", system_get_sound_volume, METH_VARARGS},

            {"set_music_volume", system_set_music_volume, METH_VARARGS},
            {"set_sound_volumef", system_set_sound_volumef, METH_VARARGS},
            {"is_software_cursor", system_is_software_cursor, METH_VARARGS},

            {"set_view_chat_flag", system_set_view_chat_flag, METH_VARARGS},
            {"is_view_chat", system_is_view_chat, METH_VARARGS},

            {"set_always_show_name_flag", system_set_always_show_name_flag, METH_VARARGS},
            {"is_always_show_name", system_is_always_show_name, METH_VARARGS},

            {"set_show_damage_flag", system_set_show_damage_flag, METH_VARARGS},
            {"is_show_damage", system_is_show_damage, METH_VARARGS},

            {"set_show_sales_text_flag", system_set_show_sales_text_flag, METH_VARARGS},
            {"is_show_sales_text", system_is_show_sales_text, METH_VARARGS},

            {"get_shadow_level", system_get_shadow_level, METH_VARARGS},
            {"set_shadow_level", system_set_shadow_level, METH_VARARGS},

            {nullptr, nullptr, 0, nullptr}
        };

        auto *module = init_python_module("metin2.settings", methods);

        PyModule_AddIntConstant(module, "WINDOW_STATUS", python_system::WINDOW_STATUS);
        PyModule_AddIntConstant(module, "WINDOW_INVENTORY", python_system::WINDOW_INVENTORY);
        PyModule_AddIntConstant(module, "WINDOW_ABILITY", python_system::WINDOW_ABILITY);
        PyModule_AddIntConstant(module, "WINDOW_SOCIETY", python_system::WINDOW_SOCIETY);
        PyModule_AddIntConstant(module, "WINDOW_JOURNAL", python_system::WINDOW_JOURNAL);
        PyModule_AddIntConstant(module, "WINDOW_COMMAND", python_system::WINDOW_COMMAND);
        PyModule_AddIntConstant(module, "WINDOW_QUICK", python_system::WINDOW_QUICK);
        PyModule_AddIntConstant(module, "WINDOW_GAUGE", python_system::WINDOW_GAUGE);
        PyModule_AddIntConstant(module, "WINDOW_MINIMAP", python_system::WINDOW_MINIMAP);
        PyModule_AddIntConstant(module, "WINDOW_CHAT", python_system::WINDOW_CHAT);
    }
}
