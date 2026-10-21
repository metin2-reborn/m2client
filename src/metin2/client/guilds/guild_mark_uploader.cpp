#include "guild_mark_uploader.h"
#include <metin2/client/app/test.h>
#include <metin2/client/protocol/packet.h>
#include <metin2/core/crc32.h>
#include <metin2/graphics/image_file.h>
#include <metin2/log/log.h>

#include <algorithm>

namespace metin2::inline client::inline guilds {

    guild_mark_uploader::guild_mark_uploader() {
        set_recv_buffer_size(1024);
        set_send_buffer_size(1024);
        inialize();
    }

    guild_mark_uploader::~guild_mark_uploader() {
        offline_state_set();
    }

    void guild_mark_uploader::disconnect() {
        offline_state_set();
    }

    auto guild_mark_uploader::load_impl(const char *file_name, uint32_t *pe_error) -> bool {
        bgra_image image;
        if (!load_image_file(file_name, image)) {
            *pe_error = ERROR_LOAD;
            return false;
        }

        if (image.width != guild_mark::WIDTH) {
            *pe_error = ERROR_WIDTH;
            return false;
        }

        if (image.height != guild_mark::HEIGHT) {
            *pe_error = ERROR_HEIGHT;
            return false;
        }

        std::copy(image.pixels.begin(), image.pixels.end(), mark_.apx_buf);
        return true;
    }

    // The symbol goes to the server as the file's bytes; decoding only checks its size.
    auto guild_mark_uploader::load_symbol(const char *file_name, uint32_t *pe_error) -> bool {
        bgra_image image;
        if (!load_image_file(file_name, image)) {
            *pe_error = ERROR_LOAD;
            return false;
        }
        if (image.width != 64) {
            *pe_error = ERROR_WIDTH;
            return false;
        }
        if (image.height != 128) {
            *pe_error = ERROR_HEIGHT;
            return false;
        }

        FILE *file = fopen(file_name, "rb");
        if (!file) {
            *pe_error = ERROR_LOAD;
            return false;
        }

        fseek(file, 0, SEEK_END);
        symbol_buf_size_ = ftell(file);
        fseek(file, 0, SEEK_SET);

        symbol_buf_.assign(symbol_buf_size_, 0);
        fread(symbol_buf_.data(), symbol_buf_size_, 1, file);

        fclose(file);

        symbol_crc32_ = get_file_crc32(file_name);
        return true;
    }

    auto guild_mark_uploader::connect(const network_address &net_addr, uint32_t handle, uint32_t random_key, uint32_t guild_id, const char *file_name, uint32_t *pe_error) -> bool {
        offline_state_set();
        set_recv_buffer_size(1024);
        set_send_buffer_size(1024);

        if (!network_stream::connect(net_addr)) {
            *pe_error = ERROR_CONNECT;
            return false;
        }

        send_type_ = SEND_TYPE_MARK;
        handle_ = handle;
        random_key_ = random_key;
        guild_id_ = guild_id;

        if (!load_impl(file_name, pe_error))
            return false;

        return true;
    }

    auto guild_mark_uploader::connect_to_send_symbol(const network_address &net_addr, uint32_t handle, uint32_t random_key, uint32_t guild_id, const char *file_name, uint32_t *pe_error) -> bool {
        offline_state_set();
        set_recv_buffer_size(1024);
        set_send_buffer_size(64 * 1024);

        if (!network_stream::connect(net_addr)) {
            *pe_error = ERROR_CONNECT;
            return false;
        }

        send_type_ = SEND_TYPE_SYMBOL;
        handle_ = handle;
        random_key_ = random_key;
        guild_id_ = guild_id;

        if (!load_symbol(file_name, pe_error))
            return false;

        return true;
    }

    void guild_mark_uploader::process() {
        network_stream::process();

        if (!state_process_impl()) {
            offline_state_set();
            disconnect();
        }
    }

    void guild_mark_uploader::on_connect_failure() {
        offline_state_set();
    }

    void guild_mark_uploader::on_connect_success() {
        login_state_set();
    }

    void guild_mark_uploader::on_remote_disconnect() {
        offline_state_set();
    }

    void guild_mark_uploader::on_disconnect() {
        offline_state_set();
    }

    void guild_mark_uploader::inialize() {
        state_value_ = STATE_OFFLINE;

        guild_id_ = 0;
        handle_ = 0;
        random_key_ = 0;

        symbol_buf_size_ = 0;
        symbol_buf_.clear();
    }

    auto guild_mark_uploader::state_process_impl() -> bool {
        switch (state_value_) {
        case STATE_LOGIN:
            return login_state_process();
            break;
        }

        return true;
    }

    void guild_mark_uploader::offline_state_set() {
        inialize();
    }

