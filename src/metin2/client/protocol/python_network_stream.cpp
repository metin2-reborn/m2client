#include <metin2/log/log.h>
#include <metin2/net/net_packet_header_map.h>

#include "network_actor_manager.h"
#include "packet.h"
#include "python_network_stream.h"

#include <metin2/client/guilds/guild_mark_downloader.h>
#include <metin2/client/guilds/guild_mark_uploader.h>
#include <metin2/client/guilds/mark_manager.h>

#include <metin2/core/timer.h>
#include <metin2/core/utf8.h>
#include <metin2/script/python_utils.h>

#include <algorithm>
#include <cstdio>
#include <metin2/window/events.h>

namespace metin2::inline client::inline protocol {

    static uint32_t next_download_mark_time = 0;

    class main_packet_header_map : public network_packet_header_map {
      public:
        enum {
            STATIC_SIZE_PACKET = false,
            DYNAMIC_SIZE_PACKET = true,
        };

      public:
        main_packet_header_map() {
            set(HEADER_GC_EMPIRE, network_packet_header_map::packet_type(sizeof(packet_gc_empire), STATIC_SIZE_PACKET));
            set(HEADER_GC_WARP, network_packet_header_map::packet_type(sizeof(packet_gc_warp), STATIC_SIZE_PACKET));
            set(HEADER_GC_QUEST_INFO, network_packet_header_map::packet_type(sizeof(packet_gc_quest_info), DYNAMIC_SIZE_PACKET));
            set(HEADER_GC_REQUEST_MAKE_GUILD, network_packet_header_map::packet_type(sizeof(packet_gc_request_make_guild), STATIC_SIZE_PACKET));
            set(HEADER_GC_PVP, network_packet_header_map::packet_type(sizeof(packet_gcpvp), STATIC_SIZE_PACKET));
            set(HEADER_GC_DUEL_START, network_packet_header_map::packet_type(sizeof(packet_gc_duel_start), DYNAMIC_SIZE_PACKET));
            set(HEADER_GC_CHARACTER_ADD, network_packet_header_map::packet_type(sizeof(packet_gc_character_add), STATIC_SIZE_PACKET));
            set(HEADER_GC_CHAR_ADDITIONAL_INFO, network_packet_header_map::packet_type(sizeof(packet_gc_character_additional_info), STATIC_SIZE_PACKET));
            set(HEADER_GC_CHARACTER_UPDATE, network_packet_header_map::packet_type(sizeof(packet_gc_character_update), STATIC_SIZE_PACKET));
            set(HEADER_GC_CHARACTER_DEL, network_packet_header_map::packet_type(sizeof(packet_gc_character_delete), STATIC_SIZE_PACKET));
            set(HEADER_GC_CHARACTER_MOVE, network_packet_header_map::packet_type(sizeof(packet_gc_move), STATIC_SIZE_PACKET));
            set(HEADER_GC_CHAT, network_packet_header_map::packet_type(sizeof(packet_gc_chat), DYNAMIC_SIZE_PACKET));

            set(HEADER_GC_SYNC_POSITION, network_packet_header_map::packet_type(sizeof(packet_gcc2c), DYNAMIC_SIZE_PACKET));

            set(HEADER_GC_LOGIN_SUCCESS4, network_packet_header_map::packet_type(sizeof(packet_gc_login_success4), STATIC_SIZE_PACKET));
            set(HEADER_GC_LOGIN_FAILURE, network_packet_header_map::packet_type(sizeof(packet_gc_login_failure), STATIC_SIZE_PACKET));

            set(HEADER_GC_PLAYER_CREATE_SUCCESS, network_packet_header_map::packet_type(sizeof(packet_gc_player_create_success), STATIC_SIZE_PACKET));
            set(HEADER_GC_PLAYER_CREATE_FAILURE, network_packet_header_map::packet_type(sizeof(packet_gc_create_failure), STATIC_SIZE_PACKET));
            set(HEADER_GC_PLAYER_DELETE_SUCCESS, network_packet_header_map::packet_type(sizeof(packet_gc_blank), STATIC_SIZE_PACKET));
            set(HEADER_GC_PLAYER_DELETE_WRONG_SOCIAL_ID, network_packet_header_map::packet_type(sizeof(packet_gc_blank), STATIC_SIZE_PACKET));

            set(HEADER_GC_STUN, network_packet_header_map::packet_type(sizeof(packet_gc_stun), STATIC_SIZE_PACKET));
            set(HEADER_GC_DEAD, network_packet_header_map::packet_type(sizeof(packet_gc_dead), STATIC_SIZE_PACKET));

            set(HEADER_GC_MAIN_CHARACTER2_EMPIRE, network_packet_header_map::packet_type(sizeof(packet_gc_main_character2_empire), STATIC_SIZE_PACKET));
            set(HEADER_GC_MAIN_CHARACTER3_BGM, network_packet_header_map::packet_type(sizeof(packet_gc_main_character3_bgm), STATIC_SIZE_PACKET));
            set(HEADER_GC_MAIN_CHARACTER4_BGM_VOL, network_packet_header_map::packet_type(sizeof(packet_gc_main_character4_bgm_vol), STATIC_SIZE_PACKET));

            set(HEADER_GC_PLAYER_POINTS, network_packet_header_map::packet_type(sizeof(packet_gc_points), STATIC_SIZE_PACKET));
            set(HEADER_GC_PLAYER_POINT_CHANGE, network_packet_header_map::packet_type(sizeof(packet_gc_point_change), STATIC_SIZE_PACKET));

            set(HEADER_GC_ITEM_SET, network_packet_header_map::packet_type(sizeof(packet_gc_item_set), STATIC_SIZE_PACKET));
            set(HEADER_GC_ITEM_SET2, network_packet_header_map::packet_type(sizeof(packet_gc_item_set2), STATIC_SIZE_PACKET));

            set(HEADER_GC_ITEM_UPDATE, network_packet_header_map::packet_type(sizeof(packet_gc_item_update), STATIC_SIZE_PACKET));

            set(HEADER_GC_ITEM_GROUND_ADD, network_packet_header_map::packet_type(sizeof(packet_gc_item_ground_add), STATIC_SIZE_PACKET));
            set(HEADER_GC_ITEM_GROUND_DEL, network_packet_header_map::packet_type(sizeof(packet_gc_item_ground_del), STATIC_SIZE_PACKET));
            set(HEADER_GC_ITEM_OWNERSHIP, network_packet_header_map::packet_type(sizeof(packet_gc_item_ownership), STATIC_SIZE_PACKET));

            set(HEADER_GC_QUICKSLOT_ADD, network_packet_header_map::packet_type(sizeof(packet_gc_quick_slot_add), STATIC_SIZE_PACKET));
            set(HEADER_GC_QUICKSLOT_DEL, network_packet_header_map::packet_type(sizeof(packet_gc_quick_slot_del), STATIC_SIZE_PACKET));
            set(HEADER_GC_QUICKSLOT_SWAP, network_packet_header_map::packet_type(sizeof(packet_gc_quick_slot_swap), STATIC_SIZE_PACKET));

            set(HEADER_GC_WHISPER, network_packet_header_map::packet_type(sizeof(packet_gc_whisper), DYNAMIC_SIZE_PACKET));

            set(HEADER_GC_CHARACTER_POSITION, network_packet_header_map::packet_type(sizeof(packet_gc_position), STATIC_SIZE_PACKET));

            set(HEADER_GC_SHOP, network_packet_header_map::packet_type(sizeof(packet_gc_shop), DYNAMIC_SIZE_PACKET));
            set(HEADER_GC_SHOP_SIGN, network_packet_header_map::packet_type(sizeof(packet_gc_shop_sign), STATIC_SIZE_PACKET));
            set(HEADER_GC_EXCHANGE, network_packet_header_map::packet_type(sizeof(packet_gc_exchange), STATIC_SIZE_PACKET));

            set(HEADER_GC_PING, network_packet_header_map::packet_type(sizeof(packet_gc_ping), STATIC_SIZE_PACKET));

            set(HEADER_GC_SCRIPT, network_packet_header_map::packet_type(sizeof(packet_gc_script), DYNAMIC_SIZE_PACKET));
            set(HEADER_GC_QUEST_CONFIRM, network_packet_header_map::packet_type(sizeof(packet_gc_quest_confirm), STATIC_SIZE_PACKET));

            set(HEADER_GC_TARGET, network_packet_header_map::packet_type(sizeof(packet_gc_target), STATIC_SIZE_PACKET));

            set(HEADER_GC_HANDSHAKE, network_packet_header_map::packet_type(sizeof(packet_gc_handshake), STATIC_SIZE_PACKET));
            set(HEADER_GC_HANDSHAKE_OK, network_packet_header_map::packet_type(sizeof(packet_gc_blank), STATIC_SIZE_PACKET));
            set(HEADER_GC_OWNERSHIP, network_packet_header_map::packet_type(sizeof(packet_gc_ownership), STATIC_SIZE_PACKET));
            set(HEADER_GC_CREATE_FLY, network_packet_header_map::packet_type(sizeof(packet_gc_create_fly), STATIC_SIZE_PACKET));
            set(HEADER_GC_KEY_AGREEMENT, network_packet_header_map::packet_type(sizeof(packet_key_agreement), STATIC_SIZE_PACKET));
            set(HEADER_GC_KEY_AGREEMENT_COMPLETED, network_packet_header_map::packet_type(sizeof(packet_key_agreement_completed), STATIC_SIZE_PACKET));
            set(HEADER_GC_ADD_FLY_TARGETING, network_packet_header_map::packet_type(sizeof(packet_gc_fly_targeting), STATIC_SIZE_PACKET));
            set(HEADER_GC_FLY_TARGETING, network_packet_header_map::packet_type(sizeof(packet_gc_fly_targeting), STATIC_SIZE_PACKET));

            set(HEADER_GC_PHASE, network_packet_header_map::packet_type(sizeof(packet_gc_phase), STATIC_SIZE_PACKET));
            set(HEADER_GC_SKILL_LEVEL_NEW, network_packet_header_map::packet_type(sizeof(packet_gc_skill_level_new), STATIC_SIZE_PACKET));

            set(HEADER_GC_MESSENGER, network_packet_header_map::packet_type(sizeof(packet_gc_messenger), DYNAMIC_SIZE_PACKET));
            set(HEADER_GC_GUILD, network_packet_header_map::packet_type(sizeof(packet_gc_guild), DYNAMIC_SIZE_PACKET));

            set(HEADER_GC_PARTY_INVITE, network_packet_header_map::packet_type(sizeof(packet_gc_party_invite), STATIC_SIZE_PACKET));
            set(HEADER_GC_PARTY_ADD, network_packet_header_map::packet_type(sizeof(packet_gc_party_add), STATIC_SIZE_PACKET));
            set(HEADER_GC_PARTY_UPDATE, network_packet_header_map::packet_type(sizeof(packet_gc_party_update), STATIC_SIZE_PACKET));
            set(HEADER_GC_PARTY_REMOVE, network_packet_header_map::packet_type(sizeof(packet_gc_party_remove), STATIC_SIZE_PACKET));
            set(HEADER_GC_PARTY_LINK, network_packet_header_map::packet_type(sizeof(packet_gc_party_link), STATIC_SIZE_PACKET));
            set(HEADER_GC_PARTY_UNLINK, network_packet_header_map::packet_type(sizeof(packet_gc_party_unlink), STATIC_SIZE_PACKET));
            set(HEADER_GC_PARTY_PARAMETER, network_packet_header_map::packet_type(sizeof(packet_gc_party_parameter), STATIC_SIZE_PACKET));

            set(HEADER_GC_SAFEBOX_SET, network_packet_header_map::packet_type(sizeof(packet_gc_item_set2), STATIC_SIZE_PACKET));
            set(HEADER_GC_SAFEBOX_DEL, network_packet_header_map::packet_type(sizeof(packet_gc_item_del), STATIC_SIZE_PACKET));
            set(HEADER_GC_SAFEBOX_WRONG_PASSWORD, network_packet_header_map::packet_type(sizeof(packet_gc_safebox_wrong_password), STATIC_SIZE_PACKET));
            set(HEADER_GC_SAFEBOX_SIZE, network_packet_header_map::packet_type(sizeof(packet_gc_safebox_size), STATIC_SIZE_PACKET));

            set(HEADER_GC_FISHING, network_packet_header_map::packet_type(sizeof(packet_gc_fishing), STATIC_SIZE_PACKET));
            set(HEADER_GC_DUNGEON, network_packet_header_map::packet_type(sizeof(packet_gc_dungeon), DYNAMIC_SIZE_PACKET));
            set(HEADER_GC_TIME, network_packet_header_map::packet_type(sizeof(packet_gc_time), STATIC_SIZE_PACKET));
            set(HEADER_GC_WALK_MODE, network_packet_header_map::packet_type(sizeof(packet_gc_walk_mode), STATIC_SIZE_PACKET));
            set(HEADER_GC_CHANGE_SKILL_GROUP, network_packet_header_map::packet_type(sizeof(packet_gc_change_skill_group), STATIC_SIZE_PACKET));
            set(HEADER_GC_REFINE_INFORMATION_NEW, network_packet_header_map::packet_type(sizeof(packet_gc_refine_information_new), STATIC_SIZE_PACKET));
            set(HEADER_GC_SEPCIAL_EFFECT, network_packet_header_map::packet_type(sizeof(packet_gc_special_effect), STATIC_SIZE_PACKET));
            set(HEADER_GC_NPC_POSITION, network_packet_header_map::packet_type(sizeof(packet_gcnpc_position), DYNAMIC_SIZE_PACKET));
            set(HEADER_GC_CHANGE_NAME, network_packet_header_map::packet_type(sizeof(packet_gc_change_name), STATIC_SIZE_PACKET));

            set(HEADER_GC_AUTH_SUCCESS, network_packet_header_map::packet_type(sizeof(packet_gc_auth_success), STATIC_SIZE_PACKET));
            set(HEADER_GC_CHANNEL, network_packet_header_map::packet_type(sizeof(packet_gc_channel), STATIC_SIZE_PACKET));
            set(HEADER_GC_VIEW_EQUIP, network_packet_header_map::packet_type(sizeof(packet_gc_view_equip), STATIC_SIZE_PACKET));
            set(HEADER_GC_LAND_LIST, network_packet_header_map::packet_type(sizeof(packet_gc_land_list), DYNAMIC_SIZE_PACKET));

            set(HEADER_GC_TARGET_UPDATE, network_packet_header_map::packet_type(sizeof(packet_gc_target_update), STATIC_SIZE_PACKET));
            set(HEADER_GC_TARGET_DELETE, network_packet_header_map::packet_type(sizeof(packet_gc_target_delete), STATIC_SIZE_PACKET));
            set(HEADER_GC_TARGET_CREATE_NEW, network_packet_header_map::packet_type(sizeof(packet_gc_target_create_new), STATIC_SIZE_PACKET));

            set(HEADER_GC_AFFECT_ADD, network_packet_header_map::packet_type(sizeof(packet_gc_affect_add), STATIC_SIZE_PACKET));
            set(HEADER_GC_AFFECT_REMOVE, network_packet_header_map::packet_type(sizeof(packet_gc_affect_remove), STATIC_SIZE_PACKET));

            set(HEADER_GC_MALL_OPEN, network_packet_header_map::packet_type(sizeof(packet_gc_mall_open), STATIC_SIZE_PACKET));
            set(HEADER_GC_MALL_SET, network_packet_header_map::packet_type(sizeof(packet_gc_item_set2), STATIC_SIZE_PACKET));
            set(HEADER_GC_MALL_DEL, network_packet_header_map::packet_type(sizeof(packet_gc_item_del), STATIC_SIZE_PACKET));

            set(HEADER_GC_LOVER_INFO, network_packet_header_map::packet_type(sizeof(packet_gc_lover_info), STATIC_SIZE_PACKET));
            set(HEADER_GC_LOVE_POINT_UPDATE, network_packet_header_map::packet_type(sizeof(packet_gc_love_point_update), STATIC_SIZE_PACKET));

            set(HEADER_GC_DIG_MOTION, network_packet_header_map::packet_type(sizeof(packet_gc_dig_motion), STATIC_SIZE_PACKET));
            set(HEADER_GC_DAMAGE_INFO, network_packet_header_map::packet_type(sizeof(packet_gc_damage_info), STATIC_SIZE_PACKET));

            set(HEADER_GC_SPECIFIC_EFFECT, network_packet_header_map::packet_type(sizeof(packet_gc_specific_effect), STATIC_SIZE_PACKET));
            set(HEADER_GC_DRAGON_SOUL_REFINE, network_packet_header_map::packet_type(sizeof(packet_gc_dragon_soul_refine), STATIC_SIZE_PACKET));
        }
    };

