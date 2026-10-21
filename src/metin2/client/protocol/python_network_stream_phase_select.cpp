#include <metin2/core/timer.h>
#include <metin2/core/utf8.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include "packet.h"
#include "python_network_stream.h"

namespace metin2::inline client::inline protocol {

    void python_network_stream::set_select_phase() {
        if ("Select" != phase_value_)
            phase_leave_func_.run();

        metin2::log::debug("Entering the character select phase");

        phase_value_ = "Select";

        changing_phase_time_ = el_timer_get_m_sec();
        phase_process_func_.set(this, &python_network_stream::select_phase);
        phase_leave_func_.set(this, &python_network_stream::leave_select_phase);

        if (direct_enter_mode_is_set()) {
            py_call_class_member_func(handler_, "SetLoadingPhase", Py_BuildValue("()"));
        } else {
            if (is_selected_empire())
                py_call_class_member_func(handler_, "SetSelectCharacterPhase", Py_BuildValue("()"));
            else
                py_call_class_member_func(handler_, "SetSelectEmpirePhase", Py_BuildValue("()"));
        }
    }

    void python_network_stream::select_phase() {
        packet_header header;

        if (!check_packet(&header))
            return;

        switch (header) {
        case HEADER_GC_PHASE:
            if (recv_phase_packet())
                return;
            break;

        case HEADER_GC_EMPIRE:
            if (recv_empire_packet())
                return;
            break;

        case HEADER_GC_LOGIN_SUCCESS4:
            if (recv_login_success_packet4())
                return;
            break;

        case HEADER_GC_PLAYER_CREATE_SUCCESS:
            if (recv_player_create_success_packet())
                return;
            break;

        case HEADER_GC_PLAYER_CREATE_FAILURE:
            if (recv_player_create_failure_packet())
                return;
            break;

        case HEADER_GC_PLAYER_DELETE_WRONG_SOCIAL_ID:
            if (recv_player_destroy_failure_packet())
                return;
            break;

        case HEADER_GC_PLAYER_DELETE_SUCCESS:
            if (recv_player_destroy_success_packet())
                return;
            break;

        case HEADER_GC_CHANGE_NAME:
            if (recv_change_name())
                return;
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

        case HEADER_GC_PLAYER_POINT_CHANGE:
            packet_gc_point_change point_change;
            recv(sizeof(packet_gc_point_change), &point_change);
            return;
            break;

        case HEADER_GC_PING:
            if (recv_ping_packet())
                return;
            break;
        }

        recv_error_packet(header);
    }

    auto python_network_stream::send_select_empire_packet(uint32_t empire_id) -> bool {
        packet_cg_empire packet_empire;
        packet_empire.header_value = HEADER_CG_EMPIRE;
        packet_empire.empire = empire_id;

        if (!send(sizeof(packet_empire), &packet_empire)) {
            metin2::log::debug("Cannot send the select empire packet");
            return false;
        }

        set_empire_id(empire_id);
        return send_sequence();
    }