    void guild_mark_uploader::complete_state_set() {
        state_value_ = STATE_COMPLETE;

        offline_state_set();
    }

    void guild_mark_uploader::login_state_set() {
        state_value_ = STATE_LOGIN;
    }

    auto guild_mark_uploader::login_state_process() -> bool {
        if (!analyze_packet(HEADER_GC_PHASE, sizeof(packet_gc_phase), &guild_mark_uploader::login_state_recv_phase))
            return false;

        if (!analyze_packet(HEADER_GC_HANDSHAKE, sizeof(packet_gc_handshake), &guild_mark_uploader::login_state_recv_handshake))
            return false;

        if (!analyze_packet(HEADER_GC_PING, sizeof(packet_gc_ping), &guild_mark_uploader::login_state_recv_ping))
            return false;

        if (!analyze_packet(HEADER_GC_KEY_AGREEMENT, sizeof(packet_key_agreement), &guild_mark_uploader::login_state_recv_key_agreement))
            return false;

        if (!analyze_packet(HEADER_GC_KEY_AGREEMENT_COMPLETED, sizeof(packet_key_agreement_completed), &guild_mark_uploader::login_state_recv_key_agreement_completed))
            return false;

        return true;
    }

    auto guild_mark_uploader::send_mark_packet() -> bool {
        packet_cg_mark_upload packet_mark_upload;
        packet_mark_upload.header_value = HEADER_CG_MARK_UPLOAD;
        packet_mark_upload.gid = guild_id_;

        assert(sizeof(packet_mark_upload.image) == sizeof(mark_.apx_buf));
        memcpy(packet_mark_upload.image, mark_.apx_buf, sizeof(packet_mark_upload.image));

        if (!send(sizeof(packet_mark_upload), &packet_mark_upload))
            return false;

        return true;
    }
    auto guild_mark_uploader::send_symbol_packet() -> bool {
        if (symbol_buf_.empty())
            return false;

        packet_cg_symbol_upload packet_symbol_upload;
        packet_symbol_upload.header_value = HEADER_CG_GUILD_SYMBOL_UPLOAD;
        packet_symbol_upload.handle = guild_id_;
        packet_symbol_upload.size = sizeof(packet_cg_symbol_upload) + symbol_buf_size_;

        if (!send(sizeof(packet_cg_symbol_upload), &packet_symbol_upload))
            return false;
        if (!send(symbol_buf_size_, symbol_buf_.data()))
            return false;

        network_stream::send_internal_buffer();
        complete_state_set();

        return true;
    }

    auto guild_mark_uploader::login_state_recv_phase() -> bool {
        packet_gc_phase packet_phase;
        if (!recv(sizeof(packet_phase), &packet_phase))
            return false;

        if (packet_phase.phase_value == PHASE_LOGIN) {

            if (SEND_TYPE_MARK == send_type_) {
                if (!send_mark_packet())
                    return false;
            } else if (SEND_TYPE_SYMBOL == send_type_) {
                if (!send_symbol_packet())
                    return false;
            }
        }

        return true;
    }

    auto guild_mark_uploader::login_state_recv_handshake() -> bool {
        packet_gc_handshake packet_handshake;
        if (!recv(sizeof(packet_handshake), &packet_handshake))
            return false;

        {
            packet_cg_mark_login packet_mark_login;
            packet_mark_login.header_value = HEADER_CG_MARK_LOGIN;
            packet_mark_login.handle = handle_;
            packet_mark_login.random_key = random_key_;
            if (!send(sizeof(packet_mark_login), &packet_mark_login))
                return false;
        }

        return true;
    }

    auto guild_mark_uploader::login_state_recv_ping() -> bool {
        packet_gc_ping packet_ping;
        if (!recv(sizeof(packet_ping), &packet_ping))
            return false;

        packet_cg_pong packet_pong;
        packet_pong.header_value = HEADER_CG_PONG;

        if (!send(sizeof(packet_cg_pong), &packet_pong))
            return false;

        if (is_security_mode())
            return send_sequence();
        else
            return true;
    }

    auto guild_mark_uploader::login_state_recv_key_agreement() -> bool {
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

    auto guild_mark_uploader::login_state_recv_key_agreement_completed() -> bool {
        packet_key_agreement_completed packet;
        if (!recv(sizeof(packet), &packet)) {
            return false;
        }

        metin2::log::debug("Key agreement completed");

        activate_cipher();

        return true;
    }

    auto guild_mark_uploader::analyze_packet(uint32_t u_header, uint32_t packet_size, bool (guild_mark_uploader::*dispatch_packet)()) -> bool {
        uint8_t b_header;
        if (!peek(sizeof(b_header), &b_header))
            return true;

        if (b_header != u_header)
            return true;

        if (!peek(packet_size))
            return true;

        return (this->*dispatch_packet)();
    }
}
