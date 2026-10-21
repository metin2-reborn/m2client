#pragma once

#include <metin2/core/cipher.h>
#include "net_address.h"

#include <memory>
#include <vector>

namespace metin2::inline net {

    class network_stream {
      public:
        network_stream();
        virtual ~network_stream();

        void set_recv_buffer_size(int recv_buf_size);
        void set_send_buffer_size(int send_buf_size);

        auto is_security_mode() -> bool;

        auto recv_buffer_size() -> int;
        auto recv_buffer_capacity() -> int;

        void clear();
        void clear_recv_buffer();

        void process();

        auto connect(const network_address &net_addr, int limit_sec = 3) -> bool;
        auto connect(const char *addr, int port, int limit_sec = 3) -> bool;
        auto connect(uint32_t addr, int port, int limit_sec = 3) -> bool;
        void disconnect();

        auto peek(int len) -> bool;
        auto peek(int len, char *dest_buf) -> bool;
        auto recv(int len) -> bool;
        auto recv(int len, char *dest_buf) -> bool;
        auto send(int len, const char *src_buf) -> bool;

        auto peek(int len, void *dest_buf) -> bool;
        auto recv(int len, void *dest_buf) -> bool;

        auto send(int len, const void *src_buf) -> bool;

        auto is_online() -> bool;

        void set_packet_sequence_mode(bool is_on);
        auto send_sequence() -> bool;

      protected:
        virtual void on_connect_success();
        virtual void on_connect_failure();
        virtual void on_remote_disconnect();
        virtual void on_disconnect();
        virtual auto on_process() -> bool;

        auto send_internal_buffer() -> bool;
        auto recv_internal_buffer() -> bool;

        void pop_send_buffer();

        auto send_buffer_size() -> int;

        auto prepare(void *buffer_value, size_t *length) -> size_t;
        auto activate(size_t agreed_length, const void *buffer_value, size_t length) -> bool;
        void activate_cipher();

      private:
        struct connection;

        std::vector<char> recv_buf_;
        int recv_buf_size_;
        int recv_buf_input_pos_;
        int recv_buf_output_pos_;

        std::vector<char> send_buf_;
        int send_buf_size_;
        int send_buf_input_pos_;
        int send_buf_output_pos_;
        int send_buf_encrypted_pos_;

        bool is_online_;

        cipher cipher_value_;

        // Owns the socket, resolver and timer; the pending handlers hold it too, so clear() can drop it
        // while operations are still queued.
        std::shared_ptr<connection> connection_;

        uint32_t sequence_;
        bool use_sequence_;
    };
}
