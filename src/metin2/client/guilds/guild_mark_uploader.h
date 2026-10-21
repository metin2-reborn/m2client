#pragma once

#include <vector>

#include <metin2/net/net_stream.h>
#include "mark_image.h"

namespace metin2::inline client::inline guilds {

    class guild_mark_uploader : public network_stream {
      public:
        enum {
            ERROR_NONE,
            ERROR_CONNECT,
            ERROR_LOAD,
            ERROR_WIDTH,
            ERROR_HEIGHT,
        };

        enum {
            SEND_TYPE_MARK,
            SEND_TYPE_SYMBOL,
        };

      public:
        guild_mark_uploader();
        ~guild_mark_uploader() override;

        void disconnect();
        auto connect(const network_address &net_addr, uint32_t handle, uint32_t random_key, uint32_t guild_id, const char *file_name, uint32_t *pe_error) -> bool;
        auto connect_to_send_symbol(const network_address &net_addr, uint32_t handle, uint32_t random_key, uint32_t guild_id, const char *file_name, uint32_t *pe_error) -> bool;

        void process();

      private:
        enum {
            STATE_OFFLINE,
            STATE_LOGIN,
            STATE_COMPLETE,
        };

      private:
        void on_connect_failure() override;
        void on_connect_success() override;
        void on_remote_disconnect() override;
        void on_disconnect() override;

        auto load_impl(const char *file_name, uint32_t *pe_error) -> bool;
        auto load_symbol(const char *file_name, uint32_t *pe_error) -> bool;

        void inialize();
        auto state_process_impl() -> bool;

        void offline_state_set();
        void complete_state_set();

        void login_state_set();
        auto login_state_process() -> bool;
        auto login_state_recv_phase() -> bool;
        auto login_state_recv_handshake() -> bool;
        auto login_state_recv_ping() -> bool;
        auto login_state_recv_key_agreement() -> bool;
        auto login_state_recv_key_agreement_completed() -> bool;

        auto analyze_packet(uint32_t header, uint32_t packet_size, bool (guild_mark_uploader::*dispatch_packet)()) -> bool;

        auto send_mark_packet() -> bool;
        auto send_symbol_packet() -> bool;

      private:
        uint32_t state_value_;

        uint32_t send_type_;
        uint32_t handle_;
        uint32_t random_key_;
        uint32_t guild_id_;

        guild_mark mark_;

        uint32_t symbol_buf_size_;
        uint32_t symbol_crc32_;
        std::vector<uint8_t> symbol_buf_;
    };

    inline auto get_guild_mark_uploader() -> guild_mark_uploader & {
        static guild_mark_uploader instance;
        return instance;
    }
}