    int last_packet[2] = {0, 0};

    void python_network_stream::exit_application() {
        if (is_not_ping()) {
            absolute_exit_application();
        } else {
            send_chat_packet("/quit");
        }
    }

    void python_network_stream::exit_game() {
        if (is_not_ping()) {
            log_out_game();
        } else {
            send_chat_packet("/phase_select");
        }
    }

    void python_network_stream::log_out_game() {
        if (is_not_ping()) {
            absolute_exit_game();
        } else {
            send_chat_packet("/logout");
        }
    }

    void python_network_stream::absolute_exit_game() {
        if (!is_online())
            return;

        on_remote_disconnect();
        disconnect();
    }

    void python_network_stream::absolute_exit_application() {
        window::request_quit();
    }

    auto python_network_stream::is_not_ping() -> bool {
        // TODO: detect a missing ping the same way the server does.
        return false;
    }

    auto python_network_stream::guild_id() -> uint32_t {
        return guild_id_;
    }

    auto python_network_stream::upload_mark(const char *image_file_name) -> uint32_t {
        // The guild ID is still 0 right after the guild is created, until the client reconnects.
        if (0 == guild_id_)
            return ERROR_MARK_UPLOAD_NEED_RECONNECT;

        next_download_mark_time = 0;

        uint32_t error = ERROR_UNKNOWN;
        guild_mark_uploader &guild_mark_uploader = get_guild_mark_uploader();
        if (!guild_mark_uploader.connect(mark_auth_value_.net_addr, mark_auth_value_.handle, mark_auth_value_.random_key, guild_id_, image_file_name, &error)) {
            switch (error) {
            case guild_mark_uploader::ERROR_CONNECT:
                return ERROR_CONNECT_MARK_SERVER;
                break;
            case guild_mark_uploader::ERROR_LOAD:
                return ERROR_LOAD_MARK;
                break;
            case guild_mark_uploader::ERROR_WIDTH:
                return ERROR_MARK_WIDTH;
                break;
            case guild_mark_uploader::ERROR_HEIGHT:
                return ERROR_MARK_HEIGHT;
                break;
            default:
                return ERROR_UNKNOWN;
            }
        }

        download_mark();

        if (guild_mark_manager::INVALID_MARK_ID == get_guild_mark_manager().mark_id(guild_id_))
            return ERROR_MARK_CHECK_NEED_RECONNECT;

        return ERROR_NONE;
    }

