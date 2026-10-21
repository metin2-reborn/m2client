#include "python_system.h"

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
        if (!py_tuple_get_object(args, 0, &handler)) {
            return py_build_exception();
        }

        get_python_system().set_interface_handler(handler);
        return py_build_none();
    }

    auto system_destroy_interface_handler(PyObject *self, PyObject *args) -> PyObject * {
        get_python_system().destroy_interface_handler();
        return py_build_none();
    }

    auto system_get_music_volume(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("f", get_python_system().music_volume());
    }

    auto system_get_sound_volume(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().sound_volume());
    }

    auto system_set_music_volume(PyObject *self, PyObject *args) -> PyObject * {
        float volume;
        if (!py_tuple_get_float(args, 0, &volume)) {
            return py_build_exception();
        }

        get_python_system().set_music_volume(volume);
        return py_build_none();
    }

    auto system_set_sound_volumef(PyObject *self, PyObject *args) -> PyObject * {
        float volume;
        if (!py_tuple_get_float(args, 0, &volume)) {
            return py_build_exception();
        }

        get_python_system().set_sound_volumef(volume);
        return py_build_none();
    }

    auto system_is_software_cursor(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().is_software_cursor());
    }

    auto system_set_view_chat_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag)) {
            return py_build_exception();
        }

        get_python_system().set_view_chat_flag(flag);
        return py_build_none();
    }

    auto system_is_view_chat(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().is_view_chat());
    }

    auto system_set_always_show_name_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag)) {
            return py_build_exception();
        }

        get_python_system().set_always_show_name_flag(flag);
        return py_build_none();
    }

    auto system_set_show_damage_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag)) {
            return py_build_exception();
        }

        get_python_system().set_show_damage_flag(flag);
        return py_build_none();
    }

    auto system_set_show_sales_text_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag)) {
            return py_build_exception();
        }

        get_python_system().set_show_sales_text_flag(flag);
        return py_build_none();
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

        if (!py_tuple_get_integer(args, 0, &level)) {
            return py_build_exception();
        }

        if (level > 0) {
            get_python_system().set_shadow_level(level);
        }

        return py_build_none();
    }

    void initsystem() {
        static PyMethodDef methods[] = {
            {"GetWidth", system_get_width, METH_VARARGS},
            {"GetHeight", system_get_height, METH_VARARGS},

            {"SetInterfaceHandler", system_set_interface_handler, METH_VARARGS},
            {"DestroyInterfaceHandler", system_destroy_interface_handler, METH_VARARGS},

            {"GetMusicVolume", system_get_music_volume, METH_VARARGS},
            {"GetSoundVolume", system_get_sound_volume, METH_VARARGS},

            {"SetMusicVolume", system_set_music_volume, METH_VARARGS},
            {"SetSoundVolumef", system_set_sound_volumef, METH_VARARGS},
            {"IsSoftwareCursor", system_is_software_cursor, METH_VARARGS},

            {"SetViewChatFlag", system_set_view_chat_flag, METH_VARARGS},
            {"IsViewChat", system_is_view_chat, METH_VARARGS},

            {"SetAlwaysShowNameFlag", system_set_always_show_name_flag, METH_VARARGS},
            {"IsAlwaysShowName", system_is_always_show_name, METH_VARARGS},

            {"SetShowDamageFlag", system_set_show_damage_flag, METH_VARARGS},
            {"IsShowDamage", system_is_show_damage, METH_VARARGS},

            {"SetShowSalesTextFlag", system_set_show_sales_text_flag, METH_VARARGS},
            {"IsShowSalesText", system_is_show_sales_text, METH_VARARGS},

            {"GetShadowLevel", system_get_shadow_level, METH_VARARGS},
            {"SetShadowLevel", system_set_shadow_level, METH_VARARGS},

            {nullptr, nullptr, 0, nullptr}
        };

        PyObject *module_value = Py_InitModule("systemSetting", methods);

        PyModule_AddIntConstant(module_value, "WINDOW_STATUS", python_system::WINDOW_STATUS);
        PyModule_AddIntConstant(module_value, "WINDOW_INVENTORY", python_system::WINDOW_INVENTORY);
        PyModule_AddIntConstant(module_value, "WINDOW_ABILITY", python_system::WINDOW_ABILITY);
        PyModule_AddIntConstant(module_value, "WINDOW_SOCIETY", python_system::WINDOW_SOCIETY);
        PyModule_AddIntConstant(module_value, "WINDOW_JOURNAL", python_system::WINDOW_JOURNAL);
        PyModule_AddIntConstant(module_value, "WINDOW_COMMAND", python_system::WINDOW_COMMAND);

        PyModule_AddIntConstant(module_value, "WINDOW_QUICK", python_system::WINDOW_QUICK);
        PyModule_AddIntConstant(module_value, "WINDOW_GAUGE", python_system::WINDOW_GAUGE);
        PyModule_AddIntConstant(module_value, "WINDOW_MINIMAP", python_system::WINDOW_MINIMAP);
        PyModule_AddIntConstant(module_value, "WINDOW_CHAT", python_system::WINDOW_CHAT);
    }
}
