#include "account_connector.h"
#include <metin2/core/random.h>
#include <metin2/core/tea.h>
#include <metin2/core/timer.h>
#include <metin2/core/utf8.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include "packet.h"
#include "python_network_stream.h"

namespace metin2::inline client::inline protocol {

    // Sent in the login packets; the server does not use it, so it is just random.
    uint32_t encrypt_key[4];

    void account_connector::build_client_key_20050304_myevan() {
        for (uint32_t &word : encrypt_key)
            word = metin2::random::secure_u32();
    }

    void account_connector::set_handler(PyObject *handler) {
        handler_ = handler;
    }

    void account_connector::set_login_info(const char *name, const char *pwd) {
        id_ = name;
        password_ = pwd;
    }

    void account_connector::clear_login_info(void) {
        password_ = "";
    }

    auto account_connector::connect(const char *addr, int port, const char *account_addr, int account_port) -> bool {

        addr_ = addr;
        port_ = port;

        offline_state_set();

        build_client_key_20050304_myevan();

        return network_stream::connect(account_addr, account_port);
    }

    void account_connector::disconnect() {
        network_stream::disconnect();
        offline_state_set();
    }

    void account_connector::process() {
        network_stream::process();

        if (!state_process_impl()) {
            offline_state_set();
            disconnect();
        }
    }

    auto account_connector::state_process_impl() -> bool {
        switch (state_value_) {
        case STATE_HANDSHAKE:
            return handshake_state_process();
            break;
        case STATE_AUTH:
            return auth_state_process();
            break;
        }

        return true;
    }

    auto account_connector::handshake_state_process() -> bool {
        if (!analyze_packet(HEADER_GC_PHASE, sizeof(packet_gc_phase), &account_connector::auth_state_recv_phase))
            return false;

        if (!analyze_packet(HEADER_GC_HANDSHAKE, sizeof(packet_gc_handshake), &account_connector::auth_state_recv_handshake))
            return false;

        if (!analyze_packet(HEADER_GC_PING, sizeof(packet_gc_ping), &account_connector::auth_state_recv_ping))
            return false;

        if (!analyze_packet(HEADER_GC_KEY_AGREEMENT, sizeof(packet_key_agreement), &account_connector::auth_state_recv_key_agreement))
            return false;

        if (!analyze_packet(HEADER_GC_KEY_AGREEMENT_COMPLETED, sizeof(packet_key_agreement_completed), &account_connector::auth_state_recv_key_agreement_completed))
            return false;

        return true;
    }

    auto account_connector::auth_state_process() -> bool {
        if (!analyze_packet(0, sizeof(uint8_t), &account_connector::auth_state_recv_empty))
            return true;

        if (!analyze_packet(HEADER_GC_PHASE, sizeof(packet_gc_phase), &account_connector::auth_state_recv_phase))
            return false;

        if (!analyze_packet(HEADER_GC_PING, sizeof(packet_gc_ping), &account_connector::auth_state_recv_ping))
            return false;

        if (!analyze_packet(HEADER_GC_AUTH_SUCCESS, sizeof(packet_gc_auth_success), &account_connector::auth_state_recv_auth_success))
            return true;

        if (!analyze_packet(HEADER_GC_LOGIN_FAILURE, sizeof(packet_gc_auth_success), &account_connector::auth_state_recv_auth_failure))
            return true;

        if (!analyze_packet(HEADER_GC_HANDSHAKE, sizeof(packet_gc_handshake), &account_connector::auth_state_recv_handshake))
            return false;

        if (!analyze_packet(HEADER_GC_KEY_AGREEMENT, sizeof(packet_key_agreement), &account_connector::auth_state_recv_key_agreement))
            return false;

        if (!analyze_packet(HEADER_GC_KEY_AGREEMENT_COMPLETED, sizeof(packet_key_agreement_completed), &account_connector::auth_state_recv_key_agreement_completed))
            return false;

        return true;
    }

    auto account_connector::auth_state_recv_empty() -> bool {
        uint8_t empty;
        recv(sizeof(uint8_t), &empty);
        return true;
    }

    auto account_connector::auth_state_recv_phase() -> bool {
        packet_gc_phase packet_phase;
        if (!recv(sizeof(packet_phase), &packet_phase))
            return false;

        if (packet_phase.phase_value == PHASE_HANDSHAKE) {
            handshake_state_set();
        } else if (packet_phase.phase_value == PHASE_AUTH) {

            packet_cg_login3 login_packet;
            login_packet.header_value = HEADER_CG_LOGIN3;

            utf8::copy_field(login_packet.name_value, id_.c_str());
            utf8::copy_field(login_packet.pwd, password_.c_str());
            login_packet.name_value[ID_MAX_NUM] = '\0';
            login_packet.pwd[PASS_MAX_NUM] = '\0';

            // Wipe the password from memory as soon as it has been sent.
            clear_login_info();
            python_network_stream &net_stream = get_python_network_stream();
            net_stream.clear_login_info();

            password_ = "";

            for (uint32_t i = 0; i < 4; ++i)
                login_packet.client_key[i] = encrypt_key[i];

            if (!send(sizeof(login_packet), &login_packet)) {
                metin2::log::debug("Cannot send the login packet");
                return false;
            }

            if (!send_sequence()) {
                return false;
            }

            auth_state_set();
        }

        return true;
    }

