#pragma once

#include <metin2/core/func_object.h>
#include <metin2/net/net_stream.h>
#include <metin2/script/python_header.h>

namespace metin2::inline client::inline protocol {

    class account_connector : public network_stream {
      public:
        enum {
            STATE_OFFLINE,
            STATE_HANDSHAKE,
            STATE_AUTH,
        };

      public:
        account_connector();
        ~account_connector() override;

        void set_handler(PyObject *handler);
        void set_login_info(const char *name, const char *pwd);
        void clear_login_info(void);

        auto connect(const char *addr, int port, const char *account_addr, int account_port) -> bool;
        void disconnect();
        void process();

      protected:
        void on_connect_failure() override;
        void on_connect_success() override;
        void on_remote_disconnect() override;
        void on_disconnect() override;

      protected:
        void inialize();
        auto state_process_impl() -> bool;

        void offline_state_set();
        void handshake_state_set();
        void auth_state_set();

        auto handshake_state_process() -> bool;
        auto auth_state_process() -> bool;

        auto auth_state_recv_empty() -> bool;
        auto auth_state_recv_phase() -> bool;
        auto auth_state_recv_handshake() -> bool;
        auto auth_state_recv_ping() -> bool;
        auto auth_state_send_pong() -> bool;
        auto auth_state_recv_auth_success() -> bool;
        auto auth_state_recv_auth_failure() -> bool;
        auto auth_state_recv_key_agreement() -> bool;
        auto auth_state_recv_key_agreement_completed() -> bool;

        auto analyze_packet(uint32_t header, uint32_t packet_size, bool (account_connector::*dispatch_packet)()) -> bool;
        // TODO: assumes a 4-byte size always follows the header; a proper fix needs a reworked
        // packet layer.
        auto analyze_var_size_packet(uint32_t header, bool (account_connector::*dispatch_packet)(int)) -> bool;

      protected:
        uint32_t state_value_;
        std::string id_;
        std::string password_;

        std::string addr_;
        int port_;
        bool is_wait_key_;

        PyObject *handler_;

        void build_client_key_20050304_myevan();
    };

    inline auto get_account_connector() -> account_connector & {
        static account_connector instance;
        return instance;
    }
}
