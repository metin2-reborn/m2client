#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <metin2/ui/python_graphic.h>
#include "python_character_manager.h"
#include "python_non_player.h"

namespace metin2::inline client::inline characters {

    auto chr_race_to_job(PyObject *self, PyObject *args) -> PyObject * {
        int race;
        if (!py_tuple_get_integer(args, 0, &race))
            return py_build_exception();

        return Py_BuildValue("i", race_to_job(race));
    }

    auto chr_race_to_sex(PyObject *self, PyObject *args) -> PyObject * {
        int race;
        if (!py_tuple_get_integer(args, 0, &race))
            return py_build_exception();

        return Py_BuildValue("i", race_to_sex(race));
    }

    auto chr_destroy(PyObject *self, PyObject *args) -> PyObject * {
        get_python_character_manager().destroy();
        return py_build_none();
    }

    auto chr_update(PyObject *self, PyObject *args) -> PyObject * {
        get_python_character_manager().update();
        return py_build_none();
    }

    auto chr_deform(PyObject *self, PyObject *args) -> PyObject * {
        get_python_character_manager().deform();
        return py_build_none();
    }

    auto chr_render(PyObject *self, PyObject *args) -> PyObject * {
        get_python_character_manager().render();
        return py_build_none();
    }

    auto chr_render_collision(PyObject *self, PyObject *args) -> PyObject * {
        get_python_character_manager().render_collision();
        return py_build_none();
    }

    auto chr_create_instance(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        if (PyTuple_Size(args) == 2) {
            PyObject *dict = PyTuple_GetItem(args, 1);
            if (!PyDict_Check(dict))
                return py_build_exception();

            instance_base::create_data create_data;
            create_data.type_value = actor_instance::TYPE_PC;
            create_data.level_value = 0;
            create_data.guild_id = 0;
            create_data.empire_id = 0;
            create_data.vid = virtual_id;
            create_data.mount_vnum = 0;
            create_data.race = 0;
            create_data.rot = instance_base::DIR_NORTH;
            create_data.pos_x = 0;
            create_data.pos_y = 0;
            create_data.name_value = "NONAME";
            create_data.state_flags = 0;
            create_data.mov_spd = 100;
            create_data.atk_spd = 100;
            create_data.alignment = 0;
            create_data.pk_mode = 0;
            create_data.affect_flags.clear();
            create_data.armor = 8;
            create_data.weapon = 0;
            create_data.hair_value = 0;
            create_data.is_main = false;

            PyObject *horse = PyDict_GetItemString(dict, "horse");
            if (horse)
                create_data.mount_vnum = PyLong_AsLong(horse);

            PyObject *x = PyDict_GetItemString(dict, "x");
            if (x)
                create_data.pos_x = PyLong_AsLong(x);

            PyObject *y = PyDict_GetItemString(dict, "y");
            if (y)
                create_data.pos_x = PyLong_AsLong(y);

            get_python_character_manager().create_instance(create_data);
        } else {
            get_python_character_manager().register_instance(virtual_id);
        }

        return py_build_none();
    }

    auto chr_delete_instance(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        get_python_character_manager().delete_instance(virtual_id);
        return py_build_none();
    }

    auto chr_select_instance(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        get_python_character_manager().select_instance(virtual_id);
        return py_build_none();
    }

    auto chr_has_instance(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        bool flag = get_python_character_manager().instance_ptr(virtual_id) ? true : false;
        return Py_BuildValue("i", flag);
    }

    auto chr_is_enemy(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        instance_base *instance = get_python_character_manager().instance_ptr(virtual_id);
        if (!instance)
            return Py_BuildValue("i", 0);

        return Py_BuildValue("i", instance->is_enemy());
    }

    auto chr_is_npc(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        instance_base *instance = get_python_character_manager().instance_ptr(virtual_id);
        if (!instance)
            return Py_BuildValue("i", 0);

        return Py_BuildValue("i", instance->is_npc());
    }

    auto chr_is_party_member(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        instance_base *instance = get_python_character_manager().instance_ptr(virtual_id);
        if (!instance)
            return Py_BuildValue("i", 0);

        return Py_BuildValue("i", instance->is_party_member());
    }

    auto chr_set_blend_render_mode(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        float alpha;
        if (!py_tuple_get_float(args, 1, &alpha))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().instance_ptr(virtual_id);

        if (!inst)
            return py_build_none();

        inst->set_alpha(alpha);

        return py_build_none();
    }

    auto chr_pick(PyObject *self, PyObject *args) -> PyObject * {
        uint32_t virtual_id = 0;
        if (get_python_character_manager().old_get_picked_instance_vid(&virtual_id))
            return Py_BuildValue("i", virtual_id);
        else
            return Py_BuildValue("i", -1);
    }

    auto chr_hide(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();

        inst->hide();
        return py_build_none();
    }

    auto chr_show(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();

        inst->show();
        return py_build_none();
    }

