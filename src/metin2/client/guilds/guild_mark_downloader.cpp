#include "guild_mark_downloader.h"
#include <metin2/client/app/test.h>
#include <metin2/client/characters/python_character_manager.h>
#include <metin2/client/protocol/packet.h>
#include <metin2/core/utils.h>
#include <metin2/files/eter_mark_manager.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/log/log.h>

#include <vector>

namespace metin2::inline client::inline guilds {

    struct mark_index {
        uint16_t guild_id;
        uint16_t mark_id;
    };

    guild_mark_downloader::guild_mark_downloader() {
        set_recv_buffer_size(640 * 1024);
        set_send_buffer_size(1024);
        initialize_impl();
    }

    guild_mark_downloader::~guild_mark_downloader() {
        offline_state_set();
    }

    auto guild_mark_downloader::connect(const network_address &net_addr, uint32_t handle, uint32_t random_key) -> bool {
        offline_state_set();

        handle_ = handle;
        random_key_ = random_key;
        todo_ = TODO_RECV_MARK;
        return network_stream::connect(net_addr);
    }

    auto guild_mark_downloader::connect_to_recv_symbol(const network_address &net_addr, uint32_t handle, uint32_t random_key, const std::vector<uint32_t> &vec_guild_id) -> bool {
        offline_state_set();

        handle_ = handle;
        random_key_ = random_key;
        todo_ = TODO_RECV_SYMBOL;
        vec_guild_id_ = vec_guild_id;
        return network_stream::connect(net_addr);
    }

    void guild_mark_downloader::process() {
        network_stream::process();

        if (!state_process_impl()) {
            offline_state_set();
            disconnect();
        }
    }

    void guild_mark_downloader::on_connect_failure() {
        offline_state_set();
    }

    void guild_mark_downloader::on_connect_success() {
        login_state_set();
    }

    void guild_mark_downloader::on_remote_disconnect() {
        offline_state_set();
    }

    void guild_mark_downloader::on_disconnect() {
        offline_state_set();
    }

    void guild_mark_downloader::initialize_impl() {
        state_value_ = STATE_OFFLINE;
        mark_mgr_ = nullptr;
        current_requesting_image_index_ = 0;
        block_index_ = 0;
        block_data_pos_ = 0;
        block_data_size_ = 0;

        handle_ = 0;
        random_key_ = 0;
        todo_ = TODO_RECV_NONE;
        vec_guild_id_.clear();
    }

    auto guild_mark_downloader::state_process_impl() -> bool {
        switch (state_value_) {
        case STATE_LOGIN:
            return login_state_process();
            break;
        case STATE_COMPLETE:
            return false;
        }

        return true;
    }

    void guild_mark_downloader::offline_state_set() {
        initialize_impl();
    }

    void guild_mark_downloader::complete_state_set() {
        state_value_ = STATE_COMPLETE;
        get_python_character_manager().refresh_all_guild_mark();
    }

    void guild_mark_downloader::login_state_set() {
        state_value_ = STATE_LOGIN;
    }

    auto guild_mark_downloader::login_state_process() -> bool {
        uint8_t header;

        if (!peek(sizeof(uint8_t), &header))
            return true;

        if (is_security_mode()) {
            if (0 == header) {
                if (!recv(sizeof(header), &header))
                    return false;

                return true;
            }
        }

        uint32_t need_packet_size = packet_size(header);

        if (!need_packet_size)
            return false;

        if (!peek(need_packet_size))
            return true;

        dispatch_packet(header);
        return true;
    }

    auto guild_mark_downloader::packet_size(uint32_t header) -> uint32_t {
        switch (header) {
        case HEADER_GC_PHASE:
            return sizeof(packet_gc_phase);
        case HEADER_GC_HANDSHAKE:
            return sizeof(packet_gc_handshake);
        case HEADER_GC_PING:
            return sizeof(packet_gc_ping);
        case HEADER_GC_MARK_IDXLIST:
            return sizeof(packet_gc_mark_idx_list);
        case HEADER_GC_MARK_BLOCK:
            return sizeof(packet_gc_mark_block);
        case HEADER_GC_GUILD_SYMBOL_DATA:
            return sizeof(packet_gc_guild_symbol_data);
        case HEADER_GC_KEY_AGREEMENT:
            return sizeof(packet_key_agreement);
        case HEADER_GC_KEY_AGREEMENT_COMPLETED:
            return sizeof(packet_key_agreement_completed);
        }
        return 0;
    }

