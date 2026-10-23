#include <metin2/client/app/test.h>
#include <metin2/core/timer.h>
#include <metin2/core/utf8.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include "account_connector.h"
#include "packet.h"
#include "python_network_stream.h"

namespace metin2::inline client::inline protocol {

    void python_network_stream::login_phase() {
        packet_header header;
        if (!check_packet(&header))
            return;

        switch (header) {
        case HEADER_GC_PHASE:
            if (recv_phase_packet())
                return;
            break;

        case HEADER_GC_LOGIN_SUCCESS4:
            if (recv_login_success_packet4())
                return;
            break;

        case HEADER_GC_LOGIN_FAILURE:
            if (recv_login_failure_packet())
                return;
            break;

        case HEADER_GC_EMPIRE:
            if (recv_empire_packet())
                return;
            break;

        case HEADER_GC_PING:
            if (recv_ping_packet())
                return;
            break;

        default:
            if (recv_default_packet(header))
                return;
            break;
        }

        recv_error_packet(header);
    }

    void python_network_stream::set_login_phase() {

        if ("Login" != phase_value_)
            phase_leave_func_.run();

        metin2::log::debug("## Network - Login Phase ##");

        phase_value_ = "Login";

        phase_process_func_.set(this, &python_network_stream::login_phase);
        phase_leave_func_.set(this, &python_network_stream::leave_login_phase);

        changing_phase_time_ = el_timer_get_m_sec();

        if (direct_enter_mode_is_set()) {
            if (0 != login_key_)
                send_login_packet_new(id_.c_str(), password_.c_str());
            else
                send_login_packet(id_.c_str(), password_.c_str());

            // Wipe the password from memory as soon as it has been sent.
            clear_login_info();
            account_connector &account_connector = get_account_connector();
            account_connector.clear_login_info();
        } else {
            if (0 != login_key_)
                send_login_packet_new(id_.c_str(), password_.c_str());
            else
                send_login_packet(id_.c_str(), password_.c_str());

            // Wipe the password from memory as soon as it has been sent.
            clear_login_info();
            account_connector &account_connector = get_account_connector();
            account_connector.clear_login_info();

            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_LOGIN], "on_login_start", Py_BuildValue("()"));

            clear_select_character_data();
        }
    }

    auto python_network_stream::recv_empire_packet() -> bool {
        packet_gc_empire packet_empire;
        if (!recv(sizeof(packet_empire), &packet_empire))
            return false;

        empire_id_ = packet_empire.empire;
        return true;
    }

    auto python_network_stream::recv_login_success_packet4() -> bool {
        packet_gc_login_success4 packet_login_success;

        if (!recv(sizeof(packet_login_success), &packet_login_success))
            return false;

        for (int i = 0; i < PLAYER_PER_ACCOUNT4; ++i) {
            simple_player_infos_[i] = packet_login_success.simple_player_information_value[i];
            simple_player_infos_[i].name_value[sizeof(simple_player_infos_[i].name_value) - 1] = '\0';
            guild_ids_[i] = packet_login_success.guild_id[i];
            guild_name_[i] = packet_string(packet_login_success.guild_name[i]);
        }

        mark_auth_value_.handle = packet_login_success.handle;
        mark_auth_value_.random_key = packet_login_success.random_key;

        if (direct_enter_mode_is_set()) {
        } else {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_SELECT], "refresh", Py_BuildValue("()"));
        }

        return true;
    }

    void python_network_stream::on_connect_failure() {
        if (direct_enter_mode_is_set()) {
            close_phase();
        } else {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_LOGIN], "on_connect_failure", Py_BuildValue("()"));
        }
    }

    auto python_network_stream::recv_login_failure_packet() -> bool {
        packet_gc_login_failure packet_failure;
        if (!recv(sizeof(packet_gc_login_failure), &packet_failure))
            return false;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_LOGIN], "on_login_failure", Py_BuildValue("(s)", packet_string(packet_failure.status).c_str()));
        return true;
    }

    auto python_network_stream::send_login_packet(const char *name, const char *password) -> bool {
        packet_cg_login login_packet;
        login_packet.header_value = HEADER_CG_LOGIN;

        utf8::copy_field(login_packet.name_value, name);
        utf8::copy_field(login_packet.pwd, password);

        login_packet.name_value[ID_MAX_NUM] = '\0';
        login_packet.pwd[PASS_MAX_NUM] = '\0';

        if (!send(sizeof(login_packet), &login_packet)) {
            metin2::log::debug("Cannot send the login packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_login_packet_new(const char *name, const char *password) -> bool {
        packet_cg_login2 login_packet;
        login_packet.header_value = HEADER_CG_LOGIN2;
        login_packet.login_key = login_key_;

        utf8::copy_field(login_packet.name_value, name);

        extern uint32_t encrypt_key[4];
        for (uint32_t i = 0; i < 4; ++i)
            login_packet.client_key[i] = encrypt_key[i];

        if (!send(sizeof(login_packet), &login_packet)) {
            metin2::log::debug("Cannot send the login packet");
            return false;
        }

        if (!send_sequence()) {
            metin2::log::debug("Cannot send the login packet");
            return false;
        }

        send_internal_buffer();

        return true;
    }

#define ROW(rows, i) ((rows >> ((4 - i - 1) * 8)) & 0x000000FF)
#define COL(cols, i) ((cols >> ((4 - i - 1) * 8)) & 0x000000FF)
}
