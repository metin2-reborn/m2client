#include <algorithm>
#include <metin2/client/characters/instance_base.h>
#include <metin2/client/guilds/python_guild.h>
#include <metin2/client/protocol/python_network_stream.h>
#include <metin2/client/social/python_chat.h>
#include <metin2/client/social/python_text_tail.h>
#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/items/item_manager.h>
#include "python_player.h"

namespace metin2::inline world::inline actors {
    extern auto is_huge_race(unsigned int vnum) -> bool;
}
namespace metin2::inline client::inline player {

    void python_player::clear_affects() {
        py_call_class_member_func(ppy_game_window_, "ClearAffects", Py_BuildValue("()"));
    }

    void python_player::set_affect(uint32_t affect) {
        py_call_class_member_func(ppy_game_window_, "SetAffect", Py_BuildValue("(i)", affect));

        uint32_t skill_index_value;
        if (!affect_index_to_skill_index(affect, &skill_index_value))
            return;

        python_skill::skill_data *skill_data_value;
        if (!get_python_skill().get_skill_data(skill_index_value, &skill_data_value))
            return;

        if (!skill_data_value->is_toggle_skill())
            return;

        uint32_t skill_slot_index_value;
        if (!skill_slot_index(skill_index_value, &skill_slot_index_value))
            return;

        activate_skill_slot(skill_slot_index_value);
    }

    void python_player::reset_affect(uint32_t affect) {
        // Forward every affect, or non-skill effects do not disappear.
        py_call_class_member_func(ppy_game_window_, "ResetAffect", Py_BuildValue("(i)", affect));

        uint32_t skill_index_value;
        if (!affect_index_to_skill_index(affect, &skill_index_value))
            return;

        python_skill::skill_data *skill_data_value;
        if (!get_python_skill().get_skill_data(skill_index_value, &skill_data_value))
            return;

        if (!skill_data_value->is_toggle_skill())
            return;

        uint32_t skill_slot_index_value;
        if (!skill_slot_index(skill_index_value, &skill_slot_index_value))
            return;

        deactivate_skill_slot(skill_slot_index_value);
    }

    auto python_player::find_skill_slot_index_by_skill_index(uint32_t skill_index, uint32_t *skill_slot_index) -> bool {
        for (int i = 0; i < SKILL_MAX_NUM; ++i) {
            skill_instance &skill_inst = player_status_value_.skill[i];
            if (skill_index == skill_inst.index_value) {
                *skill_slot_index = i;
                return true;
            }
        }

        return false;
    }

