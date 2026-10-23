#include <algorithm>
#include <metin2/client/background/python_background.h>
#include <metin2/core/utils.h>
#include <metin2/log/log.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/actors/race_manager.h>
#include "instance_base.h"
#include "python_character_manager.h"

namespace metin2::inline client::inline characters {
    auto chrmgr_set_empire_name_mode(PyObject *self, PyObject *args) -> PyObject * {
        int enable;
        if (!PyArg_ParseTuple(args, "i", &enable)) {
            return nullptr;
        }

        instance_base::set_empire_name_mode(enable ? true : false);
        get_python_character_manager().refresh_all_pc_text_tail();

        Py_RETURN_NONE;
    }

    auto chrmgr_register_title_name(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        char *title_name;

        if (!PyArg_ParseTuple(args, "is", &index, &title_name)) {
            return nullptr;
        }

        instance_base::register_title_name(index, title_name);
        Py_RETURN_NONE;
    }

    auto chrmgr_register_name_color(PyObject *self, PyObject *args) -> PyObject * {
        int index, ir, ig, ib;
        if (!PyArg_ParseTuple(args, "iiii", &index, &ir, &ig, &ib)) {
            return nullptr;
        }

        instance_base::register_name_color(index, ir, ig, ib);
        Py_RETURN_NONE;
    }

    auto chrmgr_register_title_color(PyObject *self, PyObject *args) -> PyObject * {
        int index, ir, ig, ib;
        if (!PyArg_ParseTuple(args, "iiii", &index, &ir, &ig, &ib)) {
            return nullptr;
        }

        instance_base::register_title_color(index, ir, ig, ib);
        Py_RETURN_NONE;
    }

    auto chrmgr_get_picked_vid(PyObject *self, PyObject *args) -> PyObject * {
        python_character_manager &chr_mgr = get_python_character_manager();

        uint32_t picked_actor_id;
        if (chr_mgr.old_get_picked_instance_vid(&picked_actor_id))
            return Py_BuildValue("i", picked_actor_id);
        else
            return Py_BuildValue("i", -1);
    }

    auto chrmgr_get_vid_info(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!PyArg_ParseTuple(args, "i", &vid)) {
            return nullptr;
        }

        python_character_manager &chr_mgr = get_python_character_manager();

        char detail[256] = "";
        instance_base *inst_base = chr_mgr.instance_ptr(vid);
        if (inst_base) {
            pixel_position p_pos_inst;
            inst_base->new_get_pixel_position(&p_pos_inst);

            int32_t x_inst = p_pos_inst.x;
            int32_t y_inst = p_pos_inst.y;

            python_background &bg = get_python_background();
            bg.local_position_to_global_position(x_inst, y_inst);
            snprintf(detail, sizeof(detail), "pos=(%d, %d)", x_inst, y_inst);
        }

        char info[1024];
        snprintf(info, sizeof(info), "VID %d (isRegistered=%d, isAlive=%d, isDead=%d) %s",
            vid,
            chr_mgr.is_registered_vid(vid),
            chr_mgr.is_alive_vid(vid),
            chr_mgr.is_dead_vid(vid),
            detail);

