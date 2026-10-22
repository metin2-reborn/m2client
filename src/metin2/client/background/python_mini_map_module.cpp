#include "python_mini_map.h"

#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline background {

    auto mini_map_set_scale(PyObject *self, PyObject *args) -> PyObject * {
        float scale;
        if (!py_tuple_get_float(args, 0, &scale)) {
            return py_build_exception();
        }

        get_python_mini_map().set_scale(scale);
        return py_build_none();
    }

    auto mini_map_destroy(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().destroy();
        return py_build_none();
    }

    auto mini_map_create(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().create();
        return py_build_none();
    }

    auto mini_map_update(PyObject *self, PyObject *args) -> PyObject * {
        float center_x, center_y;
        if (!py_tuple_get_float(args, 0, &center_x) ||
            !py_tuple_get_float(args, 1, &center_y)) {
            return py_build_exception();
        }

        get_python_mini_map().update(center_x, center_y);
        return py_build_none();
    }

    auto mini_map_render(PyObject *self, PyObject *args) -> PyObject * {
        float screen_x, screen_y;
        if (!py_tuple_get_float(args, 0, &screen_x) || !py_tuple_get_float(args, 1, &screen_y)) {
            return py_build_exception();
        }

        get_python_mini_map().render(screen_x, screen_y);
        return py_build_none();
    }

    auto mini_map_show(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().show();
        return py_build_none();
    }

    auto mini_map_hide(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().hide();
        return py_build_none();
    }

    auto mini_mapis_show(PyObject *self, PyObject *args) -> PyObject * {
        bool show = get_python_mini_map().can_show();
        return Py_BuildValue("b", show);
    }

    auto mini_map_scale_up(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().scale_up();
        return py_build_none();
    }

    auto mini_map_scale_down(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().scale_down();
        return py_build_none();
    }

    auto mini_map_get_info(PyObject *self, PyObject *args) -> PyObject * {
        float screen_x, screen_y;
        if (!py_tuple_get_float(args, 0, &screen_x) ||
            !py_tuple_get_float(args, 1, &screen_y)) {
            return py_build_exception();
        }

        std::string string;
        float f_pos_x, f_pos_y;
        uint32_t text_color;

        bool find = get_python_mini_map().picked_instance_info(screen_x, screen_y, string, &f_pos_x, &f_pos_y, &text_color);

        int i_pos_x, i_pos_y;
        PR_FLOAT_TO_INT(f_pos_x, i_pos_x);
        PR_FLOAT_TO_INT(f_pos_y, i_pos_y);

        i_pos_x /= 100;
        i_pos_y /= 100;

        return Py_BuildValue("isiil", (int)find, string.c_str(), i_pos_x, i_pos_y, long(int32_t(text_color)));
    }

    auto mini_map_update_atlas(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().update_atlas();
        return py_build_none();
    }

    auto mini_map_render_atlas(PyObject *self, PyObject *args) -> PyObject * {
        float screen_x, screen_y;
        if (!py_tuple_get_float(args, 0, &screen_x) ||
            !py_tuple_get_float(args, 1, &screen_y)) {
            return py_build_exception();
        }

        get_python_mini_map().render_atlas(screen_x, screen_y);
        return py_build_none();
    }

    auto mini_map_show_atlas(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().show_atlas();
        return py_build_none();
    }

    auto mini_map_hide_atlas(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().hide_atlas();
        return py_build_none();
    }

    auto mini_map_is_atlas(PyObject *self, PyObject *args) -> PyObject * {
        bool is_data = get_python_mini_map().is_atlas();
        return Py_BuildValue("b", is_data);
    }

    auto mini_map_get_atlas_info(PyObject *self, PyObject *args) -> PyObject * {
        float screen_x, screen_y;
        if (!py_tuple_get_float(args, 0, &screen_x) ||
            !py_tuple_get_float(args, 1, &screen_y)) {
            return py_build_exception();
        }

        std::string string = "";
        float f_pos_x = 0.0f;
        float f_pos_y = 0.0f;
        uint32_t text_color = 0;
        uint32_t guild_id = 0;

        bool find = get_python_mini_map().atlas_info(screen_x, screen_y, string, &f_pos_x, &f_pos_y, &text_color, &guild_id);

        int i_pos_x, i_pos_y;
        PR_FLOAT_TO_INT(f_pos_x, i_pos_x);
        PR_FLOAT_TO_INT(f_pos_y, i_pos_y);

        i_pos_x /= 100;
        i_pos_y /= 100;

        return Py_BuildValue("isiili", (int)find, string.c_str(), i_pos_x, i_pos_y, long(int32_t(text_color)), guild_id);
    }

    auto mini_map_get_atlas_size(PyObject *self, PyObject *args) -> PyObject * {
        float f_size_x, f_size_y;

        bool get = get_python_mini_map().atlas_size(&f_size_x, &f_size_y);

        int i_size_x, i_size_y;
        PR_FLOAT_TO_INT(f_size_x, i_size_x);
        PR_FLOAT_TO_INT(f_size_y, i_size_y);

        return Py_BuildValue("bii", (int)get, i_size_x, i_size_y);
    }

    auto mini_map_register_atlas_window(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *handler;
        if (!py_tuple_get_object(args, 0, &handler)) {
            return py_build_exception();
        }

        get_python_mini_map().register_atlas_window(handler);
        return py_build_none();
    }

    auto mini_map_unregister_atlas_window(PyObject *self, PyObject *args) -> PyObject * {
        get_python_mini_map().unregister_atlas_window();
        return py_build_none();
    }

    auto mini_map_get_guild_area_id(PyObject *self, PyObject *args) -> PyObject * {
        float fx, fy;
        if (!py_tuple_get_float(args, 0, &fx) ||
            !py_tuple_get_float(args, 1, &fy)) {
            return py_build_exception();
        }

        uint32_t guild_id = get_python_mini_map().guild_area_id(fx, fy);
        return Py_BuildValue("i", guild_id);
    }

    void init_world_minimap_module() {
        static PyMethodDef methods[] = {
            {"SetScale", mini_map_set_scale, METH_VARARGS},
            {"ScaleUp", mini_map_scale_up, METH_VARARGS},
            {"ScaleDown", mini_map_scale_down, METH_VARARGS},

            {"Destroy", mini_map_destroy, METH_VARARGS},
            {"Create", mini_map_create, METH_VARARGS},
            {"Update", mini_map_update, METH_VARARGS},
            {"Render", mini_map_render, METH_VARARGS},

            {"Show", mini_map_show, METH_VARARGS},
            {"Hide", mini_map_hide, METH_VARARGS},

            {"isShow", mini_mapis_show, METH_VARARGS},

            {"GetInfo", mini_map_get_info, METH_VARARGS},

            {"UpdateAtlas", mini_map_update_atlas, METH_VARARGS},
            {"RenderAtlas", mini_map_render_atlas, METH_VARARGS},
            {"ShowAtlas", mini_map_show_atlas, METH_VARARGS},
            {"HideAtlas", mini_map_hide_atlas, METH_VARARGS},
            {"IsAtlas", mini_map_is_atlas, METH_VARARGS},
            {"GetAtlasInfo", mini_map_get_atlas_info, METH_VARARGS},
            {"GetAtlasSize", mini_map_get_atlas_size, METH_VARARGS},

            {"RegisterAtlasWindow", mini_map_register_atlas_window, METH_VARARGS},
            {"UnregisterAtlasWindow", mini_map_unregister_atlas_window, METH_VARARGS},

            {"GetGuildAreaID", mini_map_get_guild_area_id, METH_VARARGS},

            {nullptr, nullptr},
        };

        PyObject *module_value = init_python_module("metin2.world.minimap", methods);

        PyModule_AddIntConstant(module_value, "TYPE_OPC", python_mini_map::TYPE_OPC);
        PyModule_AddIntConstant(module_value, "TYPE_OPCPVP", python_mini_map::TYPE_OPCPVP);
        PyModule_AddIntConstant(module_value, "TYPE_OPCPVPSELF", python_mini_map::TYPE_OPCPVPSELF);
        PyModule_AddIntConstant(module_value, "TYPE_NPC", python_mini_map::TYPE_NPC);
        PyModule_AddIntConstant(module_value, "TYPE_MONSTER", python_mini_map::TYPE_MONSTER);
        PyModule_AddIntConstant(module_value, "TYPE_WARP", python_mini_map::TYPE_WARP);
        PyModule_AddIntConstant(module_value, "TYPE_WAYPOINT", python_mini_map::TYPE_WAYPOINT);
        PyModule_AddIntConstant(module_value, "TYPE_PARTY", python_mini_map::TYPE_PARTY);
        PyModule_AddIntConstant(module_value, "TYPE_EMPIRE", python_mini_map::TYPE_EMPIRE);
    }
}
