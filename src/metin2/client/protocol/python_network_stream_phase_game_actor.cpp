#include <metin2/client/background/python_background.h>
#include <metin2/log/log.h>
#include "network_actor_manager.h"
#include "python_network_stream.h"

#include <metin2/client/app/abstract_player.h>
#include <metin2/client/app/python_application.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/actors/actor_instance.h>

namespace metin2::inline client::inline protocol {

    void python_network_stream::global_position_to_local_position_impl(int32_t &global_x, int32_t &global_y) {
        python_background &bg_mgr = get_python_background();
        bg_mgr.global_position_to_local_position(global_x, global_y);
    }

    void python_network_stream::local_position_to_global_position_impl(int32_t &local_x, int32_t &local_y) {
        python_background &bg_mgr = get_python_background();
        bg_mgr.local_position_to_global_position(local_x, local_y);
    }

    auto python_network_stream::can_act_main_instance() -> bool {
        python_character_manager &chr_mgr = get_python_character_manager();
        instance_base *inst_main = chr_mgr.main_instance_ptr();
        if (!inst_main)
            return false;

        return inst_main->can_act();
    }

    void python_network_stream::clear_network_actor_manager() {
        rok_net_actor_mgr_->destroy();
    }

    void set_weapon_power(abstract_player &player, uint32_t weapon_id) {
        uint32_t min_power = 0;
        uint32_t max_power = 0;
        uint32_t min_magic_power = 0;
        uint32_t max_magic_power = 0;
        uint32_t add_power = 0;

        item_data *weapon;
        if (get_item_manager().item_data_pointer(weapon_id, &weapon)) {
            if (weapon->get_type() == item_data::ITEM_TYPE_WEAPON) {
                min_power = weapon->value(3);
                max_power = weapon->value(4);
                min_magic_power = weapon->value(1);
                max_magic_power = weapon->value(2);
                add_power = weapon->value(5);
            }
        }

        player.set_weapon_power(min_power, max_power, min_magic_power, max_magic_power, add_power);
    }

    // Races whose table entry is named "."; drop this once the server stops sending them.
    auto is_invisible_race(uint16_t race_num) -> bool {
        switch (race_num) {
        case 20025:
        case 20038:
        case 20039:
            return true;
        default:
            return false;
        }
    }

    static network_actor_data net_actor_data;

    auto python_network_stream::recv_character_append_packet() -> bool {
        packet_gc_character_add chr_add_packet;
        if (!recv(sizeof(chr_add_packet), &chr_add_packet))
            return false;

        global_position_to_local_position_impl(chr_add_packet.x, chr_add_packet.y);

        network_actor_data net_actor_data_value;
        net_actor_data_value.type_value = chr_add_packet.type_value;
        net_actor_data_value.mov_spd = chr_add_packet.moving_speed;
        net_actor_data_value.atk_spd = chr_add_packet.attack_speed;
        net_actor_data_value.race = chr_add_packet.race_num;

        net_actor_data_value.state_flags = chr_add_packet.state_flag;
        net_actor_data_value.vid = chr_add_packet.vid;
        net_actor_data_value.rot = chr_add_packet.angle;

        net_actor_data_value.name_value = "";

        net_actor_data_value.name_value = "";
        net_actor_data_value.affect_flags.copy_data(0, sizeof(chr_add_packet.affect_flag[0]), &chr_add_packet.affect_flag[0]);
        net_actor_data_value.affect_flags.copy_data(32, sizeof(chr_add_packet.affect_flag[1]), &chr_add_packet.affect_flag[1]);

        net_actor_data_value.set_position(chr_add_packet.x, chr_add_packet.y);

        net_actor_data_value.alignment = 0;
        ;
        net_actor_data_value.pk_mode = 0;
        ;
        net_actor_data_value.guild_id = 0;
        ;
        net_actor_data_value.empire_id = 0;
        ;
        net_actor_data_value.armor = 0;
        ;
        net_actor_data_value.weapon = 0;
        ;
        net_actor_data_value.hair_value = 0;
        ;
        net_actor_data_value.mount_vnum = 0;
        ;

        net_actor_data_value.level_value = 0; // monster levels are not shown

        if (net_actor_data_value.type_value != actor_instance::TYPE_PC &&
            net_actor_data_value.type_value != actor_instance::TYPE_NPC) {
            const char *name;
            python_non_player &non_player = get_python_non_player();
            if (non_player.get_name(net_actor_data_value.race, &name))
                net_actor_data_value.name_value = name;

            recv_character_append_packet_impl(&net_actor_data_value);
        } else {
            net_actor_data = net_actor_data_value;
        }

        return true;
    }