        return Py_BuildValue("s", info);
    }

    auto chrmgr_set_path_name(PyObject *self, PyObject *args) -> PyObject * {
        char *path_name;
        if (!PyArg_ParseTuple(args, "s", &path_name)) {
            return nullptr;
        }

        get_race_manager().set_path_name(path_name);
        Py_RETURN_NONE;
    }

    auto chrmgr_create_race(PyObject *self, PyObject *args) -> PyObject * {
        int race;
        if (!PyArg_ParseTuple(args, "i", &race)) {
            return nullptr;
        }

        get_race_manager().create_race(race);
        Py_RETURN_NONE;
    }

    auto chrmgr_select_race(PyObject *self, PyObject *args) -> PyObject * {
        int race;
        if (!PyArg_ParseTuple(args, "i", &race)) {
            return nullptr;
        }

        get_race_manager().select_race(race);
        Py_RETURN_NONE;
    }

    auto chrmgr_register_attaching_bone_name(PyObject *self, PyObject *args) -> PyObject * {
        int part_index;
        char *bone_name;

        if (!PyArg_ParseTuple(args, "is", &part_index, &bone_name)) {
            return nullptr;
        }

        race_data *race_data = get_race_manager().selected_race_data_pointer();
        if (!race_data) {
            PyErr_SetString(PyExc_RuntimeError, "No race selected");
            return nullptr;
        }

        race_data->register_attaching_bone_name(part_index, bone_name);
        Py_RETURN_NONE;
    }

    auto chrmgr_register_motion_mode(PyObject *self, PyObject *args) -> PyObject * {
        int motion_index;
        if (!PyArg_ParseTuple(args, "i", &motion_index)) {
            return nullptr;
        }

        race_data *race_data = get_race_manager().selected_race_data_pointer();
        if (!race_data) {
            PyErr_SetString(PyExc_RuntimeError, "No race selected");
            return nullptr;
        }

        race_data->register_motion_mode(motion_index);
        Py_RETURN_NONE;
    }

    auto chrmgr_set_motion_random_weight(PyObject *self, PyObject *args) -> PyObject * {
        int mode, motion, sub_motion, percentage;
        if (!PyArg_ParseTuple(args, "iiii", &mode, &motion, &sub_motion, &percentage)) {
            return nullptr;
        }

        race_data *race_data = get_race_manager().selected_race_data_pointer();
        if (!race_data) {
            PyErr_SetString(PyExc_RuntimeError, "No race selected");
            return nullptr;
        }

        if (!race_data->set_motion_random_weight(mode, motion, sub_motion, percentage)) {
            PyErr_SetString(PyExc_ValueError, "Cannot set the motion random weight");
            return nullptr;
        }

        Py_RETURN_NONE;
    }

    auto chrmgr_register_normal_attack(PyObject *self, PyObject *args) -> PyObject * {
        int mode, motion;
        if (!PyArg_ParseTuple(args, "ii", &mode, &motion)) {
            return nullptr;
        }

        race_data *race_data = get_race_manager().selected_race_data_pointer();
        if (!race_data) {
            PyErr_SetString(PyExc_RuntimeError, "No race selected");
            return nullptr;
        }

        race_data->register_normal_attack(mode, motion);
        Py_RETURN_NONE;
    }

    auto chrmgr_reserve_combo_attack_new(PyObject *self, PyObject *args) -> PyObject * {
        int motion_mode, combo_type, count;
        if (!PyArg_ParseTuple(args, "iii", &motion_mode, &combo_type, &count)) {
            return nullptr;
        }

        race_data *race_data = get_race_manager().selected_race_data_pointer();
        if (!race_data) {
            PyErr_SetString(PyExc_RuntimeError, "No race selected");
            return nullptr;
        }

        race_data->reserve_combo_attack(motion_mode, combo_type, count);
        Py_RETURN_NONE;
    }

    auto chrmgr_register_combo_attack_new(PyObject *self, PyObject *args) -> PyObject * {
        int motion_mode, combo_type, combo_index, motion_index;
        if (!PyArg_ParseTuple(args, "iiii", &motion_mode, &combo_type, &combo_index, &motion_index)) {
            return nullptr;
        }

        race_data *race_data = get_race_manager().selected_race_data_pointer();
        if (!race_data) {
            PyErr_SetString(PyExc_RuntimeError, "No race selected");
            return nullptr;
        }

        race_data->register_combo_attack(motion_mode, combo_type, combo_index, motion_index);
        Py_RETURN_NONE;
    }

    auto chrmgr_load_race_data(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        race_data *race_data = get_race_manager().selected_race_data_pointer();
        if (!race_data) {
            PyErr_SetString(PyExc_RuntimeError, "No race selected");
            return nullptr;
        }

        const char *full_file_name = get_race_manager().full_path_file_name(file_name);
        if (!race_data->load_race_data(full_file_name)) {
            metin2::log::error("Cannot load race data {}", full_file_name);
        }

        Py_RETURN_NONE;
    }

    auto chrmgr_load_local_race_data(PyObject *self, PyObject *args) -> PyObject * {
        char *file_name;
        if (!PyArg_ParseTuple(args, "s", &file_name)) {
            return nullptr;
        }

        race_data *race_data = get_race_manager().selected_race_data_pointer();
        if (!race_data) {
            PyErr_SetString(PyExc_RuntimeError, "No race selected");
            return nullptr;
        }

        if (!race_data->load_race_data(file_name)) {
            metin2::log::error("Cannot load race data {}", file_name);
        }

        Py_RETURN_NONE;
    }

    auto chrmgr_register_cache_motion_data(PyObject *self, PyObject *args) -> PyObject * {
        int mode, motion, weight = 0;
        char *file_name;

        if (!PyArg_ParseTuple(args, "iis|i", &mode, &motion, &file_name, &weight)) {
            return nullptr;
        }

        weight = std::min(100, weight);

        race_data *race_data = get_race_manager().selected_race_data_pointer();
        if (!race_data) {
            PyErr_SetString(PyExc_RuntimeError, "No race selected");
            return nullptr;
        }

        const char *full_file_name = get_race_manager().full_path_file_name(file_name);
        graphic_thing *motion_thing = race_data->register_motion_data(mode, motion, full_file_name, weight);

        if (motion_thing)
            get_resource_manager().load_static_cache(motion_thing->file_name());

        Py_RETURN_NONE;
    }

    auto chrmgr_register_motion_data(PyObject *self, PyObject *args) -> PyObject * {
        int mode, motion, weight = 0;
        char *file_name;

        if (!PyArg_ParseTuple(args, "iis|i", &mode, &motion, &file_name, &weight)) {
            return nullptr;
        }

        weight = std::min(100, weight);

        race_data *race_data = get_race_manager().selected_race_data_pointer();
        if (!race_data) {
            PyErr_SetString(PyExc_RuntimeError, "No race selected");
            return nullptr;
        }

        const char *full_file_name = get_race_manager().full_path_file_name(file_name);
        race_data->register_motion_data(mode, motion, full_file_name, weight);

        Py_RETURN_NONE;
    }

    auto chrmgr_register_race_src_name(PyObject *self, PyObject *args) -> PyObject * {
        char *name, *src_name;
        if (!PyArg_ParseTuple(args, "ss", &name, &src_name)) {
            return nullptr;
        }

        get_race_manager().register_race_src_name(name, src_name);

        Py_RETURN_NONE;
    }

    auto chrmgr_register_race_name(PyObject *self, PyObject *args) -> PyObject * {
        int race_index;
        char *name;

        if (!PyArg_ParseTuple(args, "is", &race_index, &name)) {
            return nullptr;
        }

        get_race_manager().register_race_name(race_index, name);

        Py_RETURN_NONE;
    }

    auto chrmgr_set_moving_speed(PyObject *self, PyObject *args) -> PyObject * {
        int mov_spd;
        if (!PyArg_ParseTuple(args, "i", &mov_spd)) {
            return nullptr;
        }

        if (mov_spd < 0) {
            PyErr_SetString(PyExc_ValueError, "Moving speed must not be negative");
            return nullptr;
        }

        instance_base *inst = get_python_character_manager().selected_instance_ptr();

        if (!inst) {
            PyErr_SetString(PyExc_RuntimeError, "No character selected");
            return nullptr;
        }

        inst->set_move_speed(mov_spd);
        Py_RETURN_NONE;
    }

    auto chrmgr_register_effect(PyObject *self, PyObject *args) -> PyObject * {
        int eft_type;
        char *bone_name, *path_name;

        if (!PyArg_ParseTuple(args, "iss", &eft_type, &bone_name, &path_name)) {
            return nullptr;
        }

        instance_base::register_effect(eft_type, bone_name, path_name, false);
        Py_RETURN_NONE;
    }

    auto chrmgr_register_cache_effect(PyObject *self, PyObject *args) -> PyObject * {
        int eft_type;
        char *bone_name, *path_name;

        if (!PyArg_ParseTuple(args, "iss", &eft_type, &bone_name, &path_name)) {
            return nullptr;
        }

        instance_base::register_effect(eft_type, bone_name, path_name, true);

        Py_RETURN_NONE;
    }

    auto chrmgr_set_dust_gap(PyObject *self, PyObject *args) -> PyObject * {
        int gap;
        if (!PyArg_ParseTuple(args, "i", &gap)) {
            return nullptr;
        }

        instance_base::set_dust_gap(gap);
        Py_RETURN_NONE;
    }

    auto chrmgr_set_horse_dust_gap(PyObject *self, PyObject *args) -> PyObject * {
        int gap;
        if (!PyArg_ParseTuple(args, "i", &gap)) {
            return nullptr;
        }

        instance_base::set_horse_dust_gap(gap);
        Py_RETURN_NONE;
    }

    auto chrmgr_toggle_direction_line(PyObject *self, PyObject *args) -> PyObject * {
        static bool is_visible = true;
        actor_instance::show_direction_line(is_visible);

        is_visible = !is_visible;
        Py_RETURN_NONE;
    }

    void python_character_manager::script_set_affect(uint32_t vid, uint32_t state, bool is_visible) {
        instance_base *inst_sel = (vid == 0xffffffff) ? selected_instance_ptr() : instance_ptr(vid);
        if (!inst_sel)
            return;

        inst_sel->script_set_affect(state, is_visible ? true : false);
    }

    auto chrmgr_set_affect(PyObject *self, PyObject *args) -> PyObject * {
        int vid, eft, visible_value;
        if (!PyArg_ParseTuple(args, "iii", &vid, &eft, &visible_value)) {
            return nullptr;
        }

        python_character_manager &chr_mgr = get_python_character_manager();
        chr_mgr.script_set_affect(vid >= 0 ? vid : 0xffffffff, eft, visible_value);
        Py_RETURN_NONE;
    }

    void python_character_manager::set_emoticon(uint32_t vid, uint32_t state) {
        instance_base *inst_sel = (vid == 0xffffffff) ? selected_instance_ptr() : instance_ptr(vid);
        if (!inst_sel)
            return;

        inst_sel->set_emoticon(state);
    }

    auto python_character_manager::is_possible_emoticon(uint32_t vid) -> bool {
        instance_base *inst_sel = (vid == 0xffffffff) ? selected_instance_ptr() : instance_ptr(vid);
        if (!inst_sel)
            return false;

        return inst_sel->is_possible_emoticon();
    }

    auto chrmgr_set_emoticon(PyObject *self, PyObject *args) -> PyObject * {
        int vid, eft;
        if (!PyArg_ParseTuple(args, "ii", &vid, &eft)) {
            return nullptr;
        }

        python_character_manager &chr_mgr = get_python_character_manager();
        chr_mgr.set_emoticon(vid >= 0 ? vid : 0xffffffff, eft);
        Py_RETURN_NONE;
    }

    auto chrmgr_is_possible_emoticon(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!PyArg_ParseTuple(args, "i", &vid)) {
            return nullptr;
        }

        python_character_manager &chr_mgr = get_python_character_manager();
        int result = chr_mgr.is_possible_emoticon(vid >= 0 ? vid : 0xffffffff);

        return Py_BuildValue("i", result);
    }

    void init_characters_manager_module() {
        static PyMethodDef s_methods[] =
            {
                {"set_empire_name_mode", chrmgr_set_empire_name_mode, METH_VARARGS},
                {"get_vid_info", chrmgr_get_vid_info, METH_VARARGS},
                {"get_picked_vid", chrmgr_get_picked_vid, METH_VARARGS},
                {"set_path_name", chrmgr_set_path_name, METH_VARARGS},
                {"load_race_data", chrmgr_load_race_data, METH_VARARGS},
                {"load_local_race_data", chrmgr_load_local_race_data, METH_VARARGS},
                {"create_race", chrmgr_create_race, METH_VARARGS},
                {"select_race", chrmgr_select_race, METH_VARARGS},
                {"register_attaching_bone_name", chrmgr_register_attaching_bone_name, METH_VARARGS},
                {"register_motion_mode", chrmgr_register_motion_mode, METH_VARARGS},
                {"set_motion_random_weight", chrmgr_set_motion_random_weight, METH_VARARGS},
                {"register_normal_attack", chrmgr_register_normal_attack, METH_VARARGS},
                {"reserve_combo_attack_new", chrmgr_reserve_combo_attack_new, METH_VARARGS},
                {"register_combo_attack_new", chrmgr_register_combo_attack_new, METH_VARARGS},
                {"register_motion_data", chrmgr_register_motion_data, METH_VARARGS},
                {"register_race_name", chrmgr_register_race_name, METH_VARARGS},
                {"register_race_src_name", chrmgr_register_race_src_name, METH_VARARGS},
                {"register_cache_motion_data", chrmgr_register_cache_motion_data, METH_VARARGS},

                {"set_affect", chrmgr_set_affect, METH_VARARGS},
                {"set_emoticon", chrmgr_set_emoticon, METH_VARARGS},
                {"is_possible_emoticon", chrmgr_is_possible_emoticon, METH_VARARGS},
                {"register_effect", chrmgr_register_effect, METH_VARARGS},
                {"register_cache_effect", chrmgr_register_cache_effect, METH_VARARGS},
                {"toggle_direction_line", chrmgr_toggle_direction_line, METH_VARARGS},
                {"set_moving_speed", chrmgr_set_moving_speed, METH_VARARGS},
                {"set_dust_gap", chrmgr_set_dust_gap, METH_VARARGS},
                {"set_horse_dust_gap", chrmgr_set_horse_dust_gap, METH_VARARGS},

                {"register_title_name", chrmgr_register_title_name, METH_VARARGS},
                {"register_name_color", chrmgr_register_name_color, METH_VARARGS},
                {"register_title_color", chrmgr_register_title_color, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module = init_python_module("metin2.characters.manager", s_methods);

        PyModule_AddIntConstant(module, "NAMECOLOR_MOB", instance_base::NAMECOLOR_NORMAL_MOB);
        PyModule_AddIntConstant(module, "NAMECOLOR_NPC", instance_base::NAMECOLOR_NORMAL_NPC);
        PyModule_AddIntConstant(module, "NAMECOLOR_PC", instance_base::NAMECOLOR_NORMAL_PC);
        PyModule_AddIntConstant(module, "NAMECOLOR_EMPIRE_MOB", instance_base::NAMECOLOR_EMPIRE_MOB);
        PyModule_AddIntConstant(module, "NAMECOLOR_EMPIRE_NPC", instance_base::NAMECOLOR_EMPIRE_NPC);
        PyModule_AddIntConstant(module, "NAMECOLOR_EMPIRE_PC", instance_base::NAMECOLOR_EMPIRE_PC);
        PyModule_AddIntConstant(module, "NAMECOLOR_FUNC", instance_base::NAMECOLOR_FUNC);
        PyModule_AddIntConstant(module, "NAMECOLOR_PK", instance_base::NAMECOLOR_PK);
        PyModule_AddIntConstant(module, "NAMECOLOR_PVP", instance_base::NAMECOLOR_PVP);
        PyModule_AddIntConstant(module, "NAMECOLOR_PARTY", instance_base::NAMECOLOR_PARTY);
        PyModule_AddIntConstant(module, "NAMECOLOR_WARP", instance_base::NAMECOLOR_WARP);
        PyModule_AddIntConstant(module, "NAMECOLOR_WAYPOINT", instance_base::NAMECOLOR_WAYPOINT);
        PyModule_AddIntConstant(module, "NAMECOLOR_EXTRA", instance_base::NAMECOLOR_EXTRA);

        PyModule_AddIntConstant(module, "EFFECT_SPAWN_DISAPPEAR", instance_base::EFFECT_SPAWN_DISAPPEAR);
        PyModule_AddIntConstant(module, "EFFECT_SPAWN_APPEAR", instance_base::EFFECT_SPAWN_APPEAR);
        PyModule_AddIntConstant(module, "EFFECT_DUST", instance_base::EFFECT_DUST);
        PyModule_AddIntConstant(module, "EFFECT_HORSE_DUST", instance_base::EFFECT_HORSE_DUST);
        PyModule_AddIntConstant(module, "EFFECT_STUN", instance_base::EFFECT_STUN);
        PyModule_AddIntConstant(module, "EFFECT_HIT", instance_base::EFFECT_HIT);
        PyModule_AddIntConstant(module, "EFFECT_FLAME_ATTACK", instance_base::EFFECT_FLAME_ATTACK);
        PyModule_AddIntConstant(module, "EFFECT_FLAME_HIT", instance_base::EFFECT_FLAME_HIT);
        PyModule_AddIntConstant(module, "EFFECT_FLAME_ATTACH", instance_base::EFFECT_FLAME_ATTACH);
        PyModule_AddIntConstant(module, "EFFECT_ELECTRIC_ATTACK", instance_base::EFFECT_ELECTRIC_ATTACK);
        PyModule_AddIntConstant(module, "EFFECT_ELECTRIC_HIT", instance_base::EFFECT_ELECTRIC_HIT);
        PyModule_AddIntConstant(module, "EFFECT_ELECTRIC_ATTACH", instance_base::EFFECT_ELECTRIC_ATTACH);
        PyModule_AddIntConstant(module, "EFFECT_SELECT", instance_base::EFFECT_SELECT);
        PyModule_AddIntConstant(module, "EFFECT_TARGET", instance_base::EFFECT_TARGET);
        PyModule_AddIntConstant(module, "EFFECT_CRITICAL", instance_base::EFFECT_CRITICAL);

        PyModule_AddIntConstant(module, "EFFECT_DAMAGE_TARGET", instance_base::EFFECT_DAMAGE_TARGET);
        PyModule_AddIntConstant(module, "EFFECT_DAMAGE_NOT_TARGET", instance_base::EFFECT_DAMAGE_NOT_TARGET);
        PyModule_AddIntConstant(module, "EFFECT_DAMAGE_SELFDAMAGE", instance_base::EFFECT_DAMAGE_SELFDAMAGE);
        PyModule_AddIntConstant(module, "EFFECT_DAMAGE_SELFDAMAGE2", instance_base::EFFECT_DAMAGE_SELFDAMAGE2);
        PyModule_AddIntConstant(module, "EFFECT_DAMAGE_POISON", instance_base::EFFECT_DAMAGE_POISON);
        PyModule_AddIntConstant(module, "EFFECT_DAMAGE_MISS", instance_base::EFFECT_DAMAGE_MISS);
        PyModule_AddIntConstant(module, "EFFECT_DAMAGE_TARGETMISS", instance_base::EFFECT_DAMAGE_TARGETMISS);
        PyModule_AddIntConstant(module, "EFFECT_DAMAGE_CRITICAL", instance_base::EFFECT_DAMAGE_CRITICAL);

        PyModule_AddIntConstant(module, "EFFECT_LEVELUP", instance_base::EFFECT_LEVELUP);
        PyModule_AddIntConstant(module, "EFFECT_SKILLUP", instance_base::EFFECT_SKILLUP);
        PyModule_AddIntConstant(module, "EFFECT_HPUP_RED", instance_base::EFFECT_HPUP_RED);
        PyModule_AddIntConstant(module, "EFFECT_SPUP_BLUE", instance_base::EFFECT_SPUP_BLUE);
        PyModule_AddIntConstant(module, "EFFECT_SPEEDUP_GREEN", instance_base::EFFECT_SPEEDUP_GREEN);
        PyModule_AddIntConstant(module, "EFFECT_DXUP_PURPLE", instance_base::EFFECT_DXUP_PURPLE);
        PyModule_AddIntConstant(module, "EFFECT_PENETRATE", instance_base::EFFECT_PENETRATE);
        PyModule_AddIntConstant(module, "EFFECT_BLOCK", instance_base::EFFECT_BLOCK);
        PyModule_AddIntConstant(module, "EFFECT_DODGE", instance_base::EFFECT_DODGE);
        PyModule_AddIntConstant(module, "EFFECT_FIRECRACKER", instance_base::EFFECT_FIRECRACKER);
        PyModule_AddIntConstant(module, "EFFECT_SPIN_TOP", instance_base::EFFECT_SPIN_TOP);

        PyModule_AddIntConstant(module, "EFFECT_WEAPON", instance_base::EFFECT_WEAPON);

        PyModule_AddIntConstant(module, "EFFECT_AFFECT", instance_base::EFFECT_AFFECT);
        PyModule_AddIntConstant(module, "EFFECT_EMOTICON", instance_base::EFFECT_EMOTICON);
        PyModule_AddIntConstant(module, "EFFECT_EMPIRE", instance_base::EFFECT_EMPIRE);

        PyModule_AddIntConstant(module, "EFFECT_REFINED", instance_base::EFFECT_REFINED);

        PyModule_AddIntConstant(module, "EFFECT_SUCCESS", instance_base::EFFECT_SUCCESS);
        PyModule_AddIntConstant(module, "EFFECT_FAIL", instance_base::EFFECT_FAIL);
        PyModule_AddIntConstant(module, "EFFECT_FR_SUCCESS", instance_base::EFFECT_FR_SUCCESS);
        PyModule_AddIntConstant(module, "EFFECT_LEVELUP_ON_14_FOR_GERMANY", instance_base::EFFECT_LEVELUP_ON_14_FOR_GERMANY);
        PyModule_AddIntConstant(module, "EFFECT_LEVELUP_UNDER_15_FOR_GERMANY", instance_base::EFFECT_LEVELUP_UNDER_15_FOR_GERMANY);
        PyModule_AddIntConstant(module, "EFFECT_PERCENT_DAMAGE1", instance_base::EFFECT_PERCENT_DAMAGE1);
        PyModule_AddIntConstant(module, "EFFECT_PERCENT_DAMAGE2", instance_base::EFFECT_PERCENT_DAMAGE2);
        PyModule_AddIntConstant(module, "EFFECT_PERCENT_DAMAGE3", instance_base::EFFECT_PERCENT_DAMAGE3);

        PyModule_AddIntConstant(module, "EFFECT_AUTO_HPUP", instance_base::EFFECT_AUTO_HPUP);
        PyModule_AddIntConstant(module, "EFFECT_AUTO_SPUP", instance_base::EFFECT_AUTO_SPUP);

        PyModule_AddIntConstant(module, "EFFECT_RAMADAN_RING_EQUIP", instance_base::EFFECT_RAMADAN_RING_EQUIP);
        PyModule_AddIntConstant(module, "EFFECT_HALLOWEEN_CANDY_EQUIP", instance_base::EFFECT_HALLOWEEN_CANDY_EQUIP);
        PyModule_AddIntConstant(module, "EFFECT_HAPPINESS_RING_EQUIP", instance_base::EFFECT_HAPPINESS_RING_EQUIP);
        PyModule_AddIntConstant(module, "EFFECT_LOVE_PENDANT_EQUIP", instance_base::EFFECT_LOVE_PENDANT_EQUIP);
    }
}