    auto chr_set_race(PyObject *self, PyObject *args) -> PyObject * {
        int race;
        if (!py_tuple_get_integer(args, 0, &race))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();

        inst->set_race(race);

        return py_build_none();
    }

    auto chr_set_hair(PyObject *self, PyObject *args) -> PyObject * {
        int race;
        if (!py_tuple_get_integer(args, 0, &race))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();

        inst->set_hair(race);

        return py_build_none();
    }

    auto chr_set_armor(PyObject *self, PyObject *args) -> PyObject * {
        int form;
        if (!py_tuple_get_integer(args, 0, &form))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();

        inst->set_armor(form);

        inst->register_bounding_sphere();

        return py_build_none();
    }

    auto chr_change_shape(PyObject *self, PyObject *args) -> PyObject * {
        int form;
        if (!py_tuple_get_integer(args, 0, &form))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();

        inst->change_armor(form);

        return py_build_none();
    }

    auto chr_set_virtual_id(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();
        inst->set_virtual_id(vid);
        return py_build_none();
    }

    auto chr_set_name_string(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();
        inst->set_name_string(name, strlen(name));
        return py_build_none();
    }

    auto chr_set_instance_type(PyObject *self, PyObject *args) -> PyObject * {
        int instance_type;
        if (!py_tuple_get_integer(args, 0, &instance_type))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();
        inst->set_instance_type(instance_type);
        return py_build_none();
    }

    auto chr_refresh(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *inst = get_python_character_manager().selected_instance_ptr();

        if (!inst)
            return py_build_none();

        // Intentionally a no-op: the select screen has no WAIT motion loaded yet.
        return py_build_none();
    }

    auto chr_set_motion_mode(PyObject *self, PyObject *args) -> PyObject * {
        int motion_mode;
        if (!py_tuple_get_integer(args, 0, &motion_mode))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();

        inst->set_motion_mode(motion_mode);
        return py_build_none();
    }

    auto chr_set_loop_motion(PyObject *self, PyObject *args) -> PyObject * {
        int motion_index;
        if (!py_tuple_get_integer(args, 0, &motion_index))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();
        inst->set_loop_motion(uint16_t(motion_index));

        return py_build_none();
    }

    auto chr_blend_loop_motion(PyObject *self, PyObject *args) -> PyObject * {
        int motion_index;
        if (!py_tuple_get_integer(args, 0, &motion_index))
            return py_build_exception();
        float blend_time;
        if (!py_tuple_get_float(args, 1, &blend_time))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();
        inst->set_loop_motion(uint16_t(motion_index), blend_time);

        return py_build_none();
    }

    auto chr_push_once_motion(PyObject *self, PyObject *args) -> PyObject * {
        int motion_index;
        if (!py_tuple_get_integer(args, 0, &motion_index))
            return py_build_exception();

        float blend_time;
        if (!py_tuple_get_float(args, 1, &blend_time)) {
            blend_time = 0.1f;
        }

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();
        inst->push_once_motion(uint16_t(motion_index), blend_time);

        return py_build_none();
    }

    auto chr_set_pixel_position(PyObject *self, PyObject *args) -> PyObject * {
        int x;
        if (!py_tuple_get_integer(args, 0, &x))
            return py_build_exception();
        int y;
        if (!py_tuple_get_integer(args, 1, &y))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();

        int z;
        if (py_tuple_get_integer(args, 2, &z)) {
            inst->new_set_pixel_position(pixel_position(x, y, z));
        } else {
            inst->script_set_pixel_position(x, y);
        }
        return py_build_none();
    }

    auto chr_set_direction(PyObject *self, PyObject *args) -> PyObject * {
        int direction;
        if (!py_tuple_get_integer(args, 0, &direction))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        inst->set_direction(direction);
        return py_build_none();
    }

    auto chr_set_rotation(PyObject *self, PyObject *args) -> PyObject * {
        float rotation;
        if (!py_tuple_get_float(args, 0, &rotation))
            return py_build_exception();

        instance_base *character_instance = get_python_character_manager().selected_instance_ptr();

        if (!character_instance)
            return py_build_none();

        rotation = fmod(rotation + 180.0f, 360.0f);

        character_instance->set_rotation(rotation);

        return py_build_none();
    }

    auto chr_set_rotation_all(PyObject *self, PyObject *args) -> PyObject * {
        float rot_x;
        if (!py_tuple_get_float(args, 0, &rot_x))
            return py_build_exception();
        float rot_y;
        if (!py_tuple_get_float(args, 1, &rot_y))
            return py_build_exception();
        float rot_z;
        if (!py_tuple_get_float(args, 2, &rot_z))
            return py_build_exception();

        instance_base *character_instance = get_python_character_manager().selected_instance_ptr();
        if (!character_instance)
            return py_build_none();

        character_instance->graphic_thing_instance_ref().set_xy_rotation(rot_x, rot_y);
        character_instance->graphic_thing_instance_ref().set_rotation(rot_z);
        return py_build_none();
    }

