#include <format>
#include <metin2/client/app/python_system.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/terrain/map_outdoor.h>
#include <string>
#include "python_background.h"

namespace metin2::inline client::inline background {

    auto background_is_software_tiling(PyObject *self, PyObject *args) -> PyObject * {
        python_background &bg = get_python_background();
        return Py_BuildValue("i", bg.is_software_tiling_enable());
    }

    auto background_enable_software_tiling(PyObject *self, PyObject *args) -> PyObject * {
        int n_is_enable;
        if (!py_tuple_get_integer(args, 0, &n_is_enable))
            return py_bad_argument();

        bool is_enable = n_is_enable ? true : false;

        python_background &bg = get_python_background();
        bg.reserve_software_tiling_enable(is_enable);

        python_system &system = get_python_system();
        system.set_software_tiling(is_enable);
        return py_build_none();
    }

    auto background_enable_snow(PyObject *self, PyObject *args) -> PyObject * {
        int is_enable;
        if (!py_tuple_get_integer(args, 0, &is_enable))
            return py_bad_argument();

        python_background &bg = get_python_background();
        if (is_enable)
            bg.enable_snow_environment();
        else
            bg.disable_snow_environment();

        return py_build_none();
    }

    auto background_load_map(PyObject *self, PyObject *args) -> PyObject * {
        char *map_path_name;

        if (!py_tuple_get_string(args, 0, &map_path_name))
            return py_bad_argument();

        float x, y, z;

        if (!py_tuple_get_float(args, 1, &x))
            return py_bad_argument();

        if (!py_tuple_get_float(args, 2, &y))
            return py_bad_argument();

        if (!py_tuple_get_float(args, 3, &z))
            return py_bad_argument();

        python_background &bg = get_python_background();
        bg.load_map(map_path_name, x, y, z);

        return py_build_none();
    }

    auto background_destroy(PyObject *self, PyObject *args) -> PyObject * {
        python_background &bg = get_python_background();
        bg.set_shadow_level(python_background::SHADOW_NONE);
        bg.destroy();
        return py_build_none();
    }

    auto background_register_environment_data(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_bad_argument();

        char *environment_file_name;
        if (!py_tuple_get_string(args, 1, &environment_file_name))
            return py_bad_argument();

        python_background &bg = get_python_background();
        if (!bg.register_environment_data(index, environment_file_name)) {
            metin2::log::error("Cannot register environment {} from {}", index, environment_file_name);

            // TODO: fall back to the default environment.
        }

        return py_build_none();
    }

    auto background_set_environment_data(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_bad_argument();

        const environment_data *environmen_data;

        python_background &bg = get_python_background();
        if (bg.get_environment_data(index, &environmen_data))
            bg.reset_environment_data_ptr(environmen_data);

        return py_build_none();
    }

    auto background_get_current_map_name(PyObject *self, PyObject *args) -> PyObject * {
        python_background &bg = get_python_background();
        return Py_BuildValue("s", bg.warp_map_name());
    }

    auto background_get_picking_point(PyObject *self, PyObject *args) -> PyObject * {
        python_background &bg = get_python_background();
        pixel_position p_pos_picked(0.0f, 0.0f, 0.0f);
        if (bg.picking_point(&p_pos_picked)) {
            p_pos_picked.y = -p_pos_picked.y;
        }
        return Py_BuildValue("fff", p_pos_picked.x, p_pos_picked.y, p_pos_picked.z);
    }

    auto background_initialize(PyObject *self, PyObject *args) -> PyObject * {
        python_background &bg = get_python_background();
        bg.create();
        return py_build_none();
    }

    auto background_render_collision(PyObject *self, PyObject *args) -> PyObject * {
        get_python_background().render_collision();
        return py_build_none();
    }

    auto background_get_height(PyObject *self, PyObject *args) -> PyObject * {
        float fx;
        if (!py_tuple_get_float(args, 0, &fx))
            return py_bad_argument();

        float fy;
        if (!py_tuple_get_float(args, 1, &fy))
            return py_bad_argument();

        float fz = get_python_background().height(fx, fy);
        return Py_BuildValue("f", fz);
    }

