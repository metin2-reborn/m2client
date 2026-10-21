#include "network_actor_manager.h"
#include <metin2/client/characters/python_character_manager.h>
#include <metin2/client/inventory/python_item.h>
#include <metin2/log/log.h>

#include <metin2/client/app/abstract_player.h>
#include <metin2/core/timer.h>

namespace metin2::inline client::inline characters {
    extern int char_stage_view_bound;
}
namespace metin2::inline client::inline characters {
    extern auto is_wall(unsigned race) -> bool;
}
namespace metin2::inline client::inline protocol {

    void network_actor_data::update_position() {
        uint32_t client_cur_time = el_timer_get_m_sec();
        uint32_t elapsed_time = client_cur_time - client_src_time;

        if (elapsed_time < duration) {
            float rate = float(elapsed_time) / float(duration);
            cur_x = int32_t((dst_x - src_x) * rate + src_x);
            cur_y = int32_t((dst_y - src_y) * rate + src_y);
        } else {
            cur_x = dst_x;
            cur_y = dst_y;
        }
    }

    void network_actor_data::set_dst_position(uint32_t server_time_value, int32_t dst_x_value, int32_t dst_y_value, uint32_t duration_value) {
        src_x = cur_x;
        src_y = cur_y;
        dst_x = dst_x_value;
        dst_y = dst_y_value;

        duration = duration_value;
        server_src_time = server_time_value;
        client_src_time = el_timer_get_m_sec();
    }

    void network_actor_data::set_position(int32_t pos_x, int32_t pos_y) {
        dst_x = src_x = cur_x = pos_x;
        dst_y = src_y = cur_y = pos_y;
    }

    network_actor_data::network_actor_data(const network_actor_data &src) {
        copy_impl(src);
    }

    void network_actor_data::operator=(const network_actor_data &src) {
        copy_impl(src);
    }

    void network_actor_data::copy_impl(const network_actor_data &src) {
        type_value = src.type_value;
        vid = src.vid;
        state_flags = src.state_flags;
        empire_id = src.empire_id;
        race = src.race;
        mov_spd = src.mov_spd;
        atk_spd = src.atk_spd;
        rot = src.rot;
        cur_x = src.cur_x;
        cur_y = src.cur_y;
        src_x = src.src_x;
        src_y = src.src_y;
        dst_x = src.dst_x;
        dst_y = src.dst_y;
        affect_flags.copy_instance(src.affect_flags);

        server_src_time = src.server_src_time;
        client_src_time = src.client_src_time;
        duration = src.duration;

        armor = src.armor;
        weapon = src.weapon;
        hair_value = src.hair_value;

        owner_vid = src.owner_vid;

        alignment = src.alignment;
        pk_mode = src.pk_mode;
        mount_vnum = src.mount_vnum;

        guild_id = src.guild_id;
        level_value = src.level_value;
        name_value = src.name_value;
    }

    network_actor_data::network_actor_data() {
        set_position(0, 0);

        type_value = 0;
        vid = 0;
        state_flags = 0;
        race = 0;
        mov_spd = 0;
        atk_spd = 0;
        rot = 0.0f;
        armor = 0;
        weapon = 0;
        hair_value = 0;
        empire_id = 0;

        owner_vid = 0;

        duration = 0;
        client_src_time = 0;
        server_src_time = 0;

        alignment = 0;
        pk_mode = 0;
        mount_vnum = 0;

        name_value = "";

        affect_flags.clear();
    }

    network_actor_manager::network_actor_manager() {
        main_vid_ = 0;
    }

    network_actor_manager::~network_actor_manager() = default;

    void network_actor_manager::destroy() {
        net_actor_dict_.clear();

        main_vid_ = 0;
        main_pos_x_ = 0;
        main_pos_y_ = 0;
    }

    void network_actor_manager::set_main_actor_vid(uint32_t vid) {
        main_vid_ = vid;
        main_pos_x_ = 0;
        main_pos_y_ = 0;

        net_actor_dict_.clear();
    }

    void network_actor_manager::update() {
        old_update();
    }

    void network_actor_manager::old_update() {
        update_main_actor();

        python_character_manager &chr_mgr = character_manager();

        std::map<uint32_t, network_actor_data>::iterator i;
        for (i = net_actor_dict_.begin(); i != net_actor_dict_.end(); ++i) {
            network_actor_data &net_actor_data_value = i->second;
            net_actor_data_value.update_position();

            instance_base *inst_find = chr_mgr.instance_ptr(net_actor_data_value.vid);
            if (!inst_find) {
                if (is_visible_actor(net_actor_data_value))
                    append_character_manager_actor(net_actor_data_value);
            }
        }
    }