    auto python_network_stream::upload_symbol(const char *image_file_name) -> uint32_t {
        uint32_t error = ERROR_UNKNOWN;
        guild_mark_uploader &guild_mark_uploader = get_guild_mark_uploader();
        if (!guild_mark_uploader.connect_to_send_symbol(mark_auth_value_.net_addr, mark_auth_value_.handle, mark_auth_value_.random_key, guild_id_, image_file_name, &error)) {
            switch (error) {
            case guild_mark_uploader::ERROR_CONNECT:
                return ERROR_CONNECT_MARK_SERVER;
                break;
            case guild_mark_uploader::ERROR_LOAD:
                return ERROR_LOAD_MARK;
                break;
            case guild_mark_uploader::ERROR_WIDTH:
                return ERROR_MARK_WIDTH;
                break;
            case guild_mark_uploader::ERROR_HEIGHT:
                return ERROR_MARK_HEIGHT;
                break;
            default:
                return ERROR_UNKNOWN;
            }
        }

        return ERROR_NONE;
    }

    void python_network_stream::download_mark() {
        // Do not download the marks again within 3 minutes.
        uint32_t cur_time = el_timer_get_m_sec();

        if (cur_time < next_download_mark_time)
            return;

        next_download_mark_time = cur_time + 60000 * 3;

        guild_mark_downloader &guild_mark_downloader = get_guild_mark_downloader();
        guild_mark_downloader.connect(mark_auth_value_.net_addr, mark_auth_value_.handle, mark_auth_value_.random_key);
    }