    auto account_connector::auth_state_recv_handshake() -> bool {
        packet_gc_handshake packet_handshake;
        if (!recv(sizeof(packet_handshake), &packet_handshake))
            return false;

        {
            metin2::log::debug("Handshake received (time {}, delta {})", packet_handshake.time, packet_handshake.delta);

            el_timer_set_server_m_sec(packet_handshake.time + packet_handshake.delta);

            packet_handshake.time = packet_handshake.time + packet_handshake.delta + packet_handshake.delta;
            packet_handshake.delta = 0;

            metin2::log::debug("Handshake sent (time {})", packet_handshake.time);

            if (!send(sizeof(packet_handshake), &packet_handshake)) {
                metin2::log::debug("Cannot send the handshake packet");
                return false;
            }
        }

        return true;
    }

    auto account_connector::auth_state_recv_ping() -> bool {
        packet_gc_ping packet_ping;
        if (!recv(sizeof(packet_ping), &packet_ping))
            return false;

        auth_state_send_pong();

        return true;
    }

    auto account_connector::auth_state_send_pong() -> bool {
        packet_cg_pong packet_pong;
        packet_pong.header_value = HEADER_CG_PONG;
        if (!send(sizeof(packet_pong), &packet_pong))
            return false;

        if (is_security_mode())
            return send_sequence();

        return true;
    }

    auto account_connector::auth_state_recv_auth_success() -> bool {
        packet_gc_auth_success auth_success_packet;
        if (!recv(sizeof(auth_success_packet), &auth_success_packet))
            return false;

        if (!auth_success_packet.result) {
            if (handler_)
                py_call_class_member_func(handler_, "on_login_failure", Py_BuildValue("(s)", "BESAMEKEY"));
        } else {
            python_network_stream &net = get_python_network_stream();
            net.set_login_key(auth_success_packet.login_key);
            net.connect(addr_.c_str(), port_);
        }

        disconnect();
        offline_state_set();

        return true;
    }

    auto account_connector::auth_state_recv_auth_failure() -> bool {
        packet_gc_login_failure packet_failure;
        if (!recv(sizeof(packet_gc_login_failure), &packet_failure))
            return false;

        if (handler_)
            py_call_class_member_func(handler_, "on_login_failure", Py_BuildValue("(s)", packet_failure.status));

        return true;
    }

#define ROW(rows, i) ((rows >> ((4 - i - 1) * 8)) & 0x000000FF)
#define COL(cols, i) ((cols >> ((4 - i - 1) * 8)) & 0x000000FF)

    auto account_connector::auth_state_recv_key_agreement() -> bool {
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
                metin2::log::debug("Cannot send the key agreement packet");
                return false;
            }
            metin2::log::debug("Key agreement sent ({} bytes)", packet_to_send.data_length);
        } else {
            disconnect();
            return false;
        }
        return true;
    }

    auto account_connector::auth_state_recv_key_agreement_completed() -> bool {
        packet_key_agreement_completed packet;
        if (!recv(sizeof(packet), &packet)) {
            return false;
        }

        metin2::log::debug("Key agreement completed");

        activate_cipher();

        return true;
    }

    auto account_connector::analyze_packet(uint32_t u_header, uint32_t packet_size, bool (account_connector::*dispatch_packet)()) -> bool {
        uint8_t b_header;
        if (!peek(sizeof(b_header), &b_header))
            return true;

        if (b_header != u_header)
            return true;

        if (!peek(packet_size))
            return true;

        return (this->*dispatch_packet)();
    }

    auto account_connector::analyze_var_size_packet(uint32_t u_header, bool (account_connector::*dispatch_packet)(int)) -> bool {
        uint8_t b_header;
        if (!peek(sizeof(b_header), &b_header))
            return true;

        if (b_header != u_header)
            return true;

        dynamic_size_packet_header dynamic_header;

        if (!peek(sizeof(dynamic_header), &dynamic_header))
            return true;

        if (!peek(dynamic_header.size))
            return true;

        return (this->*dispatch_packet)(dynamic_header.size);
    }

    void account_connector::offline_state_set() {
        inialize();
    }

    void account_connector::handshake_state_set() {
        state_value_ = STATE_HANDSHAKE;
    }

    void account_connector::auth_state_set() {
        state_value_ = STATE_AUTH;
    }

    void account_connector::on_connect_failure() {
        if (handler_)
            py_call_class_member_func(handler_, "on_connect_failure", Py_BuildValue("()"));

        offline_state_set();
    }

    void account_connector::on_connect_success() {
        state_value_ = STATE_HANDSHAKE;
    }

    void account_connector::on_remote_disconnect() {
        offline_state_set();
    }

    void account_connector::on_disconnect() {
        offline_state_set();
    }

    void account_connector::inialize() {
        state_value_ = STATE_OFFLINE;
    }

    account_connector::account_connector() {
        handler_ = nullptr;
        addr_ = "";
        port_ = 0;

        set_login_info("", "");
        set_recv_buffer_size(1024 * 128);
        set_send_buffer_size(2048);
        inialize();
    }

    account_connector::~account_connector() {
        offline_state_set();
    }
}