    auto guild_mark_downloader::dispatch_packet(uint32_t header) -> bool {
        switch (header) {
        case HEADER_GC_PHASE:
            return login_state_recv_phase();
        case HEADER_GC_HANDSHAKE:
            return login_state_recv_handshake();
        case HEADER_GC_PING:
            return login_state_recv_ping();
        case HEADER_GC_MARK_IDXLIST:
            return login_state_recv_mark_index();
        case HEADER_GC_MARK_BLOCK:
            return login_state_recv_mark_block();
        case HEADER_GC_GUILD_SYMBOL_DATA:
            return login_state_recv_symbol_data();
        case HEADER_GC_KEY_AGREEMENT:
            return login_state_recv_key_agreement();
        case HEADER_GC_KEY_AGREEMENT_COMPLETED:
            return login_state_recv_key_agreement_completed();
        }
        return false;
    }

    auto guild_mark_downloader::login_state_recv_handshake() -> bool {
        packet_gc_handshake packet_handshake;
        if (!recv(sizeof(packet_handshake), &packet_handshake))
            return false;

        packet_cg_mark_login packet_mark_login;

        packet_mark_login.header_value = HEADER_CG_MARK_LOGIN;
        packet_mark_login.handle = handle_;
        packet_mark_login.random_key = random_key_;

        if (!send(sizeof(packet_mark_login), &packet_mark_login))
            return false;

        return true;
    }

