#include "python_application.h"
#include "python_system.h"

#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <metin2/window/main_window.h>

#include <algorithm>

namespace metin2::inline client::inline app {
    // The interface size in logical pixels (the window size over the interface scale).
    auto system_get_width(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_application().interface_width());
    }

    auto system_get_height(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_application().interface_height());
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

        if (level >= 0) {
            get_python_system().set_shadow_level(level);
        }

        Py_RETURN_NONE;
    }

    auto system_get_gamma(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().gamma());
    }

    auto system_set_gamma(PyObject *self, PyObject *args) -> PyObject * {
        int gamma;
        if (!PyArg_ParseTuple(args, "i", &gamma)) {
            return nullptr;
        }

        get_python_system().set_gamma(std::clamp(gamma, 1, 5));
        Py_RETURN_NONE;
    }

    auto system_is_antialias(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().is_antialias() ? 1 : 0);
    }

    auto system_set_antialias(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        get_python_system().set_antialias(flag != 0);
        Py_RETURN_NONE;
    }

    auto system_is_vsync(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().is_vsync() ? 1 : 0);
    }

    auto system_set_vsync(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!PyArg_ParseTuple(args, "i", &flag)) {
            return nullptr;
        }

        get_python_system().set_vsync(flag != 0);
        main_window::set_vsync(flag != 0);
        Py_RETURN_NONE;
    }

    auto system_get_ui_scale(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().ui_scale());
    }

    auto system_set_ui_scale(PyObject *self, PyObject *args) -> PyObject * {
        int scale;
        if (!PyArg_ParseTuple(args, "i", &scale)) {
            return nullptr;
        }

        get_python_system().set_ui_scale(std::clamp(scale, 0, 3));
        get_python_application().apply_interface_scale();
        Py_RETURN_NONE;
    }

    auto system_get_key(PyObject *self, PyObject *args) -> PyObject * {
        char *action;
        int default_dik;
        if (!PyArg_ParseTuple(args, "si", &action, &default_dik)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_system().key_binding(action, default_dik));
    }

    auto system_get_terrain_detail(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().terrain_detail());
    }

    auto system_set_terrain_detail(PyObject *self, PyObject *args) -> PyObject * {
        int level;
        if (!PyArg_ParseTuple(args, "i", &level)) {
            return nullptr;
        }
        get_python_system().set_terrain_detail(level);
        Py_RETURN_NONE;
    }

    auto system_get_grass_density(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_system().grass_density());
    }

    auto system_set_grass_density(PyObject *self, PyObject *args) -> PyObject * {
        int density;
        if (!PyArg_ParseTuple(args, "i", &density)) {
            return nullptr;
        }
        get_python_system().set_grass_density(density);
        Py_RETURN_NONE;
    }

    void init_settings_module() {
        static PyMethodDef methods[] = {
            {"get_terrain_detail", system_get_terrain_detail, METH_VARARGS},
            {"set_terrain_detail", system_set_terrain_detail, METH_VARARGS},
            {"get_grass_density", system_get_grass_density, METH_VARARGS},
            {"set_grass_density", system_set_grass_density, METH_VARARGS},
            {"get_key", system_get_key, METH_VARARGS},
            {"get_width", system_get_width, METH_VARARGS},
            {"get_height", system_get_height, METH_VARARGS},
            {"get_gamma", system_get_gamma, METH_VARARGS},
            {"set_gamma", system_set_gamma, METH_VARARGS},
            {"is_antialias", system_is_antialias, METH_VARARGS},
            {"set_antialias", system_set_antialias, METH_VARARGS},
            {"is_vsync", system_is_vsync, METH_VARARGS},
            {"set_vsync", system_set_vsync, METH_VARARGS},
            {"get_ui_scale", system_get_ui_scale, METH_VARARGS},
            {"set_ui_scale", system_set_ui_scale, METH_VARARGS},

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