    auto network_actor_manager::find_actor(network_actor_data &net_actor_data_value, int32_t dst_x, int32_t dst_y) -> instance_base * {
        python_character_manager &chr_mgr = character_manager();
        instance_base *inst_find = chr_mgr.instance_ptr(net_actor_data_value.vid);
        if (!inst_find) {
            if (is_visible_pos(dst_x, dst_y))
                return append_character_manager_actor(net_actor_data_value);

            return nullptr;
        }

        return inst_find;
    }

    auto network_actor_manager::find_actor(network_actor_data &net_actor_data_value) -> instance_base * {
        python_character_manager &chr_mgr = character_manager();
        instance_base *inst_find = chr_mgr.instance_ptr(net_actor_data_value.vid);
        if (!inst_find) {
            if (is_visible_actor(net_actor_data_value))
                return append_character_manager_actor(net_actor_data_value);

            return nullptr;
        }

        return inst_find;
    }

    void network_actor_manager::remove_all_ground_items() {
        python_item &item_mgr = get_python_item();
        item_mgr.delete_all_items();
    }

    void network_actor_manager::remove_dynamic_actors() {
        // Collect the VIDs first; deleting instances while iterating would invalidate the iterator.
        std::vector<uint32_t> character_vid_list;

        python_character_manager &chr_mgr = get_python_character_manager();
        for (python_character_manager::character_iterator i = chr_mgr.character_instance_begin(); i != chr_mgr.character_instance_end(); ++i) {
            character_vid_list.push_back((*i)->virtual_id());
        }

        for (size_t i = 0; i < character_vid_list.size(); ++i) {
            instance_base *inst_each = chr_mgr.instance_ptr(character_vid_list[i]);
            if (!inst_each)
                continue;

            actor_instance *actor_each = inst_each->graphic_thing_instance_ptr();
            if (actor_each->is_pc() || actor_each->is_npc() || actor_each->is_enemy()) {
                chr_mgr.delete_instance(character_vid_list[i]);
                auto it = net_actor_dict_.find(character_vid_list[i]);
                if (it != net_actor_dict_.end()) {
                    net_actor_dict_.erase(it);
                }
            }
        }

        chr_mgr.destroy_dead_instance_list();
    }

    void network_actor_manager::update_main_actor() {
        python_character_manager &chr_mgr = get_python_character_manager();
        instance_base *inst_main = chr_mgr.main_instance_ptr();
        if (!inst_main)
            return;

        pixel_position p_pos_main;
        inst_main->new_get_pixel_position(&p_pos_main);

        main_pos_x_ = int32_t(p_pos_main.x);
        main_pos_y_ = int32_t(p_pos_main.y);
    }

    auto network_actor_manager::is_visible_actor(const network_actor_data &net_actor_data_value) -> bool {
        if (is_main_actor_vid(net_actor_data_value.vid))
            return true;

        if (net_actor_data_value.affect_flags.is_set(instance_base::AFFECT_SHOW_ALWAYS))
            return true;

        if (is_visible_pos(net_actor_data_value.cur_x, net_actor_data_value.cur_y))
            return true;

        if (is_wall(net_actor_data_value.race))
            return true;

        return false;
    }

    auto network_actor_manager::is_visible_pos(int32_t pos_x, int32_t pos_y) -> bool {
        int32_t dx = pos_x - main_pos_x_;
        int32_t dy = pos_y - main_pos_y_;
        auto len = (int32_t)sqrt(double(dx * dx + dy * dy));

        if (len < char_stage_view_bound) // distance limit in cm
            return true;

        return false;
    }

    auto network_actor_manager::is_main_actor_vid(uint32_t vid) -> bool {
        if (vid == main_vid_)
            return true;

        return false;
    }

    auto network_actor_manager::character_manager() -> python_character_manager & {
        return get_python_character_manager();
    }

    void network_actor_manager::remove_character_manager_actor(network_actor_data &net_actor_data_value) {
        python_character_manager &chr_mgr = get_python_character_manager();

        if (is_main_actor_vid(net_actor_data_value.vid)) {
            chr_mgr.delete_instance(net_actor_data_value.vid);
        } else {
            chr_mgr.delete_instance_by_fade(net_actor_data_value.vid);
        }
    }