    auto python_network_stream::recv_character_additional_info() -> bool {
        packet_gc_character_additional_info chr_info_packet;
        if (!recv(sizeof(chr_info_packet), &chr_info_packet))
            return false;

        network_actor_data net_actor_data_value = net_actor_data;
        if (is_invisible_race(net_actor_data_value.race))
            return true;

        if (net_actor_data_value.vid == chr_info_packet.vid) {
            net_actor_data_value.name_value = packet_string(chr_info_packet.name_value).c_str();
            net_actor_data_value.guild_id = chr_info_packet.guild_id;
            net_actor_data_value.level_value = chr_info_packet.level_value;
            net_actor_data_value.alignment = chr_info_packet.alignment;
            net_actor_data_value.pk_mode = chr_info_packet.pk_mode;
            net_actor_data_value.guild_id = chr_info_packet.guild_id;
            net_actor_data_value.empire_id = chr_info_packet.empire;
            net_actor_data_value.armor = chr_info_packet.part_value[CHR_EQUIPPART_ARMOR];
            net_actor_data_value.weapon = chr_info_packet.part_value[CHR_EQUIPPART_WEAPON];
            net_actor_data_value.hair_value = chr_info_packet.part_value[CHR_EQUIPPART_HAIR];
            net_actor_data_value.mount_vnum = chr_info_packet.mount_vnum;

            recv_character_append_packet_impl(&net_actor_data_value);
        } else {
            metin2::log::error("Character additional info for {} (vid {}, race {}) does not match the pending character", packet_string(chr_info_packet.name_value).c_str(), chr_info_packet.vid, net_actor_data_value.race);
        }
        return true;
    }

    auto python_network_stream::recv_character_update_packet() -> bool {
        packet_gc_character_update chr_update_packet;
        if (!recv(sizeof(chr_update_packet), &chr_update_packet))
            return false;

        network_update_actor_data net_update_actor_data;
        net_update_actor_data.guild_id = chr_update_packet.guild_id;
        net_update_actor_data.mov_spd = chr_update_packet.moving_speed;
        net_update_actor_data.atk_spd = chr_update_packet.attack_speed;
        net_update_actor_data.armor = chr_update_packet.part_value[CHR_EQUIPPART_ARMOR];
        net_update_actor_data.weapon = chr_update_packet.part_value[CHR_EQUIPPART_WEAPON];
        net_update_actor_data.hair_value = chr_update_packet.part_value[CHR_EQUIPPART_HAIR];
        net_update_actor_data.vid = chr_update_packet.vid;
        net_update_actor_data.affect_flags.copy_data(0, sizeof(chr_update_packet.affect_flag[0]), &chr_update_packet.affect_flag[0]);
        net_update_actor_data.affect_flags.copy_data(32, sizeof(chr_update_packet.affect_flag[1]), &chr_update_packet.affect_flag[1]);
        net_update_actor_data.alignment = chr_update_packet.alignment;
        net_update_actor_data.pk_mode = chr_update_packet.pk_mode;
        net_update_actor_data.state_flags = chr_update_packet.state_flag;
        net_update_actor_data.mount_vnum = chr_update_packet.mount_vnum;
        recv_character_update_packet_impl(&net_update_actor_data);

        return true;
    }