    void python_network_stream::download_symbol(const std::vector<uint32_t> &vec_guild_id) {
        guild_mark_downloader &guild_mark_downloader = get_guild_mark_downloader();
        guild_mark_downloader.connect_to_recv_symbol(mark_auth_value_.net_addr, mark_auth_value_.handle, mark_auth_value_.random_key, vec_guild_id);
    }

    void python_network_stream::set_phase_window(uint32_t e_phase_wnd, PyObject *po_phase_wnd) {
        if (e_phase_wnd >= PHASE_WINDOW_NUM)
            return;

        apo_phase_wnd_[e_phase_wnd] = po_phase_wnd;
    }

    void python_network_stream::clear_phase_window(uint32_t e_phase_wnd, PyObject *po_phase_wnd) {
        if (e_phase_wnd >= PHASE_WINDOW_NUM)
            return;

        if (po_phase_wnd != apo_phase_wnd_[e_phase_wnd])
            return;

        apo_phase_wnd_[e_phase_wnd] = nullptr;
    }

    void python_network_stream::set_server_command_parser_window(PyObject *wnd) {
        ser_command_parser_wnd_ = wnd;
    }

    auto python_network_stream::is_selected_empire() -> bool {
        if (empire_id_)
            return true;

        return false;
    }

    auto python_network_stream::account_character_slot_datau(uint32_t slot, uint32_t type) -> uint32_t {
        if (slot >= PLAYER_PER_ACCOUNT4)
            return 0;

        simple_player_information &simple_player_info = simple_player_infos_[slot];

        switch (type) {
        case ACCOUNT_CHARACTER_SLOT_ID:
            return simple_player_info.id;
        case ACCOUNT_CHARACTER_SLOT_RACE:
            return simple_player_info.job;
        case ACCOUNT_CHARACTER_SLOT_LEVEL:
            return simple_player_info.level_value;
        case ACCOUNT_CHARACTER_SLOT_STR:
            return simple_player_info.st;
        case ACCOUNT_CHARACTER_SLOT_DEX:
            return simple_player_info.dx;
        case ACCOUNT_CHARACTER_SLOT_HTH:
            return simple_player_info.ht;
        case ACCOUNT_CHARACTER_SLOT_INT:
            return simple_player_info.iq;
        case ACCOUNT_CHARACTER_SLOT_PLAYTIME:
            return simple_player_info.play_minutes;
        case ACCOUNT_CHARACTER_SLOT_FORM:
            return simple_player_info.main_part;
        case ACCOUNT_CHARACTER_SLOT_PORT:
            return simple_player_info.port;
        case ACCOUNT_CHARACTER_SLOT_GUILD_ID:
            return guild_ids_[slot];
            break;
        case ACCOUNT_CHARACTER_SLOT_CHANGE_NAME_FLAG:
            return simple_player_info.change_name;
            break;
        case ACCOUNT_CHARACTER_SLOT_HAIR:
            return simple_player_info.hair_part;
            break;
        }
        return 0;
    }