    auto network_actor_manager::append_character_manager_actor(network_actor_data &net_actor_data_value) -> instance_base * {
        metin2::log::debug("Adding character {} ({}) to the character manager", net_actor_data_value.vid, net_actor_data_value.name_value.c_str());

        uint32_t vid = net_actor_data_value.vid;

        python_character_manager &chr_mgr = get_python_character_manager();

        instance_base::create_data create_data;
        create_data.type_value = net_actor_data_value.type_value;
        create_data.level_value = net_actor_data_value.level_value;
        create_data.guild_id = net_actor_data_value.guild_id;
        create_data.empire_id = net_actor_data_value.empire_id;
        create_data.vid = net_actor_data_value.vid;
        create_data.mount_vnum = net_actor_data_value.mount_vnum;
        create_data.race = net_actor_data_value.race;
        create_data.rot = net_actor_data_value.rot;
        create_data.pos_x = net_actor_data_value.cur_x;
        create_data.pos_y = net_actor_data_value.cur_y;
        create_data.name_value = net_actor_data_value.name_value;
        create_data.state_flags = net_actor_data_value.state_flags;
        create_data.mov_spd = net_actor_data_value.mov_spd;
        create_data.atk_spd = net_actor_data_value.atk_spd;
        create_data.alignment = net_actor_data_value.alignment;
        create_data.pk_mode = net_actor_data_value.pk_mode;
        create_data.affect_flags = net_actor_data_value.affect_flags;
        create_data.armor = net_actor_data_value.armor;
        create_data.weapon = net_actor_data_value.weapon;
        create_data.hair_value = net_actor_data_value.hair_value;
        create_data.is_main = is_main_actor_vid(vid);

        instance_base *old_instance = chr_mgr.instance_ptr(vid);
        if (old_instance) {
            pixel_position old_pos;
            old_instance->graphic_thing_instance_ptr()->get_pixel_position(&old_pos);

            bool is_mounting_horse = old_instance->is_mounting_horse();
            if ((is_mounting_horse && create_data.mount_vnum == 0) ||
                (!is_mounting_horse && create_data.mount_vnum != 0)) {
                // Keep the current position when the actor mounts or dismounts.
                create_data.pos_x = old_pos.x;
                create_data.pos_y = -old_pos.y;
            }

            chr_mgr.delete_instance(vid);
        }

        instance_base *new_instance = chr_mgr.create_instance(create_data);
        if (!new_instance)
            return nullptr;

        if (create_data.is_main) {
            abstract_player &player = get_abstract_player();
            player.set_main_character_index(vid);

            main_pos_x_ = net_actor_data_value.cur_x;
            main_pos_y_ = net_actor_data_value.cur_y;
        }

        uint32_t client_cur_time = el_timer_get_m_sec();
        uint32_t elapsed_time = client_cur_time - net_actor_data_value.client_src_time;

        if (elapsed_time < net_actor_data_value.duration) {
            pixel_position p_pos_dst;
            p_pos_dst.x = float(net_actor_data_value.dst_x);
            p_pos_dst.y = float(net_actor_data_value.dst_y);
            p_pos_dst.z = 0;
            new_instance->push_tcp_state(net_actor_data_value.server_src_time + elapsed_time, p_pos_dst, net_actor_data_value.rot, instance_base::FUNC_MOVE, 0);
        }
        return new_instance;
    }

    void network_actor_manager::append_actor(const network_actor_data &c_rk_net_actor_data) {
        if (is_main_actor_vid(c_rk_net_actor_data.vid)) {
            bool change_mount_status = false;

            instance_base *main_instance = get_python_character_manager().instance_ptr(c_rk_net_actor_data.vid);
            if (main_instance) {
                bool is_mounting_horse = main_instance->is_mounting_horse();
                change_mount_status = (is_mounting_horse && c_rk_net_actor_data.mount_vnum == 0) || (!is_mounting_horse && c_rk_net_actor_data.mount_vnum != 0);
            }

            if (!change_mount_status) {
                remove_dynamic_actors();
                remove_all_ground_items();
            }
        }

        network_actor_data &rk_net_actor_data = net_actor_dict_[c_rk_net_actor_data.vid];
        rk_net_actor_data = c_rk_net_actor_data;

        if (is_visible_actor(rk_net_actor_data)) {
            if (!append_character_manager_actor(rk_net_actor_data))
                net_actor_dict_.erase(c_rk_net_actor_data.vid);
        }
    }

