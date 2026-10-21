#pragma once

#include <metin2/net/net_stream.h>
#include "mark_manager.h"

namespace metin2::inline client::inline guilds {

    class guild_mark_downloader : public network_stream {
      public:
        guild_mark_downloader();
        ~guild_mark_downloader() override;

        auto connect(const network_address &net_addr, uint32_t handle, uint32_t random_key) -> bool;
        auto connect_to_recv_symbol(const network_address &net_addr, uint32_t handle, uint32_t random_key, const std::vector<uint32_t> &vec_guild_id) -> bool;

        void process();

      private:
        enum {
            STATE_OFFLINE,
            STATE_LOGIN,
            STATE_COMPLETE,
        };

        enum {
            TODO_RECV_NONE,
            TODO_RECV_MARK,
            TODO_RECV_SYMBOL,
        };

      private:
        void on_connect_failure() override;
        void on_connect_success() override;
        void on_remote_disconnect() override;
        void on_disconnect() override;

        void initialize_impl();
        auto state_process_impl() -> bool;

        auto packet_size(uint32_t header) -> uint32_t;
        auto dispatch_packet(uint32_t header) -> bool;

        void offline_state_set();
        void complete_state_set();

        void login_state_set();
        auto login_state_process() -> bool;
        auto login_state_recv_phase() -> bool;
        auto login_state_recv_handshake() -> bool;
        auto login_state_recv_ping() -> bool;
        auto login_state_recv_mark_index() -> bool;
        auto login_state_recv_mark_block() -> bool;
        auto login_state_recv_symbol_data() -> bool;
        auto login_state_recv_key_agreement() -> bool;
        auto login_state_recv_key_agreement_completed() -> bool;
        auto send_mark_idx_list() -> bool;
        auto send_mark_crc_list() -> bool;
        auto send_symbol_crc_list() -> bool;

      private:
        uint32_t handle_;
        uint32_t random_key_;
        uint32_t todo_;

        std::vector<uint32_t> vec_guild_id_;

        uint32_t state_value_;

        uint8_t current_requesting_image_index_;

        guild_mark_manager *mark_mgr_;

        uint32_t block_index_;
        uint32_t block_data_size_;
        uint32_t block_data_pos_;
    };

    inline auto get_guild_mark_downloader() -> guild_mark_downloader & {
        static guild_mark_downloader instance;
        return instance;
    }
}
