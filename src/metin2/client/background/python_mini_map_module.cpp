#include "python_mini_map.h"

#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>

#include <cmath>

namespace metin2::inline client::inline background {
    auto mini_map_set_scale(PyObject *self, PyObject *args) -> PyObject * {
        float scale;
        if (!PyArg_ParseTuple(args, "f", &scale)) {
            return nullptr;
        }

        get_python_mini_map().set_scale(scale);
        Py_RETURN_NONE;
    }

    auto mini_map_destroy(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().destroy();
        Py_RETURN_NONE;
    }

    auto mini_map_create(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().create();
        Py_RETURN_NONE;
    }

    auto mini_map_update(PyObject *self, PyObject *args) -> PyObject * {
        float center_x, center_y;
        if (!PyArg_ParseTuple(args, "ff", &center_x, &center_y)) {
            return nullptr;
        }

        get_python_mini_map().update(center_x, center_y);
        Py_RETURN_NONE;
    }

    auto mini_map_render(PyObject *self, PyObject *args) -> PyObject * {
        float screen_x, screen_y;
        if (!PyArg_ParseTuple(args, "ff", &screen_x, &screen_y)) {
            return nullptr;
        }

        get_python_mini_map().render(screen_x, screen_y);
        Py_RETURN_NONE;
    }

    auto mini_map_show(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().show();
        Py_RETURN_NONE;
    }

    auto mini_map_hide(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().hide();
        Py_RETURN_NONE;
    }

    auto mini_map_is_show(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("b", get_python_mini_map().can_show());
    }

    auto mini_map_scale_up(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().scale_up();
        Py_RETURN_NONE;
    }

    auto mini_map_scale_down(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().scale_down();
        Py_RETURN_NONE;
    }

    auto mini_map_get_info(PyObject *self, PyObject *args) -> PyObject * {
        float screen_x, screen_y;
        if (!PyArg_ParseTuple(args, "ff", &screen_x, &screen_y)) {
            return nullptr;
        }

        std::string string;
        float pos_x, pos_y;
        uint32_t text_color;

        auto found = get_python_mini_map().picked_instance_info(screen_x, screen_y, string, &pos_x, &pos_y, &text_color);

        return Py_BuildValue("isiil",
            (int)found, string.c_str(),
            static_cast<int>(std::floor(pos_x)) / 100,
            static_cast<int>(std::floor(pos_y)) / 100,
            long(int32_t(text_color)));
    }

    auto mini_map_update_atlas(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().update_atlas();
        Py_RETURN_NONE;
    }

    auto mini_map_render_atlas(PyObject *self, PyObject *args) -> PyObject * {
        float screen_x, screen_y;
        if (!PyArg_ParseTuple(args, "ff", &screen_x, &screen_y)) {
            return nullptr;
        }

        get_python_mini_map().render_atlas(screen_x, screen_y);
        Py_RETURN_NONE;
    }

    auto mini_map_show_atlas(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().show_atlas();
        Py_RETURN_NONE;
    }

    auto mini_map_hide_atlas(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().hide_atlas();
        Py_RETURN_NONE;
    }

    auto mini_map_is_atlas(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("b", get_python_mini_map().is_atlas());
    }

    auto mini_map_get_atlas_info(PyObject *self, PyObject *args) -> PyObject * {
        float screen_x, screen_y;
        if (!PyArg_ParseTuple(args, "ff", &screen_x, &screen_y)) {
            return nullptr;
        }

        std::string string = "";
        float pos_x = 0.0f;
        float pos_y = 0.0f;
        uint32_t text_color = 0;
        uint32_t guild_id = 0;

        auto found = get_python_mini_map().atlas_info(screen_x, screen_y, string, &pos_x, &pos_y, &text_color, &guild_id);

        return Py_BuildValue("isiili",
            (int)found, string.c_str(),
            static_cast<int>(std::floor(pos_x)) / 100,
            static_cast<int>(std::floor(pos_y)) / 100,
            long(int32_t(text_color)),
            guild_id);
    }