    auto chr_get_race(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *character_instance = get_python_character_manager().selected_instance_ptr();

        if (!character_instance)
            return Py_BuildValue("i", 0);

        return Py_BuildValue("i", character_instance->race());
    }

    auto chr_get_name_by_vid(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        instance_base *instance = get_python_character_manager().instance_ptr(virtual_id);

        if (!instance)
            return Py_BuildValue("s", "None");

        return Py_BuildValue("s", instance->name_string());
    }

    auto chr_get_guild_id(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        instance_base *instance = get_python_character_manager().instance_ptr(virtual_id);

        if (!instance)
            return Py_BuildValue("i", 0);

        return Py_BuildValue("i", instance->guild_id());
    }

    auto chr_get_project_position(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();
        int height;
        if (!py_tuple_get_integer(args, 1, &height))
            return py_build_exception();

        instance_base *instance = get_python_character_manager().instance_ptr(virtual_id);

        if (!instance)
            return Py_BuildValue("ii", -100, -100);

        pixel_position pixel_position;
        instance->new_get_pixel_position(&pixel_position);

        python_graphic &rpy_graphic = get_python_graphic();

        float fx, fy, fz;
        rpy_graphic.project_position(pixel_position.x,
            -pixel_position.y,
            pixel_position.z + float(height),
            &fx, &fy, &fz);

        if (1 == int(fz))
            return Py_BuildValue("ii", -100, -100);

        return Py_BuildValue("ii", int(fx), int(fy));
    }

    auto chr_get_instance_type(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().instance_ptr(virtual_id);

        if (nullptr != inst)
            return Py_BuildValue("i", inst->instance_type());

        return Py_BuildValue("i", actor_instance::TYPE_PC);
    }

    auto chr_get_bound_box_only_xy(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *inst = get_python_character_manager().selected_instance_ptr();

        if (!inst)
            return Py_BuildValue("ffff", 0.0f, 0.0f, 0.0f, 0.0f);

        math::vector3 min_value, max_value;
        inst->get_bound_box(&min_value, &max_value);

        return Py_BuildValue("ffff", min_value.x, min_value.y, max_value.x, max_value.y);
    }

    auto chrtest_get_pk_data(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *inst = get_python_character_manager().selected_instance_ptr();

        if (!inst)
            return Py_BuildValue("ii", 0, 4);

        return Py_BuildValue("ii", inst->alignment(), inst->alignment_grade());
    }

    auto chrtest_set_combo_type(PyObject *self, PyObject *args) -> PyObject * {
        int combo_type;
        if (!py_tuple_get_integer(args, 0, &combo_type))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().selected_instance_ptr();
        if (!inst)
            return py_build_none();

        inst->graphic_thing_instance_ref().set_combo_type(combo_type);

        return py_build_none();
    }

    auto chrtest_set_add_render_mode(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();
        int color;
        if (!py_tuple_get_integer(args, 1, &color))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().instance_ptr(virtual_id);
        if (inst) {
            inst->set_add_render_mode();
            inst->set_add_color(0xff000000 | color);
        }

        return py_build_none();
    }

    auto chrtest_set_modulate_render_mode(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();
        int color;
        if (!py_tuple_get_integer(args, 1, &color))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().instance_ptr(virtual_id);
        if (inst) {
            inst->set_modulate_render_mode();
            inst->set_add_color(0xff000000 | color);
        }

        return py_build_none();
    }

    auto chrtest_set_add_render_mode_rgb(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();
        float fr;
        if (!py_tuple_get_float(args, 1, &fr))
            return py_build_exception();
        float fg;
        if (!py_tuple_get_float(args, 2, &fg))
            return py_build_exception();
        float fb;
        if (!py_tuple_get_float(args, 3, &fb))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().instance_ptr(virtual_id);
        if (inst) {
            inst->set_add_render_mode();
            inst->set_add_color(math::color(fr, fg, fb, 1.0f));
        }

        return py_build_none();
    }

    auto chrtest_set_modulate_render_mode_rgb(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();
        float fr;
        if (!py_tuple_get_float(args, 1, &fr))
            return py_build_exception();
        float fg;
        if (!py_tuple_get_float(args, 2, &fg))
            return py_build_exception();
        float fb;
        if (!py_tuple_get_float(args, 3, &fb))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().instance_ptr(virtual_id);
        if (inst) {
            inst->set_modulate_render_mode();
            inst->set_add_color(math::color(fr, fg, fb, 1.0f));
        }

        return py_build_none();
    }

    auto chrtest_set_specular_render_mode(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();
        int part;
        if (!py_tuple_get_integer(args, 1, &part))
            return py_build_exception();
        float alpha;
        if (!py_tuple_get_float(args, 2, &alpha))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().instance_ptr(virtual_id);
        if (inst) {
            inst->graphic_thing_instance_ref().set_specular_info(true, part, alpha);
        }

        return py_build_none();
    }