    auto guild_mark_downloader::login_state_recv_ping() -> bool {
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

    auto guild_mark_downloader::login_state_recv_phase() -> bool {
        packet_gc_phase packet_phase;

        if (!recv(sizeof(packet_phase), &packet_phase))
            return false;

        if (packet_phase.phase_value == PHASE_LOGIN) {

            switch (todo_) {
            case TODO_RECV_NONE:
                {
                    assert(!"CGuildMarkDownloader::__LoginState_RecvPhase - Todo type is none");
                    break;
                }
            case TODO_RECV_MARK:
                {
                    if (!send_mark_idx_list())
                        return false;
                    break;
                }
            case TODO_RECV_SYMBOL:
                {
                    if (!send_symbol_crc_list())
                        return false;
                    break;
                }
            }
        }

        return true;
    }

    auto guild_mark_downloader::send_mark_idx_list() -> bool {
        packet_cg_mark_idx_list packet_mark_idx_list;
        packet_mark_idx_list.header_value = HEADER_CG_MARK_IDXLIST;
        if (!send(sizeof(packet_mark_idx_list), &packet_mark_idx_list))
            return false;

        return true;
    }

    auto guild_mark_downloader::login_state_recv_mark_index() -> bool {
        packet_gc_mark_idx_list packet_mark_index;

        if (!peek(sizeof(packet_mark_index), &packet_mark_index))
            return false;

        if (!peek(packet_mark_index.buf_size))
            return false;

        recv(sizeof(packet_mark_index));

        uint16_t guild_id, mark_id;

        for (uint32_t i = 0; i < packet_mark_index.count; ++i) {
            recv(sizeof(uint16_t), &guild_id);
            recv(sizeof(uint16_t), &mark_id);

            get_guild_mark_manager().add_mark_id_by_guild_id(guild_id, mark_id);
        }

        // Load every mark image file, creating the missing ones.
        get_guild_mark_manager().load_mark_images();

        current_requesting_image_index_ = 0;
        send_mark_crc_list();
        return true;
    }

    auto guild_mark_downloader::send_mark_crc_list() -> bool {
        packet_cg_mark_crc_list packet_mark_crc_list;

        if (!get_guild_mark_manager().block_crc_list(current_requesting_image_index_, packet_mark_crc_list.crclist))
            complete_state_set();
        else {
            packet_mark_crc_list.header_value = HEADER_CG_MARK_CRCLIST;
            packet_mark_crc_list.img_idx = current_requesting_image_index_;
            ++current_requesting_image_index_;

            if (!send(sizeof(packet_mark_crc_list), &packet_mark_crc_list))
                return false;
        }
        return true;
    }

    auto guild_mark_downloader::login_state_recv_mark_block() -> bool {
        packet_gc_mark_block packet;

        if (!peek(sizeof(packet), &packet))
            return false;

        if (!peek(packet.buf_size))
            return false;

        recv(sizeof(packet));

        uint8_t pos_block;
        uint32_t comp_size;
        char comp_buf[guild_mark_block::MAX_COMP_SIZE];

        for (uint32_t i = 0; i < packet.count; ++i) {
            recv(sizeof(uint8_t), &pos_block);
            recv(sizeof(uint32_t), &comp_size);

            if (comp_size > guild_mark_block::MAX_COMP_SIZE) {
                metin2::log::warn("Guild mark block data is corrupted and is skipped");
                recv(comp_size);
            } else {
                recv(comp_size, comp_buf);
                // Store the compressed block; the CRC and the other data are built from it.
                get_guild_mark_manager().save_block_from_compressed_data(packet.img_idx, pos_block, (const uint8_t *)comp_buf, comp_size);
            }
        }

        if (packet.count > 0) {
            get_guild_mark_manager().save_mark_image(packet.img_idx);

            // Reload the image resource so the player's own mark shows without reconnecting.
            std::string image_path_value;

            if (get_guild_mark_manager().mark_image_filename(packet.img_idx, image_path_value)) {
                resource *resource = get_resource_manager().resource_pointer(image_path_value.c_str());
                if (resource->is_type(graphic_image::type())) {
                    auto *grp_img = static_cast<graphic_image *>(resource);
                    grp_img->reload();
                }
            }
        }

        if (current_requesting_image_index_ < get_guild_mark_manager().mark_image_count())
            send_mark_crc_list();
        else
            complete_state_set();

        return true;
    }

    auto guild_mark_downloader::login_state_recv_key_agreement() -> bool {
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

    auto guild_mark_downloader::login_state_recv_key_agreement_completed() -> bool {
        packet_key_agreement_completed packet;
        if (!recv(sizeof(packet), &packet)) {
            return false;
        }

        metin2::log::debug("Key agreement completed");

        activate_cipher();

        return true;
    }

    auto guild_mark_downloader::send_symbol_crc_list() -> bool {
        for (uint32_t i = 0; i < vec_guild_id_.size(); ++i) {
            packet_cg_symbol_crc symbol_crc_packet;
            symbol_crc_packet.header_value = HEADER_CG_GUILD_SYMBOL_CRC;
            symbol_crc_packet.guild_id = vec_guild_id_[i];

            std::string file_name = get_mark_manager().path(get_guild_symbol_file_name(vec_guild_id_[i]));
            symbol_crc_packet.crc = get_file_crc32(file_name.c_str());
            symbol_crc_packet.size = get_file_size(file_name.c_str());
            if (!send(sizeof(symbol_crc_packet), &symbol_crc_packet))
                return false;
        }

        return true;
    }

    auto guild_mark_downloader::login_state_recv_symbol_data() -> bool {
        packet_gc_blank_dynamic packet;
        if (!peek(sizeof(packet_gc_blank_dynamic), &packet))
            return true;

        if (packet.size > recv_buffer_size())
            return true;

        packet_gc_guild_symbol_data packet_symbol_data;
        if (!recv(sizeof(packet_symbol_data), &packet_symbol_data))
            return false;

        uint16_t data_size = packet_symbol_data.size - sizeof(packet_symbol_data);
        uint32_t guild_id = packet_symbol_data.guild_id;
        std::vector<uint8_t> data(data_size);
        if (!recv(data_size, data.data()))
            return false;

        std::string file_name = get_mark_manager().path(get_guild_symbol_file_name(guild_id));

        FILE *file = fopen(file_name.c_str(), "wb");
        if (!file)
            return false;
        fwrite(data.data(), data_size, 1, file);
        fclose(file);
        return true;
    }
}
