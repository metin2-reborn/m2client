#include <metin2/core/utf8.h>
#include <metin2/core/utils.h>
#include <metin2/log/log.h>
#include "packet.h"
#include "python_network_stream.h"

#include <metin2/client/app/python_application.h>
#include <metin2/client/background/python_background.h>
#include <metin2/client/background/python_mini_map.h>
#include <metin2/client/characters/python_character_manager.h>
#include <metin2/client/guilds/python_guild.h>
#include <metin2/client/inventory/python_item.h>
#include <metin2/client/player/python_player.h>
#include <metin2/client/quest/python_event_manager.h>
#include <metin2/client/quest/python_quest.h>
#include <metin2/client/social/python_chat.h>
#include <metin2/client/social/python_messenger.h>
#include <metin2/client/social/python_text_tail.h>
#include <metin2/client/trade/python_exchange.h>
#include <metin2/client/trade/python_shop.h>

#include <metin2/world/items/item_manager.h>

#include <metin2/client/app/abstract_application.h>
#include <metin2/client/app/abstract_character_manager.h>
#include <metin2/client/characters/instance_base.h>

#include <metin2/core/file_name.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline protocol {

    void python_network_stream::refresh_alignment_window() {
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_alignment", Py_BuildValue("()"));
    }

    void python_network_stream::refresh_target_board_by_vid(uint32_t vid) {
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_target_board_by_vid", Py_BuildValue("(i)", vid));
    }

    void python_network_stream::refresh_target_board_by_name(const char *name) {
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_target_board_by_name", Py_BuildValue("(s)", name));
    }

    void python_network_stream::refresh_target_board() {
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_target_board", Py_BuildValue("()"));
    }

    void python_network_stream::refresh_guild_window_grade_page() {
        is_refresh_guild_wnd_grade_page_ = true;
    }

    void python_network_stream::refresh_guild_window_skill_page() {
        is_refresh_guild_wnd_skill_page_ = true;
    }

    void python_network_stream::refresh_guild_window_member_page_grade_combo_box() {
        is_refresh_guild_wnd_member_page_grade_combo_box_ = true;
    }

    void python_network_stream::refresh_guild_window_member_page() {
        is_refresh_guild_wnd_member_page_ = true;
    }

    void python_network_stream::refresh_guild_window_board_page() {
        is_refresh_guild_wnd_board_page_ = true;
    }

    void python_network_stream::refresh_guild_window_info_page() {
        is_refresh_guild_wnd_info_page_ = true;
    }

    void python_network_stream::refresh_messenger_window() {
        is_refresh_messenger_wnd_ = true;
    }

    void python_network_stream::refresh_safebox_window() {
        is_refresh_safebox_wnd_ = true;
    }

    void python_network_stream::refresh_mall_window() {
        is_refresh_mall_wnd_ = true;
    }

    void python_network_stream::refresh_skill_window() {
        is_refresh_skill_wnd_ = true;
    }

    void python_network_stream::refresh_exchange_window() {
        is_refresh_exchange_wnd_ = true;
    }

    void python_network_stream::refresh_status() {
        is_refresh_status_ = true;
    }

    void python_network_stream::refresh_character_window() {
        is_refresh_character_wnd_ = true;
    }

    void python_network_stream::refresh_inventory_window() {
        is_refresh_inventory_wnd_ = true;
    }

    void python_network_stream::refresh_equipment_window() {
        is_refresh_equipment_wnd_ = true;
    }

    void python_network_stream::set_guild_id(uint32_t id) {
        if (guild_id_ != id) {
            guild_id_ = id;
            abstract_player &player = get_abstract_player();

            for (int i = 0; i < PLAYER_PER_ACCOUNT4; ++i)
                if (!strncmp(simple_player_infos_[i].name_value, player.get_name(), CHARACTER_NAME_MAX_LEN)) {
                    guild_ids_[i] = id;

                    std::string guild_name;
                    if (get_python_guild().guild_name(id, &guild_name)) {
                        guild_name_[i] = guild_name;
                    } else {
                        guild_name_[i] = "";
                    }
                }
        }
    }

    struct perf_packet_info {
        uint32_t count;
        uint32_t time;

        perf_packet_info() {
            count = 0;
            time = 0;
        }
    };

    void python_network_stream::game_phase() {
        if (!que_hack_.empty()) {
            send_hack(que_hack_.front().c_str());
            que_hack_.pop_front();
        }

        packet_header header = 0;
        bool ret = true;

        const uint32_t max_recv_count = 4;
        const uint32_t safe_recv_bufsize = 8192;
        uint32_t recv_count = 0;

        while (ret) {
            if (recv_count++ >= max_recv_count - 1 && uint32_t(recv_buffer_size()) < safe_recv_bufsize && phase_value_ == "Game") // also reached outside the game phase
                break;

            if (!check_packet(&header))
                break;

            switch (header) {
            case HEADER_GC_WARP:
                ret = recv_warp_packet();
                break;

            case HEADER_GC_PHASE:
                ret = recv_phase_packet();
                return; // the phase changed, so stop handling game packets
                break;

            case HEADER_GC_PVP:
                ret = recv_pvp_packet();
                break;

            case HEADER_GC_DUEL_START:
                ret = recv_duel_start_packet();
                break;

            case HEADER_GC_CHARACTER_ADD:
                ret = recv_character_append_packet();
                break;

            case HEADER_GC_CHAR_ADDITIONAL_INFO:
                ret = recv_character_additional_info();
                break;

            case HEADER_GC_CHARACTER_UPDATE:
                ret = recv_character_update_packet();
                break;

            case HEADER_GC_CHARACTER_DEL:
                ret = recv_character_delete_packet();
                break;

            case HEADER_GC_CHAT:
                ret = recv_chat_packet();
                break;

            case HEADER_GC_SYNC_POSITION:
                ret = recv_sync_position_packet();
                break;

            case HEADER_GC_OWNERSHIP:
                ret = recv_owner_ship_packet();
                break;

            case HEADER_GC_WHISPER:
                ret = recv_whisper_packet();
                break;

            case HEADER_GC_CHARACTER_MOVE:
                ret = recv_character_move_packet();
                break;

            case HEADER_GC_CHARACTER_POSITION:
                ret = recv_character_position_packet();
                break;

            case HEADER_GC_STUN:
                ret = recv_stun_packet();
                break;

            case HEADER_GC_DEAD:
                ret = recv_dead_packet();
                break;

            case HEADER_GC_PLAYER_POINT_CHANGE:
                ret = recv_point_change();
                break;

            case HEADER_GC_ITEM_SET:
                ret = recv_item_set_packet();
                break;

            case HEADER_GC_ITEM_SET2:
                ret = recv_item_set_packet2();
                break;

            case HEADER_GC_ITEM_UPDATE:
                ret = recv_item_update_packet();
                break;

            case HEADER_GC_ITEM_GROUND_ADD:
                ret = recv_item_ground_add_packet();
                break;

            case HEADER_GC_ITEM_GROUND_DEL:
                ret = recv_item_ground_del_packet();
                break;

            case HEADER_GC_ITEM_OWNERSHIP:
                ret = recv_item_ownership();
                break;

            case HEADER_GC_QUICKSLOT_ADD:
                ret = recv_quick_slot_add_packet();
                break;

            case HEADER_GC_QUICKSLOT_DEL:
                ret = recv_quick_slot_del_packet();
                break;

            case HEADER_GC_QUICKSLOT_SWAP:
                ret = recv_quick_slot_move_packet();
                break;

            case HEADER_GC_SHOP:
                ret = recv_shop_packet();
                break;

            case HEADER_GC_SHOP_SIGN:
                ret = recv_shop_sign_packet();
                break;

            case HEADER_GC_EXCHANGE:
                ret = recv_exchange_packet();
                break;

            case HEADER_GC_QUEST_INFO:
                ret = recv_quest_info_packet();
                break;

            case HEADER_GC_REQUEST_MAKE_GUILD:
                ret = recv_request_make_guild();
                break;

            case HEADER_GC_PING:
                ret = recv_ping_packet();
                break;

            case HEADER_GC_SCRIPT:
                ret = recv_script_packet();
                break;

            case HEADER_GC_QUEST_CONFIRM:
                ret = recv_quest_confirm_packet();
                break;

            case HEADER_GC_TARGET:
                ret = recv_target_packet();
                break;

            case HEADER_GC_DAMAGE_INFO:
                ret = recv_damage_info_packet();
                break;

            case HEADER_GC_PLAYER_POINTS:
                ret = recv_player_points();
                break;

            case HEADER_GC_CREATE_FLY:
                ret = recv_create_fly_packet();
                break;

            case HEADER_GC_FLY_TARGETING:
                ret = recv_fly_targeting_packet();
                break;

            case HEADER_GC_ADD_FLY_TARGETING:
                ret = recv_add_fly_targeting_packet();
                break;

            case HEADER_GC_SKILL_LEVEL_NEW:
                ret = recv_skill_level_new();
                break;

            case HEADER_GC_MESSENGER:
                ret = recv_messenger();
                break;

            case HEADER_GC_GUILD:
                ret = recv_guild();
                break;

            case HEADER_GC_PARTY_INVITE:
                ret = recv_party_invite();
                break;

            case HEADER_GC_PARTY_ADD:
                ret = recv_party_add();
                break;

            case HEADER_GC_PARTY_UPDATE:
                ret = recv_party_update();
                break;

            case HEADER_GC_PARTY_REMOVE:
                ret = recv_party_remove();
                break;

            case HEADER_GC_PARTY_LINK:
                ret = recv_party_link();
                break;

            case HEADER_GC_PARTY_UNLINK:
                ret = recv_party_unlink();
                break;

            case HEADER_GC_PARTY_PARAMETER:
                ret = recv_party_parameter();
                break;

            case HEADER_GC_SAFEBOX_SET:
                ret = recv_safe_box_set_packet();
                break;

            case HEADER_GC_SAFEBOX_DEL:
                ret = recv_safe_box_del_packet();
                break;

            case HEADER_GC_SAFEBOX_WRONG_PASSWORD:
                ret = recv_safe_box_wrong_password_packet();
                break;

            case HEADER_GC_SAFEBOX_SIZE:
                ret = recv_safe_box_size_packet();
                break;

            case HEADER_GC_FISHING:
                ret = recv_fishing();
                break;

            case HEADER_GC_DUNGEON:
                ret = recv_dungeon();
                break;

            case HEADER_GC_TIME:
                ret = recv_time_packet();
                break;

            case HEADER_GC_WALK_MODE:
                ret = recv_walk_mode_packet();
                break;

            case HEADER_GC_CHANGE_SKILL_GROUP:
                ret = recv_change_skill_group_packet();
                break;

            case HEADER_GC_REFINE_INFORMATION_NEW:
                ret = recv_refine_information_packet_new();
                break;

            case HEADER_GC_SEPCIAL_EFFECT:
                ret = recv_special_effect();
                break;

            case HEADER_GC_NPC_POSITION:
                ret = recv_npc_list();
                break;

            case HEADER_GC_CHANNEL:
                ret = recv_channel_packet();
                break;

            case HEADER_GC_VIEW_EQUIP:
                ret = recv_view_equip_packet();
                break;

            case HEADER_GC_LAND_LIST:
                ret = recv_land_packet();
                break;

            case HEADER_GC_TARGET_CREATE_NEW:
                ret = recv_target_create_packet_new();
                break;

            case HEADER_GC_TARGET_UPDATE:
                ret = recv_target_update_packet();
                break;

            case HEADER_GC_TARGET_DELETE:
                ret = recv_target_delete_packet();
                break;

            case HEADER_GC_AFFECT_ADD:
                ret = recv_affect_add_packet();
                break;

            case HEADER_GC_AFFECT_REMOVE:
                ret = recv_affect_remove_packet();
                break;

            case HEADER_GC_MALL_OPEN:
                ret = recv_mall_open_packet();
                break;

            case HEADER_GC_MALL_SET:
                ret = recv_mall_item_set_packet();
                break;

            case HEADER_GC_MALL_DEL:
                ret = recv_mall_item_del_packet();
                break;

            case HEADER_GC_LOVER_INFO:
                ret = recv_lover_info_packet();
                break;

            case HEADER_GC_LOVE_POINT_UPDATE:
                ret = recv_love_point_update_packet();
                break;

            case HEADER_GC_DIG_MOTION:
                ret = recv_dig_motion_packet();
                break;

            case HEADER_GC_HANDSHAKE:
                recv_handshake_packet();
                return;
                break;

            case HEADER_GC_HANDSHAKE_OK:
                recv_handshake_ok_packet();
                return;
                break;

            case HEADER_GC_KEY_AGREEMENT:
                recv_key_agreement_packet();
                return;
                break;

            case HEADER_GC_KEY_AGREEMENT_COMPLETED:
                recv_key_agreement_completed_packet();
                return;
                break;

            case HEADER_GC_SPECIFIC_EFFECT:
                ret = recv_specific_effect();
                break;

            case HEADER_GC_DRAGON_SOUL_REFINE:
                ret = recv_dragon_soul_refine();
                break;

            default:
                ret = recv_default_packet(header);
                break;
            }
        }

        if (!ret)
            recv_error_packet(header);

        static uint32_t next_refresh_time = el_timer_get_m_sec();

        uint32_t cur_time = el_timer_get_m_sec();
        if (next_refresh_time > cur_time)
            return;

        if (is_refresh_character_wnd_) {
            is_refresh_character_wnd_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_character", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_equipment_wnd_) {
            is_refresh_equipment_wnd_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_equipment", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_inventory_wnd_) {
            is_refresh_inventory_wnd_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_inventory", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_exchange_wnd_) {
            is_refresh_exchange_wnd_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_exchange", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_skill_wnd_) {
            is_refresh_skill_wnd_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_skill", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_safebox_wnd_) {
            is_refresh_safebox_wnd_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_safebox", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_mall_wnd_) {
            is_refresh_mall_wnd_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_mall", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_status_) {
            is_refresh_status_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_status", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_messenger_wnd_) {
            is_refresh_messenger_wnd_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_messenger", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_guild_wnd_info_page_) {
            is_refresh_guild_wnd_info_page_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_guild_info_page", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_guild_wnd_board_page_) {
            is_refresh_guild_wnd_board_page_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_guild_board_page", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_guild_wnd_member_page_) {
            is_refresh_guild_wnd_member_page_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_guild_member_page", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_guild_wnd_member_page_grade_combo_box_) {
            is_refresh_guild_wnd_member_page_grade_combo_box_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_guild_member_page_grade_combo_box", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_guild_wnd_skill_page_) {
            is_refresh_guild_wnd_skill_page_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_guild_skill_page", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }

        if (is_refresh_guild_wnd_grade_page_) {
            is_refresh_guild_wnd_grade_page_ = false;
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_guild_grade_page", Py_BuildValue("()"));
            next_refresh_time = cur_time + 300;
        }
    }

    void python_network_stream::initialize_game_phase() {
        server_time_sync_initialize();

        is_refresh_status_ = false;
        is_refresh_character_wnd_ = false;
        is_refresh_equipment_wnd_ = false;
        is_refresh_inventory_wnd_ = false;
        is_refresh_exchange_wnd_ = false;
        is_refresh_skill_wnd_ = false;
        is_refresh_safebox_wnd_ = false;
        is_refresh_mall_wnd_ = false;
        is_refresh_messenger_wnd_ = false;
        is_refresh_guild_wnd_info_page_ = false;
        is_refresh_guild_wnd_board_page_ = false;
        is_refresh_guild_wnd_member_page_ = false;
        is_refresh_guild_wnd_member_page_grade_combo_box_ = false;
        is_refresh_guild_wnd_skill_page_ = false;
        is_refresh_guild_wnd_grade_page_ = false;

        emoticon_string_vector_.clear();

        inst_target_ = nullptr;
    }

    void python_network_stream::warp(int32_t global_x, int32_t global_y) {
        python_background &bg_mgr = get_python_background();
        bg_mgr.destroy();
        bg_mgr.create();
        bg_mgr.warp(global_x, global_y);
        bg_mgr.refresh_shadow_level();

        // After a warp the centre position has height 0 and the camera would sit in the ground,
        // because the height is only updated when moving, so set the position once explicitly
        // after a map change.
        int32_t local_x = global_x;
        int32_t local_y = global_y;
        global_position_to_local_position_impl(local_x, local_y);
        float height = get_python_background().height(float(local_x), float(local_y));

        abstract_application &app = get_abstract_application();
        app.set_center_position(float(local_x), float(local_y), height);

        show_map_name(local_x, local_y);
    }

    void python_network_stream::show_map_name(int32_t local_x, int32_t local_y) {
        const std::string &map_file_name = get_python_background().warp_map_name();
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "show_map_name", Py_BuildValue("(sii)", map_file_name.c_str(), local_x, local_y));
    }

    void python_network_stream::leave_game_phase() {
        instance_base::clear_pvp_key_system();

        clear_network_actor_manager();

        combo_skill_flag_ = false;

        abstract_character_manager &chr_mgr = get_abstract_character_manager();
        chr_mgr.destroy();

        python_item &item_mgr = get_python_item();
        item_mgr.destroy();
    }

    void python_network_stream::set_game_phase() {
        if ("Game" != phase_value_)
            phase_leave_func_.run();

        metin2::log::debug("Entering the game phase");

        phase_value_ = "Game";

        changing_phase_time_ = el_timer_get_m_sec();
        phase_process_func_.set(this, &python_network_stream::game_phase);
        phase_leave_func_.set(this, &python_network_stream::leave_game_phase);

        abstract_player &player = get_abstract_player();
        player.set_main_character_index(main_actor_vid());

        refresh_status();
    }

    auto python_network_stream::recv_warp_packet() -> bool {
        packet_gc_warp warp_packet;

        if (!recv(sizeof(warp_packet), &warp_packet))
            return false;

        direct_enter_mode_set(selected_character_index_);

        network_stream::connect((uint32_t)warp_packet.addr, warp_packet.port);

        return true;
    }

    auto python_network_stream::recv_duel_start_packet() -> bool {
        packet_gc_duel_start duel_start_packet;
        if (!recv(sizeof(duel_start_packet), &duel_start_packet))
            return false;

        uint32_t count = (duel_start_packet.size - sizeof(duel_start_packet)) / sizeof(uint32_t);

        python_character_manager &chr_mgr = get_python_character_manager();

        instance_base *inst_main = chr_mgr.main_instance_ptr();
        if (!inst_main) {
            metin2::log::error("Cannot start the duel: there is no main character");
            return true;
        }
        uint32_t vid_src = inst_main->virtual_id();
        uint32_t vid_dest;

        for (uint32_t i = 0; i < count; i++) {
            recv(sizeof(vid_dest), &vid_dest);
            instance_base::insert_duel_key(vid_src, vid_dest);
        }

        if (count == 0)
            inst_main->set_duel_mode(instance_base::DUEL_CANNOTATTACK);
        else
            inst_main->set_duel_mode(instance_base::DUEL_START);

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "close_target_board", Py_BuildValue("()"));

        chr_mgr.refresh_all_pc_text_tail();

        return true;
    }

    auto python_network_stream::recv_pvp_packet() -> bool {
        packet_gcpvp pvp_packet;
        if (!recv(sizeof(pvp_packet), &pvp_packet))
            return false;

        python_character_manager &chr_mgr = get_python_character_manager();
        python_player &player = get_python_player();

        switch (pvp_packet.mode_value) {
        case PVP_MODE_AGREE:
            chr_mgr.remove_pvp_key(pvp_packet.vid_src, pvp_packet.vid_dst);

            // The other side asked the main character to agree to a duel.
            if (player.is_main_character_index(pvp_packet.vid_dst))
                player.remember_challenge_instance(pvp_packet.vid_src);

            // The main character asked; it cannot fight until the other side agrees.
            if (player.is_main_character_index(pvp_packet.vid_src))
                player.remember_cant_fight_instance(pvp_packet.vid_dst);
            break;
        case PVP_MODE_REVENGE:
            {
                chr_mgr.remove_pvp_key(pvp_packet.vid_src, pvp_packet.vid_dst);

                uint32_t killer = pvp_packet.vid_src;
                uint32_t victim = pvp_packet.vid_dst;

                // The main character was killed and may take revenge.
                if (player.is_main_character_index(victim))
                    player.remember_revenge_instance(killer);

                // The main character is the killer and cannot fight while the victim takes revenge.
                if (player.is_main_character_index(killer))
                    player.remember_cant_fight_instance(victim);
                break;
            }

        case PVP_MODE_FIGHT:
            chr_mgr.insert_pvp_key(pvp_packet.vid_src, pvp_packet.vid_dst);
            player.forget_instance(pvp_packet.vid_src);
            player.forget_instance(pvp_packet.vid_dst);
            break;
        case PVP_MODE_NONE:
            chr_mgr.remove_pvp_key(pvp_packet.vid_src, pvp_packet.vid_dst);
            player.forget_instance(pvp_packet.vid_src);
            player.forget_instance(pvp_packet.vid_dst);
            break;
        }

        refresh_target_board_by_vid(pvp_packet.vid_src);
        refresh_target_board_by_vid(pvp_packet.vid_dst);

        return true;
    }

    void python_network_stream::notify_hack(const char *msg) {
        if (!que_hack_.empty())
            if (msg == que_hack_.back())
                return;

        que_hack_.push_back(msg);
    }

    auto python_network_stream::send_hack(const char *msg) -> bool {
        metin2::log::debug("Sending hack report: {}", msg);

        packet_cg_hack packet_hack;
        packet_hack.header_value = HEADER_CG_HACK;
        utf8::copy_field(packet_hack.buf, msg);

        if (!send(sizeof(packet_hack), &packet_hack))
            return false;

        return send_sequence();
    }

    auto python_network_stream::send_messenger_add_by_vid_packet(uint32_t vid) -> bool {
        packet_cg_messenger packet;
        packet.header_value = HEADER_CG_MESSENGER;
        packet.subheader = MESSENGER_SUBHEADER_CG_ADD_BY_VID;
        if (!send(sizeof(packet), &packet))
            return false;
        if (!send(sizeof(vid), &vid))
            return false;
        return send_sequence();
    }

    auto python_network_stream::send_messenger_add_by_name_packet(const char *c_sz_name) -> bool {
        packet_cg_messenger packet;
        packet.header_value = HEADER_CG_MESSENGER;
        packet.subheader = MESSENGER_SUBHEADER_CG_ADD_BY_NAME;
        if (!send(sizeof(packet), &packet))
            return false;
        char sz_name[CHARACTER_NAME_MAX_LEN];
        utf8::copy_field(sz_name, c_sz_name);

        if (!send(sizeof(sz_name), &sz_name))
            return false;
        metin2::log::debug("Sending messenger add request for {}", c_sz_name);
        return send_sequence();
    }

    auto python_network_stream::send_messenger_remove_packet(const char *c_sz_key, const char *name) -> bool {
        packet_cg_messenger packet;
        packet.header_value = HEADER_CG_MESSENGER;
        packet.subheader = MESSENGER_SUBHEADER_CG_REMOVE;
        if (!send(sizeof(packet), &packet))
            return false;
        char sz_key[CHARACTER_NAME_MAX_LEN];
        utf8::copy_field(sz_key, c_sz_key);
        if (!send(sizeof(sz_key), &sz_key))
            return false;
        refresh_target_board_by_name(name);
        return send_sequence();
    }

    auto python_network_stream::send_character_state_packet(const pixel_position &p_pos_dst, float dst_rot, uint32_t func, uint32_t arg) -> bool {
        if (!can_act_main_instance())
            return true;

        if (dst_rot < 0.0f)
            dst_rot = 360 + dst_rot;
        else if (dst_rot > 360.0f)
            dst_rot = fmodf(dst_rot, 360.0f);

        packet_cg_move state_packet;
        state_packet.header_value = HEADER_CG_CHARACTER_MOVE;
        state_packet.func = func;
        state_packet.arg_value = arg;
        state_packet.rot = dst_rot / 5.0f;
        state_packet.x = int32_t(p_pos_dst.x);
        state_packet.y = int32_t(p_pos_dst.y);
        state_packet.time = el_timer_get_server_m_sec();

        assert(state_packet.x >= 0 && state_packet.x < 204800);

        local_position_to_global_position_impl(state_packet.x, state_packet.y);

        if (!send(sizeof(state_packet), &state_packet)) {
            metin2::log::debug("Cannot send the character state packet (time {}, position ({:.6f}, {:.6f}), rotation {:.6f}, function {}, argument {})", state_packet.time, float(state_packet.x), float(state_packet.y), dst_rot, state_packet.func, state_packet.arg_value);
            return false;
        }
        return send_sequence();
    }

    auto python_network_stream::send_use_skill_packet(uint32_t skill_index, uint32_t target_vid) -> bool {
        packet_cg_use_skill use_skill_packet;
        use_skill_packet.header_value = HEADER_CG_USE_SKILL;
        use_skill_packet.vnum = skill_index;
        use_skill_packet.target_vid = target_vid;
        if (!send(sizeof(packet_cg_use_skill), &use_skill_packet)) {
            metin2::log::debug("Cannot send the use skill packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_chat_packet(const char *chat, uint8_t type) -> bool {
        if (strlen(chat) == 0)
            return true;

        if (strlen(chat) >= 512)
            return true;

        if (chat[0] == '/') {
            if (1 == strlen(chat)) {
                if (!last_command_.empty())
                    chat = last_command_.c_str();
            } else {
                last_command_ = chat;
            }
        }

        if (client_command(chat))
            return true;

        int text_len = strlen(chat) + 1;
        packet_cg_chat chat_packet;
        chat_packet.header_value = HEADER_CG_CHAT;
        chat_packet.length = sizeof(chat_packet) + text_len;
        chat_packet.type_value = type;

        if (!send(sizeof(chat_packet), &chat_packet))
            return false;

        if (!send(text_len, chat))
            return false;

        return send_sequence();
    }

    void python_network_stream::register_emoticon_string(const char *emoticon_string) {
        if (emoticon_string_vector_.size() >= instance_base::EMOTICON_NUM) {
            metin2::log::error("Cannot register the emoticon string: all {} slots are in use", emoticon_string_vector_.size());
            return;
        }
        emoticon_string_vector_.push_back(emoticon_string);
    }

    auto python_network_stream::parse_emoticon(const char *chat_msg, uint32_t *emoticon) -> bool {
        for (uint32_t emoticon_index = 0; emoticon_index < emoticon_string_vector_.size(); ++emoticon_index) {
            if (strlen(chat_msg) > emoticon_string_vector_[emoticon_index].size())
                continue;

            const char *find = strstr(chat_msg, emoticon_string_vector_[emoticon_index].c_str());

            if (find != chat_msg)
                continue;

            *emoticon = emoticon_index;

            return true;
        }

        return false;
    }

    auto python_network_stream::recv_chat_packet() -> bool {
        packet_gc_chat chat;
        char buf[1024 + 1];
        char line[1024 + 1];

        if (!recv(sizeof(chat), &chat))
            return false;

        if (!recv_packet_text(buf, sizeof(buf)))
            return false;

        if (chat.type_value >= CHAT_TYPE_MAX_NUM)
            return true;

        if (CHAT_TYPE_COMMAND == chat.type_value) {
            server_command(buf);
            return true;
        }

        if (chat.vid != 0) {
            python_character_manager &chr_mgr = get_python_character_manager();
            instance_base *inst_chatter = chr_mgr.instance_ptr(chat.vid);
            if (nullptr == inst_chatter)
                return true;

            switch (chat.type_value) {
            case CHAT_TYPE_TALKING:
            case CHAT_TYPE_PARTY:
            case CHAT_TYPE_GUILD:
            case CHAT_TYPE_SHOUT:
            case CHAT_TYPE_WHISPER: // client-only enum, not shared with the server
                {
                    char *p = strchr(buf, ':');

                    if (p)
                        p += 2;
                    else
                        p = buf;

                    uint32_t emoticon;

                    if (parse_emoticon(p, &emoticon)) {
                        inst_chatter->set_emoticon(emoticon);
                        return true;
                    } else {
                        if (is_enable_chat_insult_filter_) {
                            if (false == inst_chatter->is_npc() && false == inst_chatter->is_enemy()) {
                                filter_insult_impl(p, strlen(p));
                            }
                        }

                        snprintf(line, sizeof(line), "%s", p);
                    }
                }
                break;
            case CHAT_TYPE_COMMAND:
            case CHAT_TYPE_INFO:
            case CHAT_TYPE_NOTICE:
            case CHAT_TYPE_BIG_NOTICE:
            case CHAT_TYPE_MAX_NUM:
            default:
                snprintf(line, sizeof(line), "%s", buf);
                break;
            }

            if (CHAT_TYPE_SHOUT != chat.type_value) {
                get_python_text_tail().register_chat_tail(chat.vid, line);
            }

            if (inst_chatter->is_pc())
                get_python_chat().append_chat(chat.type_value, buf);
        } else {
            if (CHAT_TYPE_NOTICE == chat.type_value) {
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "set_tip_message", Py_BuildValue("(s)", buf));
            } else if (CHAT_TYPE_BIG_NOTICE == chat.type_value) {
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "set_big_message", Py_BuildValue("(s)", buf));
            } else if (CHAT_TYPE_SHOUT == chat.type_value) {
                char *p = strchr(buf, ':');

                if (p) {
                    if (is_enable_chat_insult_filter_)
                        filter_insult_impl(p, strlen(p));
                }
            }

            get_python_chat().append_chat(chat.type_value, buf);
        }
        return true;
    }

    auto python_network_stream::recv_whisper_packet() -> bool {
        packet_gc_whisper whisper_packet;
        char buf[512 + 1];

        if (!recv(sizeof(whisper_packet), &whisper_packet))
            return false;

        if (!recv_packet_text(buf, sizeof(buf)))
            return false;

        const std::string name_from = packet_string(whisper_packet.name_from);

        static char line[CHARACTER_NAME_MAX_LEN + 3 + sizeof(buf)]; // "name : text", never cut
        if (python_chat::WHISPER_TYPE_CHAT == whisper_packet.type_value || python_chat::WHISPER_TYPE_GM == whisper_packet.type_value) {
            snprintf(line, sizeof(line), "%s : %s", name_from.c_str(), buf);
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_recv_whisper", Py_BuildValue("(iss)", (int)whisper_packet.type_value, name_from.c_str(), line));
        } else if (python_chat::WHISPER_TYPE_SYSTEM == whisper_packet.type_value || python_chat::WHISPER_TYPE_ERROR == whisper_packet.type_value) {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_recv_whisper_system_message", Py_BuildValue("(iss)", (int)whisper_packet.type_value, name_from.c_str(), buf));
        } else {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_recv_whisper_error", Py_BuildValue("(iss)", (int)whisper_packet.type_value, name_from.c_str(), buf));
        }

        return true;
    }

    auto python_network_stream::send_whisper_packet(const char *name, const char *chat) -> bool {
        if (strlen(chat) >= 255)
            return true;

        int text_len = strlen(chat) + 1;
        packet_cg_whisper whisper_packet;
        whisper_packet.header_value = HEADER_CG_WHISPER;
        whisper_packet.size = sizeof(whisper_packet) + text_len;

        utf8::copy_field(whisper_packet.name_to, name);

        if (!send(sizeof(whisper_packet), &whisper_packet))
            return false;

        if (!send(text_len, chat))
            return false;

        return send_sequence();
    }

    auto python_network_stream::recv_point_change() -> bool {
        packet_gc_point_change point_change;

        if (!recv(sizeof(packet_gc_point_change), &point_change)) {
            metin2::log::debug("Cannot read the point change packet");
            return false;
        }

        python_character_manager &chr_mgr = get_python_character_manager();
        chr_mgr.show_point_effect(point_change.type_value, point_change.vid);

        instance_base *instance = get_python_character_manager().main_instance_ptr();

        if (instance) {
            if (point_change.vid == instance->virtual_id()) {
                python_player &player = get_python_player();
                player.set_status(point_change.type_value, point_change.value);

                switch (point_change.type_value) {
                case POINT_STAT_RESET_COUNT:
                    refresh_status();
                    break;
                case POINT_LEVEL:
                case POINT_ST:
                case POINT_DX:
                case POINT_HT:
                case POINT_IQ:
                    refresh_status();
                    refresh_skill_window();
                    break;
                case POINT_SKILL:
                case POINT_SUB_SKILL:
                case POINT_HORSE_SKILL:
                    refresh_skill_window();
                    break;
                case POINT_ENERGY:
                    if (point_change.value == 0) {
                        player.set_status(POINT_ENERGY_END_TIME, 0);
                    }
                    refresh_status();
                    break;
                default:
                    refresh_status();
                    break;
                }

                if (POINT_GOLD == point_change.type_value) {
                    if (point_change.amount > 0) {
                        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_pick_money", Py_BuildValue("(i)", point_change.amount));
                    }
                }
            } else {
                if (point_change.type_value == POINT_LEVEL) {
                    instance = get_python_character_manager().instance_ptr(point_change.vid);
                    if (instance) {
                        instance->set_level(point_change.value);
                        instance->update_text_tail_level(point_change.value);
                    }
                }
            }
        }
        return true;
    }

    auto python_network_stream::recv_stun_packet() -> bool {
        packet_gc_stun stun_packet;

        if (!recv(sizeof(stun_packet), &stun_packet)) {
            metin2::log::debug("Cannot read the stun packet");
            return false;
        }

        python_character_manager &chr_mgr = get_python_character_manager();
        instance_base *inst_sel = chr_mgr.instance_ptr(stun_packet.vid);

        if (inst_sel) {
            if (get_python_character_manager().main_instance_ptr() == inst_sel)
                inst_sel->die();
            else
                inst_sel->stun();
        }

        return true;
    }

    auto python_network_stream::recv_dead_packet() -> bool {
        packet_gc_dead dead_packet;
        if (!recv(sizeof(dead_packet), &dead_packet)) {
            metin2::log::debug("Cannot read the dead packet");
            return false;
        }

        python_character_manager &chr_mgr = get_python_character_manager();
        instance_base *chr_inst_sel = chr_mgr.instance_ptr(dead_packet.vid);
        if (chr_inst_sel) {
            instance_base *inst_main = chr_mgr.main_instance_ptr();
            if (inst_main == chr_inst_sel) {
                metin2::log::debug("Main character died");
                if (false == inst_main->duel_mode()) {
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_game_over", Py_BuildValue("()"));
                }
                get_python_player().notify_dead_main_character();
            }

            chr_inst_sel->die();
        }

        return true;
    }

    auto python_network_stream::send_on_click_packet(uint32_t vid) -> bool {
        packet_cg_on_click on_click_packet;
        on_click_packet.header_value = HEADER_CG_ON_CLICK;
        on_click_packet.vid = vid;

        if (!send(sizeof(on_click_packet), &on_click_packet)) {
            metin2::log::debug("Cannot send the click packet");
            return false;
        }

        metin2::log::debug("Sent the click packet");
        return send_sequence();
    }

    auto python_network_stream::recv_character_position_packet() -> bool {
        packet_gc_position position_packet;

        if (!recv(sizeof(packet_gc_position), &position_packet))
            return false;

        instance_base *chr_instance = get_python_character_manager().instance_ptr(position_packet.vid);

        if (!chr_instance)
            return true;

        return true;
    }

    auto python_network_stream::recv_shop_packet() -> bool {
        packet_gc_shop packet_shop;
        if (!recv(sizeof(packet_shop), &packet_shop))
            return false;

        switch (packet_shop.subheader) {
        case SHOP_SUBHEADER_GC_START:
            {
                packet_gc_shop_start start;
                if (!recv(sizeof(start), &start))
                    return false;

                get_python_shop().clear();
                for (uint8_t item_index = 0; item_index < SHOP_HOST_ITEM_MAX_NUM; ++item_index)
                    get_python_shop().set_item_data(item_index, start.items[item_index]);

                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "start_shop", Py_BuildValue("(i)", start.owner_vid));
            }
            break;

        case SHOP_SUBHEADER_GC_START_EX:
            {
                packet_gc_shop_start_ex start;
                if (!recv(sizeof(start), &start))
                    return false;

                get_python_shop().clear();
                const uint8_t shop_tab_count = std::min<uint8_t>(start.shop_tab_count, SHOP_TAB_COUNT_MAX);
                get_python_shop().set_tab_count(shop_tab_count);

                for (uint8_t i = 0; i < shop_tab_count; i++) {
                    packet_gc_shop_start_ex::sub_packet_shop_tab tab;
                    if (!recv(sizeof(tab), &tab))
                        return false;

                    get_python_shop().set_tab_coin_type(i, tab.coin_type);
                    get_python_shop().set_tab_name(i, packet_string(tab.name_value).c_str());
                    for (uint8_t j = 0; j < SHOP_HOST_ITEM_MAX_NUM; j++)
                        get_python_shop().set_item_data(i, j, tab.items[j]);
                }

                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "start_shop", Py_BuildValue("(i)", start.owner_vid));
            }
            break;

        case SHOP_SUBHEADER_GC_END:
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "end_shop", Py_BuildValue("()"));
            break;

        case SHOP_SUBHEADER_GC_UPDATE_ITEM:
            {
                packet_gc_shop_update_item update;
                if (!recv(sizeof(update), &update))
                    return false;
                get_python_shop().set_item_data(update.pos, update.item_value);
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_shop", Py_BuildValue("()"));
            }
            break;

        case SHOP_SUBHEADER_GC_UPDATE_PRICE:
            {
                int32_t price;
                if (!recv(sizeof(price), &price))
                    return false;
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "set_shop_selling_price", Py_BuildValue("(i)", price));
            }
            break;

        case SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY:
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_shop_error", Py_BuildValue("(s)", "NOT_ENOUGH_MONEY"));
            break;

        case SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY_EX:
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_shop_error", Py_BuildValue("(s)", "NOT_ENOUGH_MONEY_EX"));
            break;

        case SHOP_SUBHEADER_GC_SOLDOUT:
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_shop_error", Py_BuildValue("(s)", "SOLDOUT"));
            break;

        case SHOP_SUBHEADER_GC_INVENTORY_FULL:
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_shop_error", Py_BuildValue("(s)", "INVENTORY_FULL"));
            break;

        case SHOP_SUBHEADER_GC_INVALID_POS:
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_shop_error", Py_BuildValue("(s)", "INVALID_POS"));
            break;

        default:
            metin2::log::warn("Shop packet with an unknown subheader is skipped");
            break;
        }

        return true;
    }

    auto python_network_stream::recv_exchange_packet() -> bool {
        packet_gc_exchange exchange_packet;

        if (!recv(sizeof(exchange_packet), &exchange_packet))
            return false;

        switch (exchange_packet.subheader) {
        case EXCHANGE_SUBHEADER_GC_START:
            get_python_exchange().clear();
            get_python_exchange().start();
            get_python_exchange().set_self_name(get_python_player().get_name());

            {
                instance_base *character_instance = get_python_character_manager().instance_ptr(exchange_packet.arg1);

                if (character_instance)
                    get_python_exchange().set_target_name(character_instance->name_string());
            }

            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "start_exchange", Py_BuildValue("()"));
            break;

        case EXCHANGE_SUBHEADER_GC_ITEM_ADD:
            if (exchange_packet.is_me) {
                int slot_index = exchange_packet.arg2.cell;
                get_python_exchange().set_item_to_self(slot_index, exchange_packet.arg1, (uint8_t)exchange_packet.arg3);
                for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
                    get_python_exchange().set_item_metin_socket_to_self(slot_index, i, exchange_packet.values[i]);
                for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j)
                    get_python_exchange().set_item_attribute_to_self(slot_index, j, exchange_packet.attr[j].type_value, exchange_packet.attr[j].value);
            } else {
                int slot_index = exchange_packet.arg2.cell;
                get_python_exchange().set_item_to_target(slot_index, exchange_packet.arg1, (uint8_t)exchange_packet.arg3);
                for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
                    get_python_exchange().set_item_metin_socket_to_target(slot_index, i, exchange_packet.values[i]);
                for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j)
                    get_python_exchange().set_item_attribute_to_target(slot_index, j, exchange_packet.attr[j].type_value, exchange_packet.attr[j].value);
            }

            refresh_exchange_window();
            refresh_inventory_window();
            break;

        case EXCHANGE_SUBHEADER_GC_ITEM_DEL:
            if (exchange_packet.is_me) {
                get_python_exchange().del_item_of_self((uint8_t)exchange_packet.arg1);
            } else {
                get_python_exchange().del_item_of_target((uint8_t)exchange_packet.arg1);
            }
            refresh_exchange_window();
            refresh_inventory_window();
            break;

        case EXCHANGE_SUBHEADER_GC_ELK_ADD:
            if (exchange_packet.is_me)
                get_python_exchange().set_elk_to_self(exchange_packet.arg1);
            else
                get_python_exchange().set_elk_to_target(exchange_packet.arg1);

            refresh_exchange_window();
            break;

        case EXCHANGE_SUBHEADER_GC_ACCEPT:
            if (exchange_packet.is_me) {
                get_python_exchange().set_accept_to_self((uint8_t)exchange_packet.arg1);
            } else {
                get_python_exchange().set_accept_to_target((uint8_t)exchange_packet.arg1);
            }
            refresh_exchange_window();
            break;

        case EXCHANGE_SUBHEADER_GC_END:
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "end_exchange", Py_BuildValue("()"));
            refresh_inventory_window();
            get_python_exchange().end();
            break;

        case EXCHANGE_SUBHEADER_GC_ALREADY:
            metin2::log::debug("Trade: the target is already trading");
            break;

        case EXCHANGE_SUBHEADER_GC_LESS_ELK:
            metin2::log::debug("Trade: not enough gold");
            break;
        };

        return true;
    }

    auto python_network_stream::recv_quest_info_packet() -> bool {
        packet_gc_quest_info quest_info;

        if (!peek(sizeof(packet_gc_quest_info), &quest_info)) {
            metin2::log::debug("Cannot read the quest info packet header");
            return false;
        }

        if (!peek(quest_info.size)) {
            metin2::log::debug("Cannot read the quest info packet body");
            return false;
        }

        recv(sizeof(packet_gc_quest_info));

        const uint16_t flag = quest_info.flag;

        // The fields follow in the order of their flags. A quest that is not ended is updated with
        // what was sent; "begun" does not reset the rest (q.start() also comes alone).
        bool end = false;
        if (0 != (flag & QUEST_SEND_IS_BEGIN)) {
            uint8_t is_begin;
            if (!recv(sizeof(is_begin), &is_begin))
                return false;
            end = !is_begin;
        }

        char title[30 + 1] = "";
        char clock_name[16 + 1] = "";
        int clock_value = 0;
        char counter_name[16 + 1] = "";
        int counter_value = 0;
        char icon_file_name[24 + 1] = "";
        uint8_t status = 0;
        uint8_t has_letter = 0;

        if (0 != (flag & QUEST_SEND_TITLE)) {
            if (!recv(sizeof(title), &title))
                return false;

            title[30] = '\0';
        }
        if (0 != (flag & QUEST_SEND_CLOCK_NAME)) {
            if (!recv(sizeof(clock_name), &clock_name))
                return false;

            clock_name[16] = '\0';
        }
        if (0 != (flag & QUEST_SEND_CLOCK_VALUE)) {
            if (!recv(sizeof(clock_value), &clock_value))
                return false;
        }
        if (0 != (flag & QUEST_SEND_COUNTER_NAME)) {
            if (!recv(sizeof(counter_name), &counter_name))
                return false;

            counter_name[16] = '\0';
        }
        if (0 != (flag & QUEST_SEND_COUNTER_VALUE)) {
            if (!recv(sizeof(counter_value), &counter_value))
                return false;
        }
        if (0 != (flag & QUEST_SEND_ICON_FILE)) {
            if (!recv(sizeof(icon_file_name), &icon_file_name))
                return false;

            icon_file_name[24] = '\0';
        }
        if (0 != (flag & QUEST_SEND_STATUS)) {
            if (!recv(sizeof(status), &status))
                return false;
        }
        if (0 != (flag & QUEST_SEND_LETTER)) {
            if (!recv(sizeof(has_letter), &has_letter))
                return false;
        }

        python_quest &quest = get_python_quest();
        const uint32_t index = quest_info.index_value;

        if (end) {
            quest.delete_quest_instance(index);
        } else {
            if (!quest.is_quest(index))
                quest.make_quest(index);

            if (strlen(title) > 0)
                quest.set_quest_title(index, title);
            if (strlen(clock_name) > 0)
                quest.set_quest_clock_name(index, clock_name);
            if (strlen(counter_name) > 0)
                quest.set_quest_counter_name(index, counter_name);
            if (strlen(icon_file_name) > 0)
                quest.set_quest_icon_file_name(index, icon_file_name);

            if (flag & QUEST_SEND_CLOCK_VALUE)
                quest.set_quest_clock_value(index, clock_value);
            if (flag & QUEST_SEND_COUNTER_VALUE)
                quest.set_quest_counter_value(index, counter_value);
            if (flag & QUEST_SEND_STATUS)
                quest.set_quest_status(index, status);
            if (flag & QUEST_SEND_LETTER)
                quest.set_quest_letter(index, has_letter != 0);
        }

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_quest", Py_BuildValue("()"));
        return true;
    }

    auto python_network_stream::recv_quest_confirm_packet() -> bool {
        packet_gc_quest_confirm quest_confirm_packet;
        if (!recv(sizeof(quest_confirm_packet), &quest_confirm_packet)) {
            metin2::log::debug("Cannot read the quest confirm packet");
            return false;
        }

        PyObject *arg = Py_BuildValue("(sii)", packet_string(quest_confirm_packet.msg).c_str(), quest_confirm_packet.timeout, quest_confirm_packet.request_pid);
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_quest_confirm", arg);
        return true;
    }

    auto python_network_stream::recv_request_make_guild() -> bool {
        packet_gc_request_make_guild packet;
        if (!recv(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot read the guild creation request packet");
            return false;
        }

        guild_creation_key_ = packet.key;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "ask_guild_name", Py_BuildValue("()"));

        return true;
    }

    void python_network_stream::toggle_game_debug_info() {
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "toggle_debug_info", Py_BuildValue("()"));
    }

    auto python_network_stream::send_exchange_start_packet(uint32_t vid) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_exchange packet;

        packet.header_value = HEADER_CG_EXCHANGE;
        packet.subheader = EXCHANGE_SUBHEADER_CG_START;
        packet.arg1 = vid;

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the trade start packet");
            return false;
        }

        metin2::log::debug("Sent trade start request to vid {}", vid);
        return send_sequence();
    }

    auto python_network_stream::send_exchange_elk_add_packet(uint32_t elk) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_exchange packet;

        packet.header_value = HEADER_CG_EXCHANGE;
        packet.subheader = EXCHANGE_SUBHEADER_CG_ELK_ADD;
        packet.arg1 = elk;

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the trade gold packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_exchange_item_add_packet(item_pos item_pos, uint8_t display_pos) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_exchange packet;

        packet.header_value = HEADER_CG_EXCHANGE;
        packet.subheader = EXCHANGE_SUBHEADER_CG_ITEM_ADD;
        packet.pos = item_pos;
        packet.arg2 = display_pos;

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the trade item add packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_exchange_item_del_packet(uint8_t pos) -> bool {
        assert(!"Can't be called function - CPythonNetworkStream::SendExchangeItemDelPacket");
        return true;

        if (!can_act_main_instance())
            return true;

        packet_cg_exchange packet;

        packet.header_value = HEADER_CG_EXCHANGE;
        packet.subheader = EXCHANGE_SUBHEADER_CG_ITEM_DEL;
        packet.arg1 = pos;

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the trade item remove packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_exchange_accept_packet() -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_exchange packet;

        packet.header_value = HEADER_CG_EXCHANGE;
        packet.subheader = EXCHANGE_SUBHEADER_CG_ACCEPT;

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the trade accept packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_exchange_exit_packet() -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_exchange packet;

        packet.header_value = HEADER_CG_EXCHANGE;
        packet.subheader = EXCHANGE_SUBHEADER_CG_CANCEL;

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the trade exit packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::is_player_attacking() -> bool {
        python_character_manager &chr_mgr = get_python_character_manager();
        instance_base *inst_main = chr_mgr.main_instance_ptr();
        if (!inst_main)
            return false;

        if (!inst_main->is_attacking())
            return false;

        return true;
    }

    auto python_network_stream::recv_script_packet() -> bool {
        packet_gc_script script_packet;

        if (!recv(sizeof(packet_gc_script), &script_packet)) {
            metin2::log::error("Cannot read the script packet");
            return false;
        }

        if (script_packet.size < sizeof(packet_gc_script)) {
            metin2::log::error("Script packet is shorter than its header");
            return false;
        }

        script_packet.size -= sizeof(packet_gc_script);

        // Exactly the script: a NUL in the string would be typed as a letter (a box glyph).
        std::string str(script_packet.size, '\0');
        if (script_packet.size > 0 && !recv(script_packet.size, &str[0]))
            return false;

        int index = get_python_event_manager().register_event_set_from_string(str);

        if (-1 != index) {
            get_python_event_manager().set_visible_line_count(index, 30);
            get_python_network_stream().on_script_event_start(script_packet.skin_value, index);
        }

        return true;
    }

    auto python_network_stream::send_script_answer_packet(int answer) -> bool {
        packet_cg_script_answer script_answer;

        script_answer.header_value = HEADER_CG_SCRIPT_ANSWER;
        script_answer.answer = (uint8_t)answer;
        if (!send(sizeof(packet_cg_script_answer), &script_answer)) {
            metin2::log::debug("Cannot send the script answer packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_script_button_packet(unsigned int index) -> bool {
        packet_cg_script_button script_button;

        script_button.header_value = HEADER_CG_SCRIPT_BUTTON;
        script_button.idx = index;
        if (!send(sizeof(packet_cg_script_button), &script_button)) {
            metin2::log::debug("Cannot send the script button packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_answer_make_guild_packet(const char *name) -> bool {
        packet_cg_answer_make_guild packet;

        packet.header_value = HEADER_CG_ANSWER_MAKE_GUILD;
        utf8::copy_field(packet.guild_name, name);
        packet.key = guild_creation_key_;
        guild_creation_key_ = 0;

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the guild creation answer packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_quest_input_string_packet(const char *string) -> bool {
        packet_cg_quest_input_string packet;
        packet.header_value = HEADER_CG_QUEST_INPUT_STRING;
        utf8::copy_field(packet.string, string);

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the quest input string packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_quest_confirm_packet(uint8_t answer, uint32_t pid) -> bool {
        packet_cg_quest_confirm packet;
        packet.header_value = HEADER_CG_QUEST_CONFIRM;
        packet.answer = answer;
        packet.request_pid = pid;

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the quest confirm packet");
            return false;
        }

        metin2::log::debug("Sent quest confirm answer {} for player {}", answer, pid);
        return send_sequence();
    }

    auto python_network_stream::recv_skill_level_new() -> bool {
        packet_gc_skill_level_new packet;

        if (!recv(sizeof(packet_gc_skill_level_new), &packet)) {
            metin2::log::debug("Cannot read the skill level packet");
            return false;
        }

        python_player &player = get_python_player();

        player.set_skill(7, 0);
        player.set_skill(8, 0);

        for (int i = 0; i < SKILL_MAX_NUM; ++i) {
            player_skill &player_skill = packet.skills[i];

            if (i >= 112 && i <= 115 && player_skill.level_value)
                player.set_skill(7, i);

            if (i >= 116 && i <= 119 && player_skill.level_value)
                player.set_skill(8, i);

            player.set_skill_level_impl(i, player_skill.master_type, player_skill.level_value);
        }

        refresh_skill_window();
        refresh_status();
        return true;
    }

    auto python_network_stream::recv_damage_info_packet() -> bool {
        packet_gc_damage_info damage_info_packet;

        if (!recv(sizeof(packet_gc_damage_info), &damage_info_packet)) {
            metin2::log::debug("Cannot read the damage info packet");
            return false;
        }

        instance_base *inst_target = get_python_character_manager().instance_ptr(damage_info_packet.vid);
        bool self = (inst_target == get_python_character_manager().main_instance_ptr());
        if (inst_target) {
            if (damage_info_packet.damage >= 0)
                inst_target->add_damage_effect(damage_info_packet.damage, damage_info_packet.flag, self);
            else
                metin2::log::warn("Damage info with a negative damage is skipped");
        }

        return true;
    }
    auto python_network_stream::recv_target_packet() -> bool {
        packet_gc_target target_packet;

        if (!recv(sizeof(packet_gc_target), &target_packet)) {
            metin2::log::debug("Cannot read the target packet");
            return false;
        }

        instance_base *inst_player = get_python_character_manager().main_instance_ptr();
        instance_base *inst_target = get_python_character_manager().instance_ptr(target_packet.vid);
        if (inst_player && inst_target) {
            if (!inst_target->is_dead()) {
                if (inst_target->is_pc() || inst_target->is_building())
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "close_target_board_if_different", Py_BuildValue("(i)", target_packet.vid));
                else if (inst_player->can_view_target_hp(*inst_target))
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "set_hp_target_board", Py_BuildValue("(ii)", target_packet.vid, target_packet.hp_percent));
                else
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "close_target_board", Py_BuildValue("()"));

                inst_target_ = inst_target;
            }
        } else {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "close_target_board", Py_BuildValue("()"));
        }

        return true;
    }

    auto python_network_stream::send_attack_packet(uint32_t mot_attack, uint32_t vid_victim) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_attack packet_atk{};

        packet_atk.header_value = HEADER_CG_ATTACK;
        packet_atk.type_value = mot_attack;
        packet_atk.victim_vid = vid_victim;

        if (!send(sizeof(packet_atk), &packet_atk)) {
            metin2::log::debug("Cannot send the attack packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::recv_add_fly_targeting_packet() -> bool {
        packet_gc_fly_targeting packet;
        if (!recv(sizeof(packet), &packet))
            return false;

        global_position_to_local_position_impl(packet.x, packet.y);

        metin2::log::debug("Character (vid {}) added a fly target", packet.shooter_vid);

        python_character_manager &rpcm = get_python_character_manager();

        instance_base *shooter = rpcm.instance_ptr(packet.shooter_vid);

        if (!shooter) {
            metin2::log::debug("Fly target shooter (vid {}) not found", packet.shooter_vid);
            return true;
        }

        instance_base *target = rpcm.instance_ptr(packet.target_vid);

        if (packet.target_vid && target) {
            shooter->graphic_thing_instance_ptr()->add_fly_target(target->graphic_thing_instance_ptr());
        } else {
            float h = get_python_background().height(packet.x, packet.y) + 60.0f; // the packet has no height, so aim a fixed 60 above the ground
            shooter->graphic_thing_instance_ptr()->add_fly_target(math::vector3(packet.x, packet.y, h));
        }

        return true;
    }

    auto python_network_stream::recv_fly_targeting_packet() -> bool {
        packet_gc_fly_targeting packet;
        if (!recv(sizeof(packet), &packet))
            return false;

        global_position_to_local_position_impl(packet.x, packet.y);

        python_character_manager &rpcm = get_python_character_manager();

        instance_base *shooter = rpcm.instance_ptr(packet.shooter_vid);

        if (!shooter) {
            return true;
        }

        instance_base *target = rpcm.instance_ptr(packet.target_vid);

        if (packet.target_vid && target) {
            shooter->graphic_thing_instance_ptr()->set_fly_target(target->graphic_thing_instance_ptr());
        } else {
            float h = get_python_background().height(packet.x, packet.y) + 60.0f; // the packet has no height, so aim a fixed 60 above the ground
            shooter->graphic_thing_instance_ptr()->set_fly_target(math::vector3(packet.x, packet.y, h));
        }

        return true;
    }

    auto python_network_stream::send_shoot_packet(uint32_t skill) -> bool {
        packet_cg_shoot packet_shoot;
        packet_shoot.header_value = HEADER_CG_SHOOT;
        packet_shoot.type_value = skill;

        if (!send(sizeof(packet_shoot), &packet_shoot)) {
            metin2::log::debug("Cannot send the shoot packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_add_fly_targeting_packet(uint32_t target_vid, const pixel_position &p_pos_target) -> bool {
        packet_cg_fly_targeting packet;

        packet.header_value = HEADER_CG_ADD_FLY_TARGETING;
        packet.target_vid = target_vid;
        packet.x = p_pos_target.x;
        packet.y = p_pos_target.y;

        local_position_to_global_position_impl(packet.x, packet.y);

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the fly targeting packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_fly_targeting_packet(uint32_t target_vid, const pixel_position &p_pos_target) -> bool {
        packet_cg_fly_targeting packet;

        packet.header_value = HEADER_CG_FLY_TARGETING;
        packet.target_vid = target_vid;
        packet.x = p_pos_target.x;
        packet.y = p_pos_target.y;

        local_position_to_global_position_impl(packet.x, packet.y);

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the fly targeting packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::recv_create_fly_packet() -> bool {
        packet_gc_create_fly packet;
        if (!recv(sizeof(packet_gc_create_fly), &packet))
            return false;

        flying_manager &fly_mgr = get_flying_manager();
        python_character_manager &chr_mgr = get_python_character_manager();

        instance_base *start_inst = chr_mgr.instance_ptr(packet.start_vid);
        instance_base *end_inst = chr_mgr.instance_ptr(packet.end_vid);
        if (!start_inst || !end_inst)
            return true;

        fly_mgr.create_indexed_fly(packet.type_value, start_inst->graphic_thing_instance_ptr(), end_inst->graphic_thing_instance_ptr());

        return true;
    }

    auto python_network_stream::send_target_packet(uint32_t vid) -> bool {
        packet_cg_target packet;
        packet.header_value = HEADER_CG_TARGET;
        packet.vid = vid;

        if (!send(sizeof(packet), &packet)) {
            metin2::log::debug("Cannot send the target packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_sync_position_element_packet(uint32_t victim_vid, uint32_t victim_x, uint32_t victim_y) -> bool {
        packet_cg_sync_position_element sync_pos;
        sync_pos.vid = victim_vid;
        sync_pos.x = victim_x;
        sync_pos.y = victim_y;

        local_position_to_global_position_impl(sync_pos.x, sync_pos.y);

        if (!send(sizeof(sync_pos), &sync_pos)) {
            metin2::log::debug("Cannot send the sync position packet");
            return false;
        }

        return true;
    }

    auto python_network_stream::recv_messenger() -> bool {
        packet_gc_messenger p;
        if (!recv(sizeof(p), &p))
            return false;

        std::string name;
        switch (p.subheader) {
        case MESSENGER_SUBHEADER_GC_LIST:
            {
                packet_gc_messenger_list_online on;
                while (packet_bytes_left() >= static_cast<int>(sizeof(on))) {
                    if (!recv(sizeof(on), &on) || !recv_string(on.length, name))
                        return false;

                    if (on.connected & MESSENGER_CONNECTED_STATE_ONLINE)
                        get_python_messenger().on_friend_login(name.c_str());
                    else
                        get_python_messenger().on_friend_logout(name.c_str());

                    if (on.connected & MESSENGER_CONNECTED_STATE_MOBILE)
                        get_python_messenger().set_mobile(name.c_str(), true);
                }
                break;
            }

        case MESSENGER_SUBHEADER_GC_LOGIN:
            {
                packet_gc_messenger_login login;
                if (!recv(sizeof(login), &login) || !recv_string(login.length, name))
                    return false;
                get_python_messenger().on_friend_login(name.c_str());
                refresh_target_board_by_name(name.c_str());
                break;
            }

        case MESSENGER_SUBHEADER_GC_LOGOUT:
            {
                packet_gc_messenger_logout logout;
                if (!recv(sizeof(logout), &logout) || !recv_string(logout.length, name))
                    return false;
                get_python_messenger().on_friend_logout(name.c_str());
                break;
            }

        case MESSENGER_SUBHEADER_GC_MOBILE:
            {
                uint8_t state; // flag: the mobile number was removed
                uint8_t length;
                if (!recv(sizeof(state), &state) || !recv(sizeof(length), &length) || !recv_string(length, name))
                    return false;
                get_python_messenger().set_mobile(name.c_str(), state);
                break;
            }
        }
        return true;
    }

    auto python_network_stream::send_party_invite_packet(uint32_t vid) -> bool {
        packet_cg_party_invite party_invite_packet;
        party_invite_packet.header_value = HEADER_CG_PARTY_INVITE;
        party_invite_packet.vid = vid;

        if (!send(sizeof(party_invite_packet), &party_invite_packet)) {
            metin2::log::debug("Cannot send the party invite packet for vid {}", vid);
            return false;
        }

        metin2::log::debug("Sent party invite to vid {}", vid);
        return send_sequence();
    }

    auto python_network_stream::send_party_invite_answer_packet(uint32_t leader_vid, uint8_t answer) -> bool {
        packet_cg_party_invite_answer party_invite_answer_packet;
        party_invite_answer_packet.header_value = HEADER_CG_PARTY_INVITE_ANSWER;
        party_invite_answer_packet.leader_pid = leader_vid;
        party_invite_answer_packet.accept = answer;

        if (!send(sizeof(party_invite_answer_packet), &party_invite_answer_packet)) {
            metin2::log::debug("Cannot send the party invite answer packet (leader vid {}, answer {})", leader_vid, answer);
            return false;
        }

        metin2::log::debug("Sent party invite answer (leader vid {}, answer {})", leader_vid, answer);
        return send_sequence();
    }

    auto python_network_stream::send_party_remove_packet(uint32_t pid) -> bool {
        packet_cg_party_remove party_invite_remove;
        party_invite_remove.header_value = HEADER_CG_PARTY_REMOVE;
        party_invite_remove.pid = pid;

        if (!send(sizeof(party_invite_remove), &party_invite_remove)) {
            metin2::log::debug("Cannot send the party remove packet for player {}", pid);
            return false;
        }

        metin2::log::debug("Sent party remove for player {}", pid);
        return send_sequence();
    }

    auto python_network_stream::send_party_set_state_packet(uint32_t vid, uint8_t state, uint8_t flag) -> bool {
        packet_cg_party_set_state party_set_state;
        party_set_state.header_value = HEADER_CG_PARTY_SET_STATE;
        party_set_state.vid = vid;
        party_set_state.state_value = state;
        party_set_state.flag = flag;

        if (!send(sizeof(party_set_state), &party_set_state)) {
            metin2::log::debug("Cannot send the party state packet (vid {}, state {})", vid, state);
            return false;
        }

        metin2::log::debug("Sent party state (vid {}, state {}, flag {})", vid, state, flag);
        return send_sequence();
    }

    auto python_network_stream::send_party_use_skill_packet(uint8_t skill_index, uint32_t vid) -> bool {
        packet_cg_party_use_skill party_use_skill;
        party_use_skill.header_value = HEADER_CG_PARTY_USE_SKILL;
        party_use_skill.skill_index = skill_index;
        party_use_skill.target_vid = vid;

        if (!send(sizeof(party_use_skill), &party_use_skill)) {
            metin2::log::debug("Cannot send the party skill packet (skill {}, vid {})", skill_index, vid);
            return false;
        }

        metin2::log::debug("Sent party skill use (skill {}, vid {})", skill_index, vid);
        return send_sequence();
    }

    auto python_network_stream::send_party_parameter_packet(uint8_t distribute_mode) -> bool {
        packet_cg_party_parameter party_parameter;
        party_parameter.header_value = HEADER_CG_PARTY_PARAMETER;
        party_parameter.distribute_mode = distribute_mode;

        if (!send(sizeof(party_parameter), &party_parameter)) {
            metin2::log::debug("Cannot send the party parameter packet (distribute mode {})", distribute_mode);
            return false;
        }

        metin2::log::debug("Sent party distribute mode {}", distribute_mode);
        return send_sequence();
    }

    auto python_network_stream::recv_party_invite() -> bool {
        packet_gc_party_invite party_invite_packet;
        if (!recv(sizeof(party_invite_packet), &party_invite_packet))
            return false;

        instance_base *instance = get_python_character_manager().instance_ptr(party_invite_packet.leader_pid);
        if (!instance) {
            metin2::log::warn("Party leader {} not found; the invite is skipped", party_invite_packet.leader_pid);
            return true;
        }

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "recv_party_invite_question", Py_BuildValue("(is)", party_invite_packet.leader_pid, instance->name_string()));
        metin2::log::debug("Received party invite from leader {} ({})", party_invite_packet.leader_pid, instance->name_string());

        return true;
    }

    auto python_network_stream::recv_party_add() -> bool {
        packet_gc_party_add party_add_packet;
        if (!recv(sizeof(party_add_packet), &party_add_packet))
            return false;

        get_python_player().append_party_member(party_add_packet.pid, packet_string(party_add_packet.name_value).c_str());
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "add_party_member", Py_BuildValue("(is)", party_add_packet.pid, packet_string(party_add_packet.name_value).c_str()));
        metin2::log::debug("Party member {} added ({})", party_add_packet.pid, packet_string(party_add_packet.name_value).c_str());

        return true;
    }

    auto python_network_stream::recv_party_update() -> bool {
        packet_gc_party_update party_update_packet;
        if (!recv(sizeof(party_update_packet), &party_update_packet))
            return false;

        python_player::party_member_info *party_member_info;
        if (!get_python_player().party_member_ptr(party_update_packet.pid, &party_member_info))
            return true;

        uint8_t old_state = party_member_info->state_value;

        get_python_player().update_party_member_info(party_update_packet.pid, party_update_packet.state_value, party_update_packet.percent_hp);
        for (int i = 0; i < PARTY_AFFECT_SLOT_MAX_NUM; ++i) {
            get_python_player().update_party_member_affect(party_update_packet.pid, i, party_update_packet.affects_value[i]);
        }

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "update_party_member_info", Py_BuildValue("(i)", party_update_packet.pid));

        // If the leader changed, update the target board buttons.
        uint32_t vid;
        if (get_python_player().party_member_pid_to_vid(party_update_packet.pid, &vid))
            if (old_state != party_update_packet.state_value) {
                refresh_target_board_by_vid(vid);
            }

        return true;
    }

    auto python_network_stream::recv_party_remove() -> bool {
        packet_gc_party_remove party_remove_packet;
        if (!recv(sizeof(party_remove_packet), &party_remove_packet))
            return false;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "remove_party_member", Py_BuildValue("(i)", party_remove_packet.pid));
        metin2::log::debug("Party member {} removed", party_remove_packet.pid);

        return true;
    }

    auto python_network_stream::recv_party_link() -> bool {
        packet_gc_party_link party_link_packet;
        if (!recv(sizeof(party_link_packet), &party_link_packet))
            return false;

        get_python_player().link_party_member(party_link_packet.pid, party_link_packet.vid);
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "link_party_member", Py_BuildValue("(ii)", party_link_packet.pid, party_link_packet.vid));
        metin2::log::debug("Party member {} linked to vid {}", party_link_packet.pid, party_link_packet.vid);

        return true;
    }

    auto python_network_stream::recv_party_unlink() -> bool {
        packet_gc_party_unlink party_unlink_packet;
        if (!recv(sizeof(party_unlink_packet), &party_unlink_packet))
            return false;

        get_python_player().unlink_party_member(party_unlink_packet.pid);

        if (get_python_player().is_main_character_index(party_unlink_packet.vid)) {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "unlink_all_party_member", Py_BuildValue("()"));
        } else {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "unlink_party_member", Py_BuildValue("(i)", party_unlink_packet.pid));
        }

        metin2::log::debug("Party member {} unlinked from vid {}", party_unlink_packet.pid, party_unlink_packet.vid);

        return true;
    }

    auto python_network_stream::recv_party_parameter() -> bool {
        packet_gc_party_parameter party_parameter_packet;
        if (!recv(sizeof(party_parameter_packet), &party_parameter_packet))
            return false;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "change_party_parameter", Py_BuildValue("(i)", party_parameter_packet.distribute_mode));
        metin2::log::debug("Party distribute mode changed to {}", party_parameter_packet.distribute_mode);

        return true;
    }

    auto python_network_stream::send_guild_add_member_packet(uint32_t vid) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_ADD_MEMBER;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;
        if (!send(sizeof(vid), &vid))
            return false;

        metin2::log::debug("Sent guild add member request for vid {}", vid);
        return send_sequence();
    }

    auto python_network_stream::send_guild_remove_member_packet(uint32_t pid) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_REMOVE_MEMBER;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;
        if (!send(sizeof(pid), &pid))
            return false;

        metin2::log::debug("Sent guild remove member request for player {}", pid);
        return send_sequence();
    }

    auto python_network_stream::send_guild_change_grade_name_packet(uint8_t grade_number, const char *c_sz_name) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;
        if (!send(sizeof(grade_number), &grade_number))
            return false;

        char sz_name[GUILD_GRADE_NAME_MAX_LEN + 1];
        utf8::copy_field(sz_name, c_sz_name);

        if (!send(sizeof(sz_name), &sz_name))
            return false;

        metin2::log::debug("Sent guild grade {} name change to {}", grade_number, c_sz_name);
        return send_sequence();
    }

    auto python_network_stream::send_guild_change_grade_authority_packet(uint8_t grade_number, uint8_t authority) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;
        if (!send(sizeof(grade_number), &grade_number))
            return false;
        if (!send(sizeof(authority), &authority))
            return false;

        metin2::log::debug("Sent guild grade {} authority change to {}", grade_number, authority);
        return send_sequence();
    }

    auto python_network_stream::send_guild_offer_packet(uint32_t experience) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_OFFER;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;
        if (!send(sizeof(experience), &experience))
            return false;

        metin2::log::debug("Sent guild experience offer {}", experience);
        return send_sequence();
    }

    auto python_network_stream::send_guild_post_comment_packet(const char *message) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_POST_COMMENT;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;

        uint8_t size = uint8_t(strlen(message)) + 1;
        if (!send(sizeof(size), &size))
            return false;
        if (!send(size, message))
            return false;

        metin2::log::debug("Sent guild comment ({} bytes): {}", size, message);
        return send_sequence();
    }

    auto python_network_stream::send_guild_delete_comment_packet(uint32_t index) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_DELETE_COMMENT;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;

        if (!send(sizeof(index), &index))
            return false;

        metin2::log::debug("Sent guild comment delete for comment {}", index);
        return send_sequence();
    }

    auto python_network_stream::send_guild_refresh_comments_packet(uint32_t highest_index) -> bool {
        static uint32_t last_time = el_timer_get_m_sec() - 1001;

        if (el_timer_get_m_sec() - last_time < 1000)
            return true;
        last_time = el_timer_get_m_sec();

        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_REFRESH_COMMENT;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;

        metin2::log::debug("Sent guild comment refresh from comment {}", highest_index);
        return send_sequence();
    }

    auto python_network_stream::send_guild_change_member_grade_packet(uint32_t pid, uint8_t grade) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;

        if (!send(sizeof(pid), &pid))
            return false;
        if (!send(sizeof(grade), &grade))
            return false;

        metin2::log::debug("Sent guild member {} grade change to {}", pid, grade);
        return send_sequence();
    }

    auto python_network_stream::send_guild_use_skill_packet(uint32_t skill_id, uint32_t target_vid) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_USE_SKILL;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;

        if (!send(sizeof(skill_id), &skill_id))
            return false;
        if (!send(sizeof(target_vid), &target_vid))
            return false;

        metin2::log::debug("Sent guild skill {} use on vid {}", skill_id, target_vid);
        return send_sequence();
    }

    auto python_network_stream::send_guild_change_member_general_packet(uint32_t pid, uint8_t flag) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;

        if (!send(sizeof(pid), &pid))
            return false;
        if (!send(sizeof(flag), &flag))
            return false;

        metin2::log::debug("Sent guild member {} general flag {}", pid, flag);
        return send_sequence();
    }

    auto python_network_stream::send_guild_invite_answer_packet(uint32_t guild_id, uint8_t answer) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;

        if (!send(sizeof(guild_id), &guild_id))
            return false;
        if (!send(sizeof(answer), &answer))
            return false;

        metin2::log::debug("Sent guild {} invite answer {}", guild_id, answer);
        return send_sequence();
    }

    auto python_network_stream::send_guild_charge_gsp_packet(uint32_t money) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_CHARGE_GSP;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;

        if (!send(sizeof(money), &money))
            return false;

        metin2::log::debug("Sent guild skill point charge of {} gold", money);
        return send_sequence();
    }

    auto python_network_stream::send_guild_deposit_money_packet(uint32_t money) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_DEPOSIT_MONEY;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;
        if (!send(sizeof(money), &money))
            return false;

        metin2::log::debug("Sent guild deposit of {} gold", money);
        return send_sequence();
    }

    auto python_network_stream::send_guild_withdraw_money_packet(uint32_t money) -> bool {
        packet_cg_guild guild_packet;
        guild_packet.header_value = HEADER_CG_GUILD;
        guild_packet.sub_header = GUILD_SUBHEADER_CG_WITHDRAW_MONEY;
        if (!send(sizeof(guild_packet), &guild_packet))
            return false;
        if (!send(sizeof(money), &money))
            return false;

        metin2::log::debug("Sent guild withdrawal of {} gold", money);
        return send_sequence();
    }

    auto python_network_stream::recv_guild() -> bool {
        packet_gc_guild guild_packet;
        if (!recv(sizeof(guild_packet), &guild_packet))
            return false;

        switch (guild_packet.subheader) {
        case GUILD_SUBHEADER_GC_LOGIN:
            {
                uint32_t pid;
                if (!recv(sizeof(uint32_t), &pid))
                    return false;

                python_guild::guild_member_data *p_guild_member_data;
                if (get_python_guild().member_data_ptr_by_pid(pid, &p_guild_member_data))
                    if (0 != p_guild_member_data->name_value.compare(get_python_player().get_name()))
                        get_python_messenger().login_guild_member(p_guild_member_data->name_value.c_str());

                break;
            }
        case GUILD_SUBHEADER_GC_LOGOUT:
            {
                uint32_t pid;
                if (!recv(sizeof(uint32_t), &pid))
                    return false;

                python_guild::guild_member_data *guild_member_data;
                if (get_python_guild().member_data_ptr_by_pid(pid, &guild_member_data))
                    if (0 != guild_member_data->name_value.compare(get_python_player().get_name()))
                        get_python_messenger().logout_guild_member(guild_member_data->name_value.c_str());

                break;
            }
        case GUILD_SUBHEADER_GC_REMOVE:
            {
                uint32_t pid;
                if (!recv(sizeof(pid), &pid))
                    return false;

                if (get_python_guild().is_main_player(pid)) {
                    get_python_guild().destroy();
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "delete_guild", Py_BuildValue("()"));
                    get_python_messenger().remove_all_guild_member();
                    set_guild_id(0);
                    refresh_messenger_window();
                    refresh_target_board();
                    refresh_character_window();
                } else {
                    std::string member_name = "";
                    python_guild::guild_member_data *p_data;
                    if (get_python_guild().member_data_ptr_by_pid(pid, &p_data)) {
                        member_name = p_data->name_value;
                        get_python_messenger().remove_guild_member(p_data->name_value.c_str());
                    }

                    get_python_guild().remove_member(pid);

                    refresh_target_board_by_name(member_name.c_str());
                    refresh_guild_window_member_page();
                }

                metin2::log::debug("Guild member {} removed", pid);
                break;
            }
        case GUILD_SUBHEADER_GC_LIST:
            {
                while (packet_bytes_left() >= static_cast<int>(sizeof(packet_gc_guild_sub_member))) {
                    packet_gc_guild_sub_member member_packet;
                    if (!recv(sizeof(member_packet), &member_packet))
                        return false;

                    char name[CHARACTER_NAME_MAX_LEN + 1] = "";
                    if (member_packet.name_flag) {
                        if (!recv(sizeof(name), &name))
                            return false;
                        name[CHARACTER_NAME_MAX_LEN] = '\0';
                    } else {
                        python_guild::guild_member_data *member_data;
                        if (get_python_guild().member_data_ptr_by_pid(member_packet.pid, &member_data)) {
                            utf8::copy_field(name, member_data->name_value.c_str());
                        }
                    }

                    python_guild::guild_member_data guild_member_data_2;
                    guild_member_data_2.pid = member_packet.pid;
                    guild_member_data_2.grade = member_packet.grade;
                    guild_member_data_2.name_value = name;
                    guild_member_data_2.job = member_packet.job;
                    guild_member_data_2.level_value = member_packet.level_value;
                    guild_member_data_2.offer = member_packet.offer;
                    guild_member_data_2.general_flag = member_packet.is_general;
                    get_python_guild().register_member(guild_member_data_2);

                    if (strcmp(name, get_python_player().get_name()))
                        get_python_messenger().append_guild_member(name);

                    refresh_target_board_by_name(name);
                }

                refresh_guild_window_info_page();
                refresh_guild_window_member_page();
                refresh_messenger_window();
                refresh_character_window();
                break;
            }
        case GUILD_SUBHEADER_GC_GRADE:
            {
                uint8_t count;
                if (!recv(sizeof(count), &count))
                    return false;

                for (uint8_t i = 0; i < count; ++i) {
                    uint8_t index;
                    if (!recv(sizeof(index), &index))
                        return false;
                    packet_gc_guild_sub_grade grade_packet;
                    if (!recv(sizeof(grade_packet), &grade_packet))
                        return false;

                    auto data = python_guild::guild_grade_data(grade_packet.auth_flag, packet_string(grade_packet.grade_name).c_str());
                    get_python_guild().set_grade_data(index, data);
                }
                refresh_guild_window_grade_page();
                refresh_guild_window_member_page_grade_combo_box();
                break;
            }
        case GUILD_SUBHEADER_GC_GRADE_NAME:
            {
                uint8_t grade_number;
                if (!recv(sizeof(grade_number), &grade_number))
                    return false;

                char grade_name[GUILD_GRADE_NAME_MAX_LEN + 1] = "";
                if (!recv(sizeof(grade_name), &grade_name))
                    return false;
                grade_name[GUILD_GRADE_NAME_MAX_LEN] = '\0';

                get_python_guild().set_grade_name(grade_number, grade_name);
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_guild_grade", Py_BuildValue("()"));

                metin2::log::debug("Guild grade {} renamed to {}", grade_number, grade_name);
                refresh_guild_window_grade_page();
                refresh_guild_window_member_page_grade_combo_box();
                break;
            }
        case GUILD_SUBHEADER_GC_GRADE_AUTH:
            {
                uint8_t grade_number;
                if (!recv(sizeof(grade_number), &grade_number))
                    return false;
                uint8_t authority_flag;
                if (!recv(sizeof(authority_flag), &authority_flag))
                    return false;

                get_python_guild().set_grade_authority(grade_number, authority_flag);
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_guild_grade", Py_BuildValue("()"));

                metin2::log::debug("Guild grade {} authority changed to {}", grade_number, authority_flag);
                refresh_guild_window_grade_page();
                break;
            }
        case GUILD_SUBHEADER_GC_INFO:
            {
                packet_gc_guild_info guild_info;
                if (!recv(sizeof(guild_info), &guild_info))
                    return false;

                get_python_guild().enable_guild();
                python_guild::guild_info &r_guild_info = get_python_guild().guild_info_ref();
                utf8::copy_field(r_guild_info.guild_name, guild_info.name_value);

                r_guild_info.guild_id = guild_info.guild_id;
                r_guild_info.master_pid = guild_info.master_pid;
                r_guild_info.guild_level = guild_info.level_value;
                r_guild_info.current_experience = guild_info.exp;
                r_guild_info.current_member_count = guild_info.member_count;
                r_guild_info.max_member_count = guild_info.max_member_count;
                r_guild_info.guild_money = guild_info.gold;
                r_guild_info.has_land = guild_info.has_land;

                refresh_guild_window_info_page();
                break;
            }
        case GUILD_SUBHEADER_GC_COMMENTS:
            {
                uint8_t count;
                if (!recv(sizeof(count), &count))
                    return false;

                get_python_guild().clear_comment();

                for (uint8_t i = 0; i < count; ++i) {
                    uint32_t comment_id;
                    if (!recv(sizeof(comment_id), &comment_id))
                        return false;

                    char name[CHARACTER_NAME_MAX_LEN + 1] = "";
                    if (!recv(sizeof(name), &name))
                        return false;
                    name[CHARACTER_NAME_MAX_LEN] = '\0';

                    char comment[GULID_COMMENT_MAX_LEN + 1] = "";
                    if (!recv(sizeof(comment), &comment))
                        return false;
                    comment[GULID_COMMENT_MAX_LEN] = '\0';

                    get_python_guild().register_comment(comment_id, name, comment);
                }

                refresh_guild_window_board_page();
                break;
            }
        case GUILD_SUBHEADER_GC_CHANGE_EXP:
            {
                uint8_t level;
                if (!recv(sizeof(level), &level))
                    return false;
                uint32_t exp;
                if (!recv(sizeof(exp), &exp))
                    return false;
                get_python_guild().set_guild_exp(level, exp);
                metin2::log::debug("Guild level {}, experience {}", level, exp);
                refresh_guild_window_info_page();
                break;
            }
        case GUILD_SUBHEADER_GC_CHANGE_MEMBER_GRADE:
            {
                uint32_t pid;
                if (!recv(sizeof(pid), &pid))
                    return false;
                uint8_t grade;
                if (!recv(sizeof(grade), &grade))
                    return false;
                get_python_guild().change_guild_member_grade(pid, grade);
                metin2::log::debug("Guild member {} grade changed to {}", pid, grade);
                refresh_guild_window_member_page();
                break;
            }
        case GUILD_SUBHEADER_GC_SKILL_INFO:
            {
                python_guild::guild_skill_data &skill_data = get_python_guild().guild_skill_data_ref();
                if (!recv(sizeof(skill_data.skill_point), &skill_data.skill_point))
                    return false;
                if (!recv(sizeof(skill_data.skill_level), skill_data.skill_level))
                    return false;
                if (!recv(sizeof(skill_data.guild_point), &skill_data.guild_point))
                    return false;
                if (!recv(sizeof(skill_data.max_guild_point), &skill_data.max_guild_point))
                    return false;

                metin2::log::debug("Guild skill points {}, guild points {} of {}", skill_data.skill_point, skill_data.guild_point, skill_data.max_guild_point);
                refresh_guild_window_skill_page();
                break;
            }
        case GUILD_SUBHEADER_GC_CHANGE_MEMBER_GENERAL:
            {
                uint32_t pid;
                if (!recv(sizeof(pid), &pid))
                    return false;
                uint8_t flag;
                if (!recv(sizeof(flag), &flag))
                    return false;

                get_python_guild().change_guild_member_general_flag(pid, flag);
                metin2::log::debug("Guild member {} general flag changed to {}", pid, flag);
                refresh_guild_window_member_page();
                break;
            }
        case GUILD_SUBHEADER_GC_GUILD_INVITE:
            {
                uint32_t guild_id;
                if (!recv(sizeof(guild_id), &guild_id))
                    return false;
                char guild_name[GUILD_NAME_MAX_LEN + 1];
                if (!recv(GUILD_NAME_MAX_LEN, &guild_name))
                    return false;

                guild_name[GUILD_NAME_MAX_LEN] = 0;

                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "recv_guild_invite_question", Py_BuildValue("(is)", guild_id, guild_name));
                metin2::log::debug("Guild {} ({}) sent an invite", guild_id, guild_name);
                break;
            }
        case GUILD_SUBHEADER_GC_WAR:
            {
                packet_gc_guild_war guild_war;
                if (!recv(sizeof(guild_war), &guild_war))
                    return false;

                switch (guild_war.war_state) {
                case GUILD_WAR_SEND_DECLARE:
                    metin2::log::debug("Guild war: declaration sent");
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
                        "guild_war_on_send_declare",
                        Py_BuildValue("(i)", guild_war.guild_opp));
                    break;
                case GUILD_WAR_RECV_DECLARE:
                    metin2::log::debug("Guild war: declaration received");
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
                        "guild_war_on_recv_declare",
                        Py_BuildValue("(ii)", guild_war.guild_opp, guild_war.type_value));
                    break;
                case GUILD_WAR_ON_WAR:
                    metin2::log::debug("Guild war: guild {} is at war with guild {}", guild_war.guild_self, guild_war.guild_opp);
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
                        "guild_war_on_start",
                        Py_BuildValue("(ii)", guild_war.guild_self, guild_war.guild_opp));
                    get_python_guild().start_guild_war(guild_war.guild_opp);
                    break;
                case GUILD_WAR_END:
                    metin2::log::debug("Guild war: ended");
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
                        "guild_war_on_end",
                        Py_BuildValue("(ii)", guild_war.guild_self, guild_war.guild_opp));
                    get_python_guild().end_guild_war(guild_war.guild_opp);
                    break;
                }
                break;
            }
        case GUILD_SUBHEADER_GC_GUILD_NAME:
            {
                uint32_t id;
                char guild_name[GUILD_NAME_MAX_LEN + 1];

                while (packet_bytes_left() >= static_cast<int>(sizeof(id) + GUILD_NAME_MAX_LEN)) {
                    if (!recv(sizeof(id), &id))
                        return false;

                    if (!recv(GUILD_NAME_MAX_LEN, &guild_name))
                        return false;

                    guild_name[GUILD_NAME_MAX_LEN] = 0;

                    get_python_guild().register_guild_name(id, guild_name);
                }
                break;
            }
        case GUILD_SUBHEADER_GC_GUILD_WAR_LIST:
            {
                uint32_t src_guild_id;
                uint32_t dst_guild_id;

                while (packet_bytes_left() >= static_cast<int>(sizeof(src_guild_id) + sizeof(dst_guild_id))) {
                    if (!recv(sizeof(src_guild_id), &src_guild_id))
                        return false;

                    if (!recv(sizeof(dst_guild_id), &dst_guild_id))
                        return false;

                    metin2::log::debug("Guild war list: guild {} against guild {}", src_guild_id, dst_guild_id);
                    instance_base::insert_gvg_key(src_guild_id, dst_guild_id);
                    get_python_character_manager().change_gvg(src_guild_id, dst_guild_id);
                }
                break;
            }
        case GUILD_SUBHEADER_GC_GUILD_WAR_END_LIST:
            {
                uint32_t src_guild_id;
                uint32_t dst_guild_id;

                while (packet_bytes_left() >= static_cast<int>(sizeof(src_guild_id) + sizeof(dst_guild_id))) {
                    if (!recv(sizeof(src_guild_id), &src_guild_id))
                        return false;

                    if (!recv(sizeof(dst_guild_id), &dst_guild_id))
                        return false;

                    metin2::log::debug("Guild war end list: guild {} against guild {}", src_guild_id, dst_guild_id);
                    instance_base::remove_gvg_key(src_guild_id, dst_guild_id);
                    get_python_character_manager().change_gvg(src_guild_id, dst_guild_id);
                }
                break;
            }
        case GUILD_SUBHEADER_GC_WAR_POINT:
            {
                packet_guild_war_point guild_war_point;
                if (!recv(sizeof(guild_war_point), &guild_war_point))
                    return false;

                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
                    "guild_war_on_recv_point",
                    Py_BuildValue("(iii)", guild_war_point.gain_guild_id, guild_war_point.opponent_guild_id, guild_war_point.point));
                break;
            }
        case GUILD_SUBHEADER_GC_MONEY_CHANGE:
            {
                uint32_t money;
                if (!recv(sizeof(money), &money))
                    return false;

                get_python_guild().set_guild_money(money);

                refresh_guild_window_info_page();
                metin2::log::debug("Guild gold changed to {}", money);
                break;
            }
        }

        return true;
    }

    auto python_network_stream::send_fishing_packet(int rotation) -> bool {
        uint8_t header = HEADER_CG_FISHING;
        if (!send(sizeof(header), &header))
            return false;
        uint8_t packet_rotation = rotation / 5;
        if (!send(sizeof(uint8_t), &packet_rotation))
            return false;

        return send_sequence();
    }

    auto python_network_stream::send_give_item_packet(uint32_t target_vid, item_pos item_pos, int item_count) -> bool {
        packet_cg_give_item give_item_packet;
        give_item_packet.header_value = HEADER_CG_GIVE_ITEM;
        give_item_packet.target_vid = target_vid;
        give_item_packet.item_pos_value = item_pos;
        give_item_packet.item_count = item_count;

        if (!send(sizeof(give_item_packet), &give_item_packet))
            return false;

        return send_sequence();
    }

    auto python_network_stream::recv_fishing() -> bool {
        packet_gc_fishing fishing_packet;
        if (!recv(sizeof(fishing_packet), &fishing_packet))
            return false;

        instance_base *fishing_instance = nullptr;
        if (FISHING_SUBHEADER_GC_FISH != fishing_packet.subheader) {
            fishing_instance = get_python_character_manager().instance_ptr(fishing_packet.info_value);
            if (!fishing_instance)
                return true;
        }

        switch (fishing_packet.subheader) {
        case FISHING_SUBHEADER_GC_START:
            fishing_instance->start_fishing(float(fishing_packet.dir_value) * 5.0f);
            break;
        case FISHING_SUBHEADER_GC_STOP:
            if (fishing_instance->is_fishing())
                fishing_instance->stop_fishing();
            break;
        case FISHING_SUBHEADER_GC_REACT:
            if (fishing_instance->is_fishing()) {
                fishing_instance->set_fish_emoticon();
                fishing_instance->react_fishing();
            }
            break;
        case FISHING_SUBHEADER_GC_SUCCESS:
            fishing_instance->catch_success();
            break;
        case FISHING_SUBHEADER_GC_FAIL:
            fishing_instance->catch_fail();
            if (fishing_instance == get_python_character_manager().main_instance_ptr()) {
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_fishing_failure", Py_BuildValue("()"));
            }
            break;
        case FISHING_SUBHEADER_GC_FISH:
            {
                uint32_t fish_id = fishing_packet.info_value;

                if (0 == fishing_packet.info_value) {
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_fishing_notify_unknown", Py_BuildValue("()"));
                    return true;
                }

                item_data *item_data;
                if (!get_item_manager().item_data_pointer(fish_id, &item_data))
                    return true;

                instance_base *main_instance = get_python_character_manager().main_instance_ptr();
                if (!main_instance)
                    return true;

                if (main_instance->is_fishing()) {
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_fishing_notify", Py_BuildValue("(is)", item_data::ITEM_TYPE_FISH == item_data->get_type(), item_data->get_name()));
                } else {
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "on_fishing_success", Py_BuildValue("(is)", item_data::ITEM_TYPE_FISH == item_data->get_type(), item_data->get_name()));
                }
                break;
            }
        }

        return true;
    }

    auto python_network_stream::recv_dungeon() -> bool {
        packet_gc_dungeon dungeon_packet;
        if (!recv(sizeof(dungeon_packet), &dungeon_packet))
            return false;

        switch (dungeon_packet.subheader) {
        case DUNGEON_SUBHEADER_GC_TIME_ATTACK_START:
            {
                break;
            }
        case DUNGEON_SUBHEADER_GC_DESTINATION_POSITION:
            {
                int32_t ulx, uly;
                if (!recv(sizeof(ulx), &ulx))
                    return false;
                if (!recv(sizeof(uly), &uly))
                    return false;

                get_python_player().set_dungeon_destination_position(ulx, uly);
                break;
            }
        }

        return true;
    }

    auto python_network_stream::send_build_private_shop_packet(const char *name, const std::vector<shop_item_table> &selling_item_stock) -> bool {
        packet_cg_my_shop packet;
        packet.header_value = HEADER_CG_MYSHOP;
        utf8::copy_field(packet.sign, name);
        packet.count = static_cast<unsigned char>(selling_item_stock.size());
        if (!send(sizeof(packet), &packet))
            return false;

        for (auto itor = selling_item_stock.begin(); itor < selling_item_stock.end(); ++itor) {
            const shop_item_table &item = *itor;
            if (!send(sizeof(item), &item))
                return false;
        }

        return send_sequence();
    }

    auto python_network_stream::recv_shop_sign_packet() -> bool {
        packet_gc_shop_sign p;
        if (!recv(sizeof(packet_gc_shop_sign), &p))
            return false;

        python_player &player = get_python_player();

        if (0 == strlen(p.sign)) {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
                "private_shop_disappear",
                Py_BuildValue("(i)", p.vid));

            if (player.is_main_character_index(p.vid))
                player.close_private_shop();
        } else {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
                "private_shop_appear",
                Py_BuildValue("(is)", p.vid, packet_string(p.sign).c_str()));

            if (player.is_main_character_index(p.vid))
                player.open_private_shop();
        }

        return true;
    }

    auto python_network_stream::recv_time_packet() -> bool {
        packet_gc_time time_packet;
        if (!recv(sizeof(time_packet), &time_packet))
            return false;

        abstract_application &app = get_abstract_application();
        app.set_server_time(time_packet.time);

        return true;
    }

    auto python_network_stream::recv_walk_mode_packet() -> bool {
        packet_gc_walk_mode walk_mode_packet;
        if (!recv(sizeof(walk_mode_packet), &walk_mode_packet))
            return false;

        instance_base *instance = get_python_character_manager().instance_ptr(walk_mode_packet.vid);
        if (instance) {
            if (WALKMODE_RUN == walk_mode_packet.mode_value) {
                instance->set_run_mode();
            } else {
                instance->set_walk_mode();
            }
        }

        return true;
    }

    auto python_network_stream::recv_change_skill_group_packet() -> bool {
        packet_gc_change_skill_group change_skill_group;
        if (!recv(sizeof(change_skill_group), &change_skill_group))
            return false;

        main_actor_skill_group_ = change_skill_group.skill_group;

        get_python_player().new_clear_skill_data();
        refresh_character_window();
        return true;
    }

    void python_network_stream::test_set_skill_group_fake(int index) {
        main_actor_skill_group_ = uint32_t(index);

        get_python_player().new_clear_skill_data();
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "refresh_character", Py_BuildValue("()"));
    }

    auto python_network_stream::send_refine_packet(uint8_t pos, uint8_t type) -> bool {
        packet_cg_refine refine_packet;
        refine_packet.header_value = HEADER_CG_REFINE;
        refine_packet.pos = pos;
        refine_packet.type_value = type;

        if (!send(sizeof(refine_packet), &refine_packet))
            return false;

        return send_sequence();
    }

    auto python_network_stream::send_select_item_packet(uint32_t item_pos) -> bool {
        packet_cg_script_select_item script_select_item;
        script_select_item.header_value = HEADER_CG_SCRIPT_SELECT_ITEM;
        script_select_item.selection = item_pos;

        if (!send(sizeof(script_select_item), &script_select_item))
            return false;

        return send_sequence();
    }

    auto python_network_stream::recv_refine_information_packet_new() -> bool {
        packet_gc_refine_information_new refine_info_packet;
        if (!recv(sizeof(refine_info_packet), &refine_info_packet))
            return false;

        refine_table &refine_table = refine_info_packet.refine_table_value;
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME],
            "open_refine_dialog",
            Py_BuildValue("(iiiii)",
                refine_info_packet.pos,
                refine_info_packet.refine_table_value.result_vnum,
                refine_table.cost,
                refine_table.prob,
                refine_info_packet.type_value));

        for (int i = 0; i < refine_table.material_count; ++i) {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "append_material_to_refine_dialog", Py_BuildValue("(ii)", refine_table.materials[i].vnum, refine_table.materials[i].count));
        }

        return true;
    }

    auto python_network_stream::recv_npc_list() -> bool {
        packet_gcnpc_position k_npc_position;
        if (!recv(sizeof(k_npc_position), &k_npc_position))
            return false;

        assert(int(k_npc_position.size) - sizeof(k_npc_position) == k_npc_position.count * sizeof(npc_position) && "HEADER_GC_NPC_POSITION");

        get_python_mini_map().clear_atlas_mark_info();

        for (int i = 0; i < k_npc_position.count; ++i) {
            npc_position npc_position;
            if (!recv(sizeof(npc_position), &npc_position))
                return false;

            get_python_mini_map().register_atlas_mark(npc_position.type_value, npc_position.name_value, npc_position.x, npc_position.y);
        }

        return true;
    }

    auto python_network_stream::send_client_version_packet() -> bool {
        std::string filename;

        get_excuted_file_name(filename);

        filename = file_name_helper::no_path(filename);
        file_name_helper::change_dos_path(filename);

        packet_cg_client_version2 version_packet;
        version_packet.header_value = HEADER_CG_CLIENT_VERSION2;
        utf8::copy_field(version_packet.filename, filename.c_str());
        utf8::copy_field(version_packet.timestamp, "1215955205"); // leading digits of Python's time.time()

        if (!send(sizeof(version_packet), &version_packet))
            metin2::log::debug("Cannot send the client report packet");

        return send_sequence();
    }

    auto python_network_stream::recv_affect_add_packet() -> bool {
        packet_gc_affect_add affect_add;
        if (!recv(sizeof(affect_add), &affect_add))
            return false;

        packet_affect_element &element = affect_add.elem;
        if (element.point_idx_apply_on == POINT_ENERGY) {
            get_python_player().set_status(POINT_ENERGY_END_TIME, get_python_application().server_time_stamp() + element.duration);
            refresh_status();
        }
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "add_affect", Py_BuildValue("(iiii)", element.type_value, element.point_idx_apply_on, element.apply_value, element.duration));

        return true;
    }

    auto python_network_stream::recv_affect_remove_packet() -> bool {
        packet_gc_affect_remove affect_remove;
        if (!recv(sizeof(affect_remove), &affect_remove))
            return false;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "remove_affect", Py_BuildValue("(ii)", affect_remove.type_value, affect_remove.apply_on));

        return true;
    }

    auto python_network_stream::recv_channel_packet() -> bool {
        packet_gc_channel channel_packet;
        if (!recv(sizeof(channel_packet), &channel_packet))
            return false;

        return true;
    }

    auto python_network_stream::recv_view_equip_packet() -> bool {
        packet_gc_view_equip view_equip_packet;
        if (!recv(sizeof(view_equip_packet), &view_equip_packet))
            return false;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "open_equipment_dialog", Py_BuildValue("(i)", view_equip_packet.vid));

        for (int i = 0; i < WEAR_MAX_NUM; ++i) {
            equipment_item_set &item_set = view_equip_packet.equips[i];
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "set_equipment_dialog_item", Py_BuildValue("(iiii)", view_equip_packet.vid, i, item_set.vnum, item_set.count));

            for (int j = 0; j < ITEM_SOCKET_SLOT_MAX_NUM; ++j)
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "set_equipment_dialog_socket", Py_BuildValue("(iiii)", view_equip_packet.vid, i, j, item_set.sockets[j]));

            for (int k = 0; k < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++k)
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "set_equipment_dialog_attr", Py_BuildValue("(iiiii)", view_equip_packet.vid, i, k, item_set.attr[k].type_value, item_set.attr[k].value));
        }

        return true;
    }

    auto python_network_stream::recv_land_packet() -> bool {
        packet_gc_land_list land_list;
        if (!recv(sizeof(land_list), &land_list))
            return false;

        std::vector<uint32_t> vec_guild_id;

        python_mini_map &mini_map = get_python_mini_map();
        python_background &bg = get_python_background();
        instance_base *main_instance = get_python_player().new_get_main_actor_ptr();

        mini_map.clear_guild_area();
        bg.clear_guild_area();

        int packet_size = (land_list.size - sizeof(packet_gc_land_list));
        for (; packet_size > 0; packet_size -= sizeof(land_packet_element)) {
            land_packet_element element;
            if (!recv(sizeof(land_packet_element), &element))
                return false;

            mini_map.register_guild_area(element.id,
                element.guild_id,
                element.x,
                element.y,
                element.width,
                element.height);

            if (main_instance)
                if (element.guild_id == main_instance->guild_id()) {
                    bg.register_guild_area(element.x,
                        element.y,
                        element.x + element.width,
                        element.y + element.height);
                }

            if (0 != element.guild_id)
                vec_guild_id.push_back(element.guild_id);
        }

        download_symbol(vec_guild_id);

        return true;
    }

    auto python_network_stream::recv_target_create_packet_new() -> bool {
        packet_gc_target_create_new target_create;
        if (!recv(sizeof(target_create), &target_create))
            return false;

        python_mini_map &rkpy_mini_map = get_python_mini_map();
        python_background &rkpy_bg = get_python_background();
        if (CREATE_TARGET_TYPE_LOCATION == target_create.type_value) {
            rkpy_mini_map.create_target(target_create.id, packet_string(target_create.target_name).c_str());
        } else {
            rkpy_mini_map.create_target(target_create.id, packet_string(target_create.target_name).c_str(), target_create.vid);
            rkpy_bg.create_target_effect(target_create.id, target_create.vid);
        }

        // The large map stays as it is: quests mark their targets at every login.
        return true;
    }

    auto python_network_stream::recv_target_update_packet() -> bool {
        packet_gc_target_update target_update;
        if (!recv(sizeof(target_update), &target_update))
            return false;

        python_mini_map &rkpy_mini_map = get_python_mini_map();
        rkpy_mini_map.update_target(target_update.id, target_update.x, target_update.y);

        python_background &rkpy_bg = get_python_background();
        rkpy_bg.create_target_effect(target_update.id, target_update.x, target_update.y);

        return true;
    }

    auto python_network_stream::recv_target_delete_packet() -> bool {
        packet_gc_target_delete target_delete;
        if (!recv(sizeof(target_delete), &target_delete))
            return false;

        python_mini_map &rkpy_mini_map = get_python_mini_map();
        rkpy_mini_map.delete_target(target_delete.id);

        python_background &rkpy_bg = get_python_background();
        rkpy_bg.delete_target_effect(target_delete.id);

        return true;
    }

    auto python_network_stream::recv_lover_info_packet() -> bool {
        packet_gc_lover_info lover_info;
        if (!recv(sizeof(lover_info), &lover_info))
            return false;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "lover_info", Py_BuildValue("(si)", packet_string(lover_info.name_value).c_str(), lover_info.love_point));
        return true;
    }

    auto python_network_stream::recv_love_point_update_packet() -> bool {
        packet_gc_love_point_update love_point_update;
        if (!recv(sizeof(love_point_update), &love_point_update))
            return false;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "update_love_point", Py_BuildValue("(i)", love_point_update.love_point));
        return true;
    }

    auto python_network_stream::recv_dig_motion_packet() -> bool {
        packet_gc_dig_motion dig_motion;
        if (!recv(sizeof(dig_motion), &dig_motion))
            return false;

        abstract_character_manager &chr_mgr = get_abstract_character_manager();
        instance_base *inst_main = chr_mgr.instance_ptr(dig_motion.vid);
        instance_base *inst_target = chr_mgr.instance_ptr(dig_motion.target_vid);
        if (nullptr == inst_main)
            return true;

        if (inst_target)
            inst_main->new_look_at_dest_instance(*inst_target);

        for (int i = 0; i < dig_motion.count; ++i)
            inst_main->push_once_motion(race_motion_data::NAME_DIG);

        return true;
    }

    auto python_network_stream::send_dragon_soul_refine_packet(uint8_t refine_type, item_pos *pos) -> bool {
        packet_cg_dragon_soul_refine pk;
        pk.header_value = HEADER_CG_DRAGON_SOUL_REFINE;
        pk.sub_type = refine_type;
        memcpy(pk.item_grid, pos, sizeof(item_pos) * DS_REFINE_WINDOW_MAX_NUM);
        if (!send(sizeof(pk), &pk)) {
            return false;
        }
        return true;
    }
}