    void python_network_stream::recv_character_append_packet_impl(network_actor_data *net_actor_data_value) {
        // The map must be updated before the main character is placed on the terrain, to get the
        // height, or the camera sinks into the ground. This happens once the character exists, not
        // when entering the game.
        abstract_player &player = get_abstract_player();
        if (player.is_main_character_index(net_actor_data_value->vid)) {
            player.set_race(net_actor_data_value->race);

            set_weapon_power(player, net_actor_data_value->weapon);

            if (player.new_get_main_actor_ptr()) {
                get_python_background().update(net_actor_data_value->cur_x, net_actor_data_value->cur_y, 0.0f);
                get_python_character_manager().update();

                // Show the map name for the devil tower also when moving there with GOTO.
                {
                    std::string map_name = get_python_background().warp_map_name();
                    if (map_name == "metin2_map_deviltower1")
                        show_map_name(net_actor_data_value->cur_x, net_actor_data_value->cur_y);
                }
            } else {
                show_map_name(net_actor_data_value->cur_x, net_actor_data_value->cur_y);
            }
        }

        rok_net_actor_mgr_->append_actor(*net_actor_data_value);

        if (main_actor_vid() == net_actor_data_value->vid) {
            player.set_target(0);
            if (combo_skill_flag_)
                player.set_combo_skill_flag(combo_skill_flag_);

            set_guild_id(net_actor_data_value->guild_id);
        }
    }

    void python_network_stream::recv_character_update_packet_impl(network_update_actor_data *net_update_actor_data) {
        rok_net_actor_mgr_->update_actor(*net_update_actor_data);

        abstract_player &player = get_abstract_player();
        if (player.is_main_character_index(net_update_actor_data->vid)) {
            set_guild_id(net_update_actor_data->guild_id);
            set_weapon_power(player, net_update_actor_data->weapon);

            refresh_status();
            refresh_alignment_window();
            refresh_equipment_window();
            refresh_inventory_window();
        } else {
            player.notify_character_update(net_update_actor_data->vid);
        }
    }

    auto python_network_stream::recv_character_delete_packet() -> bool {
        packet_gc_character_delete chr_del_packet;

        if (!recv(sizeof(chr_del_packet), &chr_del_packet)) {
            metin2::log::error("Cannot read the character delete packet");
            return false;
        }

        rok_net_actor_mgr_->remove_actor(chr_del_packet.vid);

        // Remove the private shop when the character disappears. Not strictly needed, since the
        // shop key is checked.
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
            "private_shop_disappear",
            Py_BuildValue("(i)", chr_del_packet.vid));

        return true;
    }

    auto python_network_stream::recv_character_move_packet() -> bool {
        packet_gc_move move_packet;
        if (!recv(sizeof(packet_gc_move), &move_packet)) {
            metin2::log::debug("Cannot read the character move packet");
            return false;
        }

        global_position_to_local_position_impl(move_packet.x, move_packet.y);

        network_move_actor_data net_move_actor_data;
        net_move_actor_data.arg_value = move_packet.arg_value;
        net_move_actor_data.func = move_packet.func;
        net_move_actor_data.time = move_packet.time;
        net_move_actor_data.vid = move_packet.vid;
        net_move_actor_data.rot = move_packet.rot * 5.0f;
        net_move_actor_data.pos_x = move_packet.x;
        net_move_actor_data.pos_y = move_packet.y;
        net_move_actor_data.duration = move_packet.duration;

        rok_net_actor_mgr_->move_actor(net_move_actor_data);

        return true;
    }

    auto python_network_stream::recv_owner_ship_packet() -> bool {
        packet_gc_ownership packet_ownership;

        if (!recv(sizeof(packet_ownership), &packet_ownership))
            return false;

        rok_net_actor_mgr_->set_actor_owner(packet_ownership.owner_vid, packet_ownership.victim_vid);

        return true;
    }

    auto python_network_stream::recv_sync_position_packet() -> bool {
        packet_gc_sync_position packet_sync_pos;
        if (!recv(sizeof(packet_sync_pos), &packet_sync_pos))
            return false;

        packet_gc_sync_position_element k_sync_pos;

        uint32_t sync_pos_count = (packet_sync_pos.size - sizeof(packet_sync_pos)) / sizeof(k_sync_pos);
        for (uint32_t i_sync_pos = 0; i_sync_pos < sync_pos_count; ++i_sync_pos) {
            if (!recv(sizeof(packet_gc_sync_position_element), &k_sync_pos))
                return false;

            global_position_to_local_position_impl(k_sync_pos.x, k_sync_pos.y);
            rok_net_actor_mgr_->sync_actor(k_sync_pos.vid, k_sync_pos.x, k_sync_pos.y);
        }

        return true;
    }
}
