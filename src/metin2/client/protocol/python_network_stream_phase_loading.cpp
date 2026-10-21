#include <metin2/client/app/python_application.h>
#include <metin2/log/log.h>
#include "network_actor_manager.h"
#include "packet.h"
#include "python_network_stream.h"

#include <metin2/client/app/abstract_player.h>

#include <metin2/files/eter_file_manager.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline protocol {

    void python_network_stream::enable_chat_insult_filter(bool is_enable) {
        is_enable_chat_insult_filter_ = is_enable;
    }

    void python_network_stream::filter_insult_impl(char *line, uint32_t line_len) {
        insult_checker_value_.filter_insult(line, line_len);
    }

    auto python_network_stream::is_chat_insult_in(const char *msg) -> bool {
        if (is_enable_chat_insult_filter_)
            return false;

        return is_insult_in(msg);
    }

    auto python_network_stream::is_insult_in(const char *msg) -> bool {
        return insult_checker_value_.is_insult_in(msg, strlen(msg));
    }

    auto python_network_stream::load_insult_list(const char *insult_list_file_name) -> bool {
        mapped_file file;
        const void *data;
        if (!get_file_manager().get(file, insult_list_file_name, &data))
            return false;

        memory_text_file_loader mem_text_file_loader;
        mem_text_file_loader.bind(file.size(), data);

        insult_checker_value_.clear();
        for (uint32_t line_index = 0; line_index < mem_text_file_loader.line_count(); ++line_index) {
            const std::string &line = mem_text_file_loader.line_string(line_index);
            insult_checker_value_.append_insult(line);
        }
        return true;
    }

    void python_network_stream::loading_phase() {
        packet_header header;

        if (!check_packet(&header))
            return;

        switch (header) {
        case HEADER_GC_PHASE:
            if (recv_phase_packet())
                return;
            break;

        case HEADER_GC_MAIN_CHARACTER2_EMPIRE:
            if (recv_main_character2_empire())
                return;
            break;

        case HEADER_GC_MAIN_CHARACTER3_BGM:
            if (recv_main_character3_bgm())
                return;
            break;

        case HEADER_GC_MAIN_CHARACTER4_BGM_VOL:
            if (recv_main_character4_bgm_vol())
                return;
            break;

        case HEADER_GC_CHARACTER_UPDATE:
            if (recv_character_update_packet())
                return;
            break;

        case HEADER_GC_PLAYER_POINTS:
            if (recv_player_points())
                return;
            break;

        case HEADER_GC_PLAYER_POINT_CHANGE:
            if (recv_point_change())
                return;
            break;

        case HEADER_GC_ITEM_SET:
            if (recv_item_set_packet())
                return;
            break;

        case HEADER_GC_PING:
            if (recv_ping_packet())
                return;
            break;

        case HEADER_GC_QUICKSLOT_ADD:
            if (recv_quick_slot_add_packet())
                return;
            break;

        default:
            game_phase();
            return;
            break;
        }

        recv_error_packet(header);
    }

    void python_network_stream::set_loading_phase() {
        if ("Loading" != phase_value_)
            phase_leave_func_.run();

        metin2::log::debug("Entering the loading phase");

        phase_value_ = "Loading";

        changing_phase_time_ = el_timer_get_m_sec();
        phase_process_func_.set(this, &python_network_stream::loading_phase);
        phase_leave_func_.set(this, &python_network_stream::leave_loading_phase);

        python_player &player = get_python_player();
        player.clear();

        get_flying_manager().delete_all_instances();
        get_effect_manager().delete_all_instances();

        direct_enter_mode_initialize();
    }

    auto python_network_stream::recv_main_character2_empire() -> bool {
        packet_gc_main_character2_empire main_chr_packet;
        if (!recv(sizeof(main_chr_packet), &main_chr_packet))
            return false;

        main_actor_vid_ = main_chr_packet.vid;
        main_actor_race_ = main_chr_packet.race_num;
        main_actor_empire_ = main_chr_packet.empire;
        main_actor_skill_group_ = main_chr_packet.skill_group;

        rok_net_actor_mgr_->set_main_actor_vid(main_actor_vid_);

        python_player &player = get_python_player();
        player.set_name(main_chr_packet.name_value);
        player.set_main_character_index(main_actor_vid());

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", main_chr_packet.x, main_chr_packet.y));

        send_client_version_packet();
        return true;
    }

    auto python_network_stream::recv_main_character3_bgm() -> bool {
        packet_gc_main_character3_bgm main_chr_packet;
        if (!recv(sizeof(main_chr_packet), &main_chr_packet))
            return false;

        main_actor_vid_ = main_chr_packet.vid;
        main_actor_race_ = main_chr_packet.race_num;
        main_actor_empire_ = main_chr_packet.empire;
        main_actor_skill_group_ = main_chr_packet.skill_group;

        rok_net_actor_mgr_->set_main_actor_vid(main_actor_vid_);

        python_player &player = get_python_player();
        player.set_name(main_chr_packet.user_name);
        player.set_main_character_index(main_actor_vid());

        set_field_music_file_name(main_chr_packet.bgm_name);

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", main_chr_packet.x, main_chr_packet.y));

        send_client_version_packet();
        return true;
    }

    auto python_network_stream::recv_main_character4_bgm_vol() -> bool {
        packet_gc_main_character4_bgm_vol main_chr_packet;
        if (!recv(sizeof(main_chr_packet), &main_chr_packet))
            return false;

        main_actor_vid_ = main_chr_packet.vid;
        main_actor_race_ = main_chr_packet.race_num;
        main_actor_empire_ = main_chr_packet.empire;
        main_actor_skill_group_ = main_chr_packet.skill_group;

        rok_net_actor_mgr_->set_main_actor_vid(main_actor_vid_);

        python_player &player = get_python_player();
        player.set_name(main_chr_packet.user_name);
        player.set_main_character_index(main_actor_vid());

        set_field_music_file_info(main_chr_packet.bgm_name, main_chr_packet.bgm_vol);

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_LOAD], "LoadData", Py_BuildValue("(ii)", main_chr_packet.x, main_chr_packet.y));

        send_client_version_packet();
        return true;
    }

    static std::string current_field_music_file_name;
    static float current_field_music_volume = 1.0f / 5.0f * 0.1f;

    void python_network_stream::set_field_music_file_name(const char *music_name) {
        current_field_music_file_name = music_name;
    }

    void python_network_stream::set_field_music_file_info(const char *music_name, float vol) {
        current_field_music_file_name = music_name;
        current_field_music_volume = vol;
    }

    auto python_network_stream::field_music_file_name() -> const char * {
        return current_field_music_file_name.c_str();
    }

    auto python_network_stream::field_music_volume() -> float {
        return current_field_music_volume;
    }

    auto python_network_stream::recv_player_points() -> bool {
        packet_gc_points points_packet;

        if (!recv(sizeof(packet_gc_points), &points_packet))
            return false;

        for (uint32_t i = 0; i < POINT_MAX_NUM; ++i)
            get_python_player().set_status(i, points_packet.points[i]);

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "RefreshStatus", Py_BuildValue("()"));
        return true;
    }

    void python_network_stream::start_game() {
        is_start_game_ = true;
    }

    auto python_network_stream::send_enter_game() -> bool {
        packet_cg_enter_front_game enter_front_game_packet;

        enter_front_game_packet.header_value = HEADER_CG_ENTERGAME;

        if (!send(sizeof(enter_front_game_packet), &enter_front_game_packet)) {
            metin2::log::debug("Cannot send the enter game packet");
            return false;
        }

        if (!send_sequence())
            return false;

        send_internal_buffer();
        return true;
    }
}