    void network_actor_manager::remove_actor(uint32_t vid) {
        auto f = net_actor_dict_.find(vid);
        if (net_actor_dict_.end() == f) {
            return;
        }

        network_actor_data &net_actor_data_value = f->second;
        remove_character_manager_actor(net_actor_data_value);

        net_actor_dict_.erase(f);
    }

    void network_actor_manager::update_actor(const network_update_actor_data &net_update_actor_data) {
        auto f = net_actor_dict_.find(net_update_actor_data.vid);
        if (net_actor_dict_.end() == f) {
            return;
        }

        network_actor_data &net_actor_data_value = f->second;

        instance_base *inst_find = find_actor(net_actor_data_value);
        if (inst_find) {
            inst_find->change_armor(net_update_actor_data.armor);
            inst_find->change_weapon(net_update_actor_data.weapon);
            inst_find->change_hair(net_update_actor_data.hair_value);
            inst_find->change_guild(net_update_actor_data.guild_id);
            inst_find->set_affect_flag_container(net_update_actor_data.affect_flags);
            inst_find->set_move_speed(net_update_actor_data.mov_spd);
            inst_find->set_attack_speed(net_update_actor_data.atk_spd);
            inst_find->set_alignment(net_update_actor_data.alignment);
            inst_find->set_pk_mode(net_update_actor_data.pk_mode);
            inst_find->set_state_flags(net_update_actor_data.state_flags);
        }

        net_actor_data_value.affect_flags = net_update_actor_data.affect_flags;
        net_actor_data_value.guild_id = net_update_actor_data.guild_id;
        net_actor_data_value.mov_spd = net_update_actor_data.mov_spd;
        net_actor_data_value.atk_spd = net_update_actor_data.atk_spd;
        net_actor_data_value.armor = net_update_actor_data.armor;
        net_actor_data_value.weapon = net_update_actor_data.weapon;
        net_actor_data_value.hair_value = net_update_actor_data.hair_value;
        net_actor_data_value.alignment = net_update_actor_data.alignment;
        net_actor_data_value.pk_mode = net_update_actor_data.pk_mode;
    }

    void network_actor_manager::move_actor(const network_move_actor_data &net_move_actor_data) {
        auto f = net_actor_dict_.find(net_move_actor_data.vid);
        if (net_actor_dict_.end() == f) {
            return;
        }

        network_actor_data &net_actor_data_value = f->second;

        instance_base *inst_find = find_actor(net_actor_data_value, net_move_actor_data.pos_x, net_move_actor_data.pos_y);
        if (inst_find) {
            pixel_position p_pos_dst;
            p_pos_dst.x = float(net_move_actor_data.pos_x);
            p_pos_dst.y = float(net_move_actor_data.pos_y);
            p_pos_dst.z = 0.0f;

            inst_find->push_tcp_state(net_move_actor_data.time, p_pos_dst,
                net_move_actor_data.rot, net_move_actor_data.func, net_move_actor_data.arg_value);
        }

        net_actor_data_value.set_dst_position(net_move_actor_data.time,
            net_move_actor_data.pos_x, net_move_actor_data.pos_y, net_move_actor_data.duration);
        net_actor_data_value.rot = net_move_actor_data.rot;
    }

    void network_actor_manager::sync_actor(uint32_t vid, int32_t pos_x, int32_t pos_y) {
        auto f = net_actor_dict_.find(vid);
        if (net_actor_dict_.end() == f) {
            return;
        }

        network_actor_data &net_actor_data_value = f->second;

        instance_base *inst_find = find_actor(net_actor_data_value);
        if (inst_find) {
            inst_find->new_sync_pixel_position(pos_x, pos_y);
        }

        net_actor_data_value.set_position(pos_x, pos_y);
    }

    void network_actor_manager::set_actor_owner(uint32_t owner_vid, uint32_t victim_vid) {
        auto f = net_actor_dict_.find(victim_vid);
        if (net_actor_dict_.end() == f) {
            return;
        }

        network_actor_data &net_actor_data_value = f->second;
        net_actor_data_value.owner_vid = owner_vid;

        instance_base *inst_find = find_actor(net_actor_data_value);
        if (inst_find) {
            inst_find->new_set_owner(net_actor_data_value.owner_vid);
        }
    }
}