    auto python_network_stream::account_character_slot_dataz(uint32_t slot, uint32_t type) -> const char * {
        static const char *empty = "";

        if (slot >= PLAYER_PER_ACCOUNT4)
            return empty;

        simple_player_information &simple_player_info = simple_player_infos_[slot];

        switch (type) {
        case ACCOUNT_CHARACTER_SLOT_ADDR:
            {
                uint8_t ip[4];

                const int len = 4;
                for (int i = 0; i < len; i++) {
                    ip[i] = uint8_t(simple_player_info.addr & 0xff);
                    simple_player_info.addr >>= 8;
                }

                static char addr[256];
                snprintf(addr, sizeof(addr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
                return addr;
            }
            break;
        case ACCOUNT_CHARACTER_SLOT_NAME:
            return simple_player_info.name_value;
            break;
        case ACCOUNT_CHARACTER_SLOT_GUILD_NAME:
            return guild_name_[slot].c_str();
            break;
        }
        return empty;
    }

    void python_network_stream::connect_login_server(const char *addr, uint32_t port) {
        network_stream::connect(addr, port);
    }

    void python_network_stream::set_mark_server(const char *addr, uint32_t port) {
        mark_auth_value_.net_addr.set(addr, port);
    }

    void python_network_stream::connect_game_server(uint32_t chr_slot) {
        if (chr_slot >= PLAYER_PER_ACCOUNT4)
            return;

        selected_character_index_ = chr_slot;

        direct_enter_mode_set(chr_slot);

        simple_player_information &simple_player_info = simple_player_infos_[chr_slot];
        network_stream::connect((uint32_t)simple_player_info.addr, simple_player_info.port);
    }

    void python_network_stream::set_login_info(const char *id, const char *password) {
        id_ = id;
        password_ = password;
    }

    void python_network_stream::clear_login_info(void) {
        password_ = "";
    }

    void python_network_stream::set_login_key(uint32_t login_key) {
        login_key_ = login_key;
    }

    // Every packet is framed by its header: a fixed size from the header map, or the size field of a
    // dynamic packet. A handler that reads less has the rest skipped; one that reads past the packet,
    // or fails halfway through it, means the stream can no longer be trusted, so the connection ends.
    // A packet nothing has read yet is checked again by the next CheckPacket (phases hand packets they
    // do not know to the game phase), and skipped only once processing is over.
    void python_network_stream::finish_packet(bool is_processing_over) {
        if (packet_rest_after_ < 0)
            return;

        const int rest = recv_buffer_size();
        if (!is_protocol_error_ && is_online()) {
            const int consumed = packet_rest_after_ + packet_size_ - rest;
            if (rest < packet_rest_after_)
                protocol_error("packet %d read past its %d bytes", packet_header_value_, packet_size_);
            else if (rest > packet_rest_after_ && (consumed != 0 || is_processing_over))
                recv(rest - packet_rest_after_);
        }
        packet_rest_after_ = -1;
    }

    void python_network_stream::protocol_error(const char *format, int a, int b) {
        char message[256];
        snprintf(message, sizeof(message), format, a, b);
        metin2::log::error("Protocol error in phase {}: {}; disconnecting", phase_value_.c_str(), message);
        is_protocol_error_ = true;
        clear_recv_buffer();
    }

    auto python_network_stream::check_packet(packet_header *ret_header) -> bool {
        *ret_header = 0;

        finish_packet(false);
        if (is_protocol_error_)
            return false;

        static main_packet_header_map packet_header_map;

        packet_header header;

        if (!peek(sizeof(packet_header), &header))
            return false;

        if (0 == header) {
            if (!recv(sizeof(packet_header), &header))
                return false;

            while (peek(sizeof(packet_header), &header)) {
                if (0 == header) {
                    if (!recv(sizeof(packet_header), &header))
                        return false;
                } else {
                    break;
                }
            }

            if (0 == header)
                return false;
        }

        network_packet_header_map::packet_type packet_type;

        if (!packet_header_map.get(header, &packet_type)) {
            protocol_error("unknown packet header %d (previous %d)", header, last_packet[1]);
            return false;
        }

        int packet_size = packet_type.packet_size;
        if (packet_type.is_dynamic_size_packet) {
            dynamic_size_packet_header dynamic_size_packet_header;

            if (!peek(sizeof(dynamic_size_packet_header), &dynamic_size_packet_header))
                return false;

            packet_size = dynamic_size_packet_header.size;
            if (packet_size < std::max<int>(packet_type.packet_size, sizeof(dynamic_size_packet_header)) || packet_size > recv_buffer_capacity()) {
                protocol_error("packet %d declares a size of %d bytes", header, packet_size);
                return false;
            }
        }

        if (!peek(packet_size))
            return false;

        *ret_header = header;
        packet_header_value_ = header;
        packet_size_ = packet_size;
        packet_rest_after_ = recv_buffer_size() - packet_size;

        last_packet[0] = last_packet[1];
        last_packet[1] = header;
        return true;
    }

    auto python_network_stream::recv_packet_text(char *buffer_value, size_t size) -> bool {
        const int length = recv_buffer_size() - packet_rest_after_;
        if (packet_rest_after_ < 0 || length < 0)
            return false;

        std::string text(length, '\0');
        if (length > 0 && !recv(length, text.data()))
            return false;

        text.resize(std::min(text.find('\0'), text.size()));
        utf8::copy_truncated(buffer_value, size, text);
        return true;
    }

    auto python_network_stream::packet_bytes_left() -> int {
        return packet_rest_after_ < 0 ? 0 : recv_buffer_size() - packet_rest_after_;
    }

    auto python_network_stream::recv_string(int length, std::string &out) -> bool {
        if (length < 0 || length > packet_bytes_left())
            return false;

        out.assign(length, '\0');
        if (length > 0 && !recv(length, out.data()))
            return false;
        out.resize(std::min(out.find('\0'), out.size()));
        return true;
    }

    auto python_network_stream::recv_error_packet(int header) -> bool {
        if (packet_rest_after_ < 0)
            return true;

        const int consumed = packet_rest_after_ + packet_size_ - recv_buffer_size();
        if (consumed == 0) {
            metin2::log::error("Phase {} does not handle packet {}; skipped", phase_value_.c_str(), header);
            recv(packet_size_);
        } else if (consumed != packet_size_)
            protocol_error("packet %d is malformed (%d bytes read)", header, consumed);
        return true;
    }

    auto python_network_stream::recv_phase_packet() -> bool {
        packet_gc_phase packet_phase;

        if (!recv(sizeof(packet_gc_phase), &packet_phase))
            return false;

        switch (packet_phase.phase_value) {
        case PHASE_CLOSE:
            close_phase();
            break;

        case PHASE_HANDSHAKE:
            set_hand_shake_phase();
            break;

        case PHASE_LOGIN:
            set_login_phase();
            break;

        case PHASE_SELECT:
            set_select_phase();

            download_mark();
            break;

        case PHASE_LOADING:
            set_loading_phase();
            break;

        case PHASE_GAME:
            set_game_phase();
            break;

        case PHASE_DEAD:
            break;
        }

        return true;
    }

    auto python_network_stream::recv_ping_packet() -> bool {
        metin2::log::debug("Ping received (security mode {})", is_security_mode());

        packet_gc_ping packet_ping;

        if (!recv(sizeof(packet_gc_ping), &packet_ping))
            return false;

        last_game_ping_time_ = el_timer_get_m_sec();

        packet_cg_pong packet_pong;
        packet_pong.header_value = HEADER_CG_PONG;

        if (!send(sizeof(packet_cg_pong), &packet_pong))
            return false;

        if (is_security_mode())
            return send_sequence();
        else
            return true;
    }

    auto python_network_stream::recv_default_packet(int header) -> bool {
        if (!header)
            return true;

        return recv_error_packet(header);
    }

    auto python_network_stream::on_process() -> bool {
        if (is_start_game_) {
            is_start_game_ = false;

            py_call_class_member_func(handler_, "SetGamePhase", Py_BuildValue("()"));
        }

        rok_net_actor_mgr_->update();

        if (phase_process_func_.is_empty())
            return true;

        phase_process_func_.run();

        finish_packet(true);
        if (is_protocol_error_) {
            is_protocol_error_ = false;
            return false;
        }
        return true;
    }

    void python_network_stream::set_off_line_phase() {
        if ("OffLine" != phase_value_)
            phase_leave_func_.run();

        phase_value_ = "OffLine";

        metin2::log::debug("Entering the offline phase");

        changing_phase_time_ = el_timer_get_m_sec();
        phase_process_func_.set(this, &python_network_stream::off_line_phase);
        phase_leave_func_.set(this, &python_network_stream::leave_offline_phase);

        set_game_offline();

        selected_character_index_ = 0;

        direct_enter_mode_initialize();
        betting_guild_war_initialize();
    }

    void python_network_stream::close_phase() {
        py_call_class_member_func(handler_, "SetLoginPhase", Py_BuildValue("()"));
    }

    void python_network_stream::set_game_online() {
        is_game_online_ = true;
    }

    void python_network_stream::set_game_offline() {
        is_game_online_ = false;
    }

    void python_network_stream::set_handler(PyObject *handler) {
        handler_ = handler;
    }

    auto python_network_stream::main_actor_vid() -> uint32_t {
        return main_actor_vid_;
    }

    auto python_network_stream::main_actor_race() -> uint32_t {
        return main_actor_race_;
    }

    auto python_network_stream::main_actor_empire() -> uint32_t {
        return main_actor_empire_;
    }

    auto python_network_stream::main_actor_skill_group() -> uint32_t {
        return main_actor_skill_group_;
    }

    void python_network_stream::set_empire_id(uint32_t empire_id) {
        empire_id_ = empire_id;
    }

    auto python_network_stream::empire_id() -> uint32_t {
        return empire_id_;
    }

    void python_network_stream::clear_select_character_data() {
        memset(&simple_player_infos_, 0, sizeof(simple_player_infos_));

        for (int i = 0; i < PLAYER_PER_ACCOUNT4; ++i) {
            guild_ids_[i] = 0;
            guild_name_[i] = "";
        }
    }

    void python_network_stream::direct_enter_mode_initialize() {
        direct_enter_mode_value_.is_set = false;
        direct_enter_mode_value_.chr_slot_index = 0;
    }

    void python_network_stream::direct_enter_mode_set(uint32_t chr_slot_index) {
        direct_enter_mode_value_.is_set = true;
        direct_enter_mode_value_.chr_slot_index = chr_slot_index;
    }

    auto python_network_stream::direct_enter_mode_is_set() -> bool {
        return direct_enter_mode_value_.is_set;
    }

    void python_network_stream::initialize_mark_auth() {
        mark_auth_value_.handle = 0;
        mark_auth_value_.random_key = 0;
    }

    void python_network_stream::betting_guild_war_initialize() {
        betting_guild_war_value_.betting_money = 0;
        betting_guild_war_value_.observer_count = 0;
    }

    void python_network_stream::server_time_sync_initialize() {
        server_time_sync_value_.change_client_time = 0;
        server_time_sync_value_.change_server_time = 0;
    }

    void python_network_stream::send_emoticon(uint32_t emoticon) {
        if (emoticon < emoticon_string_vector_.size())
            send_chat_packet(emoticon_string_vector_[emoticon].c_str());
        else
            assert(false && "SendEmoticon Error");
    }

    python_network_stream::python_network_stream() {
        rok_net_actor_mgr_ = new network_actor_manager;

        memset(simple_player_infos_, 0, sizeof(simple_player_infos_));

        phase_process_func_.clear();

        empire_id_ = 0;
        guild_id_ = 0;
        guild_creation_key_ = 0;

        main_actor_vid_ = 0;
        main_actor_race_ = 0;
        main_actor_empire_ = 0;
        main_actor_skill_group_ = 0;
        handler_ = nullptr;

        last_game_ping_time_ = 0;

        login_key_ = 0;
        is_start_game_ = false;
        is_enable_chat_insult_filter_ = false;
        combo_skill_flag_ = false;
        phase_value_ = "OffLine";

        initialize_game_phase();
        initialize_mark_auth();

        direct_enter_mode_initialize();
        betting_guild_war_initialize();

        std::fill(apo_phase_wnd_, apo_phase_wnd_ + PHASE_WINDOW_NUM, (PyObject *)nullptr);
        ser_command_parser_wnd_ = nullptr;

        set_off_line_phase();
    }

    python_network_stream::~python_network_stream() {
        metin2::log::debug("Clearing the network stream");
    }
}
