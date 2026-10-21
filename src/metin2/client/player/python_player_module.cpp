#include <metin2/client/app/python_application.h>
#include <metin2/script/python_utils.h>
#include "python_player.h"

namespace metin2::inline client::inline app {
    extern int two_handed_weapon_attack_speed_decrease;
}
namespace metin2::inline client::inline player {

    extern const uint32_t c_iSkillIndex_Tongsol = 121;
    extern const uint32_t c_iSkillIndex_Combo = 122;
    extern const uint32_t c_iSkillIndex_Fishing = 123;
    extern const uint32_t c_iSkillIndex_Mining = 124;
    extern const uint32_t c_iSkillIndex_Making = 125;
    extern const uint32_t c_iSkillIndex_Language1 = 126;
    extern const uint32_t c_iSkillIndex_Language2 = 127;
    extern const uint32_t c_iSkillIndex_Language3 = 128;
    extern const uint32_t c_iSkillIndex_Polymorph = 129;
    extern const uint32_t c_iSkillIndex_Riding = 130;
    extern const uint32_t skill_index_summon = 131;

    enum {
        EMOTION_CLAP = 1,
        EMOTION_CHEERS_1,
        EMOTION_CHEERS_2,
        EMOTION_DANCE_1,
        EMOTION_DANCE_2,
        EMOTION_DANCE_3,
        EMOTION_DANCE_4,
        EMOTION_DANCE_5,
        EMOTION_DANCE_6, // Gangnam Style
        EMOTION_CONGRATULATION,
        EMOTION_FORGIVE,
        EMOTION_ANGRY,

        EMOTION_ATTRACTIVE,
        EMOTION_SAD,
        EMOTION_SHY,
        EMOTION_CHEERUP,
        EMOTION_BANTER,
        EMOTION_JOY,

        EMOTION_KISS = 51,
        EMOTION_FRENCH_KISS,
        EMOTION_SLAP,

    };

    std::map<int, graphic_image *> map_emotion_index_icon_image;

    class belt_inventory_helper {
      public:
        using grade_unit = uint8_t;

        static auto belt_grade_by_refine_level(int refine_level) -> grade_unit {
            static grade_unit belt_grade_by_level_table[] =
                {
                    0, // belt +0
                    1, // +1
                    1, // +2
                    2, // +3
                    2, // +4
                    3, // +5
                    4, // +6
                    5, // +7
                    6, // +8
                    7, // +9
            };

            return belt_grade_by_level_table[refine_level];
        }

        // Cells usable at each belt grade. A belt has levels +0 to +9, which map to 7 grades that unlock the
        // belt inventory (4x4, 16 cells). A cell is usable when the current grade >= the cell's grade; at
        // grade 0 nothing is usable.
        static auto available_rule_table_by_grade() -> const grade_unit * {
            static grade_unit available_rule_by_grade[c_Belt_Inventory_Slot_Count] = {
                1, 2, 4, 6,
                3, 3, 4, 6,
                5, 5, 5, 6,
                7, 7, 7, 7};

            return available_rule_by_grade;
        }

        static auto is_available_cell(uint16_t cell, int belt_grade) -> bool {
            const grade_unit *rule_table = available_rule_table_by_grade();

            return rule_table[cell] <= belt_grade;
        }
    };

    auto player_pick_close_item(PyObject *self, PyObject *args) -> PyObject * {
        get_python_player().pick_close_item();
        return py_build_none();
    }

    auto player_set_game_window(PyObject *self, PyObject *args) -> PyObject * {
        PyObject *py_handle;
        if (!py_tuple_get_object(args, 0, &py_handle))
            return py_bad_argument();

        python_player &player = get_python_player();
        player.set_game_window(py_handle);
        return py_build_none();
    }

    auto player_set_quick_camera_mode(PyObject *self, PyObject *args) -> PyObject * {
        int is_enable;
        if (!py_tuple_get_integer(args, 0, &is_enable))
            return py_bad_argument();

        python_player &player = get_python_player();
        player.set_quick_camera_mode(is_enable ? true : false);

        return py_build_none();
    }

    auto player_set_main_character_index(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        get_python_player().set_main_character_index(vid);
        get_python_character_manager().set_main_instance(vid);

        return py_build_none();
    }

