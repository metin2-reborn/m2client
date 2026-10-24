#include "python_background.h"

#include <metin2/client/app/python_system.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/terrain/map_outdoor.h>

#include <format>
#include <string>

namespace metin2::inline client::inline background {
    auto background_enable_snow(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!PyArg_ParseTuple(args, "i", &enable)) {
            return nullptr;
        }

        auto &background = get_python_background();
        if (enable) {
            background.enable_snow_environment();
        } else {
            background.disable_snow_environment();
        }

        Py_RETURN_NONE;
    }

    auto background_load_map(PyObject *self, PyObject *args) -> PyObject * {
        char *map_path;
        float x, y, z;

        if (!PyArg_ParseTuple(args, "sfff", &map_path, &x, &y, &z)) {
            return nullptr;
        }

        get_python_background().load_map(map_path, x, y, z);
        Py_RETURN_NONE;
    }

    auto background_destroy(PyObject *self, PyObject *args) -> PyObject * {
        auto &background = get_python_background();

        background.set_shadow_level(python_background::SHADOW_NONE);
        background.destroy();

        Py_RETURN_NONE;
    }

    auto background_register_environment_data(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        char *environment_file_name;
        if (!PyArg_ParseTuple(args, "is", &index, &environment_file_name)) {
            return nullptr;
        }

        if (!get_python_background().register_environment_data(index, environment_file_name)) {
            metin2::log::error("Cannot register environment {} from {}", index, environment_file_name);

            // TODO: fall back to the default environment.
        }

        Py_RETURN_NONE;
    }

    auto background_set_environment_data(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!PyArg_ParseTuple(args, "i", &index)) {
            return nullptr;
        }

        auto &background = get_python_background();

        const environment_data *environmen_data;
        if (background.get_environment_data(index, &environmen_data)) {
            background.reset_environment_data_ptr(environmen_data);
        }

        Py_RETURN_NONE;
    }

    auto background_get_current_map_name(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("s", get_python_background().warp_map_name());
    }

    auto background_get_picking_point(PyObject *self, PyObject *args) -> PyObject * {
        pixel_position pt(0.0f, 0.0f, 0.0f);
        if (get_python_background().picking_point(&pt)) {
            pt.y = -pt.y;
        }

        return Py_BuildValue("fff", pt.x, pt.y, pt.z);
    }

    auto background_initialize(PyObject *self, PyObject *args) -> PyObject * {
        get_python_background().create();
        Py_RETURN_NONE;
    }

    auto background_render_collision(PyObject *self, PyObject *args) -> PyObject * {
        get_python_background().render_collision();
        Py_RETURN_NONE;
    }

    auto background_get_height(PyObject *self, PyObject *args) -> PyObject * {
        float x, y;
        if (!PyArg_ParseTuple(args, "ff", &x, &y)) {
            return nullptr;
        }

        return Py_BuildValue("f", get_python_background().height(x, y));
    }

    auto background_get_rendered_splat_num(PyObject *self, PyObject *args) -> PyObject * {
        int patch, splat;
        float splat_ratio;

        auto &texture_num_vector = get_python_background().rendered_splat_num(&patch, &splat, &splat_ratio);

        std::string output;
        for (int texture_num : texture_num_vector) {
            output += std::format("{} ", texture_num);
        }

        return Py_BuildValue("iifs", patch, splat, splat_ratio, output.c_str());
    }

    auto background_get_rendered_gti_num(PyObject *self, PyObject *args) -> PyObject * {
        uint32_t graphic_thing_instance_num;
        uint32_t crc_num;

        get_python_background().rendered_graphic_thing_instance_num(&graphic_thing_instance_num, &crc_num);

        return Py_BuildValue("ii", graphic_thing_instance_num, crc_num);
    }

    auto background_get_render_shadow_time(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_background().render_shadow_time());
    }

    auto background_set_shadow_level(PyObject *self, PyObject *args) -> PyObject * {
        int level;
        if (!PyArg_ParseTuple(args, "i", &level)) {
            return nullptr;
        }

        get_python_background().set_shadow_level(level);
        Py_RETURN_NONE;
    }

    auto background_set_visible_part(PyObject *self, PyObject *args) -> PyObject * {
        int part, is_visible;
        if (!PyArg_ParseTuple(args, "ii", &part, &is_visible)) {
            return nullptr;
        }

        if (part >= map_outdoor::PART_NUM) {
            PyErr_Format(PyExc_ValueError, "part(%d) < background.PART_NUM(%d)", part, map_outdoor::PART_NUM);
            return nullptr;
        }

        get_python_background().set_visible_part(part, is_visible ? true : false);
        Py_RETURN_NONE;
    }

    auto background_set_splat_limit(PyObject *self, PyObject *args) -> PyObject * {
        int splat_limit;
        if (!PyArg_ParseTuple(args, "i", &splat_limit)) {
            return nullptr;
        }

        if (splat_limit < 0) {
            PyErr_Format(PyExc_ValueError, "world.set_splat_limit(%d): the limit must not be negative", splat_limit);
            return nullptr;
        }

        get_python_background().set_splat_limit(splat_limit);
        Py_RETURN_NONE;
    }

    auto background_select_view_distance_num(PyObject *self, PyObject *args) -> PyObject * {
        int view_distance_num;
        if (!PyArg_ParseTuple(args, "i", &view_distance_num)) {
            return nullptr;
        }

        get_python_background().select_view_distance_num(view_distance_num);
        Py_RETURN_NONE;
    }

    auto background_set_view_distance_set(PyObject *self, PyObject *args) -> PyObject * {
        int num;
        float far_clip;

        if (!PyArg_ParseTuple(args, "if", &num, &far_clip)) {
            return nullptr;
        }

        get_python_background().set_view_distance_set(num, far_clip);
        Py_RETURN_NONE;
    }

    auto background_get_distance_set_info(PyObject *self, PyObject *args) -> PyObject * {
        int num;
        float start, end, far_clip;

        get_python_background().distance_set_info(&num, &start, &end, &far_clip);
        return Py_BuildValue("ifff", num, start, end, far_clip);
    }

    auto background_set_bg_loading(PyObject *self, PyObject *args) -> PyObject * {
        int bg_loading;
        if (!PyArg_ParseTuple(args, "i", &bg_loading)) {
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto background_set_render_sort(PyObject *self, PyObject *args) -> PyObject * {
        int render_sort;
        if (!PyArg_ParseTuple(args, "i", &render_sort)) {
            return nullptr;
        }

        get_python_background().set_terrain_render_sort((map_outdoor::terrain_render_sort)render_sort);
        Py_RETURN_NONE;
    }

    auto background_set_transparent_tree(PyObject *self, PyObject *args) -> PyObject * {
        int transparent;
        if (!PyArg_ParseTuple(args, "i", &transparent)) {
            return nullptr;
        }

        get_python_background().set_transparent_tree(transparent ? true : false);
        Py_RETURN_NONE;
    }

    auto background_global_position_to_map_info(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!PyArg_ParseTuple(args, "ii", &x, &y)) {
            return nullptr;
        }

        auto *map_info = get_python_background().global_position_to_map_info(x, y);
        if (map_info) {
            return Py_BuildValue("sii", map_info->name_value.c_str(), map_info->base_x, map_info->base_y);
        }

        return Py_BuildValue("sii", "", 0, 0);
    }

    auto background_warp_test(PyObject *self, PyObject *args) -> PyObject * {
        int x, y;
        if (!PyArg_ParseTuple(args, "ii", &x, &y)) {
            return nullptr;
        }

        get_python_background().warp((uint32_t)x * 100, (uint32_t)y * 100);
        Py_RETURN_NONE;
    }

    auto background_set_x_mas_tree(PyObject *self, PyObject *args) -> PyObject * {
        int grade;
        if (!PyArg_ParseTuple(args, "i", &grade)) {
            return nullptr;
        }

        get_python_background().set_x_max_tree(grade);
        Py_RETURN_NONE;
    }

    auto background_register_dungeon_map_name(PyObject *self, PyObject *args) -> PyObject * {
        char *map_name;
        if (!PyArg_ParseTuple(args, "s", &map_name)) {
            return nullptr;
        }

        get_python_background().register_dungeon_map_name(map_name);
        Py_RETURN_NONE;
    }

    auto background_visible_guild_area(PyObject *self, PyObject *args) -> PyObject * {
        get_python_background().visible_guild_area();
        Py_RETURN_NONE;
    }

    auto background_disable_guild_area(PyObject *self, PyObject *args) -> PyObject * {
        get_python_background().disable_guild_area();
        Py_RETURN_NONE;
    }

    void init_world_module() {
        static PyMethodDef methods[] = {
            {"enable_snow", background_enable_snow, METH_VARARGS},
            {"global_position_to_map_info", background_global_position_to_map_info, METH_VARARGS},
            {"get_render_shadow_time", background_get_render_shadow_time, METH_VARARGS},
            {"load_map", background_load_map, METH_VARARGS},
            {"destroy", background_destroy, METH_VARARGS},
            {"register_environment_data", background_register_environment_data, METH_VARARGS},
            {"set_environment_data", background_set_environment_data, METH_VARARGS},
            {"get_current_map_name", background_get_current_map_name, METH_VARARGS},
            {"get_picking_point", background_get_picking_point, METH_VARARGS},

            {"initialize", background_initialize, METH_VARARGS},
            {"render_collision", background_render_collision, METH_VARARGS},
            {"get_height", background_get_height, METH_VARARGS},

            {"set_shadow_level", background_set_shadow_level, METH_VARARGS},

            {"set_visible_part", background_set_visible_part, METH_VARARGS},
            {"set_splat_limit", background_set_splat_limit, METH_VARARGS},
            {"get_rendered_splat_num", background_get_rendered_splat_num, METH_VARARGS},
            {"get_rendered_graphic_thing_instance_num", background_get_rendered_gti_num, METH_VARARGS},
            {"select_view_distance_num", background_select_view_distance_num, METH_VARARGS},
            {"set_view_distance_set", background_set_view_distance_set, METH_VARARGS},
            {"get_distance_set_info", background_get_distance_set_info, METH_VARARGS},
            {"set_bg_loading", background_set_bg_loading, METH_VARARGS},
            {"set_render_sort", background_set_render_sort, METH_VARARGS},
            {"set_transparent_tree", background_set_transparent_tree, METH_VARARGS},
            {"set_xmas_tree", background_set_x_mas_tree, METH_VARARGS},
            {"register_dungeon_map_name", background_register_dungeon_map_name, METH_VARARGS},

            {"visible_guild_area", background_visible_guild_area, METH_VARARGS},
            {"disable_guild_area", background_disable_guild_area, METH_VARARGS},

            {"warp_test", background_warp_test, METH_VARARGS},

            {nullptr, nullptr, 0, nullptr},
        };

        auto *module = init_python_module("metin2.world", methods);

        PyModule_AddIntConstant(module, "PART_SKY", map_outdoor::PART_SKY);
        PyModule_AddIntConstant(module, "PART_TREE", map_outdoor::PART_TREE);
        PyModule_AddIntConstant(module, "PART_CLOUD", map_outdoor::PART_CLOUD);
        PyModule_AddIntConstant(module, "PART_WATER", map_outdoor::PART_WATER);
        PyModule_AddIntConstant(module, "PART_OBJECT", map_outdoor::PART_OBJECT);
        PyModule_AddIntConstant(module, "PART_TERRAIN", map_outdoor::PART_TERRAIN);

        PyModule_AddIntConstant(module, "SKY_RENDER_MODE_DEFAULT", sky_object::SKY_RENDER_MODE_DEFAULT);
        PyModule_AddIntConstant(module, "SKY_RENDER_MODE_DIFFUSE", sky_object::SKY_RENDER_MODE_DIFFUSE);
        PyModule_AddIntConstant(module, "SKY_RENDER_MODE_TEXTURE", sky_object::SKY_RENDER_MODE_TEXTURE);
        PyModule_AddIntConstant(module, "SKY_RENDER_MODE_MODULATE", sky_object::SKY_RENDER_MODE_MODULATE);
        PyModule_AddIntConstant(module, "SKY_RENDER_MODE_MODULATE2X", sky_object::SKY_RENDER_MODE_MODULATE2X);
        PyModule_AddIntConstant(module, "SKY_RENDER_MODE_MODULATE4X", sky_object::SKY_RENDER_MODE_MODULATE4X);

        PyModule_AddIntConstant(module, "SHADOW_NONE", python_background::SHADOW_NONE);
        PyModule_AddIntConstant(module, "SHADOW_GROUND", python_background::SHADOW_GROUND);
        PyModule_AddIntConstant(module, "SHADOW_GROUND_AND_SOLO", python_background::SHADOW_GROUND_AND_SOLO);
        PyModule_AddIntConstant(module, "SHADOW_ALL", python_background::SHADOW_ALL);
        PyModule_AddIntConstant(module, "SHADOW_ALL_HIGH", python_background::SHADOW_ALL_HIGH);
        PyModule_AddIntConstant(module, "SHADOW_ALL_MAX", python_background::SHADOW_ALL_MAX);

        PyModule_AddIntConstant(module, "DISTANCE0", python_background::DISTANCE0);
        PyModule_AddIntConstant(module, "DISTANCE1", python_background::DISTANCE1);
        PyModule_AddIntConstant(module, "DISTANCE2", python_background::DISTANCE2);
        PyModule_AddIntConstant(module, "DISTANCE3", python_background::DISTANCE3);
        PyModule_AddIntConstant(module, "DISTANCE4", python_background::DISTANCE4);

        PyModule_AddIntConstant(module, "DISTANCE_SORT", map_outdoor::DISTANCE_SORT);
        PyModule_AddIntConstant(module, "TEXTURE_SORT", map_outdoor::TEXTURE_SORT);
    }
}