    void python_player::change_current_skill_number_only(uint32_t slot_index) {
        if (slot_index >= SKILL_MAX_NUM)
            return;

        skill_instance &skill_inst = player_status_value_.skill[slot_index];

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_inst.index_value, &skill_data))
            return;

        if (!skill_data->is_can_use_skill())
            return;

        if (!is_right_button_skill_mode()) {
            if (!is_target()) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NEED_TARGET"));
                return;
            }

            click_skill_slot(slot_index);
        } else {
            dwcur_skill_slot_index_ = slot_index;
            py_call_class_member_func(ppy_game_window_, "ChangeCurrentSkill", Py_BuildValue("(i)", slot_index));
        }
    }

    void python_player::click_skill_slot(uint32_t slot_index) {
        if (slot_index >= SKILL_MAX_NUM)
            return;

        skill_instance &skill_inst = player_status_value_.skill[slot_index];

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_inst.index_value, &skill_data))
            return;

        if (python_skill::SKILL_TYPE_GUILD == skill_data->type_value) {
            use_guild_skill(slot_index);
            return;
        }

        if (!skill_data->is_can_use_skill())
            return;

        if (skill_data->is_standing_skill()) {
            if (skill_data->is_toggle_skill()) {
                if (is_skill_active(slot_index)) {
                    instance_base *inst_main = new_get_main_actor_ptr();
                    if (!inst_main)
                        return;
                    if (inst_main->is_using_skill())
                        return;

                    get_python_network_stream().send_use_skill_packet(skill_inst.index_value);
                    return;
                }
            }

            use_skill(slot_index);
            return;
        }

        if (dwcur_skill_slot_index_ == slot_index) {
            use_skill(dwcur_skill_slot_index_);
            return;
        }

        if (!is_right_button_skill_mode()) {
            use_skill(slot_index);
        } else {
            dwcur_skill_slot_index_ = slot_index;
            py_call_class_member_func(ppy_game_window_, "ChangeCurrentSkill", Py_BuildValue("(i)", slot_index));
        }
    }

    auto python_player::check_skill_usable(uint32_t slot_index) -> bool {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return false;

        if (slot_index >= SKILL_MAX_NUM)
            return false;

        skill_instance &skill_inst = player_status_value_.skill[slot_index];

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_inst.index_value, &skill_data))
            return false;

        if (inst_main->is_mounting_horse()) {
            if (!skill_data->is_horse_skill()) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NOT_HORSE_SKILL"));
                return false;
            }
        }

        if (skill_data->is_horse_skill()) {
            if (!inst_main->is_mounting_horse()) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "HAVE_TO_RIDE"));
                return false;
            }
        }

        if (skill_data->is_attack_skill()) {
            if (inst_main->is_in_safe()) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "IN_SAFE"));

                return false;
            }
        }

        // Passive skills cannot be used.
        if (!skill_data->is_can_use_skill())
            return false;

        // Assassin only: item 27995 is the empty bottle.
        if (skill_data->is_need_empty_bottle()) {
            if (!has_item(27995)) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NEED_EMPTY_BOTTLE"));
                return false;
            }
        }

        // Assassin only: item 27996 is the poison bottle.
        if (skill_data->is_need_poison_bottle()) {
            if (!has_item(27996)) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NEED_POISON_BOTTLE"));
                return false;
            }
        }

        if (inst_main->is_fishing_mode()) {
            py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "REMOVE_FISHING_ROD"));
            return false;
        }

        if (sys_is_level_limit_) {
            if (skill_inst.level_value <= 0) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NOT_YET_LEARN"));
                return false;
            }
        }

        if (!skill_data->can_use_weapon_type(inst_main->weapon_type())) {
            py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NOT_MATCHABLE_WEAPON"));
            return false;
        }

        if (!skill_data->is_horse_skill()) // Some horse skills do not use arrows.
        {
            if (check_short_arrow(skill_inst, *skill_data))
                return false;

            if (skill_data->is_need_bow()) {
                if (!has_enough_arrow())
                    return false;
            }
        }

        if (check_dash_affect(*inst_main)) {
            if (!skill_data->is_charge_skill()) {
                if (check_rest_skill_cool_time(slot_index)) {
                    py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "WAIT_COOLTIME"));
                    return false;
                }
            }
        } else {
            if (check_rest_skill_cool_time(slot_index)) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "WAIT_COOLTIME"));
                return false;
            }

            if (check_short_life(skill_inst, *skill_data)) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NOT_ENOUGH_HP"));
                return false;
            }

            if (check_short_mana(skill_inst, *skill_data)) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NOT_ENOUGH_SP"));
                return false;
            }
        }

        return true;
    }

    auto python_player::check_short_arrow(skill_instance &skill_inst, python_skill::skill_data &skill_data) -> bool {
        if (!skill_data.is_need_bow())
            return false;

        if (has_enough_arrow())
            return false;

        return true;
    }

    auto python_player::check_short_mana(skill_instance &skill_inst, python_skill::skill_data &skill_data) -> bool {
        extern const uint32_t skill_index_summon;
        if (skill_index_summon == skill_inst.index_value)
            return false;

        int need_sp = skill_data.need_sp(skill_inst.fcur_efficient_percentage);
        int icur_sp = status(POINT_SP);

        // A non-toggle skill with an SP cost of 0 means the skill data is broken.
        if (!skill_data.is_toggle_skill()) {
            if (need_sp == 0) {
                get_python_chat().append_chat(CHAT_TYPE_INFO, "!!! Find strange game data. Please reinstall metin2.");
                return true;
            }
        }

        if (skill_data.can_use_if_not_enough()) {
            if (icur_sp <= 0)
                return true;
        } else {
            if (-1 != need_sp)
                if (need_sp > icur_sp)
                    return true;
        }

        return false;
    }

    auto python_player::check_short_life(skill_instance &skill_inst, python_skill::skill_data &skill_data) -> bool {
        if (!skill_data.is_use_hp_skill())
            return false;

        uint32_t need_hp = skill_data.need_sp(skill_inst.fcur_efficient_percentage);
        if (need_hp <= uint32_t(status(POINT_HP)))
            return false;

        return true;
    }

    auto python_player::check_rest_skill_cool_time(uint32_t slot_index) -> bool {
        if (!sys_is_cool_time_)
            return false;

        if (slot_index >= SKILL_MAX_NUM)
            return false;

        float elapsed_time = get_timer().current_second() - player_status_value_.skill[slot_index].last_used_time;
        if (elapsed_time >= player_status_value_.skill[slot_index].cool_time)
            return false;

        return true;
    }

    auto python_player::check_dash_affect(instance_base &inst_main) -> bool {
        return inst_main.is_affect(instance_base::AFFECT_DASH);
    }

    void python_player::use_current_skill() {
        use_skill(dwcur_skill_slot_index_);
    }

    auto python_player::skill_target_range(python_skill::skill_data &skill_data) -> uint32_t {
        return skill_data.get_target_range() + status(POINT_BOW_DISTANCE) * 100;
    }

    auto python_player::process_enemy_skill_target_range(instance_base &inst_main, instance_base &inst_target, python_skill::skill_data &skill_data, uint32_t skill_slot_index) -> bool {
        uint32_t dw_skill_target_range = skill_target_range(skill_data);
        auto f_skill_target_range = float(dw_skill_target_range);
        if (f_skill_target_range <= 0.0f)
            return true;

        // Huge races cannot be approached, so allow extra distance (#0000806: Tanhwangyeok failed on the water dragon).
        float target_distance = inst_main.distance(&inst_target);

        if (is_huge_race(inst_target.race())) {
            target_distance -= 200.0f; // TODO: hard-coded; this should use the bounding sphere.
        }

        if (target_distance >= f_skill_target_range) {
            if (skill_data.is_charge_skill()) {
                if (!is_reserved_use_skill(skill_slot_index))
                    send_use_skill(skill_slot_index, 0);
            }

            reserve_use_skill(inst_target.virtual_id(), skill_slot_index, dw_skill_target_range);

            return false;
        }

        // Check the destination is reachable first, or Gungsin Tanyoung gets stuck in the map.
        pixel_position p_pos_target;
        inst_target.new_get_pixel_position(&p_pos_target);

        background_interface &bg = get_background_interface();
        if (bg.is_block(p_pos_target.x, p_pos_target.y)) {
            py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "CANNOT_APPROACH"));
            return false;
        }

        return true;
    }

    auto python_player::can_use_skill_impl() -> bool {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return false;

        if (is_observer_mode())
            return false;

        // TODO: the server should do this check. Below horse riding level 20 no riding skills can be used.
        if (inst_main->is_mounting_horse() && (skill_grade(109) < 1 && skill_level(109) < 20)) {
            return false;
        }

        return inst_main->can_use_skill();
    }

    auto python_player::use_skill(uint32_t slot_index) -> bool {
        if (is_open_private_shop()) {
            return true;
        }

        if (!can_use_skill_impl()) {
            return false;
        }

        if (slot_index >= SKILL_MAX_NUM) {
            metin2::log::debug("Slot {}: not a usable skill slot", slot_index);
            return false;
        }

        skill_instance &skill_inst = player_status_value_.skill[slot_index];

        if (check_special_skill(skill_inst.index_value)) {
            return true;
        }

        python_skill::skill_data *skill_data_value;
        if (!get_python_skill().get_skill_data(skill_inst.index_value, &skill_data_value)) {
            metin2::log::debug("Slot {}: no skill data", slot_index);
            return false;
        }

        if (skill_data_value->is_toggle_skill()) {
            if (is_skill_active(slot_index)) {
                get_python_network_stream().send_use_skill_packet(skill_inst.index_value, slot_index);
                return false;
            }
        }

        if (!check_skill_usable(slot_index)) {
            return false;
        }

        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main) {
            metin2::log::debug("Slot {}: cannot use skill without a main player", slot_index);
            return false;
        }

        if (inst_main->is_using_skill()) {
            return false;
        }

        instance_base *inst_target = nullptr;

        if (skill_data_value->is_need_target() ||
            skill_data_value->can_change_direction() ||
            skill_data_value->is_auto_search_target()) {
            if (skill_data_value->is_need_corpse())
                inst_target = dead_target_instance_ptr();
            else
                inst_target = alive_target_instance_ptr();

            if (!inst_target) {
                change_target_to_picked_instance();

                if (skill_data_value->is_need_corpse())
                    inst_target = dead_target_instance_ptr();
                else
                    inst_target = alive_target_instance_ptr();
            }

            if (inst_target) {
                if (skill_data_value->is_only_for_alliance()) {
                    if (inst_main == inst_target) {
                        if (!skill_data_value->can_use_for_me()) {
                            py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "CANNOT_USE_SELF"));
                            return false;
                        }
                    } else if (!inst_main->is_attackable_instance(*inst_target) && inst_target->is_pc()) {
                        uint32_t skill_range = skill_target_range(*skill_data_value);

                        if (skill_range > 0) {
                            float distance_value = inst_main->distance(inst_target);
                            if (distance_value >= float(skill_range)) {
                                reserve_use_skill(inst_target->virtual_id(), slot_index, skill_range);
                                return false;
                            }
                        }
                    } else {
                        if (skill_data_value->can_use_for_me()) {
                            inst_target = inst_main;
                            metin2::log::debug("Skill for allies targeted an enemy, retargeted at the player");
                        } else {
                            py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "ONLY_FOR_ALLIANCE"));
                            return false;
                        }
                    }
                } else {
                    if (inst_target->is_in_safe()) {
                        py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "CANNOT_ATTACK_ENEMY_IN_SAFE_AREA"));
                        return false;
                    }

                    if (inst_main->is_attackable_instance(*inst_target)) {
                        if (!process_enemy_skill_target_range(*inst_main, *inst_target, *skill_data_value, slot_index))
                            return false;
                    } else {
                        py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "CANNOT_ATTACK"));
                        return false;
                    }
                }

                inst_main->set_fly_target_instance(*inst_target);

                if (inst_main != inst_target) {
                    if (inst_main->is_fly_target_object()) {
                        inst_main->new_look_at_fly_target();
                    } else {
                        inst_main->new_look_at_dest_instance(*inst_target);
                    }
                }
            } else {
                if (skill_data_value->is_auto_search_target()) {
                    if (inst_main->new_get_front_instance(&inst_target, 2000.0f)) {
                        set_target(inst_target->virtual_id());
                        if (!process_enemy_skill_target_range(*inst_main, *inst_target, *skill_data_value, slot_index))
                            return false;
                    } else {
                        py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NEED_TARGET"));
                        return false;
                    }
                }

                if (skill_data_value->can_use_for_me()) {
                    inst_target = inst_main;
                    inst_main->set_fly_target_instance(*inst_main);
                    metin2::log::debug("No target, skill used on the player");
                } else if (skill_data_value->is_need_corpse()) {
                    py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "ONLY_FOR_CORPSE"));
                    return false;
                } else {
                    py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NEED_TARGET"));
                    return false;
                }
            }
        }

        if (skill_data_value->can_change_direction()) {
            uint32_t picked_actor_id_value;
            pixel_position p_pos_picked_ground;

            if (inst_target && inst_target != inst_main) {
                inst_main->new_look_at_dest_instance(*inst_target);
            } else if (picked_actor_id(&picked_actor_id_value)) {
                instance_base *inst_victim = new_find_actor_ptr(picked_actor_id_value);
                if (inst_victim)
                    inst_main->new_look_at_dest_instance(*inst_victim);
            } else if (picked_ground_pos(&p_pos_picked_ground)) {
                inst_main->new_look_at_dest_pixel_position(p_pos_picked_ground);
            } else {
                metin2::log::debug("Slot {}: the skill direction must be set relative to the screen", slot_index);
            }
        }

        // Multi-target skills such as Gwangyeoksul.
        uint32_t target_max_count = skill_data_value->target_count(skill_inst.fcur_efficient_percentage);
        uint32_t range_value = skill_target_range(*skill_data_value);
        if (target_max_count > 0 && inst_target) {
            uint32_t target_count_value = 1;
            std::vector<instance_base *> vct_inst_target;

            if (skill_data_value->is_fan_range()) {
                if (inst_main->new_get_instance_vector_in_fan_range(float(range_value), *inst_target, &vct_inst_target)) {
                    std::vector<instance_base *>::iterator i;
                    for (i = vct_inst_target.begin(); i != vct_inst_target.end(); ++i) {
                        if (target_count_value >= target_max_count)
                            break;

                        instance_base *inst_each = *i;

                        if (inst_target != inst_each && !inst_each->is_dead()) {
                            inst_main->add_fly_target_instance(*inst_each);
                            get_python_network_stream().send_add_fly_targeting_packet(inst_each->virtual_id(), inst_each->graphic_thing_instance_ref().on_get_fly_target_position());

                            target_count_value++;
                        }
                    }
                }
            } else if (skill_data_value->is_circle_range()) {
                if (inst_main->new_get_instance_vector_in_circle_range(float(range_value), &vct_inst_target)) {
                    std::vector<instance_base *>::iterator i;
                    for (i = vct_inst_target.begin(); i != vct_inst_target.end(); ++i) {
                        if (target_count_value >= target_max_count)
                            break;

                        instance_base *inst_each = *i;

                        if (inst_target != inst_each && !inst_each->is_dead()) {
                            inst_main->add_fly_target_instance(*inst_each);
                            get_python_network_stream().send_add_fly_targeting_packet(inst_each->virtual_id(), inst_each->graphic_thing_instance_ref().on_get_fly_target_position());

                            target_count_value++;
                        }
                    }
                }
            }

            if (target_count_value < target_max_count) {
                while (target_count_value < target_max_count) {
                    pixel_position p_pos_dst;
                    inst_main->new_get_random_position_in_fan_range(*inst_target, &p_pos_dst);

                    p_pos_dst.x = p_pos_dst.x;
                    p_pos_dst.y = -p_pos_dst.y;

                    inst_main->add_fly_target_position(p_pos_dst);
                    get_python_network_stream().send_add_fly_targeting_packet(0, p_pos_dst);

                    target_count_value++;
                }
            }
        }

        // Clear the reservation, or a target clicked from afar is attacked right after the skill.
        clear_reserved_action();

        if (!skill_data_value->is_no_motion()) {
            uint32_t motion_index = skill_data_value->skill_motion_index(skill_inst.grade);
            uint32_t loop_count_value = skill_data_value->motion_loop_count(skill_inst.fcur_efficient_percentage);
            if (!inst_main->new_use_skill(skill_inst.index_value, motion_index, loop_count_value, skill_data_value->is_moving_skill() ? true : false)) {
                metin2::log::debug("Slot {}: main instance cannot use the skill", slot_index);
                return false;
            }
        }

        uint32_t target_vid_value = inst_target ? inst_target->virtual_id() : 0;

        send_use_skill(slot_index, target_vid_value);
        return true;
    }

    void python_player::send_use_skill(uint32_t skill_slot_index, uint32_t target_vid) {
        skill_instance &skill_inst = player_status_value_.skill[skill_slot_index];

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_use_skill_packet(skill_inst.index_value, target_vid);

        run_cool_time(skill_slot_index);
    }

    auto python_player::skill_type(uint32_t skill_slot_index) -> uint8_t {
        skill_instance &skill_inst = player_status_value_.skill[skill_slot_index];

        python_skill::skill_data *skill_data;
        python_skill &python_skill = get_python_skill();
        if (!python_skill.get_skill_data(skill_inst.index_value, &skill_data)) {
            return 0;
        }
        return skill_data->get_type();
    }

    void python_player::run_cool_time(uint32_t skill_slot_index) {
        skill_instance &skill_inst = player_status_value_.skill[skill_slot_index];

        python_skill::skill_data *pk_skill_data;
        if (!get_python_skill().get_skill_data(skill_inst.index_value, &pk_skill_data)) {
            metin2::log::error("Slot {}: no skill data for the cooldown", skill_slot_index);
            return;
        }

        python_skill::skill_data &rk_skill_data = *pk_skill_data;

        skill_inst.cool_time = rk_skill_data.skill_cool_time(skill_inst.fcur_efficient_percentage);
        skill_inst.last_used_time = get_timer().current_second();

        int spd = 100 - status(POINT_CASTING_SPEED);
        if (spd > 0)
            spd = 100 + spd;
        else if (spd < 0)
            spd = 10000 / (100 - spd);
        else
            spd = 100;

        skill_inst.cool_time = skill_inst.cool_time * spd / 100;

        py_call_class_member_func(ppy_game_window_, "RunUseSkillEvent", Py_BuildValue("(if)", skill_slot_index, skill_inst.cool_time));
    }

    auto python_player::has_enough_arrow() -> bool {
        item_data *item_data;
        if (get_item_manager().item_data_pointer(item_index(item_pos(INVENTORY, c_Equipment_Arrow)), &item_data))
            if (item_data::ITEM_TYPE_WEAPON == item_data->get_type())
                if (item_data::WEAPON_ARROW == item_data->sub_type()) {
                    return true;
                }

        py_call_class_member_func(ppy_game_window_, "OnCannotShotError", Py_BuildValue("(is)", main_character_index(), "EMPTY_ARROW"));
        return false;
    }

    auto python_player::has_item(uint32_t item_id) -> bool {
        for (uint32_t i = 0; i < c_Inventory_Count; ++i) {
            if (item_id == item_index(item_pos(INVENTORY, i)))
                return true;
        }
        for (uint32_t i = 0; i < c_DragonSoul_Inventory_Count; ++i) {
            if (item_id == item_index(item_pos(DRAGON_SOUL_INVENTORY, i)))
                return true;
        }

        return false;
    }

    extern const uint32_t c_iSkillIndex_Tongsol;
    extern const uint32_t c_iSkillIndex_Fishing;
    extern const uint32_t c_iSkillIndex_Mining;
    extern const uint32_t c_iSkillIndex_Making;
    extern const uint32_t c_iSkillIndex_Combo;
    extern const uint32_t c_iSkillIndex_Language1;
    extern const uint32_t c_iSkillIndex_Language2;
    extern const uint32_t c_iSkillIndex_Language3;
    extern const uint32_t c_iSkillIndex_Polymorph;

    void python_player::use_guild_skill(uint32_t skill_slot_index) {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return;
        if (!inst_main->can_use_skill())
            return;

        if (skill_slot_index >= SKILL_MAX_NUM) {
            metin2::log::debug("Guild skill slot {}: not a usable skill slot", skill_slot_index);
            return;
        }

        skill_instance &skill_inst = player_status_value_.skill[skill_slot_index];

        uint32_t skill_index = skill_inst.index_value;

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return;

        if (check_rest_skill_cool_time(skill_slot_index)) {
            py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "WAIT_COOLTIME"));
            return;
        }

        if (skill_data->is_only_for_guild_war()) {
            if (!get_python_guild().is_doing_guild_war()) {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "ONLY_FOR_GUILD_WAR"));
                return;
            }
        }

        uint32_t motion_index = skill_data->skill_motion_index();
        if (!inst_main->new_use_skill(skill_index, motion_index, 1, false)) {
            metin2::log::debug("Guild skill {}: main instance cannot use the skill", skill_index);
        }

        get_python_network_stream().send_guild_use_skill_packet(skill_index, 0);
        run_cool_time(skill_slot_index);
    }

    void python_player::set_combo_skill_flag(bool flag) {
        uint32_t slot_index;
        if (!skill_slot_index(c_iSkillIndex_Combo, &slot_index)) {
            metin2::log::debug("Combo skill {}: slot not found", c_iSkillIndex_Combo);
            return;
        }

        int level = skill_level(slot_index);
        if (level <= 0) {
            metin2::log::debug("Combo skill {}: invalid level {}", c_iSkillIndex_Combo, level);
            return;
        }

        level = std::min(level, 2);

        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return;

        if (flag) {
            inst_main->set_combo_type(level);
            activate_skill_slot(slot_index);
        } else {
            inst_main->set_combo_type(0);
            deactivate_skill_slot(slot_index);
        }
    }

    auto python_player::check_special_skill(uint32_t skill_index) -> bool {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return false;

        if (c_iSkillIndex_Fishing == skill_index) {
            if (inst_main->is_fishing_mode()) {
                new_fishing();
            } else {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "EQUIP_FISHING_ROD"));
            }
            return true;
        }

        else if (c_iSkillIndex_Combo == skill_index) {
            uint32_t slot_index;
            if (!skill_slot_index(skill_index, &slot_index))
                return false;

            int level = skill_level(slot_index);
            if (level > 0) {
                get_python_network_stream().send_use_skill_packet(skill_index);
            } else {
                py_call_class_member_func(ppy_game_window_, "OnCannotUseSkill", Py_BuildValue("(is)", main_character_index(), "NOT_YET_LEARN"));
            }

            return true;
        }

        return false;
    }
}