    auto background_get_rendered_splat_num(PyObject *self, PyObject *args) -> PyObject * {
        int patch;
        int splat;
        float splat_ratio;

        std::vector<int> &texture_num_vector = get_python_background().rendered_splat_num(&patch, &splat, &splat_ratio);

        std::string output;
        for (int texture_num : texture_num_vector)
            output += std::format("{} ", texture_num);

        return Py_BuildValue("iifs", patch, splat, splat_ratio, output.c_str());
    }

    auto background_get_rendered_gti_num(PyObject *self, PyObject *args) -> PyObject * {
        uint32_t graphic_thing_instance_num;
        uint32_t crc_num;

        get_python_background().rendered_graphic_thing_instance_num(&graphic_thing_instance_num, &crc_num);

        return Py_BuildValue("ii", graphic_thing_instance_num, crc_num);
    }

    auto background_get_render_shadow_time(PyObject *self, PyObject *args) -> PyObject * {
        python_background &bg = get_python_background();
        return Py_BuildValue("i", bg.render_shadow_time());
    }

    auto background_set_shadow_level(PyObject *self, PyObject *args) -> PyObject * {
        int level;

        if (!py_tuple_get_integer(args, 0, &level))
            return py_bad_argument();

        python_background &bg = get_python_background();
        bg.set_shadow_level(level);
        return py_build_none();
    }

    auto background_set_visible_part(PyObject *self, PyObject *args) -> PyObject * {
        int part;
        if (!py_tuple_get_integer(args, 0, &part))
            return py_bad_argument();

        int is_visible;
        if (!py_tuple_get_integer(args, 1, &is_visible))
            return py_bad_argument();

        if (part >= map_outdoor::PART_NUM)
            return py_build_exception("ePart(%d)<background.PART_NUM(%d)", part, map_outdoor::PART_NUM);

        python_background &bg = get_python_background();
        bg.set_visible_part(part, is_visible ? true : false);

        return py_build_none();
    }

    auto background_set_spalt_limit(PyObject *self, PyObject *args) -> PyObject * {
        int splat_num;
        if (!py_tuple_get_integer(args, 0, &splat_num))
            return py_bad_argument();

        if (splat_num < 0)
            return py_build_exception("background.SetSplatLimit(iSplatNum(%d)>=0)", splat_num);

        python_background &bg = get_python_background();
        bg.set_splat_limit(splat_num);

        return py_build_none();
    }

    auto background_select_view_distance_num(PyObject *self, PyObject *args) -> PyObject * {
        int num;

        if (!py_tuple_get_integer(args, 0, &num))
            return py_bad_argument();

        python_background &bg = get_python_background();
        bg.select_view_distance_num(num);

        return py_build_none();
    }

    auto background_set_view_distance_set(PyObject *self, PyObject *args) -> PyObject * {
        int num;
        if (!py_tuple_get_integer(args, 0, &num))
            return py_bad_argument();

        float far_clip;
        if (!py_tuple_get_float(args, 1, &far_clip))
            return py_bad_argument();

        python_background &bg = get_python_background();
        bg.set_view_distance_set(num, far_clip);
        return py_build_none();
    }

    auto background_get_distance_set_info(PyObject *self, PyObject *args) -> PyObject * {
        int num;
        float start, end, far_clip;
        get_python_background().distance_set_info(&num, &start, &end, &far_clip);
        return Py_BuildValue("ifff", num, start, end, far_clip);
    }

    auto background_set_bg_loading(PyObject *self, PyObject *args) -> PyObject * {
        bool bg_loading;
        if (!py_tuple_get_boolean(args, 0, &bg_loading))
            return py_bad_argument();

        return py_build_none();
    }