    auto mini_map_get_atlas_size(PyObject *self, PyObject *args) -> PyObject * {
        float size_x, size_y;

        auto get = get_python_mini_map().atlas_size(&size_x, &size_y);

        return Py_BuildValue("bii",
            (int)get,
            static_cast<int>(std::floor(size_x)),
            static_cast<int>(std::floor(size_y)));
    }

    auto mini_map_register_atlas_window(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handler;
        if (!PyArg_ParseTuple(args, "O", &handler)) {
            return nullptr;
        }

        get_python_mini_map().register_atlas_window(handler);
        Py_RETURN_NONE;
    }

    auto mini_map_unregister_atlas_window(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().unregister_atlas_window();
        Py_RETURN_NONE;
    }

    auto mini_map_get_guild_area_id(PyObject *self, PyObject *args) -> PyObject * {
        float x, y;
        if (!PyArg_ParseTuple(args, "ff", &x, &y)) {
            return nullptr;
        }

        return Py_BuildValue("i", get_python_mini_map().guild_area_id(x, y));
    }

    void init_world_minimap_module() {
        static PyMethodDef methods[] = {
            {"set_scale", mini_map_set_scale, METH_VARARGS},
            {"scale_up", mini_map_scale_up, METH_VARARGS},
            {"scale_down", mini_map_scale_down, METH_VARARGS},

            {"destroy", mini_map_destroy, METH_VARARGS},
            {"create", mini_map_create, METH_VARARGS},
            {"update", mini_map_update, METH_VARARGS},
            {"render", mini_map_render, METH_VARARGS},

            {"show", mini_map_show, METH_VARARGS},
            {"hide", mini_map_hide, METH_VARARGS},

            {"is_show", mini_map_is_show, METH_VARARGS},

            {"get_info", mini_map_get_info, METH_VARARGS},

            {"update_atlas", mini_map_update_atlas, METH_VARARGS},
            {"render_atlas", mini_map_render_atlas, METH_VARARGS},
            {"show_atlas", mini_map_show_atlas, METH_VARARGS},
            {"hide_atlas", mini_map_hide_atlas, METH_VARARGS},
            {"is_atlas", mini_map_is_atlas, METH_VARARGS},
            {"get_atlas_info", mini_map_get_atlas_info, METH_VARARGS},
            {"get_atlas_size", mini_map_get_atlas_size, METH_VARARGS},

            {"register_atlas_window", mini_map_register_atlas_window, METH_VARARGS},
            {"unregister_atlas_window", mini_map_unregister_atlas_window, METH_VARARGS},

            {"get_guild_area_id", mini_map_get_guild_area_id, METH_VARARGS},

            {nullptr, nullptr},
        };

        auto *module = init_python_module("metin2.world.minimap", methods);

        PyModule_AddIntConstant(module, "TYPE_OPC", python_mini_map::TYPE_OPC);
        PyModule_AddIntConstant(module, "TYPE_OPCPVP", python_mini_map::TYPE_OPCPVP);
        PyModule_AddIntConstant(module, "TYPE_OPCPVPSELF", python_mini_map::TYPE_OPCPVPSELF);
        PyModule_AddIntConstant(module, "TYPE_NPC", python_mini_map::TYPE_NPC);
        PyModule_AddIntConstant(module, "TYPE_MONSTER", python_mini_map::TYPE_MONSTER);
        PyModule_AddIntConstant(module, "TYPE_WARP", python_mini_map::TYPE_WARP);
        PyModule_AddIntConstant(module, "TYPE_WAYPOINT", python_mini_map::TYPE_WAYPOINT);
        PyModule_AddIntConstant(module, "TYPE_PARTY", python_mini_map::TYPE_PARTY);
        PyModule_AddIntConstant(module, "TYPE_EMPIRE", python_mini_map::TYPE_EMPIRE);
    }
}