    auto chrtest_restore_render_mode(PyObject *self, PyObject *args) -> PyObject * {
        int virtual_id;
        if (!py_tuple_get_integer(args, 0, &virtual_id))
            return py_build_exception();

        instance_base *inst = get_python_character_manager().instance_ptr(virtual_id);
        if (inst) {
            inst->restore_render_mode();
        }

        return py_build_none();
    }

    auto chr_weapon_trace_set_texture(PyObject *self, PyObject *args) -> PyObject * {
        char *path_name;
        if (!py_tuple_get_string(args, 0, &path_name))
            return py_bad_argument();

        instance_base *instance = get_python_character_manager().main_instance_ptr();
        if (instance) {
            instance->graphic_thing_instance_ref().set_weapon_trace_texture(path_name);
        }
        return py_build_none();
    }

    auto chr_weapon_trace_use_texture(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *instance = get_python_character_manager().main_instance_ptr();
        if (instance) {
            instance->graphic_thing_instance_ref().use_texture_weapon_trace();
        }
        return py_build_none();
    }

    auto chr_weapon_trace_use_alpha(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *instance = get_python_character_manager().main_instance_ptr();
        if (instance) {
            instance->graphic_thing_instance_ref().use_alpha_weapon_trace();
        }
        return py_build_none();
    }