    auto python_network_stream::send_select_character_packet(uint8_t index) -> bool {
        packet_cg_select_character select_character_packet;

        select_character_packet.header_value = HEADER_CG_PLAYER_SELECT;
        select_character_packet.player_index = index;

        if (!send(sizeof(packet_cg_select_character), &select_character_packet)) {
            metin2::log::debug("Cannot send the select character packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_destroy_character_packet(uint8_t index, const char *private_code) -> bool {
        packet_cg_destroy_character destroy_character_packet;

        destroy_character_packet.header_value = HEADER_CG_PLAYER_DESTROY;
        destroy_character_packet.index_value = index;
        utf8::copy_field(destroy_character_packet.private_code, private_code);

        if (!send(sizeof(packet_cg_destroy_character), &destroy_character_packet)) {
            metin2::log::debug("Cannot send the delete character packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_create_character_packet(uint8_t index, const char *name, uint8_t job, uint8_t shape, uint8_t con, uint8_t int_value, uint8_t str, uint8_t dex) -> bool {
        packet_cg_create_character create_character_packet;

        create_character_packet.header_value = HEADER_CG_PLAYER_CREATE;
        create_character_packet.index_value = index;
        utf8::copy_field(create_character_packet.name_value, name);
        create_character_packet.job = job;
        create_character_packet.shape_value = shape;
        create_character_packet.con = con;
        create_character_packet.int_value = int_value;
        create_character_packet.str = str;
        create_character_packet.dex = dex;

        if (!send(sizeof(packet_cg_create_character), &create_character_packet)) {
            metin2::log::debug("Cannot send the create character packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_change_name_packet(uint8_t index, const char *name) -> bool {
        packet_cg_change_name change_name_packet;
        change_name_packet.header_value = HEADER_CG_CHANGE_NAME;
        change_name_packet.index_value = index;
        utf8::copy_field(change_name_packet.name_value, name);

        if (!send(sizeof(packet_cg_change_name), &change_name_packet)) {
            metin2::log::debug("Cannot send the change name packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::recv_player_create_success_packet() -> bool {
        packet_gc_player_create_success create_success_packet;

        if (!recv(sizeof(create_success_packet), &create_success_packet))
            return false;

        if (create_success_packet.account_character_slot >= PLAYER_PER_ACCOUNT4) {
            metin2::log::warn("Created character slot {} out of range (must be below {})", create_success_packet.account_character_slot, PLAYER_PER_ACCOUNT4);
            return true;
        }

        simple_player_infos_[create_success_packet.account_character_slot] = create_success_packet.simple_player_infomation;
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_CREATE], "OnCreateSuccess", Py_BuildValue("()"));
        return true;
    }

    auto python_network_stream::recv_player_create_failure_packet() -> bool {
        packet_gc_create_failure packet;

        if (!recv(sizeof(packet_gc_create_failure), &packet))
            return false;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_CREATE], "OnCreateFailure", Py_BuildValue("(i)", packet.type_value));
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_SELECT], "OnCreateFailure", Py_BuildValue("(i)", packet.type_value));
        return true;
    }

    auto python_network_stream::recv_player_destroy_success_packet() -> bool {
        packet_gc_destroy_character_success packet;
        if (!recv(sizeof(packet_gc_destroy_character_success), &packet))
            return false;

        if (packet.account_index >= PLAYER_PER_ACCOUNT4)
            return true;

        memset(&simple_player_infos_[packet.account_index], 0, sizeof(simple_player_infos_[packet.account_index]));
        guild_ids_[packet.account_index] = 0;
        guild_name_[packet.account_index] = "";

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_SELECT], "OnDeleteSuccess", Py_BuildValue("(i)", packet.account_index));
        return true;
    }

    auto python_network_stream::recv_player_destroy_failure_packet() -> bool {
        packet_gc_blank packet_blank;
        if (!recv(sizeof(packet_gc_blank), &packet_blank))
            return false;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_SELECT], "OnDeleteFailure", Py_BuildValue("()"));
        return true;
    }

    auto python_network_stream::recv_change_name() -> bool {
        packet_gc_change_name change_name_packet;
        if (!recv(sizeof(packet_gc_change_name), &change_name_packet))
            return false;
        const std::string name = packet_string(change_name_packet.name_value);

        for (int i = 0; i < PLAYER_PER_ACCOUNT4; ++i) {
            if (change_name_packet.pid == simple_player_infos_[i].id) {
                simple_player_infos_[i].change_name = false;
                snprintf(simple_player_infos_[i].name_value, sizeof(simple_player_infos_[i].name_value), "%s", name.c_str());

                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_SELECT], "OnChangeName", Py_BuildValue("(is)", i, name.c_str()));
                return true;
            }
        }

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_SELECT], "OnCreateFailure", Py_BuildValue("(i)", 100));
        return true;
    }
}