    auto background_set_render_sort(PyObject *self, PyObject *args) -> PyObject * {
        int sort;
        if (!py_tuple_get_integer(args, 0, &sort))
            return py_bad_argument();

        get_python_background().set_terrain_render_sort((map_outdoor::terrain_render_sort)sort);
        return py_build_none();
    }

    auto background_set_transparent_tree(PyObject *self, PyObject *args) -> PyObject * {
        int transparent;
        if (!py_tuple_get_integer(args, 0, &transparent))
            return py_bad_argument();

        get_python_background().set_transparent_tree(transparent ? true : false);
        return py_build_none();
    }

    auto background_global_position_to_map_info(PyObject *self, PyObject *args) -> PyObject * {
        int x;
        if (!py_tuple_get_integer(args, 0, &x))
            return py_bad_argument();

        int y;
        if (!py_tuple_get_integer(args, 1, &y))
            return py_bad_argument();

        python_background &bg = get_python_background();
        python_background::map_info *map_info = bg.global_position_to_map_info(x, y);

        if (map_info)
            return Py_BuildValue("sii", map_info->name_value.c_str(), map_info->base_x, map_info->base_y);
        else
            return Py_BuildValue("sii", "", 0, 0);
    }

    auto background_warp_test(PyObject *self, PyObject *args) -> PyObject * {
        int x;
        if (!py_tuple_get_integer(args, 0, &x))
            return py_bad_argument();

        int y;
        if (!py_tuple_get_integer(args, 1, &y))
            return py_bad_argument();

        get_python_background().warp((uint32_t)x * 100, (uint32_t)y * 100);
        return py_build_none();
    }

    auto background_set_x_mas_tree(PyObject *self, PyObject *args) -> PyObject * {
        int grade;
        if (!py_tuple_get_integer(args, 0, &grade))
            return py_bad_argument();

        get_python_background().set_x_max_tree(grade);
        return py_build_none();
    }

    auto background_register_dungeon_map_name(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_bad_argument();

        get_python_background().register_dungeon_map_name(name);
        return py_build_none();
    }

    auto background_visible_guild_area(PyObject *self, PyObject *args) -> PyObject * {
        get_python_background().visible_guild_area();
        return py_build_none();
    }

    auto background_disable_guild_area(PyObject *self, PyObject *args) -> PyObject * {
        get_python_background().disable_guild_area();
        return py_build_none();
    }