    auto player_get_main_character_index(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().main_character_index());
    }

    auto player_get_main_character_name(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("s", get_python_player().get_name());
    }

    auto player_get_main_character_position(PyObject *self, PyObject *args) -> PyObject * {
        pixel_position p_pos_main_actor;
        python_player &player = get_python_player();
        player.new_get_main_actor_position(&p_pos_main_actor);
        return Py_BuildValue("fff", p_pos_main_actor.x, p_pos_main_actor.y, p_pos_main_actor.z);
    }

    auto player_is_main_character_index(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        return Py_BuildValue("i", get_python_player().is_main_character_index(vid));
    }

    auto player_is_acting_emotion(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *main_instance = get_python_player().new_get_main_actor_ptr();
        if (!main_instance)
            return Py_BuildValue("i", 0);

        return Py_BuildValue("i", main_instance->is_acting_emotion());
    }

    auto player_is_pvp_instance(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        instance_base *main_instance = get_python_player().new_get_main_actor_ptr();
        instance_base *target_instance = get_python_character_manager().instance_ptr(vid);
        if (!main_instance)
            return Py_BuildValue("i", 0);
        if (!target_instance)
            return Py_BuildValue("i", 0);

        return Py_BuildValue("i", main_instance->is_pvp_instance(*target_instance));
    }

    auto player_is_same_empire(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        instance_base *main_instance = get_python_player().new_get_main_actor_ptr();
        instance_base *target_instance = get_python_character_manager().instance_ptr(vid);
        if (!main_instance)
            return Py_BuildValue("i", false);
        if (!target_instance)
            return Py_BuildValue("i", false);

        return Py_BuildValue("i", main_instance->is_same_empire(*target_instance));
    }

    auto player_is_challenge_instance(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        return Py_BuildValue("i", get_python_player().is_challenge_instance(vid));
    }

    auto player_is_revenge_instance(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        return Py_BuildValue("i", get_python_player().is_revenge_instance(vid));
    }

    auto player_is_cant_fight_instance(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        return Py_BuildValue("i", get_python_player().is_cant_fight_instance(vid));
    }

    auto player_get_character_distance(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        instance_base *main_instance = get_python_player().new_get_main_actor_ptr();
        instance_base *target_instance = get_python_character_manager().instance_ptr(vid);
        if (!main_instance)
            return Py_BuildValue("f", -1.0f);
        if (!target_instance)
            return Py_BuildValue("f", -1.0f);

        return Py_BuildValue("f", main_instance->distance(target_instance));
    }

    auto player_is_in_safe_area(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *main_instance = get_python_player().new_get_main_actor_ptr();
        if (!main_instance)
            return Py_BuildValue("i", false);

        return Py_BuildValue("i", main_instance->is_in_safe());
    }

    auto player_is_mounting_horse(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *main_instance = get_python_player().new_get_main_actor_ptr();
        if (!main_instance)
            return Py_BuildValue("i", false);

        return Py_BuildValue("i", main_instance->is_mounting_horse());
    }

    auto player_is_observer_mode(PyObject *self, PyObject *args) -> PyObject * {
        python_player &player = get_python_player();
        return Py_BuildValue("i", player.is_observer_mode());
    }

    auto player_register_cache_effect(PyObject *self, PyObject *args) -> PyObject * {
        int eft;
        if (!py_tuple_get_integer(args, 0, &eft))
            return py_bad_argument();

        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_bad_argument();

        python_player &player = get_python_player();
        if (!player.register_effect(eft, file_name, true))
            return py_build_exception("CPythonPlayer::RegisterEffect(eEft=%d, szFileName=%s", eft, file_name);

        return py_build_none();
    }

    auto player_set_attack_key_state(PyObject *self, PyObject *args) -> PyObject * {
        int is_pressed;
        if (!py_tuple_get_integer(args, 0, &is_pressed))
            return py_build_exception("playerSetAttackKeyState(isPressed) - There is no first argument");

        python_player &player = get_python_player();
        player.set_attack_key_state(is_pressed ? true : false);
        return py_build_none();
    }

    auto player_set_single_dik_key_state(PyObject *self, PyObject *args) -> PyObject * {
        int dik;
        if (!py_tuple_get_integer(args, 0, &dik))
            return py_build_exception("playerSetSingleDIKKeyState(eDIK, isPressed) - There is no first argument");

        int is_pressed;
        if (!py_tuple_get_integer(args, 1, &is_pressed))
            return py_build_exception("playerSetSingleDIKKeyState(eDIK, isPressed) - There is no second argument");

        python_player &player = get_python_player();
        player.new_set_single_dik_key_state(dik, is_pressed ? true : false);
        return py_build_none();
    }

    auto player_end_key_walking_immediately(PyObject *self, PyObject *args) -> PyObject * {
        get_python_player().new_stop();
        return py_build_none();
    }

    auto player_end_mouse_walking(PyObject *self, PyObject *args) -> PyObject * {
        return py_build_none();
    }

    auto player_reset_camera_rotation(PyObject *self, PyObject *args) -> PyObject * {
        get_python_player().new_reset_camera_rotation();
        return py_build_none();
    }

    auto player_set_auto_camera_rotation_speed(PyObject *self, PyObject *args) -> PyObject * {
        float cmr_rot_spd;
        if (!py_tuple_get_float(args, 0, &cmr_rot_spd))
            return py_build_exception();

        python_player &player = get_python_player();
        player.new_set_auto_camera_rotation_speed(cmr_rot_spd);
        return py_build_none();
    }

    auto player_set_mouse_state(PyObject *self, PyObject *args) -> PyObject * {
        int mbt;
        if (!py_tuple_get_integer(args, 0, &mbt))
            return py_build_exception();

        int mbs;
        if (!py_tuple_get_integer(args, 1, &mbs))
            return py_build_exception();

        python_player &player = get_python_player();
        player.new_set_mouse_state(mbt, mbs);

        return py_build_none();
    }

    auto player_set_mouse_func(PyObject *self, PyObject *args) -> PyObject * {
        int mbt;
        if (!py_tuple_get_integer(args, 0, &mbt))
            return py_build_exception();

        int mbs;
        if (!py_tuple_get_integer(args, 1, &mbs))
            return py_build_exception();

        python_player &player = get_python_player();
        player.new_set_mouse_func(mbt, mbs);

        return py_build_none();
    }

    auto player_set_mouse_middle_button_state(PyObject *self, PyObject *args) -> PyObject * {
        int mbs;
        if (!py_tuple_get_integer(args, 0, &mbs))
            return py_build_exception();

        python_player &player = get_python_player();
        player.new_set_mouse_middle_button_state(mbs);

        return py_build_none();
    }

    auto player_get_name(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("s", get_python_player().get_name());
    }

    auto player_get_race(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().race());
    }

    auto player_get_play_time(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().play_time());
    }

    auto player_set_play_time(PyObject *self, PyObject *args) -> PyObject * {
        int time;
        if (!py_tuple_get_integer(args, 0, &time))
            return py_build_exception();

        get_python_player().set_play_time(time);
        return py_build_none();
    }

    auto player_is_skill_cool_time(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        return Py_BuildValue("i", get_python_player().is_skill_cool_time(slot_index));
    }

    auto player_get_skill_cool_time(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        float cool_time = get_python_player().skill_cool_time(slot_index);
        float elapsed_cool_time = get_python_player().skill_elapsed_cool_time(slot_index);
        return Py_BuildValue("ff", cool_time, elapsed_cool_time);
    }

    auto player_is_skill_active(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        return Py_BuildValue("i", get_python_player().is_skill_active(slot_index));
    }

    auto player_use_guild_skill(PyObject *self, PyObject *args) -> PyObject * {
        int skill_slot_index;
        if (!py_tuple_get_integer(args, 0, &skill_slot_index))
            return py_build_exception();

        get_python_player().use_guild_skill(skill_slot_index);
        return py_build_none();
    }

    auto player_affect_index_to_skill_index(PyObject *self, PyObject *args) -> PyObject * {
        int affect_index;
        if (!py_tuple_get_integer(args, 0, &affect_index))
            return py_build_exception();

        uint32_t skill_index;
        if (!get_python_player().affect_index_to_skill_index(affect_index, &skill_index))
            return Py_BuildValue("i", 0);

        return Py_BuildValue("i", skill_index);
    }

    auto player_get_exp(PyObject *self, PyObject *args) -> PyObject * {
        uint32_t exp = get_python_player().status(POINT_EXP);
        return Py_BuildValue("l", long(exp));
    }

    auto player_get_status(PyObject *self, PyObject *args) -> PyObject * {
        int type;
        if (!py_tuple_get_integer(args, 0, &type))
            return py_build_exception();

        int32_t value = get_python_player().status(type);

        if (POINT_ATT_SPEED == type) {
            instance_base *instance = get_python_player().new_get_main_actor_ptr();
            if (instance && (item_data::WEAPON_TWO_HANDED == instance->weapon_type())) {
                value -= two_handed_weapon_attack_speed_decrease;
            }
        }

        return Py_BuildValue("i", value);
    }

    auto player_get_elk(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().status(POINT_GOLD));
    }

    auto player_get_guild_id(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *instance = get_python_player().new_get_main_actor_ptr();
        if (!instance)
            return Py_BuildValue("i", 0);
        return Py_BuildValue("i", instance->guild_id());
    }

    auto player_get_guild_name(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *instance = get_python_player().new_get_main_actor_ptr();
        if (instance) {
            uint32_t id = instance->guild_id();
            std::string name;
            if (get_python_guild().guild_name(id, &name))
                return Py_BuildValue("s", name.c_str());
        }
        return Py_BuildValue("s", "");
    }

    auto player_get_alignment_data(PyObject *self, PyObject *args) -> PyObject * {
        instance_base *instance = get_python_player().new_get_main_actor_ptr();
        int alignment_point = 0;
        int alignment_grade = 4;
        if (instance) {
            alignment_point = instance->alignment();
            alignment_grade = instance->alignment_grade();
        }
        return Py_BuildValue("ii", alignment_point, alignment_grade);
    }

    auto player_set_skill(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        int skill_index;
        if (!py_tuple_get_integer(args, 1, &skill_index))
            return py_build_exception();

        get_python_player().set_skill(slot_index, skill_index);
        return py_build_none();
    }

    auto player_get_skill_index(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        return Py_BuildValue("i", get_python_player().skill_index(slot_index));
    }

    auto player_get_skill_slot_index(PyObject *self, PyObject *args) -> PyObject * {
        int skill_index;
        if (!py_tuple_get_integer(args, 0, &skill_index))
            return py_build_exception();

        uint32_t slot_index;
        if (!get_python_player().skill_slot_index(skill_index, &slot_index))
            return py_build_exception();

        return Py_BuildValue("i", slot_index);
    }

    auto player_get_skill_grade(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        return Py_BuildValue("i", get_python_player().skill_grade(slot_index));
    }

    auto player_get_skill_level(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        return Py_BuildValue("i", get_python_player().skill_level(slot_index));
    }

    auto player_get_skill_current_efficient_percentage(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        return Py_BuildValue("f", get_python_player().skill_current_efficient_percentage(slot_index));
    }
    auto player_get_skill_next_efficient_percentage(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        return Py_BuildValue("f", get_python_player().skill_next_efficient_percentage(slot_index));
    }

    auto player_click_skill_slot(PyObject *self, PyObject *args) -> PyObject * {
        int skill_slot;
        if (!py_tuple_get_integer(args, 0, &skill_slot))
            return py_bad_argument();

        get_python_player().click_skill_slot(skill_slot);

        return py_build_none();
    }

    auto player_change_current_skill_number_only(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_bad_argument();

        get_python_player().change_current_skill_number_only(slot_index);

        return py_build_none();
    }

    auto player_clear_skill_dict(PyObject *self, PyObject *args) -> PyObject * {
        get_python_player().clear_skill_dict();
        return py_build_none();
    }

    auto player_get_item_index(PyObject *self, PyObject *args) -> PyObject * {
        switch (PyTuple_Size(args)) {
        case 1:
            {
                int slot_index;
                if (!py_tuple_get_integer(args, 0, &slot_index))
                    return py_build_exception();

                int item_index = get_python_player().item_index(item_pos(INVENTORY, slot_index));
                return Py_BuildValue("i", item_index);
            }
        case 2:
            {
                item_pos cell;
                if (!py_tuple_get_byte(args, 0, &cell.window_type))
                    return py_build_exception();
                if (!py_tuple_get_integer(args, 1, &cell.cell))
                    return py_build_exception();

                int item_index = get_python_player().item_index(cell);
                return Py_BuildValue("i", item_index);
            }
        default:
            return py_build_exception();
        }
    }

    auto player_get_item_flags(PyObject *self, PyObject *args) -> PyObject * {
        switch (PyTuple_Size(args)) {
        case 1:
            {
                int slot_index;
                if (!py_tuple_get_integer(args, 0, &slot_index))
                    return py_build_exception();

                uint32_t flags = get_python_player().item_flags(item_pos(INVENTORY, slot_index));
                return Py_BuildValue("i", flags);
            }
        case 2:
            {
                item_pos cell;
                if (!py_tuple_get_byte(args, 0, &cell.window_type))
                    return py_build_exception();

                if (!py_tuple_get_integer(args, 1, &cell.cell))
                    return py_build_exception();

                uint32_t flags = get_python_player().item_flags(cell);
                return Py_BuildValue("i", flags);
            }
        default:
            return py_build_exception();
        }
    }

    auto player_get_item_count(PyObject *self, PyObject *args) -> PyObject * {
        switch (PyTuple_Size(args)) {
        case 1:
            {
                int slot_index;
                if (!py_tuple_get_integer(args, 0, &slot_index))
                    return py_build_exception();

                int item_num = get_python_player().item_count(item_pos(INVENTORY, slot_index));
                return Py_BuildValue("i", item_num);
            }
        case 2:
            {
                item_pos cell;
                if (!py_tuple_get_byte(args, 0, &cell.window_type))
                    return py_build_exception();

                if (!py_tuple_get_integer(args, 1, &cell.cell))
                    return py_build_exception();

                int item_num = get_python_player().item_count(cell);

                return Py_BuildValue("i", item_num);
            }
        default:
            return py_build_exception();
        }
    }

    auto player_set_item_count(PyObject *self, PyObject *args) -> PyObject * {
        switch (PyTuple_Size(args)) {
        case 2:
            {
                int slot_index;
                if (!py_tuple_get_integer(args, 0, &slot_index))
                    return py_build_exception();

                uint8_t count;
                if (!py_tuple_get_integer(args, 1, &count))
                    return py_build_exception();

                if (0 == count)
                    return py_build_exception();

                get_python_player().set_item_count(item_pos(INVENTORY, slot_index), count);
                return py_build_none();
            }
        case 3:
            {
                item_pos cell;
                if (!py_tuple_get_byte(args, 0, &cell.window_type))
                    return py_build_exception();

                if (!py_tuple_get_integer(args, 1, &cell.cell))
                    return py_build_exception();

                uint8_t count;
                if (!py_tuple_get_integer(args, 2, &count))
                    return py_build_exception();

                get_python_player().set_item_count(cell, count);

                return py_build_none();
            }
        default:
            return py_build_exception();
        }
    }

    auto player_get_item_count_by_vnum(PyObject *self, PyObject *args) -> PyObject * {
        int ivnum;
        if (!py_tuple_get_integer(args, 0, &ivnum))
            return py_build_exception();

        int item_num = get_python_player().item_count_by_vnum(ivnum);
        return Py_BuildValue("i", item_num);
    }

    auto player_get_item_metin_socket(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;
        int metin_socket_index;

        switch (PyTuple_Size(args)) {
        case 2:
            if (!py_tuple_get_integer(args, 0, &cell.cell))
                return py_build_exception();

            if (!py_tuple_get_integer(args, 1, &metin_socket_index))
                return py_build_exception();

            break;
        case 3:
            if (!py_tuple_get_byte(args, 0, &cell.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &cell.cell))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 2, &metin_socket_index))
                return py_build_exception();

            break;

        default:
            return py_build_exception();
        }
        int metin_socket_value = get_python_player().item_metin_socket(cell, metin_socket_index);
        return Py_BuildValue("i", metin_socket_value);
    }

    auto player_get_item_attribute(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;
        int attribute_slot_index;
        switch (PyTuple_Size(args)) {
        case 2:
            if (!py_tuple_get_integer(args, 0, &cell.cell))
                return py_build_exception();

            if (!py_tuple_get_integer(args, 1, &attribute_slot_index))
                return py_build_exception();

            break;
        case 3:
            if (!py_tuple_get_byte(args, 0, &cell.window_type))
                return py_build_exception();

            if (!py_tuple_get_integer(args, 1, &cell.cell))
                return py_build_exception();

            if (!py_tuple_get_integer(args, 2, &attribute_slot_index))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }
        uint8_t type;
        short value;
        get_python_player().item_attribute(cell, attribute_slot_index, &type, &value);

        return Py_BuildValue("ii", type, value);
    }

    auto player_get_item_link(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;

        switch (PyTuple_Size(args)) {
        case 1:
            if (!py_tuple_get_integer(args, 0, &cell.cell))
                return py_build_exception();
            break;
        case 2:
            if (!py_tuple_get_byte(args, 0, &cell.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &cell.cell))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }
        const packet_item *player_item = get_python_player().get_item_data(cell);
        item_data *item_data = nullptr;
        char buf[1024];

        if (player_item && get_item_manager().item_data_pointer(player_item->vnum, &item_data)) {
            char itemlink[256];
            int len;
            bool is_attr = false;

            len = snprintf(itemlink, sizeof(itemlink), "item:%x:%x:%x:%x:%x",
                player_item->vnum, player_item->flags_value,
                player_item->sockets[0], player_item->sockets[1], player_item->sockets[2]);

            for (int i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
                if (player_item->attr[i].type_value != 0) {
                    len += snprintf(itemlink + len, sizeof(itemlink) - len, ":%x:%d",
                        player_item->attr[i].type_value, player_item->attr[i].value);
                    is_attr = true;
                }

            if (is_attr)
                // "item:vnum:flags:socket0:socket1:socket2"
                snprintf(buf, sizeof(buf), "|cffffc700|H%s|h[%s]|h|r", itemlink, item_data->get_name());
            else
                snprintf(buf, sizeof(buf), "|cfff1e6c0|H%s|h[%s]|h|r", itemlink, item_data->get_name());
        } else
            buf[0] = '\0';

        return Py_BuildValue("s", buf);
    }

    auto player_get_i_sell_item_price(PyObject *self, PyObject *args) -> PyObject * {
        item_pos cell;

        switch (PyTuple_Size(args)) {
        case 1:
            if (!py_tuple_get_integer(args, 0, &cell.cell))
                return py_build_exception();
            break;
        case 2:
            if (!py_tuple_get_byte(args, 0, &cell.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &cell.cell))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }
        item_data *item_data;

        if (!get_item_manager().item_data_pointer(get_python_player().item_index(cell), &item_data))
            return Py_BuildValue("i", 0);

        int price;

        if (item_data->is_flag(item_data::ITEM_FLAG_COUNT_PER_1GOLD))
            price = get_python_player().item_count(cell) / item_data->i_sell_item_price();
        else
            price = item_data->i_sell_item_price() * get_python_player().item_count(cell);

        price /= 5;
        return Py_BuildValue("i", price);
    }

    auto player_get_quick_page(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().quick_page());
    }

    auto player_set_quick_page(PyObject *self, PyObject *args) -> PyObject * {
        int page_index;
        if (!py_tuple_get_integer(args, 0, &page_index))
            return py_build_exception();

        get_python_player().set_quick_page(page_index);
        return py_build_none();
    }

    auto player_local_quick_slot_index_to_global_quick_slot_index(PyObject *self, PyObject *args) -> PyObject * {
        int local_slot_index;
        if (!py_tuple_get_integer(args, 0, &local_slot_index))
            return py_build_exception();

        python_player &player = get_python_player();
        return Py_BuildValue("i", player.local_quick_slot_index_to_global_quick_slot_index(local_slot_index));
    }

    auto player_get_local_quick_slot(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        uint32_t wnd_type;
        uint32_t wnd_item_pos;

        python_player &player = get_python_player();
        player.local_quick_slot_data(slot_index, &wnd_type, &wnd_item_pos);

        return Py_BuildValue("ii", wnd_type, wnd_item_pos);
    }

    auto player_get_global_quick_slot(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        uint32_t wnd_type;
        uint32_t wnd_item_pos;

        python_player &player = get_python_player();
        player.global_quick_slot_data(slot_index, &wnd_type, &wnd_item_pos);

        return Py_BuildValue("ii", wnd_type, wnd_item_pos);
    }

    auto player_request_add_local_quick_slot(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        int wnd_type;
        int wnd_item_pos;

        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        if (!py_tuple_get_integer(args, 1, &wnd_type))
            return py_build_exception();

        if (!py_tuple_get_integer(args, 2, &wnd_item_pos))
            return py_build_exception();

        python_player &player = get_python_player();
        player.request_add_local_quick_slot(slot_index, wnd_type, wnd_item_pos);

        return py_build_none();
    }

    auto player_request_add_to_empty_local_quick_slot(PyObject *self, PyObject *args) -> PyObject * {
        int wnd_type;
        if (!py_tuple_get_integer(args, 0, &wnd_type))
            return py_build_exception();

        int wnd_item_pos;
        if (!py_tuple_get_integer(args, 1, &wnd_item_pos))
            return py_build_exception();

        python_player &player = get_python_player();
        player.request_add_to_empty_local_quick_slot(wnd_type, wnd_item_pos);

        return py_build_none();
    }

    auto player_request_delete_global_quick_slot(PyObject *self, PyObject *args) -> PyObject * {
        int global_slot_index;
        if (!py_tuple_get_integer(args, 0, &global_slot_index))
            return py_build_exception();

        python_player &player = get_python_player();
        player.request_delete_global_quick_slot(global_slot_index);
        return py_build_none();
    }

    auto player_request_move_global_quick_slot_to_local_quick_slot(PyObject *self, PyObject *args) -> PyObject * {
        int global_src_slot_index;
        int local_dst_slot_index;

        if (!py_tuple_get_integer(args, 0, &global_src_slot_index))
            return py_build_exception();

        if (!py_tuple_get_integer(args, 1, &local_dst_slot_index))
            return py_build_exception();

        python_player &player = get_python_player();
        player.request_move_global_quick_slot_to_local_quick_slot(global_src_slot_index, local_dst_slot_index);
        return py_build_none();
    }

    auto player_request_use_local_quick_slot(PyObject *self, PyObject *args) -> PyObject * {
        int local_position;
        if (!py_tuple_get_integer(args, 0, &local_position))
            return py_build_exception();

        get_python_player().request_use_local_quick_slot(local_position);

        return py_build_none();
    }

    auto player_is_belt_inventory_slot(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        char flag = get_python_player().is_belt_inventory_slot(item_pos(INVENTORY, slot_index));

        return Py_BuildValue("i", flag);
    }

    auto player_is_equipment_slot(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        if (uint32_t(slot_index) >= c_Equipment_Start)
            if (uint32_t(slot_index) <= c_DragonSoul_Equip_End)
                return Py_BuildValue("i", 1);

        return Py_BuildValue("i", 0);
    }

    auto player_is_ds_equipment_slot(PyObject *self, PyObject *args) -> PyObject * {
        uint8_t window_type;
        if (!py_tuple_get_integer(args, 0, &window_type))
            return py_build_exception();
        int slot_index;
        if (!py_tuple_get_integer(args, 1, &slot_index))
            return py_build_exception();

        if (INVENTORY == window_type)
            if (uint32_t(slot_index) >= c_DragonSoul_Equip_Start)
                if (uint32_t(slot_index) <= c_DragonSoul_Equip_End)
                    return Py_BuildValue("i", 1);

        return Py_BuildValue("i", 0);
    }

    auto player_is_costume_slot(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        if (uint32_t(slot_index) >= c_Costume_Slot_Start)
            if (uint32_t(slot_index) <= c_Costume_Slot_End)
                return Py_BuildValue("i", 1);

        return Py_BuildValue("i", 0);
    }

    auto player_is_open_private_shop(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().is_open_private_shop());
    }

    auto player_is_valuable_item(PyObject *self, PyObject *args) -> PyObject * {
        item_pos slot_index;

        switch (PyTuple_Size(args)) {
        case 1:
            if (!py_tuple_get_integer(args, 0, &slot_index.cell))
                return py_build_exception();
            break;
        case 2:
            if (!py_tuple_get_integer(args, 0, &slot_index.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 0, &slot_index.cell))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }

        uint32_t item_index = get_python_player().item_index(slot_index);
        get_item_manager().select_item_data(item_index);
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Can't find item data");

        bool has_metin_socket = false;
        bool is_high_price = false;

        for (int i = 0; i < METIN_SOCKET_COUNT; ++i)
            if (python_player::METIN_SOCKET_TYPE_NONE != get_python_player().item_metin_socket(slot_index, i))
                has_metin_socket = true;

        uint32_t value = item_data->i_sell_item_price();
        if (value > 5000)
            is_high_price = true;

        return Py_BuildValue("i", has_metin_socket || is_high_price);
    }

    auto get_item_grade(const char *item_name) -> int {
        std::string name = item_name;
        if (name.empty())
            return 0;

        char ch_grade = name[name.length() - 1];
        if (ch_grade < '0' || ch_grade > '9')
            ch_grade = '0';

        int i_grade = ch_grade - '0';
        return i_grade;
    }

    auto player_get_item_grade(PyObject *self, PyObject *args) -> PyObject * {
        item_pos slot_index;
        switch (PyTuple_Size(args)) {
        case 1:
            if (!py_tuple_get_integer(args, 0, &slot_index.cell))
                return py_build_exception();
            break;
        case 2:
            if (!py_tuple_get_integer(args, 0, &slot_index.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &slot_index.cell))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }

        int item_index = get_python_player().item_index(slot_index);
        get_item_manager().select_item_data(item_index);
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Can't find item data");

        return Py_BuildValue("i", get_item_grade(item_data->get_name()));
    }

    enum {
        REFINE_SCROLL_TYPE_MAKE_SOCKET = 1,
        REFINE_SCROLL_TYPE_UP_GRADE = 2,
    };

    enum {
        REFINE_CANT,
        REFINE_OK,
        REFINE_ALREADY_MAX_SOCKET_COUNT,
        REFINE_NEED_MORE_GOOD_SCROLL,
        REFINE_CANT_MAKE_SOCKET_ITEM,
        REFINE_NOT_NEXT_GRADE_ITEM,
        REFINE_CANT_REFINE_METIN_TO_EQUIPMENT,
        REFINE_CANT_REFINE_ROD,
    };

    auto player_can_refine(PyObject *self, PyObject *args) -> PyObject * {
        int scroll_item_index;
        item_pos target_slot_index;

        switch (PyTuple_Size(args)) {
        case 2:
            if (!py_tuple_get_integer(args, 0, &scroll_item_index))
                return py_bad_argument();
            if (!py_tuple_get_integer(args, 1, &target_slot_index.cell))
                return py_bad_argument();
            break;
        case 3:
            if (!py_tuple_get_integer(args, 0, &scroll_item_index))
                return py_bad_argument();
            if (!py_tuple_get_integer(args, 1, &target_slot_index.window_type))
                return py_bad_argument();
            if (!py_tuple_get_integer(args, 2, &target_slot_index.cell))
                return py_bad_argument();
            break;
        default:
            return py_bad_argument();
        }

        if (get_python_player().is_equipment_slot(target_slot_index)) {
            return Py_BuildValue("i", REFINE_CANT_REFINE_METIN_TO_EQUIPMENT);
        }

        get_item_manager().select_item_data(scroll_item_index);
        item_data *scroll_item_data = get_item_manager().selected_item_data_pointer();
        if (!scroll_item_data)
            return Py_BuildValue("i", REFINE_CANT);
        int scroll_type = scroll_item_data->get_type();
        int scroll_sub_type = scroll_item_data->sub_type();
        if (scroll_type != item_data::ITEM_TYPE_USE)
            return Py_BuildValue("i", REFINE_CANT);
        if (scroll_sub_type != item_data::USE_TUNING)
            return Py_BuildValue("i", REFINE_CANT);

        int target_item_index = get_python_player().item_index(target_slot_index);
        get_item_manager().select_item_data(target_item_index);
        item_data *target_item_data = get_item_manager().selected_item_data_pointer();
        if (!target_item_data)
            return Py_BuildValue("i", REFINE_CANT);
        int target_type = target_item_data->get_type();
        if (item_data::ITEM_TYPE_ROD == target_type)
            return Py_BuildValue("i", REFINE_CANT_REFINE_ROD);

        if (target_item_data->has_next_grade()) {
            return Py_BuildValue("i", REFINE_OK);
        } else {
            return Py_BuildValue("i", REFINE_NOT_NEXT_GRADE_ITEM);
        }

        return Py_BuildValue("i", REFINE_CANT);
    }

    enum {
        ATTACH_METIN_CANT,
        ATTACH_METIN_OK,
        ATTACH_METIN_NOT_MATCHABLE_ITEM,
        ATTACH_METIN_NO_MATCHABLE_SOCKET,
        ATTACH_METIN_NOT_EXIST_GOLD_SOCKET,
        ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT,
    };

    auto player_can_attach_metin(PyObject *self, PyObject *args) -> PyObject * {
        int metin_item_id;
        item_pos target_slot_index;

        switch (PyTuple_Size(args)) {
        case 2:
            if (!py_tuple_get_integer(args, 0, &metin_item_id))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &target_slot_index.cell))
                return py_build_exception();
            break;
        case 3:
            if (!py_tuple_get_integer(args, 0, &metin_item_id))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &target_slot_index.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 2, &target_slot_index.cell))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }
        if (get_python_player().is_equipment_slot(target_slot_index)) {
            return Py_BuildValue("i", ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT);
        }

        item_data *metin_item_data;
        if (!get_item_manager().item_data_pointer(metin_item_id, &metin_item_data))
            return py_build_exception("can't find item data");

        uint32_t target_item_index = get_python_player().item_index(target_slot_index);
        item_data *target_item_data;
        if (!get_item_manager().item_data_pointer(target_item_index, &target_item_data))
            return py_build_exception("can't find item data");

        uint32_t metin_wear_flags = metin_item_data->wear_flags();
        uint32_t target_wear_flags = target_item_data->wear_flags();
        if (0 == (metin_wear_flags & target_wear_flags))
            return Py_BuildValue("i", ATTACH_METIN_NOT_MATCHABLE_ITEM);
        if (item_data::ITEM_TYPE_ROD == target_item_data->get_type())
            return Py_BuildValue("i", ATTACH_METIN_CANT);

        bool not_exist_gold_socket = false;

        int sub_type = metin_item_data->sub_type();
        for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i) {
            uint32_t socket_type = get_python_player().item_metin_socket(target_slot_index, i);
            if (item_data::METIN_NORMAL == sub_type) {
                if (python_player::METIN_SOCKET_TYPE_SILVER == socket_type ||
                    python_player::METIN_SOCKET_TYPE_GOLD == socket_type) {
                    return Py_BuildValue("i", ATTACH_METIN_OK);
                }
            } else if (item_data::METIN_GOLD == sub_type) {
                if (python_player::METIN_SOCKET_TYPE_GOLD == socket_type) {
                    return Py_BuildValue("i", ATTACH_METIN_OK);
                } else if (python_player::METIN_SOCKET_TYPE_SILVER == socket_type) {
                    not_exist_gold_socket = true;
                }
            }
        }

        if (not_exist_gold_socket) {
            return Py_BuildValue("i", ATTACH_METIN_NOT_EXIST_GOLD_SOCKET);
        }

        return Py_BuildValue("i", ATTACH_METIN_NO_MATCHABLE_SOCKET);
    }

    enum {
        DETACH_METIN_CANT,
        DETACH_METIN_OK,
    };

    auto player_can_detach(PyObject *self, PyObject *args) -> PyObject * {
        int scroll_item_index;
        item_pos target_slot_index;
        switch (PyTuple_Size(args)) {
        case 2:
            if (!py_tuple_get_integer(args, 0, &scroll_item_index))
                return py_bad_argument();
            if (!py_tuple_get_integer(args, 1, &target_slot_index.cell))
                return py_bad_argument();
            break;
        case 3:
            if (!py_tuple_get_integer(args, 0, &scroll_item_index))
                return py_bad_argument();
            if (!py_tuple_get_integer(args, 1, &target_slot_index.window_type))
                return py_bad_argument();
            if (!py_tuple_get_integer(args, 2, &target_slot_index.cell))
                return py_bad_argument();
            break;
        default:
            return py_bad_argument();
        }

        get_item_manager().select_item_data(scroll_item_index);
        item_data *scroll_item_data = get_item_manager().selected_item_data_pointer();
        if (!scroll_item_data)
            return py_build_exception("Can't find item data");
        int scroll_type = scroll_item_data->get_type();
        int scroll_sub_type = scroll_item_data->sub_type();
        if (scroll_type != item_data::ITEM_TYPE_USE)
            return Py_BuildValue("i", DETACH_METIN_CANT);
        if (scroll_sub_type != item_data::USE_DETACHMENT)
            return Py_BuildValue("i", DETACH_METIN_CANT);

        int target_item_index = get_python_player().item_index(target_slot_index);
        get_item_manager().select_item_data(target_item_index);
        item_data *target_item_data = get_item_manager().selected_item_data_pointer();
        if (!target_item_data)
            return py_build_exception("Can't find item data");

        if (target_item_data->is_flag(item_data::ITEM_FLAG_REFINEABLE)) {
            for (int slot_count = 0; slot_count < METIN_SOCKET_COUNT; ++slot_count)
                if (get_python_player().item_metin_socket(target_slot_index, slot_count) > 2) {
                    return Py_BuildValue("i", DETACH_METIN_OK);
                }
        }

        return Py_BuildValue("i", DETACH_METIN_CANT);
    }

    auto player_can_unlock(PyObject *self, PyObject *args) -> PyObject * {
        int key_item_index;
        item_pos target_slot_index;
        switch (PyTuple_Size(args)) {
        case 2:
            if (!py_tuple_get_integer(args, 0, &key_item_index))
                return py_bad_argument();
            if (!py_tuple_get_integer(args, 1, &target_slot_index.cell))
                return py_bad_argument();
            break;
        case 3:
            if (!py_tuple_get_integer(args, 0, &key_item_index))
                return py_bad_argument();
            if (!py_tuple_get_integer(args, 1, &target_slot_index.window_type))
                return py_bad_argument();
            if (!py_tuple_get_integer(args, 2, &target_slot_index.cell))
                return py_bad_argument();
            break;
        default:
            return py_bad_argument();
        }

        get_item_manager().select_item_data(key_item_index);
        item_data *key_item_data = get_item_manager().selected_item_data_pointer();
        if (!key_item_data)
            return py_build_exception("Can't find item data");
        int key_type = key_item_data->get_type();
        if (key_type != item_data::ITEM_TYPE_TREASURE_KEY)
            return Py_BuildValue("i", false);

        int target_item_index = get_python_player().item_index(target_slot_index);
        get_item_manager().select_item_data(target_item_index);
        item_data *target_item_data = get_item_manager().selected_item_data_pointer();
        if (!target_item_data)
            return py_build_exception("Can't find item data");
        int target_type = target_item_data->get_type();
        if (target_type != item_data::ITEM_TYPE_TREASURE_BOX)
            return Py_BuildValue("i", false);

        return Py_BuildValue("i", true);
    }

    auto player_is_refine_grade_scroll(PyObject *self, PyObject *args) -> PyObject * {
        item_pos scroll_slot_index;
        switch (PyTuple_Size(args)) {
        case 1:
            if (!py_tuple_get_integer(args, 0, &scroll_slot_index.cell))
                return py_build_exception();
            break;
        case 2:
            if (!py_tuple_get_integer(args, 0, &scroll_slot_index.window_type))
                return py_build_exception();
            if (!py_tuple_get_integer(args, 1, &scroll_slot_index.cell))
                return py_build_exception();
            break;
        default:
            return py_build_exception();
        }

        int scroll_item_index = get_python_player().item_index(scroll_slot_index);
        get_item_manager().select_item_data(scroll_item_index);
        item_data *scroll_item_data = get_item_manager().selected_item_data_pointer();
        if (!scroll_item_data)
            return py_build_exception("Can't find item data");

        return Py_BuildValue("i", REFINE_SCROLL_TYPE_UP_GRADE == scroll_item_data->value(0));
    }

    auto player_clear_target(PyObject *self, PyObject *args) -> PyObject * {
        get_python_player().set_target(0);
        return py_build_none();
    }

    auto player_set_target(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        get_python_player().set_target(vid);
        return py_build_none();
    }

    auto player_open_character_menu(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        get_python_player().open_character_menu(vid);
        return py_build_none();
    }

    auto player_is_party_member(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        return Py_BuildValue("i", get_python_player().is_party_member_by_vid(vid));
    }

    auto player_is_party_leader(PyObject *self, PyObject *args) -> PyObject * {
        int vid;
        if (!py_tuple_get_integer(args, 0, &vid))
            return py_build_exception();

        uint32_t pid;
        if (!get_python_player().party_member_vid_to_pid(vid, &pid))
            return Py_BuildValue("i", false);

        python_player::party_member_info *party_member_info;
        if (!get_python_player().party_member_ptr(pid, &party_member_info))
            return Py_BuildValue("i", false);

        return Py_BuildValue("i", python_player::PARTY_ROLE_LEADER == party_member_info->state_value);
    }

    auto player_get_party_member_hp_percentage(PyObject *self, PyObject *args) -> PyObject * {
        int pid;
        if (!py_tuple_get_integer(args, 0, &pid))
            return py_build_exception();

        python_player::party_member_info *party_member_info;
        if (!get_python_player().party_member_ptr(pid, &party_member_info))
            return Py_BuildValue("i", false);

        return Py_BuildValue("i", party_member_info->hp_percentage);
    }

    auto player_get_party_member_state(PyObject *self, PyObject *args) -> PyObject * {
        int pid;
        if (!py_tuple_get_integer(args, 0, &pid))
            return py_build_exception();

        python_player::party_member_info *party_member_info;
        if (!get_python_player().party_member_ptr(pid, &party_member_info))
            return Py_BuildValue("i", false);

        return Py_BuildValue("i", party_member_info->state_value);
    }

    auto player_get_party_member_affects(PyObject *self, PyObject *args) -> PyObject * {
        int pid;
        if (!py_tuple_get_integer(args, 0, &pid))
            return py_build_exception();

        python_player::party_member_info *party_member_info;
        if (!get_python_player().party_member_ptr(pid, &party_member_info))
            return Py_BuildValue("i", false);

        return Py_BuildValue("iiiiiii", party_member_info->affects_value[0],
            party_member_info->affects_value[1],
            party_member_info->affects_value[2],
            party_member_info->affects_value[3],
            party_member_info->affects_value[4],
            party_member_info->affects_value[5],
            party_member_info->affects_value[6]);
    }

    auto player_remove_party_member(PyObject *self, PyObject *args) -> PyObject * {
        int pid;
        if (!py_tuple_get_integer(args, 0, &pid))
            return py_build_exception();

        get_python_player().remove_party_member(pid);
        return py_build_none();
    }

    auto player_exit_party(PyObject *self, PyObject *args) -> PyObject * {
        get_python_player().exit_party();
        return py_build_none();
    }

    auto player_get_pk_mode(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().pk_mode());
    }

    auto player_has_mobile_phone_number(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().has_mobile_phone_number());
    }

    auto player_set_weapon_attack_bonus_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag))
            return py_build_exception();

        return py_build_none();
    }

    auto player_toggle_cool_time(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().toggle_cool_time());
    }

    auto player_toggle_level_limit(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().toggle_level_limit());
    }

    auto player_get_target_vid(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_player().get_target_vid());
    }

    auto player_register_emotion_icon(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();
        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_build_exception();

        auto *image = (graphic_image *)get_resource_manager().resource_pointer(file_name);
        map_emotion_index_icon_image.insert(std::make_pair(index, image));

        return py_build_none();
    }

    auto player_get_emotion_icon_image(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        if (map_emotion_index_icon_image.end() == map_emotion_index_icon_image.find(index))
            return Py_BuildValue("n", (Py_ssize_t)0);

        return Py_BuildValue("n", (Py_ssize_t)map_emotion_index_icon_image[index]);
    }

    auto player_set_item_data(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        int virtual_id;
        if (!py_tuple_get_integer(args, 1, &virtual_id))
            return py_build_exception();

        int num;
        if (!py_tuple_get_integer(args, 2, &num))
            return py_build_exception();

        packet_item item_inst;
        memset(&item_inst, 0, sizeof(item_inst));
        item_inst.vnum = virtual_id;
        item_inst.count = num;
        get_python_player().set_item_data(item_pos(INVENTORY, slot_index), item_inst);
        return py_build_none();
    }

    auto player_set_auto_potion_info(PyObject *self, PyObject *args) -> PyObject * {
        int potion_type = 0;
        if (!py_tuple_get_integer(args, 0, &potion_type))
            return py_bad_argument();

        python_player *player = (&get_python_player());

        python_player::auto_potion_info &potion_info = player->get_auto_potion_info(potion_type);

        if (!py_tuple_get_boolean(args, 1, &potion_info.activated))
            return py_bad_argument();

        if (!py_tuple_get_integer(args, 2, &potion_info.current_amount))
            return py_bad_argument();

        if (!py_tuple_get_integer(args, 3, &potion_info.total_amount))
            return py_bad_argument();

        if (!py_tuple_get_integer(args, 4, &potion_info.inventory_slot_index))
            return py_bad_argument();

        return py_build_none();
    }

    auto player_get_auto_potion_info(PyObject *self, PyObject *args) -> PyObject * {
        python_player *player = (&get_python_player());

        int potion_type = 0;
        if (!py_tuple_get_integer(args, 0, &potion_type))
            return py_bad_argument();

        python_player::auto_potion_info &potion_info = player->get_auto_potion_info(potion_type);

        return Py_BuildValue("biii", potion_info.activated, int(potion_info.current_amount), int(potion_info.total_amount), int(potion_info.inventory_slot_index));
    }

    auto player_slot_type_to_inven_type(PyObject *self, PyObject *args) -> PyObject * {
        int slot_type = 0;
        if (!py_tuple_get_integer(args, 0, &slot_type))
            return py_bad_argument();

        return Py_BuildValue("i", slot_type_to_inven_type((uint8_t)slot_type));
    }

    // Whether a belt inventory cell can be used; depends on how far the worn belt is refined.
    auto player_is_available_belt_inventory_cell(PyObject *self, PyObject *args) -> PyObject * {
        const python_player *player = (&get_python_player());
        const packet_item *data = player->get_item_data(item_pos(EQUIPMENT, c_Equipment_Belt));

        if (nullptr == data || 0 == data->count)
            return Py_BuildValue("b", false);

        get_item_manager().select_item_data(data->vnum);
        item_data *item = get_item_manager().selected_item_data_pointer();

        int32_t belt_grade = item->value(0);

        int pos = 0;
        if (!py_tuple_get_integer(args, 0, &pos))
            return py_bad_argument();

        return Py_BuildValue("b", belt_inventory_helper::is_available_cell(pos - c_Belt_Inventory_Slot_Start, belt_grade));
    }

    auto player_send_dragon_soul_refine(PyObject *self, PyObject *args) -> PyObject * {
        uint8_t sub_header;
        PyObject *dic;
        item_pos refine_item_poses[DS_REFINE_WINDOW_MAX_NUM];
        if (!py_tuple_get_byte(args, 0, &sub_header))
            return py_build_exception();
        switch (sub_header) {
        case DS_SUB_HEADER_CLOSE:
            break;
        case DS_SUB_HEADER_DO_UPGRADE:
        case DS_SUB_HEADER_DO_IMPROVEMENT:
        case DS_SUB_HEADER_DO_REFINE:
            {
                if (!py_tuple_get_object(args, 1, &dic))
                    return py_build_exception();
                Py_ssize_t pos = 0;
                PyObject *key, *value;

                while (PyDict_Next(dic, &pos, &key, &value)) {
                    int i = PyInt_AsLong(key);
                    if (i > DS_REFINE_WINDOW_MAX_NUM)
                        return py_build_exception();

                    if (!py_tuple_get_byte(value, 0, &refine_item_poses[i].window_type) || !py_tuple_get_integer(value, 1, &refine_item_poses[i].cell)) {
                        return py_build_exception();
                    }
                }
            }
            break;
        }

        python_network_stream &rns = get_python_network_stream();
        rns.send_dragon_soul_refine_packet(sub_header, refine_item_poses);

        return py_build_none();
    }
    void init_player() {
        static PyMethodDef s_methods[] =
            {
                {"GetAutoPotionInfo", player_get_auto_potion_info, METH_VARARGS},
                {"SetAutoPotionInfo", player_set_auto_potion_info, METH_VARARGS},

                {"PickCloseItem", player_pick_close_item, METH_VARARGS},
                {"SetGameWindow", player_set_game_window, METH_VARARGS},
                {"RegisterCacheEffect", player_register_cache_effect, METH_VARARGS},
                {"SetMouseState", player_set_mouse_state, METH_VARARGS},
                {"SetMouseFunc", player_set_mouse_func, METH_VARARGS},
                {"SetMouseMiddleButtonState", player_set_mouse_middle_button_state, METH_VARARGS},
                {"SetMainCharacterIndex", player_set_main_character_index, METH_VARARGS},
                {"GetMainCharacterIndex", player_get_main_character_index, METH_VARARGS},
                {"GetMainCharacterName", player_get_main_character_name, METH_VARARGS},
                {"GetMainCharacterPosition", player_get_main_character_position, METH_VARARGS},
                {"IsMainCharacterIndex", player_is_main_character_index, METH_VARARGS},
                {"IsActingEmotion", player_is_acting_emotion, METH_VARARGS},
                {"IsPVPInstance", player_is_pvp_instance, METH_VARARGS},
                {"IsSameEmpire", player_is_same_empire, METH_VARARGS},
                {"IsChallengeInstance", player_is_challenge_instance, METH_VARARGS},
                {"IsRevengeInstance", player_is_revenge_instance, METH_VARARGS},
                {"IsCantFightInstance", player_is_cant_fight_instance, METH_VARARGS},
                {"GetCharacterDistance", player_get_character_distance, METH_VARARGS},
                {"IsInSafeArea", player_is_in_safe_area, METH_VARARGS},
                {"IsMountingHorse", player_is_mounting_horse, METH_VARARGS},
                {"IsObserverMode", player_is_observer_mode, METH_VARARGS},

                {"SetAutoCameraRotationSpeed", player_set_auto_camera_rotation_speed, METH_VARARGS},
                {"SetAttackKeyState", player_set_attack_key_state, METH_VARARGS},
                {"SetSingleDIKKeyState", player_set_single_dik_key_state, METH_VARARGS},
                {"EndKeyWalkingImmediately", player_end_key_walking_immediately, METH_VARARGS},
                {"EndMouseWalking", player_end_mouse_walking, METH_VARARGS},
                {"ResetCameraRotation", player_reset_camera_rotation, METH_VARARGS},
                {"SetQuickCameraMode", player_set_quick_camera_mode, METH_VARARGS},

                {"SetSkill", player_set_skill, METH_VARARGS},
                {"GetSkillIndex", player_get_skill_index, METH_VARARGS},
                {"GetSkillSlotIndex", player_get_skill_slot_index, METH_VARARGS},
                {"GetSkillGrade", player_get_skill_grade, METH_VARARGS},
                {"GetSkillLevel", player_get_skill_level, METH_VARARGS},
                {"GetSkillCurrentEfficientPercentage", player_get_skill_current_efficient_percentage, METH_VARARGS},
                {"GetSkillNextEfficientPercentage", player_get_skill_next_efficient_percentage, METH_VARARGS},
                {"ClickSkillSlot", player_click_skill_slot, METH_VARARGS},
                {"ChangeCurrentSkillNumberOnly", player_change_current_skill_number_only, METH_VARARGS},
                {"ClearSkillDict", player_clear_skill_dict, METH_VARARGS},

                {"GetItemIndex", player_get_item_index, METH_VARARGS},
                {"GetItemFlags", player_get_item_flags, METH_VARARGS},
                {"GetItemCount", player_get_item_count, METH_VARARGS},
                {"GetItemCountByVnum", player_get_item_count_by_vnum, METH_VARARGS},
                {"GetItemMetinSocket", player_get_item_metin_socket, METH_VARARGS},
                {"GetItemAttribute", player_get_item_attribute, METH_VARARGS},
                {"GetISellItemPrice", player_get_i_sell_item_price, METH_VARARGS},

                {"GetName", player_get_name, METH_VARARGS},
                {"GetRace", player_get_race, METH_VARARGS},
                {"GetPlayTime", player_get_play_time, METH_VARARGS},
                {"SetPlayTime", player_set_play_time, METH_VARARGS},

                {"IsSkillCoolTime", player_is_skill_cool_time, METH_VARARGS},
                {"GetSkillCoolTime", player_get_skill_cool_time, METH_VARARGS},
                {"IsSkillActive", player_is_skill_active, METH_VARARGS},
                {"UseGuildSkill", player_use_guild_skill, METH_VARARGS},
                {"AffectIndexToSkillIndex", player_affect_index_to_skill_index, METH_VARARGS},
                {"GetEXP", player_get_exp, METH_VARARGS},
                {"GetStatus", player_get_status, METH_VARARGS},
                {"GetElk", player_get_elk, METH_VARARGS},
                {"GetMoney", player_get_elk, METH_VARARGS},
                {"GetGuildID", player_get_guild_id, METH_VARARGS},
                {"GetGuildName", player_get_guild_name, METH_VARARGS},
                {"GetAlignmentData", player_get_alignment_data, METH_VARARGS},
                {"RequestAddLocalQuickSlot", player_request_add_local_quick_slot, METH_VARARGS},
                {"RequestAddToEmptyLocalQuickSlot", player_request_add_to_empty_local_quick_slot, METH_VARARGS},
                {"RequestDeleteGlobalQuickSlot", player_request_delete_global_quick_slot, METH_VARARGS},
                {"RequestMoveGlobalQuickSlotToLocalQuickSlot", player_request_move_global_quick_slot_to_local_quick_slot, METH_VARARGS},
                {"RequestUseLocalQuickSlot", player_request_use_local_quick_slot, METH_VARARGS},
                {"LocalQuickSlotIndexToGlobalQuickSlotIndex", player_local_quick_slot_index_to_global_quick_slot_index, METH_VARARGS},

                {"GetQuickPage", player_get_quick_page, METH_VARARGS},
                {"SetQuickPage", player_set_quick_page, METH_VARARGS},
                {"GetLocalQuickSlot", player_get_local_quick_slot, METH_VARARGS},
                {"GetGlobalQuickSlot", player_get_global_quick_slot, METH_VARARGS},

                {"IsEquipmentSlot", player_is_equipment_slot, METH_VARARGS},
                {"IsDSEquipmentSlot", player_is_ds_equipment_slot, METH_VARARGS},
                {"IsCostumeSlot", player_is_costume_slot, METH_VARARGS},
                {"IsValuableItem", player_is_valuable_item, METH_VARARGS},
                {"IsOpenPrivateShop", player_is_open_private_shop, METH_VARARGS},

                {"IsBeltInventorySlot", player_is_belt_inventory_slot, METH_VARARGS},
                {"IsAvailableBeltInventoryCell", player_is_available_belt_inventory_cell, METH_VARARGS},

                {"GetItemGrade", player_get_item_grade, METH_VARARGS},
                {"CanRefine", player_can_refine, METH_VARARGS},
                {"CanDetach", player_can_detach, METH_VARARGS},
                {"CanUnlock", player_can_unlock, METH_VARARGS},
                {"CanAttachMetin", player_can_attach_metin, METH_VARARGS},
                {"IsRefineGradeScroll", player_is_refine_grade_scroll, METH_VARARGS},

                {"ClearTarget", player_clear_target, METH_VARARGS},
                {"SetTarget", player_set_target, METH_VARARGS},
                {"OpenCharacterMenu", player_open_character_menu, METH_VARARGS},

                {"IsPartyMember", player_is_party_member, METH_VARARGS},
                {"IsPartyLeader", player_is_party_leader, METH_VARARGS},
                {"GetPartyMemberHPPercentage", player_get_party_member_hp_percentage, METH_VARARGS},
                {"GetPartyMemberState", player_get_party_member_state, METH_VARARGS},
                {"GetPartyMemberAffects", player_get_party_member_affects, METH_VARARGS},
                {"RemovePartyMember", player_remove_party_member, METH_VARARGS},
                {"ExitParty", player_exit_party, METH_VARARGS},

                {"GetPKMode", player_get_pk_mode, METH_VARARGS},

                {"HasMobilePhoneNumber", player_has_mobile_phone_number, METH_VARARGS},

                {"RegisterEmotionIcon", player_register_emotion_icon, METH_VARARGS},
                {"GetEmotionIconImage", player_get_emotion_icon_image, METH_VARARGS},

                {"SetWeaponAttackBonusFlag", player_set_weapon_attack_bonus_flag, METH_VARARGS},
                {"ToggleCoolTime", player_toggle_cool_time, METH_VARARGS},
                {"ToggleLevelLimit", player_toggle_level_limit, METH_VARARGS},
                {"GetTargetVID", player_get_target_vid, METH_VARARGS},

                {"SetItemData", player_set_item_data, METH_VARARGS},
                {"SetItemCount", player_set_item_count, METH_VARARGS},

                {"GetItemLink", player_get_item_link, METH_VARARGS},
                {"SlotTypeToInvenType", player_slot_type_to_inven_type, METH_VARARGS},
                {"SendDragonSoulRefine", player_send_dragon_soul_refine, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = Py_InitModule("player", s_methods);
        PyModule_AddIntConstant(module_value, "LEVEL", POINT_LEVEL);
        PyModule_AddIntConstant(module_value, "VOICE", POINT_VOICE);
        PyModule_AddIntConstant(module_value, "EXP", POINT_EXP);
        PyModule_AddIntConstant(module_value, "NEXT_EXP", POINT_NEXT_EXP);
        PyModule_AddIntConstant(module_value, "HP", POINT_HP);
        PyModule_AddIntConstant(module_value, "MAX_HP", POINT_MAX_HP);
        PyModule_AddIntConstant(module_value, "SP", POINT_SP);
        PyModule_AddIntConstant(module_value, "MAX_SP", POINT_MAX_SP);
        PyModule_AddIntConstant(module_value, "STAMINA", POINT_STAMINA);
        PyModule_AddIntConstant(module_value, "MAX_STAMINA", POINT_MAX_STAMINA);
        PyModule_AddIntConstant(module_value, "ELK", POINT_GOLD);
        PyModule_AddIntConstant(module_value, "ST", POINT_ST);
        PyModule_AddIntConstant(module_value, "HT", POINT_HT);
        PyModule_AddIntConstant(module_value, "DX", POINT_DX);
        PyModule_AddIntConstant(module_value, "IQ", POINT_IQ);
        PyModule_AddIntConstant(module_value, "ATT_POWER", POINT_ATT_POWER);
        PyModule_AddIntConstant(module_value, "ATT_MIN", POINT_MIN_ATK);
        PyModule_AddIntConstant(module_value, "ATT_MAX", POINT_MAX_ATK);
        PyModule_AddIntConstant(module_value, "MIN_MAGIC_WEP", POINT_MIN_MAGIC_WEP);
        PyModule_AddIntConstant(module_value, "MAX_MAGIC_WEP", POINT_MAX_MAGIC_WEP);
        PyModule_AddIntConstant(module_value, "ATT_SPEED", POINT_ATT_SPEED);
        PyModule_AddIntConstant(module_value, "ATT_BONUS", POINT_ATT_GRADE_BONUS);
        PyModule_AddIntConstant(module_value, "EVADE_RATE", POINT_EVADE_RATE);
        PyModule_AddIntConstant(module_value, "MOVING_SPEED", POINT_MOV_SPEED);
        PyModule_AddIntConstant(module_value, "DEF_GRADE", POINT_DEF_GRADE);
        PyModule_AddIntConstant(module_value, "DEF_BONUS", POINT_DEF_GRADE_BONUS);
        PyModule_AddIntConstant(module_value, "CASTING_SPEED", POINT_CASTING_SPEED);
        PyModule_AddIntConstant(module_value, "MAG_ATT", POINT_MAGIC_ATT_GRADE);
        PyModule_AddIntConstant(module_value, "MAG_DEF", POINT_MAGIC_DEF_GRADE);
        PyModule_AddIntConstant(module_value, "EMPIRE_POINT", POINT_EMPIRE_POINT);
        PyModule_AddIntConstant(module_value, "STAT", POINT_STAT);
        PyModule_AddIntConstant(module_value, "SKILL_PASSIVE", POINT_SUB_SKILL);
        PyModule_AddIntConstant(module_value, "SKILL_SUPPORT", POINT_SUB_SKILL);
        PyModule_AddIntConstant(module_value, "SKILL_ACTIVE", POINT_SKILL);
        PyModule_AddIntConstant(module_value, "SKILL_HORSE", POINT_HORSE_SKILL);
        PyModule_AddIntConstant(module_value, "PLAYTIME", POINT_PLAYTIME);
        PyModule_AddIntConstant(module_value, "BOW_DISTANCE", POINT_BOW_DISTANCE);
        PyModule_AddIntConstant(module_value, "HP_RECOVERY", POINT_HP_RECOVERY);
        PyModule_AddIntConstant(module_value, "SP_RECOVERY", POINT_SP_RECOVERY);
        PyModule_AddIntConstant(module_value, "ATTACKER_BONUS", POINT_PARTY_ATT_GRADE);
        PyModule_AddIntConstant(module_value, "MAX_NUM", POINT_MAX_NUM);
        PyModule_AddIntConstant(module_value, "POINT_CRITICAL_PCT", POINT_CRITICAL_PCT);
        PyModule_AddIntConstant(module_value, "POINT_PENETRATE_PCT", POINT_PENETRATE_PCT);
        PyModule_AddIntConstant(module_value, "POINT_MALL_ATTBONUS", POINT_MALL_ATTBONUS);
        PyModule_AddIntConstant(module_value, "POINT_MALL_DEFBONUS", POINT_MALL_DEFBONUS);
        PyModule_AddIntConstant(module_value, "POINT_MALL_EXPBONUS", POINT_MALL_EXPBONUS);
        PyModule_AddIntConstant(module_value, "POINT_MALL_ITEMBONUS", POINT_MALL_ITEMBONUS);
        PyModule_AddIntConstant(module_value, "POINT_MALL_GOLDBONUS", POINT_MALL_GOLDBONUS);
        PyModule_AddIntConstant(module_value, "POINT_MAX_HP_PCT", POINT_MAX_HP_PCT);
        PyModule_AddIntConstant(module_value, "POINT_MAX_SP_PCT", POINT_MAX_SP_PCT);

        PyModule_AddIntConstant(module_value, "POINT_SKILL_DAMAGE_BONUS", POINT_SKILL_DAMAGE_BONUS);
        PyModule_AddIntConstant(module_value, "POINT_NORMAL_HIT_DAMAGE_BONUS", POINT_NORMAL_HIT_DAMAGE_BONUS);
        PyModule_AddIntConstant(module_value, "POINT_SKILL_DEFEND_BONUS", POINT_SKILL_DEFEND_BONUS);
        PyModule_AddIntConstant(module_value, "POINT_NORMAL_HIT_DEFEND_BONUS", POINT_NORMAL_HIT_DEFEND_BONUS);
        PyModule_AddIntConstant(module_value, "POINT_PC_BANG_EXP_BONUS", POINT_PC_BANG_EXP_BONUS);
        PyModule_AddIntConstant(module_value, "POINT_PC_BANG_DROP_BONUS", POINT_PC_BANG_DROP_BONUS);

        PyModule_AddIntConstant(module_value, "ENERGY", POINT_ENERGY);
        PyModule_AddIntConstant(module_value, "ENERGY_END_TIME", POINT_ENERGY_END_TIME);

        PyModule_AddIntConstant(module_value, "SKILL_GRADE_NORMAL", python_player::SKILL_NORMAL);
        PyModule_AddIntConstant(module_value, "SKILL_GRADE_MASTER", python_player::SKILL_MASTER);
        PyModule_AddIntConstant(module_value, "SKILL_GRADE_GRAND_MASTER", python_player::SKILL_GRAND_MASTER);
        PyModule_AddIntConstant(module_value, "SKILL_GRADE_PERFECT_MASTER", python_player::SKILL_PERFECT_MASTER);

        PyModule_AddIntConstant(module_value, "CATEGORY_ACTIVE", python_player::CATEGORY_ACTIVE);
        PyModule_AddIntConstant(module_value, "CATEGORY_PASSIVE", python_player::CATEGORY_PASSIVE);

        PyModule_AddIntConstant(module_value, "INVENTORY_PAGE_SIZE", c_Inventory_Page_Size);
        PyModule_AddIntConstant(module_value, "INVENTORY_PAGE_COUNT", c_Inventory_Page_Count);
        PyModule_AddIntConstant(module_value, "INVENTORY_SLOT_COUNT", c_Inventory_Count);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_SLOT_START", c_Equipment_Start);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_PAGE_COUNT", c_Equipment_Count);

        PyModule_AddIntConstant(module_value, "NEW_EQUIPMENT_SLOT_START", c_New_Equipment_Start);
        PyModule_AddIntConstant(module_value, "NEW_EQUIPMENT_SLOT_COUNT", c_New_Equipment_Count);

        PyModule_AddIntConstant(module_value, "MBF_SKILL", python_player::MBF_SKILL);
        PyModule_AddIntConstant(module_value, "MBF_ATTACK", python_player::MBF_ATTACK);
        PyModule_AddIntConstant(module_value, "MBF_CAMERA", python_player::MBF_CAMERA);
        PyModule_AddIntConstant(module_value, "MBF_SMART", python_player::MBF_SMART);
        PyModule_AddIntConstant(module_value, "MBF_MOVE", python_player::MBF_MOVE);
        PyModule_AddIntConstant(module_value, "MBF_AUTO", python_player::MBF_AUTO);
        PyModule_AddIntConstant(module_value, "MBS_PRESS", python_player::MBS_PRESS);
        PyModule_AddIntConstant(module_value, "MBS_CLICK", python_player::MBS_CLICK);
        PyModule_AddIntConstant(module_value, "MBT_RIGHT", python_player::MBT_RIGHT);
        PyModule_AddIntConstant(module_value, "MBT_LEFT", python_player::MBT_LEFT);

        // Shared with the server.
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_NONE", SLOT_TYPE_NONE);
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_INVENTORY", SLOT_TYPE_INVENTORY);
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_SKILL", SLOT_TYPE_SKILL);
        // Client-only slot types.
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_SHOP", SLOT_TYPE_SHOP);
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_EXCHANGE_OWNER", SLOT_TYPE_EXCHANGE_OWNER);
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_EXCHANGE_TARGET", SLOT_TYPE_EXCHANGE_TARGET);
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_QUICK_SLOT", SLOT_TYPE_QUICK_SLOT);
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_SAFEBOX", SLOT_TYPE_SAFEBOX);
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_PRIVATE_SHOP", SLOT_TYPE_PRIVATE_SHOP);
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_MALL", SLOT_TYPE_MALL);
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_EMOTION", SLOT_TYPE_EMOTION);
        PyModule_AddIntConstant(module_value, "SLOT_TYPE_DRAGON_SOUL_INVENTORY", SLOT_TYPE_DRAGON_SOUL_INVENTORY);

        PyModule_AddIntConstant(module_value, "RESERVED_WINDOW", RESERVED_WINDOW);
        PyModule_AddIntConstant(module_value, "INVENTORY", INVENTORY);
        PyModule_AddIntConstant(module_value, "EQUIPMENT", EQUIPMENT);
        PyModule_AddIntConstant(module_value, "SAFEBOX", SAFEBOX);
        PyModule_AddIntConstant(module_value, "MALL", MALL);
        PyModule_AddIntConstant(module_value, "DRAGON_SOUL_INVENTORY", DRAGON_SOUL_INVENTORY);
        PyModule_AddIntConstant(module_value, "GROUND", GROUND);

        PyModule_AddIntConstant(module_value, "ITEM_MONEY", -1);

        PyModule_AddIntConstant(module_value, "SKILL_SLOT_COUNT", SKILL_MAX_NUM);

        PyModule_AddIntConstant(module_value, "EFFECT_PICK", python_player::EFFECT_PICK);

        PyModule_AddIntConstant(module_value, "METIN_SOCKET_TYPE_NONE", python_player::METIN_SOCKET_TYPE_NONE);
        PyModule_AddIntConstant(module_value, "METIN_SOCKET_TYPE_SILVER", python_player::METIN_SOCKET_TYPE_SILVER);
        PyModule_AddIntConstant(module_value, "METIN_SOCKET_TYPE_GOLD", python_player::METIN_SOCKET_TYPE_GOLD);
        PyModule_AddIntConstant(module_value, "METIN_SOCKET_MAX_NUM", ITEM_SOCKET_SLOT_MAX_NUM);
        PyModule_AddIntConstant(module_value, "ATTRIBUTE_SLOT_MAX_NUM", ITEM_ATTRIBUTE_SLOT_MAX_NUM);

        PyModule_AddIntConstant(module_value, "REFINE_CANT", REFINE_CANT);
        PyModule_AddIntConstant(module_value, "REFINE_OK", REFINE_OK);
        PyModule_AddIntConstant(module_value, "REFINE_ALREADY_MAX_SOCKET_COUNT", REFINE_ALREADY_MAX_SOCKET_COUNT);
        PyModule_AddIntConstant(module_value, "REFINE_NEED_MORE_GOOD_SCROLL", REFINE_NEED_MORE_GOOD_SCROLL);
        PyModule_AddIntConstant(module_value, "REFINE_CANT_MAKE_SOCKET_ITEM", REFINE_CANT_MAKE_SOCKET_ITEM);
        PyModule_AddIntConstant(module_value, "REFINE_NOT_NEXT_GRADE_ITEM", REFINE_NOT_NEXT_GRADE_ITEM);
        PyModule_AddIntConstant(module_value, "REFINE_CANT_REFINE_METIN_TO_EQUIPMENT", REFINE_CANT_REFINE_METIN_TO_EQUIPMENT);
        PyModule_AddIntConstant(module_value, "REFINE_CANT_REFINE_ROD", REFINE_CANT_REFINE_ROD);
        PyModule_AddIntConstant(module_value, "ATTACH_METIN_CANT", ATTACH_METIN_CANT);
        PyModule_AddIntConstant(module_value, "ATTACH_METIN_OK", ATTACH_METIN_OK);
        PyModule_AddIntConstant(module_value, "ATTACH_METIN_NOT_MATCHABLE_ITEM", ATTACH_METIN_NOT_MATCHABLE_ITEM);
        PyModule_AddIntConstant(module_value, "ATTACH_METIN_NO_MATCHABLE_SOCKET", ATTACH_METIN_NO_MATCHABLE_SOCKET);
        PyModule_AddIntConstant(module_value, "ATTACH_METIN_NOT_EXIST_GOLD_SOCKET", ATTACH_METIN_NOT_EXIST_GOLD_SOCKET);
        PyModule_AddIntConstant(module_value, "ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT", ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT);
        PyModule_AddIntConstant(module_value, "DETACH_METIN_CANT", DETACH_METIN_CANT);
        PyModule_AddIntConstant(module_value, "DETACH_METIN_OK", DETACH_METIN_OK);

        PyModule_AddIntConstant(module_value, "PARTY_STATE_NORMAL", python_player::PARTY_ROLE_NORMAL);
        PyModule_AddIntConstant(module_value, "PARTY_STATE_LEADER", python_player::PARTY_ROLE_LEADER);
        PyModule_AddIntConstant(module_value, "PARTY_STATE_ATTACKER", python_player::PARTY_ROLE_ATTACKER);
        PyModule_AddIntConstant(module_value, "PARTY_STATE_TANKER", python_player::PARTY_ROLE_TANKER);
        PyModule_AddIntConstant(module_value, "PARTY_STATE_BUFFER", python_player::PARTY_ROLE_BUFFER);
        PyModule_AddIntConstant(module_value, "PARTY_STATE_SKILL_MASTER", python_player::PARTY_ROLE_SKILL_MASTER);
        PyModule_AddIntConstant(module_value, "PARTY_STATE_BERSERKER", python_player::PARTY_ROLE_BERSERKER);
        PyModule_AddIntConstant(module_value, "PARTY_STATE_DEFENDER", python_player::PARTY_ROLE_DEFENDER);
        PyModule_AddIntConstant(module_value, "PARTY_STATE_MAX_NUM", python_player::PARTY_ROLE_MAX_NUM);

        PyModule_AddIntConstant(module_value, "SKILL_INDEX_TONGSOL", c_iSkillIndex_Tongsol);
        PyModule_AddIntConstant(module_value, "SKILL_INDEX_FISHING", c_iSkillIndex_Fishing);
        PyModule_AddIntConstant(module_value, "SKILL_INDEX_MINING", c_iSkillIndex_Mining);
        PyModule_AddIntConstant(module_value, "SKILL_INDEX_MAKING", c_iSkillIndex_Making);
        PyModule_AddIntConstant(module_value, "SKILL_INDEX_COMBO", c_iSkillIndex_Combo);
        PyModule_AddIntConstant(module_value, "SKILL_INDEX_LANGUAGE1", c_iSkillIndex_Language1);
        PyModule_AddIntConstant(module_value, "SKILL_INDEX_LANGUAGE2", c_iSkillIndex_Language2);
        PyModule_AddIntConstant(module_value, "SKILL_INDEX_LANGUAGE3", c_iSkillIndex_Language3);
        PyModule_AddIntConstant(module_value, "SKILL_INDEX_POLYMORPH", c_iSkillIndex_Polymorph);
        PyModule_AddIntConstant(module_value, "SKILL_INDEX_RIDING", c_iSkillIndex_Riding);
        PyModule_AddIntConstant(module_value, "SKILL_INDEX_SUMMON", skill_index_summon);

        PyModule_AddIntConstant(module_value, "PK_MODE_PEACE", PK_MODE_PEACE);
        PyModule_AddIntConstant(module_value, "PK_MODE_REVENGE", PK_MODE_REVENGE);
        PyModule_AddIntConstant(module_value, "PK_MODE_FREE", PK_MODE_FREE);
        PyModule_AddIntConstant(module_value, "PK_MODE_PROTECT", PK_MODE_PROTECT);
        PyModule_AddIntConstant(module_value, "PK_MODE_GUILD", PK_MODE_GUILD);
        PyModule_AddIntConstant(module_value, "PK_MODE_MAX_NUM", PK_MODE_MAX_NUM);

        PyModule_AddIntConstant(module_value, "BLOCK_EXCHANGE", BLOCK_EXCHANGE);
        PyModule_AddIntConstant(module_value, "BLOCK_PARTY", BLOCK_PARTY_INVITE);
        PyModule_AddIntConstant(module_value, "BLOCK_GUILD", BLOCK_GUILD_INVITE);
        PyModule_AddIntConstant(module_value, "BLOCK_WHISPER", BLOCK_WHISPER);
        PyModule_AddIntConstant(module_value, "BLOCK_FRIEND", BLOCK_MESSENGER_INVITE);
        PyModule_AddIntConstant(module_value, "BLOCK_PARTY_REQUEST", BLOCK_PARTY_REQUEST);

        PyModule_AddIntConstant(module_value, "PARTY_EXP_NON_DISTRIBUTION", PARTY_EXP_DISTRIBUTION_NON_PARITY);
        PyModule_AddIntConstant(module_value, "PARTY_EXP_DISTRIBUTION_PARITY", PARTY_EXP_DISTRIBUTION_PARITY);

        PyModule_AddIntConstant(module_value, "EMOTION_CLAP", EMOTION_CLAP);
        PyModule_AddIntConstant(module_value, "EMOTION_CHEERS_1", EMOTION_CHEERS_1);
        PyModule_AddIntConstant(module_value, "EMOTION_CHEERS_2", EMOTION_CHEERS_2);
        PyModule_AddIntConstant(module_value, "EMOTION_DANCE_1", EMOTION_DANCE_1);
        PyModule_AddIntConstant(module_value, "EMOTION_DANCE_2", EMOTION_DANCE_2);
        PyModule_AddIntConstant(module_value, "EMOTION_DANCE_3", EMOTION_DANCE_3);
        PyModule_AddIntConstant(module_value, "EMOTION_DANCE_4", EMOTION_DANCE_4);
        PyModule_AddIntConstant(module_value, "EMOTION_DANCE_5", EMOTION_DANCE_5);
        PyModule_AddIntConstant(module_value, "EMOTION_DANCE_6", EMOTION_DANCE_6);
        PyModule_AddIntConstant(module_value, "EMOTION_CONGRATULATION", EMOTION_CONGRATULATION);
        PyModule_AddIntConstant(module_value, "EMOTION_FORGIVE", EMOTION_FORGIVE);
        PyModule_AddIntConstant(module_value, "EMOTION_ANGRY", EMOTION_ANGRY);
        PyModule_AddIntConstant(module_value, "EMOTION_ATTRACTIVE", EMOTION_ATTRACTIVE);
        PyModule_AddIntConstant(module_value, "EMOTION_SAD", EMOTION_SAD);
        PyModule_AddIntConstant(module_value, "EMOTION_SHY", EMOTION_SHY);
        PyModule_AddIntConstant(module_value, "EMOTION_CHEERUP", EMOTION_CHEERUP);
        PyModule_AddIntConstant(module_value, "EMOTION_BANTER", EMOTION_BANTER);
        PyModule_AddIntConstant(module_value, "EMOTION_JOY", EMOTION_JOY);

        PyModule_AddIntConstant(module_value, "EMOTION_KISS", EMOTION_KISS);
        PyModule_AddIntConstant(module_value, "EMOTION_FRENCH_KISS", EMOTION_FRENCH_KISS);
        PyModule_AddIntConstant(module_value, "EMOTION_SLAP", EMOTION_SLAP);

        PyModule_AddIntConstant(module_value, "AUTO_POTION_TYPE_HP", python_player::AUTO_POTION_TYPE_HP);
        PyModule_AddIntConstant(module_value, "AUTO_POTION_TYPE_SP", python_player::AUTO_POTION_TYPE_SP);

        PyModule_AddIntConstant(module_value, "DRAGON_SOUL_PAGE_SIZE", c_DragonSoul_Inventory_Box_Size);
        PyModule_AddIntConstant(module_value, "DRAGON_SOUL_PAGE_COUNT", DRAGON_SOUL_GRADE_MAX);
        PyModule_AddIntConstant(module_value, "DRAGON_SOUL_SLOT_COUNT", c_DragonSoul_Inventory_Count);
        PyModule_AddIntConstant(module_value, "DRAGON_SOUL_EQUIPMENT_SLOT_START", c_DragonSoul_Equip_Start);
        PyModule_AddIntConstant(module_value, "DRAGON_SOUL_EQUIPMENT_PAGE_COUNT", DS_DECK_MAX_NUM);
        PyModule_AddIntConstant(module_value, "DRAGON_SOUL_EQUIPMENT_FIRST_SIZE", c_DragonSoul_Equip_Slot_Max);

        PyModule_AddIntConstant(module_value, "DRAGON_SOUL_REFINE_CLOSE", DS_SUB_HEADER_CLOSE);
        PyModule_AddIntConstant(module_value, "DS_SUB_HEADER_DO_UPGRADE", DS_SUB_HEADER_DO_UPGRADE);
        PyModule_AddIntConstant(module_value, "DS_SUB_HEADER_DO_IMPROVEMENT", DS_SUB_HEADER_DO_IMPROVEMENT);
        PyModule_AddIntConstant(module_value, "DS_SUB_HEADER_DO_REFINE", DS_SUB_HEADER_DO_REFINE);
    }
}