    void init_characters_module() {
        static PyMethodDef s_methods[] =
            {

                {"Destroy", chr_destroy, METH_VARARGS},
                {"Update", chr_update, METH_VARARGS},
                {"Deform", chr_deform, METH_VARARGS},
                {"Render", chr_render, METH_VARARGS},
                {"RenderCollision", chr_render_collision, METH_VARARGS},

                {"CreateInstance", chr_create_instance, METH_VARARGS},
                {"DeleteInstance", chr_delete_instance, METH_VARARGS},
                {"SelectInstance", chr_select_instance, METH_VARARGS},

                {"HasInstance", chr_has_instance, METH_VARARGS},
                {"IsEnemy", chr_is_enemy, METH_VARARGS},
                {"IsNPC", chr_is_npc, METH_VARARGS},
                {"IsPartyMember", chr_is_party_member, METH_VARARGS},

                {"SetBlendRenderMode", chr_set_blend_render_mode, METH_VARARGS},

                {"Hide", chr_hide, METH_VARARGS},
                {"Show", chr_show, METH_VARARGS},
                {"Pick", chr_pick, METH_VARARGS},

                {"SetArmor", chr_set_armor, METH_VARARGS},
                {"ChangeShape", chr_change_shape, METH_VARARGS},
                {"SetRace", chr_set_race, METH_VARARGS},
                {"SetHair", chr_set_hair, METH_VARARGS},
                {"SetVirtualID", chr_set_virtual_id, METH_VARARGS},
                {"SetNameString", chr_set_name_string, METH_VARARGS},
                {"SetInstanceType", chr_set_instance_type, METH_VARARGS},

                {"SetPixelPosition", chr_set_pixel_position, METH_VARARGS},
                {"SetDirection", chr_set_direction, METH_VARARGS},
                {"Refresh", chr_refresh, METH_VARARGS},

                {"SetMotionMode", chr_set_motion_mode, METH_VARARGS},
                {"SetLoopMotion", chr_set_loop_motion, METH_VARARGS},
                {"BlendLoopMotion", chr_blend_loop_motion, METH_VARARGS},
                {"PushOnceMotion", chr_push_once_motion, METH_VARARGS},
                {"SetRotation", chr_set_rotation, METH_VARARGS},
                {"SetRotationAll", chr_set_rotation_all, METH_VARARGS},
                {"GetRace", chr_get_race, METH_VARARGS},
                {"GetNameByVID", chr_get_name_by_vid, METH_VARARGS},
                {"GetGuildID", chr_get_guild_id, METH_VARARGS},
                {"GetProjectPosition", chr_get_project_position, METH_VARARGS},

                {"GetInstanceType", chr_get_instance_type, METH_VARARGS},

                {"GetBoundBoxOnlyXY", chr_get_bound_box_only_xy, METH_VARARGS},

                {"RaceToJob", chr_race_to_job, METH_VARARGS},
                {"RaceToSex", chr_race_to_sex, METH_VARARGS},

                {"testGetPKData", chrtest_get_pk_data, METH_VARARGS},
                {"WeaponTraceSetTexture", chr_weapon_trace_set_texture, METH_VARARGS},
                {"WeaponTraceUseAlpha", chr_weapon_trace_use_alpha, METH_VARARGS},
                {"WeaponTraceUseTexture", chr_weapon_trace_use_texture, METH_VARARGS},
                {"testSetComboType", chrtest_set_combo_type, METH_VARARGS},
                {"testSetAddRenderMode", chrtest_set_add_render_mode, METH_VARARGS},
                {"testSetModulateRenderMode", chrtest_set_modulate_render_mode, METH_VARARGS},
                {"testSetAddRenderModeRGB", chrtest_set_add_render_mode_rgb, METH_VARARGS},
                {"testSetModulateRenderModeRGB", chrtest_set_modulate_render_mode_rgb, METH_VARARGS},
                {"testSetSpecularRenderMode", chrtest_set_specular_render_mode, METH_VARARGS},
                {"testRestoreRenderMode", chrtest_restore_render_mode, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = init_python_module("metin2.characters", s_methods);

        PyModule_AddIntConstant(module_value, "PLAYER_NAME_MAX_LEN", PLAYER_NAME_MAX_LEN);

        PyModule_AddIntConstant(module_value, "MOTION_NONE", race_motion_data::NAME_NONE);

        PyModule_AddIntConstant(module_value, "MOTION_SPAWN", race_motion_data::NAME_SPAWN);
        PyModule_AddIntConstant(module_value, "MOTION_WAIT", race_motion_data::NAME_WAIT);
        PyModule_AddIntConstant(module_value, "MOTION_WALK", race_motion_data::NAME_WALK);
        PyModule_AddIntConstant(module_value, "MOTION_RUN", race_motion_data::NAME_RUN);
        PyModule_AddIntConstant(module_value, "MOTION_CHANGE_WEAPON", race_motion_data::NAME_CHANGE_WEAPON);
        PyModule_AddIntConstant(module_value, "MOTION_DAMAGE", race_motion_data::NAME_DAMAGE);
        PyModule_AddIntConstant(module_value, "MOTION_DAMAGE_FLYING", race_motion_data::NAME_DAMAGE_FLYING);
        PyModule_AddIntConstant(module_value, "MOTION_STAND_UP", race_motion_data::NAME_STAND_UP);
        PyModule_AddIntConstant(module_value, "MOTION_DAMAGE_BACK", race_motion_data::NAME_DAMAGE_BACK);
        PyModule_AddIntConstant(module_value, "MOTION_DAMAGE_FLYING_BACK", race_motion_data::NAME_DAMAGE_FLYING_BACK);
        PyModule_AddIntConstant(module_value, "MOTION_STAND_UP_BACK", race_motion_data::NAME_STAND_UP_BACK);
        PyModule_AddIntConstant(module_value, "MOTION_DEAD", race_motion_data::NAME_DEAD);
        PyModule_AddIntConstant(module_value, "MOTION_DEAD_BACK", race_motion_data::NAME_DEAD_BACK);
        PyModule_AddIntConstant(module_value, "MOTION_NORMAL_ATTACK", race_motion_data::NAME_NORMAL_ATTACK);
        PyModule_AddIntConstant(module_value, "MOTION_COMBO_ATTACK_1", race_motion_data::NAME_COMBO_ATTACK_1);
        PyModule_AddIntConstant(module_value, "MOTION_COMBO_ATTACK_2", race_motion_data::NAME_COMBO_ATTACK_2);
        PyModule_AddIntConstant(module_value, "MOTION_COMBO_ATTACK_3", race_motion_data::NAME_COMBO_ATTACK_3);
        PyModule_AddIntConstant(module_value, "MOTION_COMBO_ATTACK_4", race_motion_data::NAME_COMBO_ATTACK_4);
        PyModule_AddIntConstant(module_value, "MOTION_COMBO_ATTACK_5", race_motion_data::NAME_COMBO_ATTACK_5);
        PyModule_AddIntConstant(module_value, "MOTION_COMBO_ATTACK_6", race_motion_data::NAME_COMBO_ATTACK_6);
        PyModule_AddIntConstant(module_value, "MOTION_COMBO_ATTACK_7", race_motion_data::NAME_COMBO_ATTACK_7);
        PyModule_AddIntConstant(module_value, "MOTION_COMBO_ATTACK_8", race_motion_data::NAME_COMBO_ATTACK_8);
        PyModule_AddIntConstant(module_value, "MOTION_INTRO_WAIT", race_motion_data::NAME_INTRO_WAIT);
        PyModule_AddIntConstant(module_value, "MOTION_INTRO_SELECTED", race_motion_data::NAME_INTRO_SELECTED);
        PyModule_AddIntConstant(module_value, "MOTION_INTRO_NOT_SELECTED", race_motion_data::NAME_INTRO_NOT_SELECTED);
        PyModule_AddIntConstant(module_value, "MOTION_FISHING_THROW", race_motion_data::NAME_FISHING_THROW);
        PyModule_AddIntConstant(module_value, "MOTION_FISHING_WAIT", race_motion_data::NAME_FISHING_WAIT);
        PyModule_AddIntConstant(module_value, "MOTION_FISHING_STOP", race_motion_data::NAME_FISHING_STOP);
        PyModule_AddIntConstant(module_value, "MOTION_FISHING_REACT", race_motion_data::NAME_FISHING_REACT);
        PyModule_AddIntConstant(module_value, "MOTION_FISHING_CATCH", race_motion_data::NAME_FISHING_CATCH);
        PyModule_AddIntConstant(module_value, "MOTION_FISHING_FAIL", race_motion_data::NAME_FISHING_FAIL);
        PyModule_AddIntConstant(module_value, "MOTION_STOP", race_motion_data::NAME_STOP);
        PyModule_AddIntConstant(module_value, "MOTION_SKILL", race_motion_data::NAME_SKILL);
        PyModule_AddIntConstant(module_value, "MOTION_CLAP", race_motion_data::NAME_CLAP);
        PyModule_AddIntConstant(module_value, "MOTION_DANCE_1", race_motion_data::NAME_DANCE_1);
        PyModule_AddIntConstant(module_value, "MOTION_DANCE_2", race_motion_data::NAME_DANCE_2);

        PyModule_AddIntConstant(module_value, "MOTION_DANCE_3", race_motion_data::NAME_DANCE_3);
        PyModule_AddIntConstant(module_value, "MOTION_DANCE_4", race_motion_data::NAME_DANCE_4);
        PyModule_AddIntConstant(module_value, "MOTION_DANCE_5", race_motion_data::NAME_DANCE_5);
        PyModule_AddIntConstant(module_value, "MOTION_DANCE_6", race_motion_data::NAME_DANCE_6);
        PyModule_AddIntConstant(module_value, "MOTION_CONGRATULATION", race_motion_data::NAME_CONGRATULATION);
        PyModule_AddIntConstant(module_value, "MOTION_FORGIVE", race_motion_data::NAME_FORGIVE);
        PyModule_AddIntConstant(module_value, "MOTION_ANGRY", race_motion_data::NAME_ANGRY);
        PyModule_AddIntConstant(module_value, "MOTION_ATTRACTIVE", race_motion_data::NAME_ATTRACTIVE);
        PyModule_AddIntConstant(module_value, "MOTION_SAD", race_motion_data::NAME_SAD);
        PyModule_AddIntConstant(module_value, "MOTION_SHY", race_motion_data::NAME_SHY);
        PyModule_AddIntConstant(module_value, "MOTION_CHEERUP", race_motion_data::NAME_CHEERUP);
        PyModule_AddIntConstant(module_value, "MOTION_BANTER", race_motion_data::NAME_BANTER);
        PyModule_AddIntConstant(module_value, "MOTION_JOY", race_motion_data::NAME_JOY);

        PyModule_AddIntConstant(module_value, "MOTION_CHEERS_1", race_motion_data::NAME_CHEERS_1);
        PyModule_AddIntConstant(module_value, "MOTION_CHEERS_2", race_motion_data::NAME_CHEERS_2);
        PyModule_AddIntConstant(module_value, "MOTION_KISS_WITH_WARRIOR", race_motion_data::NAME_KISS_WITH_WARRIOR);
        PyModule_AddIntConstant(module_value, "MOTION_KISS_WITH_ASSASSIN", race_motion_data::NAME_KISS_WITH_ASSASSIN);
        PyModule_AddIntConstant(module_value, "MOTION_KISS_WITH_SURA", race_motion_data::NAME_KISS_WITH_SURA);
        PyModule_AddIntConstant(module_value, "MOTION_KISS_WITH_SHAMAN", race_motion_data::NAME_KISS_WITH_SHAMAN);
        PyModule_AddIntConstant(module_value, "MOTION_FRENCH_KISS_WITH_WARRIOR", race_motion_data::NAME_FRENCH_KISS_WITH_WARRIOR);
        PyModule_AddIntConstant(module_value, "MOTION_FRENCH_KISS_WITH_ASSASSIN", race_motion_data::NAME_FRENCH_KISS_WITH_ASSASSIN);
        PyModule_AddIntConstant(module_value, "MOTION_FRENCH_KISS_WITH_SURA", race_motion_data::NAME_FRENCH_KISS_WITH_SURA);
        PyModule_AddIntConstant(module_value, "MOTION_FRENCH_KISS_WITH_SHAMAN", race_motion_data::NAME_FRENCH_KISS_WITH_SHAMAN);
        PyModule_AddIntConstant(module_value, "MOTION_SLAP_HIT_WITH_WARRIOR", race_motion_data::NAME_SLAP_HIT_WITH_WARRIOR);
        PyModule_AddIntConstant(module_value, "MOTION_SLAP_HIT_WITH_ASSASSIN", race_motion_data::NAME_SLAP_HIT_WITH_ASSASSIN);
        PyModule_AddIntConstant(module_value, "MOTION_SLAP_HIT_WITH_SURA", race_motion_data::NAME_SLAP_HIT_WITH_SURA);
        PyModule_AddIntConstant(module_value, "MOTION_SLAP_HIT_WITH_SHAMAN", race_motion_data::NAME_SLAP_HIT_WITH_SHAMAN);
        PyModule_AddIntConstant(module_value, "MOTION_SLAP_HURT_WITH_WARRIOR", race_motion_data::NAME_SLAP_HURT_WITH_WARRIOR);
        PyModule_AddIntConstant(module_value, "MOTION_SLAP_HURT_WITH_ASSASSIN", race_motion_data::NAME_SLAP_HURT_WITH_ASSASSIN);
        PyModule_AddIntConstant(module_value, "MOTION_SLAP_HURT_WITH_SURA", race_motion_data::NAME_SLAP_HURT_WITH_SURA);
        PyModule_AddIntConstant(module_value, "MOTION_SLAP_HURT_WITH_SHAMAN", race_motion_data::NAME_SLAP_HURT_WITH_SHAMAN);
        PyModule_AddIntConstant(module_value, "MOTION_DIG", race_motion_data::NAME_DIG);

        PyModule_AddIntConstant(module_value, "MOTION_MODE_RESERVED", race_motion_data::MODE_RESERVED);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_GENERAL", race_motion_data::MODE_GENERAL);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_ONEHAND_SWORD", race_motion_data::MODE_ONEHAND_SWORD);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_TWOHAND_SWORD", race_motion_data::MODE_TWOHAND_SWORD);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_DUALHAND_SWORD", race_motion_data::MODE_DUALHAND_SWORD);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_BOW", race_motion_data::MODE_BOW);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_FAN", race_motion_data::MODE_FAN);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_BELL", race_motion_data::MODE_BELL);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_FISHING", race_motion_data::MODE_FISHING);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_HORSE", race_motion_data::MODE_HORSE);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_HORSE_ONEHAND_SWORD", race_motion_data::MODE_HORSE_ONEHAND_SWORD);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_HORSE_TWOHAND_SWORD", race_motion_data::MODE_HORSE_TWOHAND_SWORD);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_HORSE_DUALHAND_SWORD", race_motion_data::MODE_HORSE_DUALHAND_SWORD);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_HORSE_BOW", race_motion_data::MODE_HORSE_BOW);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_HORSE_FAN", race_motion_data::MODE_HORSE_FAN);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_HORSE_BELL", race_motion_data::MODE_HORSE_BELL);
        PyModule_AddIntConstant(module_value, "MOTION_MODE_WEDDING_DRESS", race_motion_data::MODE_WEDDING_DRESS);

        PyModule_AddIntConstant(module_value, "DIR_NORTH", instance_base::DIR_NORTH);
        PyModule_AddIntConstant(module_value, "DIR_NORTHEAST", instance_base::DIR_NORTHEAST);
        PyModule_AddIntConstant(module_value, "DIR_EAST", instance_base::DIR_EAST);
        PyModule_AddIntConstant(module_value, "DIR_SOUTHEAST", instance_base::DIR_SOUTHEAST);
        PyModule_AddIntConstant(module_value, "DIR_SOUTH", instance_base::DIR_SOUTH);
        PyModule_AddIntConstant(module_value, "DIR_SOUTHWEST", instance_base::DIR_SOUTHWEST);
        PyModule_AddIntConstant(module_value, "DIR_WEST", instance_base::DIR_WEST);
        PyModule_AddIntConstant(module_value, "DIR_NORTHWEST", instance_base::DIR_NORTHWEST);

        PyModule_AddIntConstant(module_value, "INSTANCE_TYPE_PLAYER", actor_instance::TYPE_PC);
        PyModule_AddIntConstant(module_value, "INSTANCE_TYPE_NPC", actor_instance::TYPE_NPC);
        PyModule_AddIntConstant(module_value, "INSTANCE_TYPE_ENEMY", actor_instance::TYPE_ENEMY);
        PyModule_AddIntConstant(module_value, "INSTANCE_TYPE_BUILDING", actor_instance::TYPE_BUILDING);
        PyModule_AddIntConstant(module_value, "INSTANCE_TYPE_OBJECT", actor_instance::TYPE_OBJECT);

        PyModule_AddIntConstant(module_value, "PART_WEAPON", race_data::PART_WEAPON);
        PyModule_AddIntConstant(module_value, "PART_HEAD", race_data::PART_HEAD);
        PyModule_AddIntConstant(module_value, "PART_WEAPON_LEFT", race_data::PART_WEAPON_LEFT);

        PyModule_AddIntConstant(module_value, "AFFECT_POISON", instance_base::AFFECT_POISON);
        PyModule_AddIntConstant(module_value, "AFFECT_SLOW", instance_base::AFFECT_SLOW);
        PyModule_AddIntConstant(module_value, "AFFECT_STUN", instance_base::AFFECT_STUN);
        PyModule_AddIntConstant(module_value, "AFFECT_MOV_SPEED_POTION", instance_base::AFFECT_MOV_SPEED_POTION);
        PyModule_AddIntConstant(module_value, "AFFECT_ATT_SPEED_POTION", instance_base::AFFECT_ATT_SPEED_POTION);
        PyModule_AddIntConstant(module_value, "AFFECT_FISH_MIND", instance_base::AFFECT_FISH_MIND);

        PyModule_AddIntConstant(module_value, "AFFECT_JEONGWI", instance_base::AFFECT_JEONGWI);
        PyModule_AddIntConstant(module_value, "AFFECT_GEOMGYEONG", instance_base::AFFECT_GEOMGYEONG);
        PyModule_AddIntConstant(module_value, "AFFECT_CHEONGEUN", instance_base::AFFECT_CHEONGEUN);
        PyModule_AddIntConstant(module_value, "AFFECT_GYEONGGONG", instance_base::AFFECT_GYEONGGONG);
        PyModule_AddIntConstant(module_value, "AFFECT_EUNHYEONG", instance_base::AFFECT_EUNHYEONG);
        PyModule_AddIntConstant(module_value, "AFFECT_GWIGEOM", instance_base::AFFECT_GWIGEOM);
        PyModule_AddIntConstant(module_value, "AFFECT_GONGPO", instance_base::AFFECT_GONGPO);
        PyModule_AddIntConstant(module_value, "AFFECT_JUMAGAP", instance_base::AFFECT_JUMAGAP);
        PyModule_AddIntConstant(module_value, "AFFECT_HOSIN", instance_base::AFFECT_HOSIN);
        PyModule_AddIntConstant(module_value, "AFFECT_BOHO", instance_base::AFFECT_BOHO);
        PyModule_AddIntConstant(module_value, "AFFECT_KWAESOK", instance_base::AFFECT_KWAESOK);
        PyModule_AddIntConstant(module_value, "AFFECT_HEUKSIN", instance_base::AFFECT_HEUKSIN);
        PyModule_AddIntConstant(module_value, "AFFECT_MUYEONG", instance_base::AFFECT_MUYEONG);
        PyModule_AddIntConstant(module_value, "AFFECT_GICHEON", instance_base::AFFECT_GICHEON);
        PyModule_AddIntConstant(module_value, "AFFECT_JEUNGRYEOK", instance_base::AFFECT_JEUNGRYEOK);
        PyModule_AddIntConstant(module_value, "AFFECT_PABEOP", instance_base::AFFECT_PABEOP);
        PyModule_AddIntConstant(module_value, "AFFECT_FALLEN_CHEONGEUN", instance_base::AFFECT_FALLEN_CHEONGEUN);
        PyModule_AddIntConstant(module_value, "AFFECT_CHINA_FIREWORK", instance_base::AFFECT_CHINA_FIREWORK);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_MALL", instance_base::NEW_AFFECT_MALL);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_NO_DEATH_PENALTY", instance_base::NEW_AFFECT_NO_DEATH_PENALTY);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_SKILL_BOOK_BONUS", instance_base::NEW_AFFECT_SKILL_BOOK_BONUS);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_SKILL_BOOK_NO_DELAY", instance_base::NEW_AFFECT_SKILL_BOOK_NO_DELAY);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_EXP_BONUS", instance_base::NEW_AFFECT_EXP_BONUS);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_EXP_BONUS_EURO_FREE", instance_base::NEW_AFFECT_EXP_BONUS_EURO_FREE);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_EXP_BONUS_EURO_FREE_UNDER_15", instance_base::NEW_AFFECT_EXP_BONUS_EURO_FREE_UNDER_15);

        PyModule_AddIntConstant(module_value, "NEW_AFFECT_ITEM_BONUS", instance_base::NEW_AFFECT_ITEM_BONUS);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_SAFEBOX", instance_base::NEW_AFFECT_SAFEBOX);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_AUTOLOOT", instance_base::NEW_AFFECT_AUTOLOOT);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_FISH_MIND", instance_base::NEW_AFFECT_FISH_MIND);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_MARRIAGE_FAST", instance_base::NEW_AFFECT_MARRIAGE_FAST);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_GOLD_BONUS", instance_base::NEW_AFFECT_GOLD_BONUS);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_AUTO_HP_RECOVERY", instance_base::NEW_AFFECT_AUTO_HP_RECOVERY);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_AUTO_SP_RECOVERY", instance_base::NEW_AFFECT_AUTO_SP_RECOVERY);

        PyModule_AddIntConstant(module_value, "NEW_AFFECT_DRAGON_SOUL_QUALIFIED", instance_base::NEW_AFFECT_DRAGON_SOUL_QUALIFIED);

        PyModule_AddIntConstant(module_value, "NEW_AFFECT_DRAGON_SOUL_DECK1", instance_base::NEW_AFFECT_DRAGON_SOUL_DECK1);
        PyModule_AddIntConstant(module_value, "NEW_AFFECT_DRAGON_SOUL_DECK2", instance_base::NEW_AFFECT_DRAGON_SOUL_DECK2);
    }
}