    void init_world_module() {
        static PyMethodDef s_methods[] =
            {
                {"IsSoftwareTiling", background_is_software_tiling, METH_VARARGS},
                {"EnableSoftwareTiling", background_enable_software_tiling, METH_VARARGS},
                {"EnableSnow", background_enable_snow, METH_VARARGS},
                {"GlobalPositionToMapInfo", background_global_position_to_map_info, METH_VARARGS},
                {"GetRenderShadowTime", background_get_render_shadow_time, METH_VARARGS},
                {"LoadMap", background_load_map, METH_VARARGS},
                {"Destroy", background_destroy, METH_VARARGS},
                {"RegisterEnvironmentData", background_register_environment_data, METH_VARARGS},
                {"SetEnvironmentData", background_set_environment_data, METH_VARARGS},
                {"GetCurrentMapName", background_get_current_map_name, METH_VARARGS},
                {"GetPickingPoint", background_get_picking_point, METH_VARARGS},

                {"Initialize", background_initialize, METH_VARARGS},
                {"RenderCollision", background_render_collision, METH_VARARGS},
                {"GetHeight", background_get_height, METH_VARARGS},

                {"SetShadowLevel", background_set_shadow_level, METH_VARARGS},

                {"SetVisiblePart", background_set_visible_part, METH_VARARGS},
                {"SetSplatLimit", background_set_spalt_limit, METH_VARARGS},
                {"GetRenderedSplatNum", background_get_rendered_splat_num, METH_VARARGS},
                {"GetRenderedGraphicThingInstanceNum", background_get_rendered_gti_num, METH_VARARGS},
                {"SelectViewDistanceNum", background_select_view_distance_num, METH_VARARGS},
                {"SetViewDistanceSet", background_set_view_distance_set, METH_VARARGS},
                {"GetDistanceSetInfo", background_get_distance_set_info, METH_VARARGS},
                {"SetBGLoading", background_set_bg_loading, METH_VARARGS},
                {"SetRenderSort", background_set_render_sort, METH_VARARGS},
                {"SetTransparentTree", background_set_transparent_tree, METH_VARARGS},
                {"SetXMasTree", background_set_x_mas_tree, METH_VARARGS},
                {"RegisterDungeonMapName", background_register_dungeon_map_name, METH_VARARGS},

                {"VisibleGuildArea", background_visible_guild_area, METH_VARARGS},
                {"DisableGuildArea", background_disable_guild_area, METH_VARARGS},

                {"WarpTest", background_warp_test, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = init_python_module("metin2.world", s_methods);

        PyModule_AddIntConstant(module_value, "PART_SKY", map_outdoor::PART_SKY);
        PyModule_AddIntConstant(module_value, "PART_TREE", map_outdoor::PART_TREE);
        PyModule_AddIntConstant(module_value, "PART_CLOUD", map_outdoor::PART_CLOUD);
        PyModule_AddIntConstant(module_value, "PART_WATER", map_outdoor::PART_WATER);
        PyModule_AddIntConstant(module_value, "PART_OBJECT", map_outdoor::PART_OBJECT);
        PyModule_AddIntConstant(module_value, "PART_TERRAIN", map_outdoor::PART_TERRAIN);

        PyModule_AddIntConstant(module_value, "SKY_RENDER_MODE_DEFAULT", sky_object::SKY_RENDER_MODE_DEFAULT);
        PyModule_AddIntConstant(module_value, "SKY_RENDER_MODE_DIFFUSE", sky_object::SKY_RENDER_MODE_DIFFUSE);
        PyModule_AddIntConstant(module_value, "SKY_RENDER_MODE_TEXTURE", sky_object::SKY_RENDER_MODE_TEXTURE);
        PyModule_AddIntConstant(module_value, "SKY_RENDER_MODE_MODULATE", sky_object::SKY_RENDER_MODE_MODULATE);
        PyModule_AddIntConstant(module_value, "SKY_RENDER_MODE_MODULATE2X", sky_object::SKY_RENDER_MODE_MODULATE2X);
        PyModule_AddIntConstant(module_value, "SKY_RENDER_MODE_MODULATE4X", sky_object::SKY_RENDER_MODE_MODULATE4X);

        PyModule_AddIntConstant(module_value, "SHADOW_NONE", python_background::SHADOW_NONE);
        PyModule_AddIntConstant(module_value, "SHADOW_GROUND", python_background::SHADOW_GROUND);
        PyModule_AddIntConstant(module_value, "SHADOW_GROUND_AND_SOLO", python_background::SHADOW_GROUND_AND_SOLO);
        PyModule_AddIntConstant(module_value, "SHADOW_ALL", python_background::SHADOW_ALL);
        PyModule_AddIntConstant(module_value, "SHADOW_ALL_HIGH", python_background::SHADOW_ALL_HIGH);
        PyModule_AddIntConstant(module_value, "SHADOW_ALL_MAX", python_background::SHADOW_ALL_MAX);

        PyModule_AddIntConstant(module_value, "DISTANCE0", python_background::DISTANCE0);
        PyModule_AddIntConstant(module_value, "DISTANCE1", python_background::DISTANCE1);
        PyModule_AddIntConstant(module_value, "DISTANCE2", python_background::DISTANCE2);
        PyModule_AddIntConstant(module_value, "DISTANCE3", python_background::DISTANCE3);
        PyModule_AddIntConstant(module_value, "DISTANCE4", python_background::DISTANCE4);

        PyModule_AddIntConstant(module_value, "DISTANCE_SORT", map_outdoor::DISTANCE_SORT);
        PyModule_AddIntConstant(module_value, "TEXTURE_SORT", map_outdoor::TEXTURE_SORT);
    }
}
