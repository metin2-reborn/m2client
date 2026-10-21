#include <metin2/client/app/python_application.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include "packet.h"
#include "python_network_stream.h"

namespace metin2::inline client::inline protocol {

    void python_network_stream::hand_shake_phase() {
        packet_header header;

        if (!check_packet(&header))
            return;

        switch (header) {
        case HEADER_GC_PHASE:
            if (recv_phase_packet())
                return;
            break;

        case HEADER_GC_HANDSHAKE:
            {
                if (!recv(sizeof(packet_gc_handshake), &handshake_data_))
                    return;

                metin2::log::debug("Handshake received (time {}, delta {})", handshake_data_.time, handshake_data_.delta);

                el_timer_set_server_m_sec(handshake_data_.time + handshake_data_.delta);

                handshake_data_.time = handshake_data_.time + handshake_data_.delta + handshake_data_.delta;
                handshake_data_.delta = 0;

                metin2::log::debug("Handshake sent (time {})", handshake_data_.time);

                if (!send(sizeof(packet_gc_handshake), &handshake_data_)) {
                    assert(!"Failed Sending Handshake");
                    return;
                }

                get_timer().set_base_time();
                return;
            }
            break;
        case HEADER_GC_PING:
            recv_ping_packet();
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
        }

        recv_error_packet(header);
    }

    void python_network_stream::set_hand_shake_phase() {
        if ("HandShake" != phase_value_)
            phase_leave_func_.run();

        metin2::log::debug("Entering the handshake phase");

        phase_value_ = "HandShake";

        changing_phase_time_ = el_timer_get_m_sec();
        phase_process_func_.set(this, &python_network_stream::hand_shake_phase);
        phase_leave_func_.set(this, &python_network_stream::leave_handshake_phase);

        set_game_online();

        if (direct_enter_mode_is_set()) {
            // Direct enter mode skips the login window.
        } else {
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_LOGIN], "OnHandShake", Py_BuildValue("()"));
        }
    }

    auto python_network_stream::recv_handshake_packet() -> bool {
        packet_gc_handshake handshake_data;
        if (!recv(sizeof(packet_gc_handshake), &handshake_data))
            return false;

        metin2::log::debug("Handshake received (time {}, delta {})", handshake_data.time, handshake_data.delta);

        server_time_sync_value_.change_server_time = handshake_data.time + handshake_data.delta;
        server_time_sync_value_.change_client_time = el_timer_get_m_sec();

        handshake_data.time = handshake_data.time + handshake_data.delta + handshake_data.delta;
        handshake_data.delta = 0;

        metin2::log::debug("Handshake sent (time {})", handshake_data.time);

        handshake_data.header_value = HEADER_CG_TIME_SYNC;
        if (!send(sizeof(packet_gc_handshake), &handshake_data)) {
            assert(!"Failed Sending Handshake");
            return false;
        }

        send_sequence();

        return true;
    }

    auto python_network_stream::recv_handshake_ok_packet() -> bool {
        packet_gc_blank blank_packet;
        if (!recv(sizeof(packet_gc_blank), &blank_packet))
            return false;

        uint32_t delta = el_timer_get_m_sec() - server_time_sync_value_.change_client_time;
        el_timer_set_server_m_sec(server_time_sync_value_.change_server_time + delta);

        metin2::log::debug("Handshake completed (server time {}, delta {})", server_time_sync_value_.change_server_time, delta);

        return true;
    }

    auto python_network_stream::recv_key_agreement_packet() -> bool {
        packet_key_agreement packet;
        if (!recv(sizeof(packet), &packet)) {
            return false;
        }

        metin2::log::debug("Key agreement received ({} bytes)", packet.data_length);

        packet_key_agreement packet_to_send;
        size_t data_length = packet_key_agreement::MAX_DATA_LEN;
        size_t agreed_length = prepare(packet_to_send.data_value, &data_length);
        if (agreed_length == 0) {
            disconnect();
            return false;
        }
        assert(data_length <= packet_key_agreement::MAX_DATA_LEN);

        if (activate(packet.agreed_length, packet.data_value, packet.data_length)) {
            packet_to_send.header_value = HEADER_CG_KEY_AGREEMENT;
            packet_to_send.agreed_length = (uint16_t)agreed_length;
            packet_to_send.data_length = (uint16_t)data_length;

            if (!send(sizeof(packet_to_send), &packet_to_send)) {
                assert(!"Failed Sending KeyAgreement");
                return false;
            }
            metin2::log::debug("Key agreement sent ({} bytes)", packet_to_send.data_length);
        } else {
            disconnect();
            return false;
        }
        return true;
    }

    auto python_network_stream::recv_key_agreement_completed_packet() -> bool {
        packet_key_agreement_completed packet;
        if (!recv(sizeof(packet), &packet)) {
            return false;
        }

        metin2::log::debug("Key agreement completed");

        activate_cipher();

        return true;
    }
}
