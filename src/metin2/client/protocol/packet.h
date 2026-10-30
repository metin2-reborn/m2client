#pragma once

#include <metin2/client/app/game_type.h>
#include <metin2/world/actors/race_data.h>
#include <metin2/world/maps/map_type.h>

#include <cstring>
#include <string>

namespace metin2::inline client::inline protocol {

    // A fixed-size string field from the server, which need not be NUL-terminated.
    template <size_t N>
    auto packet_string(const char (&field)[N]) -> std::string {
        return std::string(field, strnlen(field, N));
    }

    using packet_header = uint8_t;

    enum {
        HEADER_CG_LOGIN = 1,
        HEADER_CG_ATTACK = 2,
        HEADER_CG_CHAT = 3,
        HEADER_CG_PLAYER_CREATE = 4,
        HEADER_CG_PLAYER_DESTROY = 5,
        HEADER_CG_PLAYER_SELECT = 6,
        HEADER_CG_CHARACTER_MOVE = 7,
        HEADER_CG_SYNC_POSITION = 8,
        HEADER_CG_ENTERGAME = 10,
        HEADER_CG_ITEM_USE = 11,
        HEADER_CG_ITEM_DROP = 12,
        HEADER_CG_ITEM_MOVE = 13,
        HEADER_CG_ITEM_PICKUP = 15,
        HEADER_CG_QUICKSLOT_ADD = 16,
        HEADER_CG_QUICKSLOT_DEL = 17,
        HEADER_CG_QUICKSLOT_SWAP = 18,
        HEADER_CG_WHISPER = 19,
        HEADER_CG_ITEM_DROP2 = 20,
        HEADER_CG_ON_CLICK = 26,
        HEADER_CG_EXCHANGE = 27,
        HEADER_CG_CHARACTER_POSITION = 28,
        HEADER_CG_SCRIPT_ANSWER = 29,
        HEADER_CG_QUEST_INPUT_STRING = 30,
        HEADER_CG_QUEST_CONFIRM = 31,
        HEADER_CG_SHOP = 50,
        HEADER_CG_FLY_TARGETING = 51,
        HEADER_CG_USE_SKILL = 52,
        HEADER_CG_ADD_FLY_TARGETING = 53,
        HEADER_CG_SHOOT = 54,
        HEADER_CG_MYSHOP = 55,
        HEADER_CG_ITEM_USE_TO_ITEM = 60,
        HEADER_CG_TARGET = 61,
        HEADER_CG_WARP = 65,
        HEADER_CG_SCRIPT_BUTTON = 66,
        HEADER_CG_MESSENGER = 67,
        HEADER_CG_MALL_CHECKOUT = 69,
        HEADER_CG_SAFEBOX_CHECKIN = 70,
        HEADER_CG_SAFEBOX_CHECKOUT = 71,
        HEADER_CG_PARTY_INVITE = 72,
        HEADER_CG_PARTY_INVITE_ANSWER = 73,
        HEADER_CG_PARTY_REMOVE = 74,
        HEADER_CG_PARTY_SET_STATE = 75,
        HEADER_CG_PARTY_USE_SKILL = 76,
        HEADER_CG_SAFEBOX_ITEM_MOVE = 77,
        HEADER_CG_PARTY_PARAMETER = 78,
        HEADER_CG_GUILD = 80,
        HEADER_CG_ANSWER_MAKE_GUILD = 81,
        HEADER_CG_FISHING = 82,
        HEADER_CG_GIVE_ITEM = 83,
        HEADER_CG_EMPIRE = 90,
        HEADER_CG_REFINE = 96,

        HEADER_CG_MARK_LOGIN = 100,
        HEADER_CG_MARK_CRCLIST = 101,
        HEADER_CG_MARK_UPLOAD = 102,
        HEADER_CG_MARK_IDXLIST = 104,

        HEADER_CG_HACK = 105,
        HEADER_CG_CHANGE_NAME = 106,
        HEADER_CG_LOGIN2 = 109,
        HEADER_CG_LOGIN3 = 111,
        HEADER_CG_GUILD_SYMBOL_UPLOAD = 112,
        HEADER_CG_GUILD_SYMBOL_CRC = 113,
        HEADER_CG_SCRIPT_SELECT_ITEM = 114,

        HEADER_CG_DRAGON_SOUL_REFINE = 205,
        HEADER_CG_STATE_CHECKER = 206,

        HEADER_CG_KEY_AGREEMENT = 0xfb,
        HEADER_CG_TIME_SYNC = 0xfc,
        HEADER_CG_CLIENT_VERSION = 0xfd,
        HEADER_CG_CLIENT_VERSION2 = 0xf1,
        HEADER_CG_PONG = 0xfe,
        HEADER_CG_HANDSHAKE = 0xff,

        HEADER_GC_CHARACTER_ADD = 1,
        HEADER_GC_CHARACTER_DEL = 2,
        HEADER_GC_CHARACTER_MOVE = 3,
        HEADER_GC_CHAT = 4,
        HEADER_GC_SYNC_POSITION = 5,
        HEADER_GC_LOGIN_FAILURE = 7,
        HEADER_GC_PLAYER_CREATE_SUCCESS = 8,
        HEADER_GC_PLAYER_CREATE_FAILURE = 9,
        HEADER_GC_PLAYER_DELETE_SUCCESS = 10,
        HEADER_GC_PLAYER_DELETE_WRONG_SOCIAL_ID = 11,
        HEADER_GC_STUN = 13,
        HEADER_GC_DEAD = 14,

        HEADER_GC_PLAYER_POINTS = 16,
        HEADER_GC_PLAYER_POINT_CHANGE = 17,
        HEADER_GC_CHARACTER_UPDATE = 19,
        HEADER_GC_ITEM_SET = 20,
        HEADER_GC_ITEM_SET2 = 21,
        HEADER_GC_ITEM_UPDATE = 25,
        HEADER_GC_ITEM_GROUND_ADD = 26,
        HEADER_GC_ITEM_GROUND_DEL = 27,
        HEADER_GC_QUICKSLOT_ADD = 28,
        HEADER_GC_QUICKSLOT_DEL = 29,
        HEADER_GC_QUICKSLOT_SWAP = 30,
        HEADER_GC_ITEM_OWNERSHIP = 31,
        HEADER_GC_LOGIN_SUCCESS4 = 32,
        HEADER_GC_WHISPER = 34,

        HEADER_GC_SHOP = 38,
        HEADER_GC_SHOP_SIGN = 39,

        HEADER_GC_DUEL_START = 40,
        HEADER_GC_PVP = 41,
        HEADER_GC_EXCHANGE = 42,
        HEADER_GC_CHARACTER_POSITION = 43,

        HEADER_GC_PING = 44,

        HEADER_GC_SCRIPT = 45,
        HEADER_GC_QUEST_CONFIRM = 46,

        HEADER_GC_OWNERSHIP = 62,
        HEADER_GC_TARGET = 63,
        HEADER_GC_WARP = 65,
        HEADER_GC_ADD_FLY_TARGETING = 69,

        HEADER_GC_CREATE_FLY = 70,
        HEADER_GC_FLY_TARGETING = 71,
        HEADER_GC_MESSENGER = 74,
        HEADER_GC_GUILD = 75,
        HEADER_GC_SKILL_LEVEL_NEW = 76,

        HEADER_GC_PARTY_INVITE = 77,
        HEADER_GC_PARTY_ADD = 78,
        HEADER_GC_PARTY_UPDATE = 79,
        HEADER_GC_PARTY_REMOVE = 80,

        HEADER_GC_QUEST_INFO = 81,
        HEADER_GC_REQUEST_MAKE_GUILD = 82,
        HEADER_GC_PARTY_PARAMETER = 83,

        HEADER_GC_SAFEBOX_SET = 85,
        HEADER_GC_SAFEBOX_DEL = 86,
        HEADER_GC_SAFEBOX_WRONG_PASSWORD = 87,
        HEADER_GC_SAFEBOX_SIZE = 88,

        HEADER_GC_FISHING = 89,

        HEADER_GC_EMPIRE = 90,

        HEADER_GC_PARTY_LINK = 91,
        HEADER_GC_PARTY_UNLINK = 92,

        HEADER_GC_VIEW_EQUIP = 99,

        HEADER_GC_MARK_BLOCK = 100,
        HEADER_GC_MARK_IDXLIST = 102,

        HEADER_GC_TIME = 106,
        HEADER_GC_CHANGE_NAME = 107,

        HEADER_GC_DUNGEON = 110,
        HEADER_GC_WALK_MODE = 111,
        HEADER_GC_CHANGE_SKILL_GROUP = 112,

        HEADER_GC_MAIN_CHARACTER2_EMPIRE = 113,

        HEADER_GC_SEPCIAL_EFFECT = 114,
        HEADER_GC_NPC_POSITION = 115,

        HEADER_GC_REFINE_INFORMATION_NEW = 119,
        HEADER_GC_CHANNEL = 121,

        HEADER_GC_MALL_OPEN = 122,
        HEADER_GC_TARGET_UPDATE = 123,
        HEADER_GC_TARGET_DELETE = 124,
        HEADER_GC_TARGET_CREATE_NEW = 125,

        HEADER_GC_AFFECT_ADD = 126,
        HEADER_GC_AFFECT_REMOVE = 127,

        HEADER_GC_MALL_SET = 128,
        HEADER_GC_MALL_DEL = 129,
        HEADER_GC_LAND_LIST = 130,
        HEADER_GC_LOVER_INFO = 131,
        HEADER_GC_LOVE_POINT_UPDATE = 132,
        HEADER_GC_GUILD_SYMBOL_DATA = 133,
        HEADER_GC_DIG_MOTION = 134,

        HEADER_GC_DAMAGE_INFO = 135,
        HEADER_GC_CHAR_ADDITIONAL_INFO = 136,

        HEADER_GC_MAIN_CHARACTER3_BGM = 137,
        HEADER_GC_MAIN_CHARACTER4_BGM_VOL = 138,

        HEADER_GC_AUTH_SUCCESS = 150,

        HEADER_GC_SPECIFIC_EFFECT = 208,
        HEADER_GC_DRAGON_SOUL_REFINE = 209,
        HEADER_GC_RESPOND_CHANNELSTATUS = 210,

        HEADER_GC_KEY_AGREEMENT_COMPLETED = 0xfa,
        HEADER_GC_KEY_AGREEMENT = 0xfb,
        HEADER_GC_HANDSHAKE_OK = 0xfc, // 252
        HEADER_GC_PHASE = 0xfd,        // 253
        HEADER_GC_HANDSHAKE = 0xff,    // 255
    };

    enum {
        ID_MAX_NUM = 30,
        PASS_MAX_NUM = 16,
        CHAT_MAX_NUM = 128,
        PATH_NODE_MAX_NUM = 64,
        SHOP_SIGN_MAX_LEN = 32,

        PLAYER_PER_ACCOUNT3 = 3,
        PLAYER_PER_ACCOUNT4 = 4,

        PLAYER_ITEM_SLOT_MAX_NUM = 20,

        QUICKSLOT_MAX_LINE = 4,
        QUICKSLOT_MAX_COUNT_PER_LINE = 8, // chosen by the client
        QUICKSLOT_MAX_COUNT = QUICKSLOT_MAX_LINE * QUICKSLOT_MAX_COUNT_PER_LINE,

        QUICKSLOT_MAX_NUM = 36, // matches the server

        SHOP_HOST_ITEM_MAX_NUM = 40,

        METIN_SOCKET_COUNT = 6,

        PARTY_AFFECT_SLOT_MAX_NUM = 7,

        GUILD_GRADE_NAME_MAX_LEN = 8,
        GUILD_NAME_MAX_LEN = 12,
        GUILD_GRADE_COUNT = 15,
        GULID_COMMENT_MAX_LEN = 50,

        MARK_CRC_NUM = 8 * 8,
        MARK_DATA_SIZE = 16 * 12,
        SYMBOL_DATA_SIZE = 128 * 256,
        QUEST_INPUT_STRING_MAX_NUM = 64,

        PRIVATE_CODE_LENGTH = 8,

        REFINE_MATERIAL_MAX_NUM = 5,

        WEAR_MAX_NUM = 11,
        VIEW_EQUIP_SLOT_NUM = 32, // the server sends its full WEAR_MAX_NUM slots; the UI shows the first WEAR_MAX_NUM

        OPENID_AUTHKEY_LEN = 32,

        SHOP_TAB_NAME_MAX = 32,
        SHOP_TAB_COUNT_MAX = 3,
    };

#pragma pack(push)
#pragma pack(1)

    struct command_mark_login {
        uint8_t header_value;
        uint32_t handle;
        uint32_t random_key;
    };
    using packet_cg_mark_login = command_mark_login;

    struct command_mark_upload {
        uint8_t header_value;
        uint32_t gid;
        uint8_t image[16 * 12 * 4];
    };
    using packet_cg_mark_upload = command_mark_upload;

    struct command_mark_idxlist {
        uint8_t header_value;
    };
    using packet_cg_mark_idx_list = command_mark_idxlist;

    struct command_mark_crclist {
        uint8_t header_value;
        uint8_t img_idx;
        uint32_t crclist[80];
    };
    using packet_cg_mark_crc_list = command_mark_crclist;

    struct packet_mark_idxlist {
        uint8_t header_value;
        uint32_t buf_size;
        uint16_t count;
        // Followed by count * (uint16_t + uint16_t) bytes of data.
    };
    using packet_gc_mark_idx_list = packet_mark_idxlist;

    struct packet_mark_block {
        uint8_t header_value;
        uint32_t buf_size;
        uint8_t img_idx;
        uint32_t count;
        // Followed by 64 x 48 x 4-byte pixels = 12288 bytes of data.
    };
    using packet_gc_mark_block = packet_mark_block;

    struct command_symbol_upload {
        uint8_t header_value;
        uint16_t size;
        uint32_t handle;
    };
    using packet_cg_symbol_upload = command_symbol_upload;

    struct command_symbol_crc {
        uint8_t header_value;
        uint32_t guild_id;
        uint32_t crc;
        uint32_t size;
    };
    using packet_cg_symbol_crc = command_symbol_crc;

    struct packet_symbol_data {
        uint8_t header_value;
        uint16_t size;
        uint32_t guild_id;
    };
    using packet_gc_guild_symbol_data = packet_symbol_data;

    struct command_login {
        uint8_t header_value;
        char name_value[ID_MAX_NUM + 1];
        char pwd[PASS_MAX_NUM + 1];
    };
    using packet_cg_login = command_login;

    struct command_login2 {
        uint8_t header_value;
        char name_value[ID_MAX_NUM + 1];
        uint32_t login_key;
        uint32_t client_key[4];
    };
    using packet_cg_login2 = command_login2;

    struct command_login3 {
        uint8_t header_value;
        char name_value[ID_MAX_NUM + 1];
        char pwd[PASS_MAX_NUM + 1];
        uint32_t client_key[4];
    };
    using packet_cg_login3 = command_login3;

    struct command_player_select {
        uint8_t header_value;
        uint8_t player_index;
    };
    using packet_cg_select_character = command_player_select;

    struct command_attack {
        uint8_t header_value;
        uint8_t type_value;
        uint32_t victim_vid;
        uint8_t crc_magic_cube_proc_piece;
        uint8_t crc_magic_cube_file_piece;
    };
    using packet_cg_attack = command_attack;

    struct command_chat {
        uint8_t header_value;
        uint16_t length;
        uint8_t type_value;
    };
    using packet_cg_chat = command_chat;

    struct command_whisper {
        uint8_t header_value;
        uint16_t size;
        char name_to[CHARACTER_NAME_MAX_LEN + 1];
    };
    using packet_cg_whisper = command_whisper;

    enum battle_mode {
        BATTLEMODE_ATTACK = 0,
        BATTLEMODE_DEFENSE = 1,
    };

    struct command_enter_front_game {
        uint8_t header_value;
    };
    using packet_cg_enter_front_game = command_enter_front_game;

    struct command_item_use {
        uint8_t header_value;
        item_pos pos;
    };
    using packet_cg_item_use = command_item_use;

    struct command_item_use_to_item {
        uint8_t header_value;
        item_pos source_pos;
        item_pos target_pos;
    };
    using packet_cg_item_use_to_item = command_item_use_to_item;

    struct command_item_drop {
        uint8_t header_value;
        item_pos pos;
        uint32_t elk;
    };
    using packet_cg_item_drop = command_item_drop;

    struct command_item_drop2 {
        uint8_t header_value;
        item_pos pos;
        uint32_t gold;
        uint8_t count;
    };
    using packet_cg_item_drop2 = command_item_drop2;

    struct command_item_move {
        uint8_t header_value;
        item_pos pos;
        item_pos change_pos;
        uint8_t num;
    };
    using packet_cg_item_move = command_item_move;

    struct command_item_pickup {
        uint8_t header_value;
        uint32_t vid;
    };
    using packet_cg_item_pick_up = command_item_pickup;

    struct command_quickslot_add {
        uint8_t header_value;
        uint8_t pos;
        quick_slot slot_value;
    };
    using packet_cg_quick_slot_add = command_quickslot_add;

    struct command_quickslot_del {
        uint8_t header_value;
        uint8_t pos;
    };
    using packet_cg_quick_slot_del = command_quickslot_del;

    struct command_quickslot_swap {
        uint8_t header_value;
        uint8_t pos;
        uint8_t change_pos;
    };
    using packet_cg_quick_slot_swap = command_quickslot_swap;

    struct command_on_click {
        uint8_t header_value;
        uint32_t vid;
    };
    using packet_cg_on_click = command_on_click;

    enum {
        SHOP_SUBHEADER_CG_END,
        SHOP_SUBHEADER_CG_BUY,
        SHOP_SUBHEADER_CG_SELL,
        SHOP_SUBHEADER_CG_SELL2,
    };

    struct command_shop {
        uint8_t header_value;
        uint8_t subheader;
    };
    using packet_cg_shop = command_shop;

    enum {
        EXCHANGE_SUBHEADER_CG_START,    // arg1 == vid of target character
        EXCHANGE_SUBHEADER_CG_ITEM_ADD, // arg1 == position of item
        EXCHANGE_SUBHEADER_CG_ITEM_DEL, // arg1 == position of item
        EXCHANGE_SUBHEADER_CG_ELK_ADD,  // arg1 == amount of elk
        EXCHANGE_SUBHEADER_CG_ACCEPT,   // arg1 == not used
        EXCHANGE_SUBHEADER_CG_CANCEL,   // arg1 == not used
    };

    struct command_exchange {
        uint8_t header_value;
        uint8_t subheader;
        uint32_t arg1;
        uint8_t arg2;
        item_pos pos;
    };
    using packet_cg_exchange = command_exchange;

    struct command_script_answer {
        uint8_t header_value;
        uint8_t answer;
    };
    using packet_cg_script_answer = command_script_answer;

    struct command_script_button {
        uint8_t header_value;
        unsigned int idx;
    };
    using packet_cg_script_button = command_script_button;

    struct command_target {
        uint8_t header_value;
        uint32_t vid;
    };
    using packet_cg_target = command_target;

    struct command_move {
        uint8_t header_value;
        uint8_t func;
        uint8_t arg_value;
        uint8_t rot;
        int32_t x;
        int32_t y;
        uint32_t time;
    };
    using packet_cg_move = command_move;

    struct command_sync_position_element {
        uint32_t vid;
        int32_t x;
        int32_t y;
    };
    using packet_cg_sync_position_element = command_sync_position_element;

    struct command_sync_position {
        uint8_t header_value;
        uint16_t size;
    };
    using packet_cg_sync_position = command_sync_position;

    struct command_fly_targeting {
        uint8_t header_value;
        uint32_t target_vid;
        int32_t x;
        int32_t y;
    };
    using packet_cg_fly_targeting = command_fly_targeting;

    struct packet_fly_targeting {
        uint8_t header_value;
        uint32_t shooter_vid;
        uint32_t target_vid;
        int32_t x;
        int32_t y;
    };
    using packet_gc_fly_targeting = packet_fly_targeting;

    struct packet_shoot {
        uint8_t header_value;
        uint8_t type_value;
    };
    using packet_cg_shoot = packet_shoot;

    enum {
        MESSENGER_SUBHEADER_GC_LIST,
        MESSENGER_SUBHEADER_GC_LOGIN,
        MESSENGER_SUBHEADER_GC_LOGOUT,
        MESSENGER_SUBHEADER_GC_INVITE,
        MESSENGER_SUBHEADER_GC_MOBILE,
    };

    struct packet_messenger {
        uint8_t header_value;
        uint16_t size;
        uint8_t subheader;
    };
    using packet_gc_messenger = packet_messenger;

    struct packet_messenger_list_offline {
        uint8_t connected; // always 0
        uint8_t length;
    };
    using packet_gc_messenger_list_offline = packet_messenger_list_offline;

    enum {
        MESSENGER_CONNECTED_STATE_OFFLINE,
        MESSENGER_CONNECTED_STATE_ONLINE,
        MESSENGER_CONNECTED_STATE_MOBILE,
    };

    struct packet_messenger_list_online {
        uint8_t connected;
        uint8_t length;
    };
    using packet_gc_messenger_list_online = packet_messenger_list_online;

    struct packet_messenger_login {
        uint8_t length;
    };
    using packet_gc_messenger_login = packet_messenger_login;

    struct packet_messenger_logout {
        uint8_t length;
    };
    using packet_gc_messenger_logout = packet_messenger_logout;

    enum {
        MESSENGER_SUBHEADER_CG_ADD_BY_VID,
        MESSENGER_SUBHEADER_CG_ADD_BY_NAME,
        MESSENGER_SUBHEADER_CG_REMOVE,
    };

    struct command_messenger {
        uint8_t header_value;
        uint8_t subheader;
    };
    using packet_cg_messenger = command_messenger;

    enum {
        SAFEBOX_MONEY_STATE_SAVE,
        SAFEBOX_MONEY_STATE_WITHDRAW,
    };

    struct command_safebox_checkout {
        uint8_t header_value;
        uint8_t safe_pos;
        item_pos item_pos_value;
    };
    using packet_cg_safebox_checkout = command_safebox_checkout;

    struct command_safebox_checkin {
        uint8_t header_value;
        uint8_t safe_pos;
        item_pos item_pos_value;
    };
    using packet_cg_safebox_checkin = command_safebox_checkin;

    struct command_mall_checkout {
        uint8_t header_value;
        uint8_t mall_pos;
        item_pos item_pos_value;
    };
    using packet_cg_mall_checkout = command_mall_checkout;

    struct command_use_skill {
        uint8_t header_value;
        uint32_t vnum;
        uint32_t target_vid;
    };
    using packet_cg_use_skill = command_use_skill;

    struct command_party_invite {
        uint8_t header_value;
        uint32_t vid;
    };
    using packet_cg_party_invite = command_party_invite;

    struct command_party_invite_answer {
        uint8_t header_value;
        uint32_t leader_pid;
        uint8_t accept;
    };
    using packet_cg_party_invite_answer = command_party_invite_answer;

    struct command_party_remove {
        uint8_t header_value;
        uint32_t pid;
    };
    using packet_cg_party_remove = command_party_remove;

    struct command_party_set_state {
        uint8_t header_value;
        uint32_t vid;
        uint8_t state_value;
        uint8_t flag;
    };
    using packet_cg_party_set_state = command_party_set_state;

    struct packet_party_link {
        uint8_t header_value;
        uint32_t pid;
        uint32_t vid;
    };
    using packet_gc_party_link = packet_party_link;

    struct packet_party_unlink {
        uint8_t header_value;
        uint32_t pid;
        uint32_t vid;
    };
    using packet_gc_party_unlink = packet_party_unlink;

    struct command_party_use_skill {
        uint8_t header_value;
        uint8_t skill_index;
        uint32_t target_vid;
    };
    using packet_cg_party_use_skill = command_party_use_skill;

    enum {
        GUILD_SUBHEADER_CG_ADD_MEMBER,
        GUILD_SUBHEADER_CG_REMOVE_MEMBER,
        GUILD_SUBHEADER_CG_CHANGE_GRADE_NAME,
        GUILD_SUBHEADER_CG_CHANGE_GRADE_AUTHORITY,
        GUILD_SUBHEADER_CG_OFFER,
        GUILD_SUBHEADER_CG_POST_COMMENT,
        GUILD_SUBHEADER_CG_DELETE_COMMENT,
        GUILD_SUBHEADER_CG_REFRESH_COMMENT,
        GUILD_SUBHEADER_CG_CHANGE_MEMBER_GRADE,
        GUILD_SUBHEADER_CG_USE_SKILL,
        GUILD_SUBHEADER_CG_CHANGE_MEMBER_GENERAL,
        GUILD_SUBHEADER_CG_GUILD_INVITE_ANSWER,
        GUILD_SUBHEADER_CG_CHARGE_GSP,
        GUILD_SUBHEADER_CG_DEPOSIT_MONEY,
        GUILD_SUBHEADER_CG_WITHDRAW_MONEY,
    };

    struct command_guild {
        uint8_t header_value;
        uint8_t sub_header;
    };
    using packet_cg_guild = command_guild;

    struct command_guild_answer_make_guild {
        uint8_t header_value;
        char guild_name[GUILD_NAME_MAX_LEN + 1];
        uint32_t key; // from packet_request_make_guild
    };
    using packet_cg_answer_make_guild = command_guild_answer_make_guild;

    // The server asks for a guild name; the answer carries the key back.
    struct packet_request_make_guild {
        uint8_t header_value;
        uint32_t key;
    };
    using packet_gc_request_make_guild = packet_request_make_guild;

    struct command_give_item {
        uint8_t header_value;
        uint32_t target_vid;
        item_pos item_pos_value;
        uint8_t item_count;
    };
    using packet_cg_give_item = command_give_item;

    struct packet_cg_hack {
        uint8_t header_value;
        char buf[255 + 1];
    };

    struct shop_item_table {
        uint32_t vnum;
        uint8_t count;

        item_pos pos;        // PC shops only
        uint32_t price;      // PC shops only
        uint8_t display_pos; // PC shops only
    };

    struct packet_cg_my_shop {
        uint8_t header_value;
        char sign[SHOP_SIGN_MAX_LEN + 1];
        uint8_t count; // number of shop_item_table entries, at most 39
    };

    struct packet_cg_refine {
        uint8_t header_value;
        uint8_t pos;
        uint8_t type_value;
    };

    struct packet_cg_change_name {
        uint8_t header_value;
        uint8_t index_value;
        char name_value[CHARACTER_NAME_MAX_LEN + 1];
    };

    struct command_client_version2 {
        uint8_t header_value;
        char filename[32 + 1];
        char timestamp[32 + 1];
    };
    using packet_cg_client_version2 = command_client_version2;

    enum party_exp_distribution_type {
        PARTY_EXP_DISTRIBUTION_NON_PARITY,
        PARTY_EXP_DISTRIBUTION_PARITY,
    };

    struct command_party_parameter {
        uint8_t header_value;
        uint8_t distribute_mode;
    };
    using packet_cg_party_parameter = command_party_parameter;

    struct command_quest_input_string {
        uint8_t header_value;
        char string[QUEST_INPUT_STRING_MAX_NUM + 1];
    };
    using packet_cg_quest_input_string = command_quest_input_string;

    struct command_quest_confirm {
        uint8_t header_value;
        uint8_t answer;
        uint32_t request_pid;
    };
    using packet_cg_quest_confirm = command_quest_confirm;

    struct command_script_select_item {
        uint8_t header_value;
        uint32_t selection;
    };
    using packet_cg_script_select_item = command_script_select_item;

    enum phase {
        PHASE_CLOSE, // closing (or about to close)
        PHASE_HANDSHAKE,
        PHASE_LOGIN,
        PHASE_SELECT,
        PHASE_LOADING,
        PHASE_GAME,
        PHASE_DEAD,

        PHASE_DBCLIENT_CONNECTING, // server-side only
        PHASE_DBCLIENT,
        PHASE_P2P,
        PHASE_AUTH,
    };

    struct packet_phase {
        uint8_t header_value;
        uint8_t phase_value;
    };
    using packet_gc_phase = packet_phase;

    struct packet_blank {
        uint8_t header_value;
    };
    using packet_gc_blank = packet_blank;

    struct packet_blank_dynamic {
        uint8_t header_value;
        uint16_t size;
    };
    using packet_gc_blank_dynamic = packet_blank_dynamic;

    struct packet_header_handshake {
        uint8_t header_value;
        uint32_t handshake;
        uint32_t time;
        int32_t delta;
    };
    using packet_gc_handshake = packet_header_handshake;

    struct packet_header_dynamic_size {
        uint8_t header_value;
        uint16_t size;
    };
    using dynamic_size_packet_header = packet_header_dynamic_size;

    struct simple_player_information {
        uint32_t id;
        char name_value[CHARACTER_NAME_MAX_LEN + 1];
        uint8_t job;
        uint8_t level_value;
        uint32_t play_minutes;
        uint8_t st, ht, dx, iq;
        uint16_t main_part;
        uint8_t change_name;
        uint16_t hair_part;
        uint8_t dummy[4];
        int32_t x, y;
        int32_t addr;
        uint16_t port;
        uint8_t skill_group;
    };

    struct packet_login_success4 {
        uint8_t header_value;
        simple_player_information simple_player_information_value[PLAYER_PER_ACCOUNT4];
        uint32_t guild_id[PLAYER_PER_ACCOUNT4];
        char guild_name[PLAYER_PER_ACCOUNT4][GUILD_NAME_MAX_LEN + 1];
        uint32_t handle;
        uint32_t random_key;
    };
    using packet_gc_login_success4 = packet_login_success4;

    enum { LOGIN_STATUS_MAX_LEN = 8 };
    struct packet_login_failure {
        uint8_t header_value;
        char status[LOGIN_STATUS_MAX_LEN + 1];
    };
    using packet_gc_login_failure = packet_login_failure;

    struct command_player_create {
        uint8_t header_value;
        uint8_t index_value;
        char name_value[CHARACTER_NAME_MAX_LEN + 1];
        uint16_t job;
        uint8_t shape_value;
        uint8_t con;
        uint8_t int_value;
        uint8_t str;
        uint8_t dex;
    };
    using packet_cg_create_character = command_player_create;

    struct command_player_create_success {
        uint8_t header_value;
        uint8_t account_character_slot;
        simple_player_information simple_player_infomation;
    };
    using packet_gc_player_create_success = command_player_create_success;

    struct command_create_failure {
        uint8_t header_value;
        uint8_t type_value;
    };
    using packet_gc_create_failure = command_create_failure;

    struct command_player_delete {
        uint8_t header_value;
        uint8_t index_value;
        char private_code[PRIVATE_CODE_LENGTH];
    };
    using packet_cg_destroy_character = command_player_delete;

    struct packet_player_delete_success {
        uint8_t header_value;
        uint8_t account_index;
    };
    using packet_gc_destroy_character_success = packet_player_delete_success;

    enum {
        ADD_CHARACTER_STATE_DEAD = (1 << 0),
        ADD_CHARACTER_STATE_SPAWN = (1 << 1),
        ADD_CHARACTER_STATE_GUNGON = (1 << 2),
        ADD_CHARACTER_STATE_KILLER = (1 << 3),
        ADD_CHARACTER_STATE_PARTY = (1 << 4),
    };

    enum pk_modes {
        PK_MODE_PEACE,
        PK_MODE_REVENGE,
        PK_MODE_FREE,
        PK_MODE_PROTECT,
        PK_MODE_GUILD,
        PK_MODE_MAX_NUM,
    };

    // Not CRaceData::PART_MAX_NUM, which does not match the server.
    enum character_equipment_part {
        CHR_EQUIPPART_ARMOR,
        CHR_EQUIPPART_WEAPON,
        CHR_EQUIPPART_HEAD,
        CHR_EQUIPPART_HAIR,

        CHR_EQUIPPART_NUM,
    };

    struct packet_char_additional_info {
        uint8_t header_value;
        uint32_t vid;
        char name_value[CHARACTER_NAME_MAX_LEN + 1];
        uint16_t part_value[CHR_EQUIPPART_NUM];
        uint8_t empire;
        uint32_t guild_id;
        uint32_t level_value;
        short alignment;
        uint8_t pk_mode;
        uint32_t mount_vnum;
    };
    using packet_gc_character_additional_info = packet_char_additional_info;

    struct packet_add_char {
        uint8_t header_value;

        uint32_t vid;

        float angle;
        int32_t x;
        int32_t y;
        int32_t z;

        uint8_t type_value;
        uint16_t race_num;
        uint16_t moving_speed;
        uint8_t attack_speed;

        uint8_t state_flag;
        uint32_t affect_flag[2];
    };
    using packet_gc_character_add = packet_add_char;

    struct packet_update_char {
        uint8_t header_value;
        uint32_t vid;

        uint16_t part_value[CHR_EQUIPPART_NUM];
        uint16_t moving_speed;
        uint8_t attack_speed;

        uint8_t state_flag;
        uint32_t affect_flag[2];

        uint32_t guild_id;
        short alignment;
        uint8_t pk_mode;
        uint32_t mount_vnum;
    };
    using packet_gc_character_update = packet_update_char;

    struct packet_del_char {
        uint8_t header_value;
        uint32_t vid;
    };
    using packet_gc_character_delete = packet_del_char;

    enum chat_type {
        CHAT_TYPE_TALKING,
        CHAT_TYPE_INFO, // item pickups, experience gains and similar
        CHAT_TYPE_NOTICE,
        CHAT_TYPE_PARTY,
        CHAT_TYPE_GUILD,
        CHAT_TYPE_COMMAND,
        CHAT_TYPE_SHOUT,
        CHAT_TYPE_WHISPER, // client-only enum, not shared with the server
        CHAT_TYPE_BIG_NOTICE,
        CHAT_TYPE_MAX_NUM,
    };

    struct packet_chatting {
        uint8_t header_value;
        uint16_t size;
        uint8_t type_value;
        uint32_t vid;
        uint8_t empire;
    };
    using packet_gc_chat = packet_chatting;

    struct packet_whisper { // variable-size packet
        uint8_t header_value;
        uint16_t size;
        uint8_t type_value;
        char name_from[CHARACTER_NAME_MAX_LEN + 1];
    };
    using packet_gc_whisper = packet_whisper;

    struct packet_stun {
        uint8_t header_value;
        uint32_t vid;
    };
    using packet_gc_stun = packet_stun;

    struct packet_dead {
        uint8_t header_value;
        uint32_t vid;
    };
    using packet_gc_dead = packet_dead;

    struct packet_main_character2_empire {
        uint8_t header_value;
        uint32_t vid;
        uint16_t race_num;
        char name_value[CHARACTER_NAME_MAX_LEN + 1];
        int32_t x, y, z;
        uint8_t empire;
        uint8_t skill_group;
    };
    using packet_gc_main_character2_empire = packet_main_character2_empire;

    struct packet_main_character3_bgm {
        enum {
            MUSIC_NAME_MAX_LEN = 24,
        };
        uint8_t header_value;
        uint32_t vid;
        uint16_t race_num;
        char user_name[CHARACTER_NAME_MAX_LEN + 1];
        char bgm_name[MUSIC_NAME_MAX_LEN + 1];
        int32_t x, y, z;
        uint8_t empire;
        uint8_t skill_group;
    };
    using packet_gc_main_character3_bgm = packet_main_character3_bgm;

    struct packet_main_character4_bgm_vol {
        enum {
            MUSIC_NAME_MAX_LEN = 24,
        };
        uint8_t header_value;
        uint32_t vid;
        uint16_t race_num;
        char user_name[CHARACTER_NAME_MAX_LEN + 1];
        char bgm_name[MUSIC_NAME_MAX_LEN + 1];
        float bgm_vol;
        int32_t x, y, z;
        uint8_t empire;
        uint8_t skill_group;
    };
    using packet_gc_main_character4_bgm_vol = packet_main_character4_bgm_vol;

    enum point_types {
        POINT_NONE,        // 0
        POINT_LEVEL,       // 1
        POINT_VOICE,       // 2
        POINT_EXP,         // 3
        POINT_NEXT_EXP,    // 4
        POINT_HP,          // 5
        POINT_MAX_HP,      // 6
        POINT_SP,          // 7
        POINT_MAX_SP,      // 8
        POINT_STAMINA,     // 9  stamina
        POINT_MAX_STAMINA, // 10 max stamina

        POINT_GOLD,            // 11
        POINT_ST,              // 12 strength
        POINT_HT,              // 13 vitality
        POINT_DX,              // 14 dexterity
        POINT_IQ,              // 15 intelligence
        POINT_ATT_POWER,       // 16 attack
        POINT_ATT_SPEED,       // 17 attack speed
        POINT_EVADE_RATE,      // 18 evasion
        POINT_MOV_SPEED,       // 19 movement speed
        POINT_DEF_GRADE,       // 20 defence grade
        POINT_CASTING_SPEED,   // 21 casting speed (cooldown*100) / (100 + this) = final cooldown
        POINT_MAGIC_ATT_GRADE, // 22 magic attack
        POINT_MAGIC_DEF_GRADE, // 23 magic defence
        POINT_EMPIRE_POINT,    // 24 empire points
        POINT_LEVEL_STEP,      // 25 step within a level.. (reward at 1 2 3, level up at 4)
        POINT_STAT,            // 26 stat points to spend
        POINT_SUB_SKILL,       // 27 sub-skill points
        POINT_SKILL,           // 28 active skill points
        POINT_MIN_ATK,         // 29 minimum damage
        POINT_MAX_ATK,         // 30 maximum damage
        POINT_PLAYTIME,        // 31 play time
        POINT_HP_REGEN,        // 32 HP regeneration
        POINT_SP_REGEN,        // 33 SP regeneration

        POINT_BOW_DISTANCE, // 34 bow range bonus (meter)

        POINT_HP_RECOVERY, // 35 HP recovery bonus
        POINT_SP_RECOVERY, // 36 SP recovery bonus

        POINT_POISON_PCT,    // 37 poison chance
        POINT_STUN_PCT,      // 38 stun chance
        POINT_SLOW_PCT,      // 39 slow chance
        POINT_CRITICAL_PCT,  // 40 critical chance
        POINT_PENETRATE_PCT, // 41 piercing hit chance
        POINT_CURSE_PCT,     // 42 curse chance

        POINT_ATTBONUS_HUMAN,   // 43 strong against humans
        POINT_ATTBONUS_ANIMAL,  // 44 % more damage against animals
        POINT_ATTBONUS_ORC,     // 45 % more damage against orcs
        POINT_ATTBONUS_MILGYO,  // 46 % more damage against the esoteric
        POINT_ATTBONUS_UNDEAD,  // 47 % more damage against undead
        POINT_ATTBONUS_DEVIL,   // 48 % more damage against devils
        POINT_ATTBONUS_INSECT,  // 49 insects
        POINT_ATTBONUS_FIRE,    // 50 fire creatures
        POINT_ATTBONUS_ICE,     // 51 ice creatures
        POINT_ATTBONUS_DESERT,  // 52 desert creatures
        POINT_ATTBONUS_UNUSED0, // 53 UNUSED0
        POINT_ATTBONUS_UNUSED1, // 54 UNUSED1
        POINT_ATTBONUS_UNUSED2, // 55 UNUSED2
        POINT_ATTBONUS_UNUSED3, // 56 UNUSED3
        POINT_ATTBONUS_UNUSED4, // 57 UNUSED4
        POINT_ATTBONUS_UNUSED5, // 58 UNUSED5
        POINT_ATTBONUS_UNUSED6, // 59 UNUSED6
        POINT_ATTBONUS_UNUSED7, // 60 UNUSED7
        POINT_ATTBONUS_UNUSED8, // 61 UNUSED8
        POINT_ATTBONUS_UNUSED9, // 62 UNUSED9

        POINT_STEAL_HP, // 63 HP absorption
        POINT_STEAL_SP, // 64 SP absorption

        POINT_MANA_BURN_PCT, // 65 mana burn

        POINT_DAMAGE_SP_RECOVER, // 66 chance to recover SP when attacked

        POINT_BLOCK, // 67 block rate
        POINT_DODGE, // 68 dodge rate

        POINT_RESIST_SWORD,   // 69
        POINT_RESIST_TWOHAND, // 70
        POINT_RESIST_DAGGER,  // 71
        POINT_RESIST_BELL,    // 72
        POINT_RESIST_FAN,     // 73
        POINT_RESIST_BOW,     // 74  arrow resistance   : less damage
        POINT_RESIST_FIRE,    // 75  fire resistance    : less damage from fire attacks
        POINT_RESIST_ELEC,    // 76  lightning resistance : less damage from lightning attacks
        POINT_RESIST_MAGIC,   // 77  magic resistance   : less damage from all magic
        POINT_RESIST_WIND,    // 78  wind resistance    : less damage from wind attacks

        POINT_REFLECT_MELEE, // 79 attack reflection

        POINT_REFLECT_CURSE, // 80 curse reflection
        POINT_POISON_REDUCE, // 81 less poison damage

        POINT_KILL_SP_RECOVER,   // 82 MP recovery on killing an enemy
        POINT_EXP_DOUBLE_BONUS,  // 83
        POINT_GOLD_DOUBLE_BONUS, // 84
        POINT_ITEM_DROP_BONUS,   // 85

        POINT_POTION_BONUS,    // 86
        POINT_KILL_HP_RECOVER, // 87

        POINT_IMMUNE_STUN, // 88
        POINT_IMMUNE_SLOW, // 89
        POINT_IMMUNE_FALL, // 90

        POINT_PARTY_ATT_GRADE, // 91
        POINT_PARTY_DEF_GRADE, // 92

        POINT_ATT_BONUS, // 93
        POINT_DEF_BONUS, // 94

        POINT_ATT_GRADE_BONUS,       // 95
        POINT_DEF_GRADE_BONUS,       // 96
        POINT_MAGIC_ATT_GRADE_BONUS, // 97
        POINT_MAGIC_DEF_GRADE_BONUS, // 98

        POINT_RESIST_NORMAL_DAMAGE, // 99

        POINT_STAT_RESET_COUNT = 112,
        POINT_HORSE_SKILL = 113,

        POINT_MALL_ATTBONUS,  // 114 attack +x%
        POINT_MALL_DEFBONUS,  // 115 defence +x%
        POINT_MALL_EXPBONUS,  // 116 experience +x%
        POINT_MALL_ITEMBONUS, // 117 item drop rate x/10
        POINT_MALL_GOLDBONUS, // 118 gold drop rate x/10
        POINT_MAX_HP_PCT,     // 119 max HP +x%
        POINT_MAX_SP_PCT,     // 120 max SP +x%

        POINT_SKILL_DAMAGE_BONUS,      // 121 skill damage *(100+x)%
        POINT_NORMAL_HIT_DAMAGE_BONUS, // 122 normal hit damage *(100+x)%

        POINT_SKILL_DEFEND_BONUS,      // 123 skill damage defence
        POINT_NORMAL_HIT_DEFEND_BONUS, // 124 normal hit damage defence
        POINT_PC_BANG_EXP_BONUS,       // 125
        POINT_PC_BANG_DROP_BONUS,      // 126 PC bang drop rate bonus

        POINT_ENERGY = 128, // 128 energy

        // The energy end time is sent as a point because the task bar UI
        // cannot read affects and the client does not track blend affects.
        POINT_ENERGY_END_TIME = 129, // 129 energy end time

        // Client-only points, never sent by the server.
        POINT_MIN_WEP = 200,
        POINT_MAX_WEP,
        POINT_MIN_MAGIC_WEP,
        POINT_MAX_MAGIC_WEP,
        POINT_HIT_RATE,
    };

    struct packet_points {
        uint8_t header_value;
        int32_t points[POINT_MAX_NUM];
    };
    using packet_gc_points = packet_points;

    struct packet_point_change {
        int header_value;

        uint32_t vid;
        uint8_t type_value;

        int32_t amount; // the change
        int32_t value;  // the current value
    };
    using packet_gc_point_change = packet_point_change;

    struct packet_set_item {
        uint8_t header_value;
        item_pos cell;
        uint32_t vnum;
        uint8_t count;
        int32_t sockets[ITEM_SOCKET_SLOT_MAX_NUM];
        player_item_attribute attr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
    };
    using packet_gc_item_set = packet_set_item;

    struct packet_set_item2 {
        uint8_t header_value;
        item_pos cell;
        uint32_t vnum;
        uint8_t count;
        uint32_t flags_value;
        uint32_t anti_flags;
        bool highlight;
        int32_t sockets[ITEM_SOCKET_SLOT_MAX_NUM];
        player_item_attribute attr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
    };
    using packet_gc_item_set2 = packet_set_item2;

    struct packet_item_del {
        uint8_t header_value;
        uint8_t pos;
    };
    using packet_gc_item_del = packet_item_del;

    struct packet_update_item {
        uint8_t header_value;
        item_pos cell;
        uint8_t count;
        int32_t sockets[ITEM_SOCKET_SLOT_MAX_NUM];
        player_item_attribute attr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
    };
    using packet_gc_item_update = packet_update_item;

    struct packet_ground_add_item {
        uint8_t header_value;
        int32_t x;
        int32_t y;
        int32_t z;

        uint32_t vid;
        uint32_t vnum;
        uint32_t count; // the amount for yang
    };
    using packet_gc_item_ground_add = packet_ground_add_item;

    struct packet_ground_del_item {
        uint8_t header_value;
        uint32_t vid;
    };
    using packet_gc_item_ground_del = packet_ground_del_item;

    struct packet_item_ownership {
        uint8_t header_value;
        uint32_t vid;
        char name_value[CHARACTER_NAME_MAX_LEN + 1];
    };
    using packet_gc_item_ownership = packet_item_ownership;

    struct packet_quickslot_add {
        uint8_t header_value;
        uint8_t pos;
        quick_slot slot_value;
    };
    using packet_gc_quick_slot_add = packet_quickslot_add;

    struct packet_quickslot_del {
        uint8_t header_value;
        uint8_t pos;
    };
    using packet_gc_quick_slot_del = packet_quickslot_del;

    struct packet_quickslot_swap {
        uint8_t header_value;
        uint8_t pos;
        uint8_t change_pos;
    };
    using packet_gc_quick_slot_swap = packet_quickslot_swap;

    struct packet_shop_start {
        uint32_t owner_vid;
        struct packet_shop_item items[SHOP_HOST_ITEM_MAX_NUM];
    };
    using packet_gc_shop_start = packet_shop_start;

    struct packet_shop_start_ex { // followed by the sub_packet_shop_tab entries
        struct sub_packet_shop_tab {
            char name_value[SHOP_TAB_NAME_MAX];
            uint8_t coin_type;
            packet_shop_item items[SHOP_HOST_ITEM_MAX_NUM];
        };
        uint32_t owner_vid;
        uint8_t shop_tab_count;
    };
    using packet_gc_shop_start_ex = packet_shop_start_ex;

    struct packet_shop_update_item {
        uint8_t pos;
        struct packet_shop_item item_value;
    };
    using packet_gc_shop_update_item = packet_shop_update_item;

    enum packet_shop_sub_headers {
        SHOP_SUBHEADER_GC_START,
        SHOP_SUBHEADER_GC_END,
        SHOP_SUBHEADER_GC_UPDATE_ITEM,
        SHOP_SUBHEADER_GC_UPDATE_PRICE,
        SHOP_SUBHEADER_GC_OK,
        SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY,
        SHOP_SUBHEADER_GC_SOLDOUT,
        SHOP_SUBHEADER_GC_INVENTORY_FULL,
        SHOP_SUBHEADER_GC_INVALID_POS,
        SHOP_SUBHEADER_GC_SOLD_OUT,
        SHOP_SUBHEADER_GC_START_EX,
        SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY_EX,
    };

    struct packet_shop {
        uint8_t header_value;
        uint16_t size;
        uint8_t subheader;
    };
    using packet_gc_shop = packet_shop;

    struct packet_exchange {
        uint8_t header_value;
        uint8_t subheader;
        uint8_t is_me;
        uint32_t arg1;
        item_pos arg2;
        uint32_t arg3;
        int32_t values[ITEM_SOCKET_SLOT_MAX_NUM];
        player_item_attribute attr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
    };
    using packet_gc_exchange = packet_exchange;

    enum {
        EXCHANGE_SUBHEADER_GC_START,    // arg1 == vid
        EXCHANGE_SUBHEADER_GC_ITEM_ADD, // arg1 == vnum  arg2 == pos  arg3 == count
        EXCHANGE_SUBHEADER_GC_ITEM_DEL, // arg1 == pos
        EXCHANGE_SUBHEADER_GC_ELK_ADD,  // arg1 == elk
        EXCHANGE_SUBHEADER_GC_ACCEPT,   // arg1 == accept
        EXCHANGE_SUBHEADER_GC_END,      // arg1 == not used
        EXCHANGE_SUBHEADER_GC_ALREADY,  // arg1 == not used
        EXCHANGE_SUBHEADER_GC_LESS_ELK, // arg1 == not used
    };

    struct packet_position {
        uint8_t header_value;
        uint32_t vid;
        uint8_t position_value;
    };
    using packet_gc_position = packet_position;

    struct packet_ping {
        uint8_t header_value;
    };
    using packet_gc_ping = packet_ping;

    struct packet_pong {
        uint8_t header_value;
    };
    using packet_cg_pong = packet_pong;

    struct packet_script {
        uint8_t header_value;
        uint16_t size;
        uint8_t skin_value;
        uint16_t src_size;
    };
    using packet_gc_script = packet_script;

    struct packet_target {
        uint8_t header_value;
        uint32_t vid;
        uint8_t hp_percent;
    };
    using packet_gc_target = packet_target;

    struct packet_damage_info {
        uint8_t header_value;
        uint32_t vid;
        uint8_t flag;
        int damage;
    };
    using packet_gc_damage_info = packet_damage_info;

    struct packet_move {
        uint8_t header_value;
        uint8_t func;
        uint8_t arg_value;
        uint8_t rot;
        uint32_t vid;
        int32_t x;
        int32_t y;
        uint32_t time;
        uint32_t duration;
    };
    using packet_gc_move = packet_move;

    enum {
        QUEST_SEND_IS_BEGIN = 1 << 0,
        QUEST_SEND_TITLE = 1 << 1,      // up to 28 characters
        QUEST_SEND_CLOCK_NAME = 1 << 2, // up to 16 characters
        QUEST_SEND_CLOCK_VALUE = 1 << 3,
        QUEST_SEND_COUNTER_NAME = 1 << 4, // up to 16 characters
        QUEST_SEND_COUNTER_VALUE = 1 << 5,
        QUEST_SEND_ICON_FILE = 1 << 6, // up to 24 characters
        QUEST_SEND_STATUS = 1 << 7,    // python_quest::status
        QUEST_SEND_LETTER = 1 << 8,    // a click opens the letter (when button), not the quest (when info)
    };

    struct packet_quest_info {
        uint8_t header_value;
        uint16_t size;
        uint16_t index_value;
        uint16_t flag;
    };
    using packet_gc_quest_info = packet_quest_info;

    struct packet_quest_confirm {
        uint8_t header_value;
        char msg[64 + 1];
        int32_t timeout;
        uint32_t request_pid;
    };
    using packet_gc_quest_confirm = packet_quest_confirm;

    struct packet_c2c {
        uint8_t header_value;
        uint16_t size;
    };
    using packet_gcc2c = packet_c2c;

    struct packetd_sync_position_element {
        uint32_t vid;
        int32_t x;
        int32_t y;
    };
    using packet_gc_sync_position_element = packetd_sync_position_element;

    struct packetd_sync_position {
        uint8_t header_value;
        uint16_t size;
    };
    using packet_gc_sync_position = packetd_sync_position;

    struct packet_ownership {
        uint8_t header_value;
        uint32_t owner_vid;
        uint32_t victim_vid;
    };
    using packet_gc_ownership = packet_ownership;

#define SKILL_MAX_NUM 255

    struct player_skill {
        uint8_t master_type;
        uint8_t level_value;
        int32_t next_read; // 32-bit on the wire (the server's wire_time_t)
    };

    struct packet_skill_level_new {
        uint8_t header_value;
        player_skill skills[SKILL_MAX_NUM];
    };
    using packet_gc_skill_level_new = packet_skill_level_new;

    struct packet_fly {
        uint8_t header_value;
        uint8_t type_value;
        uint32_t start_vid;
        uint32_t end_vid;
    };
    using packet_gc_create_fly = packet_fly;

    enum pvp_modes {
        PVP_MODE_NONE,
        PVP_MODE_AGREE,
        PVP_MODE_FIGHT,
        PVP_MODE_REVENGE,
    };

    struct packet_duel_start {
        uint8_t header_value;
        uint16_t size; // followed by (size - sizeof(packet_duel_start)) / 4 uint32_t VIDs
    };
    using packet_gc_duel_start = packet_duel_start;

    struct packet_pvp {
        uint8_t header_value;
        uint32_t vid_src;
        uint32_t vid_dst;
        uint8_t mode_value;
    };
    using packet_gcpvp = packet_pvp;

    struct packet_warp {
        uint8_t header_value;
        int32_t x;
        int32_t y;
        int32_t addr;
        uint16_t port;
    };
    using packet_gc_warp = packet_warp;

    struct packet_party_invite {
        uint8_t header_value;
        uint32_t leader_pid;
    };
    using packet_gc_party_invite = packet_party_invite;

    struct packet_party_add {
        uint8_t header_value;
        uint32_t pid;
        char name_value[CHARACTER_NAME_MAX_LEN + 1];
    };
    using packet_gc_party_add = packet_party_add;

    struct packet_party_update {
        uint8_t header_value;
        uint32_t pid;
        uint8_t state_value;
        uint8_t percent_hp;
        short affects_value[PARTY_AFFECT_SLOT_MAX_NUM];
    };
    using packet_gc_party_update = packet_party_update;

    struct packet_party_remove {
        uint8_t header_value;
        uint32_t pid;
    };
    using packet_gc_party_remove = packet_party_remove;

    using packet_gc_safebox_checkout = packet_cg_safebox_checkout;
    using packet_gc_safebox_checkin = packet_cg_safebox_checkin;

    struct packet_safebox_wrong_password {
        uint8_t header_value;
    };
    using packet_gc_safebox_wrong_password = packet_safebox_wrong_password;

    struct packet_safebox_size {
        uint8_t header_value;
        uint8_t size;
    };
    using packet_gc_safebox_size = packet_safebox_size;

    struct command_empire {
        uint8_t header_value;
        uint8_t empire;
    };
    using packet_cg_empire = command_empire;

    struct packet_empire {
        uint8_t header_value;
        uint8_t empire;
    };
    using packet_gc_empire = packet_empire;

    enum {
        FISHING_SUBHEADER_GC_START,
        FISHING_SUBHEADER_GC_STOP,
        FISHING_SUBHEADER_GC_REACT,
        FISHING_SUBHEADER_GC_SUCCESS,
        FISHING_SUBHEADER_GC_FAIL,
        FISHING_SUBHEADER_GC_FISH,
    };

    struct packet_fishing {
        uint8_t header_value;
        uint8_t subheader;
        uint32_t info_value;
        uint8_t dir_value;
    };
    using packet_gc_fishing = packet_fishing;

    struct paryt_parameter {
        uint8_t header_value;
        uint8_t distribute_mode;
    };
    using packet_gc_party_parameter = paryt_parameter;

    enum {
        GUILD_SUBHEADER_GC_LOGIN,
        GUILD_SUBHEADER_GC_LOGOUT,
        GUILD_SUBHEADER_GC_LIST,
        GUILD_SUBHEADER_GC_GRADE,
        GUILD_SUBHEADER_GC_ADD,
        GUILD_SUBHEADER_GC_REMOVE,
        GUILD_SUBHEADER_GC_GRADE_NAME,
        GUILD_SUBHEADER_GC_GRADE_AUTH,
        GUILD_SUBHEADER_GC_INFO,
        GUILD_SUBHEADER_GC_COMMENTS,
        GUILD_SUBHEADER_GC_CHANGE_EXP,
        GUILD_SUBHEADER_GC_CHANGE_MEMBER_GRADE,
        GUILD_SUBHEADER_GC_SKILL_INFO,
        GUILD_SUBHEADER_GC_CHANGE_MEMBER_GENERAL,
        GUILD_SUBHEADER_GC_GUILD_INVITE,
        GUILD_SUBHEADER_GC_WAR,
        GUILD_SUBHEADER_GC_GUILD_NAME,
        GUILD_SUBHEADER_GC_GUILD_WAR_LIST,
        GUILD_SUBHEADER_GC_GUILD_WAR_END_LIST,
        GUILD_SUBHEADER_GC_WAR_POINT,
        GUILD_SUBHEADER_GC_MONEY_CHANGE,
    };

    struct packet_guild {
        uint8_t header_value;
        uint16_t size;
        uint8_t subheader;
    };
    using packet_gc_guild = packet_guild;

    enum {
        GUILD_AUTH_ADD_MEMBER = (1 << 0),
        GUILD_AUTH_REMOVE_MEMBER = (1 << 1),
        GUILD_AUTH_NOTICE = (1 << 2),
        GUILD_AUTH_SKILL = (1 << 3),
    };

    struct packet_guild_sub_grade {
        char grade_name[GUILD_GRADE_NAME_MAX_LEN + 1];
        uint8_t auth_flag;
    };
    using packet_gc_guild_sub_grade = packet_guild_sub_grade;

    struct packet_guild_sub_member {
        uint32_t pid;
        uint8_t grade;
        uint8_t is_general;
        uint8_t job;
        uint8_t level_value;
        uint32_t offer;
        uint8_t name_flag;
    };
    using packet_gc_guild_sub_member = packet_guild_sub_member;

    struct packet_guild_sub_info {
        uint16_t member_count;
        uint16_t max_member_count;
        uint32_t guild_id;
        uint32_t master_pid;
        uint32_t exp;
        uint8_t level_value;
        char name_value[GUILD_NAME_MAX_LEN + 1];
        uint32_t gold;
        uint8_t has_land;
    };
    using packet_gc_guild_info = packet_guild_sub_info;

    enum guild_war_state {
        GUILD_WAR_NONE,
        GUILD_WAR_SEND_DECLARE,
        GUILD_WAR_REFUSE,
        GUILD_WAR_RECV_DECLARE,
        GUILD_WAR_WAIT_START,
        GUILD_WAR_CANCEL,
        GUILD_WAR_ON_WAR,
        GUILD_WAR_END,

        GUILD_WAR_DURATION = 2 * 60 * 60, // 2 hours
    };

    struct packet_guild_war {
        uint32_t guild_self;
        uint32_t guild_opp;
        uint8_t type_value;
        uint8_t war_state;
    };
    using packet_gc_guild_war = packet_guild_war;

    struct packet_guild_war_point {
        uint32_t gain_guild_id;
        uint32_t opponent_guild_id;
        int32_t point;
    };

    enum {
        DUNGEON_SUBHEADER_GC_TIME_ATTACK_START = 0,
        DUNGEON_SUBHEADER_GC_DESTINATION_POSITION = 1,
    };

    struct packet_dungeon {
        uint8_t header_value;
        uint16_t size;
        uint8_t subheader;
    };
    using packet_gc_dungeon = packet_dungeon;

    struct packet_gc_shop_sign {
        uint8_t header_value;
        uint32_t vid;
        char sign[SHOP_SIGN_MAX_LEN + 1];
    };

    struct packet_gc_time {
        uint8_t header_value;
        int32_t time; // 32-bit on the wire (the server's wire_time_t)
    };

    enum {
        WALKMODE_RUN,
        WALKMODE_WALK,
    };

    struct packet_gc_walk_mode {
        uint8_t header_value;
        uint32_t vid;
        uint8_t mode_value;
    };

    struct packet_gc_change_skill_group {
        uint8_t header_value;
        uint8_t skill_group;
    };

    struct refine_material {
        uint32_t vnum;
        uint32_t count;
    };

    struct refine_table {
        uint32_t src_vnum;
        uint32_t result_vnum;
        uint8_t material_count;
        int cost;
        int prob;
        refine_material materials[REFINE_MATERIAL_MAX_NUM];
    };

    struct packet_gc_refine_information_new {
        uint8_t header_value;
        uint8_t type_value;
        uint8_t pos;
        refine_table refine_table_value;
    };

    enum special_effect {
        SE_NONE,
        SE_HPUP_RED,
        SE_SPUP_BLUE,
        SE_SPEEDUP_GREEN,
        SE_DXUP_PURPLE,
        SE_CRITICAL,
        SE_PENETRATE,
        SE_BLOCK,
        SE_DODGE,
        SE_CHINA_FIREWORK,
        SE_SPIN_TOP,
        SE_SUCCESS,
        SE_FAIL,
        SE_FR_SUCCESS,
        SE_LEVELUP_ON_14_FOR_GERMANY,    // level up at 14 (Germany only)
        SE_LEVELUP_UNDER_15_FOR_GERMANY, // level up at 15 (Germany only)
        SE_PERCENT_DAMAGE1,
        SE_PERCENT_DAMAGE2,
        SE_PERCENT_DAMAGE3,
        SE_AUTO_HPUP,
        SE_AUTO_SPUP,
        SE_EQUIP_RAMADAN_RING,    // effect when the crescent ring is put on
        SE_EQUIP_HALLOWEEN_CANDY, // effect when the Halloween candy is put on
        SE_EQUIP_HAPPINESS_RING,  // effect when the Christmas ring of happiness is put on
        SE_EQUIP_LOVE_PENDANT,    // effect when the Valentine love pendant (71145) is put on (a one-shot effect, not a lasting one)
    };

    struct packet_gc_special_effect {
        uint8_t header_value;
        uint8_t type_value;
        uint32_t vid;
    };

    struct packet_gcnpc_position {
        uint8_t header_value;
        uint16_t size;
        uint16_t count;
    };

    struct npc_position {
        uint8_t type_value;
        char name_value[CHARACTER_NAME_MAX_LEN + 1];
        int32_t x;
        int32_t y;
    };

    struct packet_gc_change_name {
        uint8_t header_value;
        uint32_t pid;
        char name_value[CHARACTER_NAME_MAX_LEN + 1];
    };

    enum block_action {
        BLOCK_EXCHANGE = (1 << 0),
        BLOCK_PARTY_INVITE = (1 << 1),
        BLOCK_GUILD_INVITE = (1 << 2),
        BLOCK_WHISPER = (1 << 3),
        BLOCK_MESSENGER_INVITE = (1 << 4),
        BLOCK_PARTY_REQUEST = (1 << 5),
    };

    struct packet_auth_success {
        uint8_t header_value;
        uint32_t login_key;
        uint8_t result;
    };
    using packet_gc_auth_success = packet_auth_success;

    struct packet_channel {
        uint8_t header_value;
        uint8_t channel_value;
    };
    using packet_gc_channel = packet_channel;

    struct equipment_item_set {
        uint32_t vnum;
        uint8_t count;
        int32_t sockets[ITEM_SOCKET_SLOT_MAX_NUM];
        player_item_attribute attr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
    };

    struct pakcet_view_equip {
        uint8_t header_value;
        uint32_t vid;
        equipment_item_set equips[VIEW_EQUIP_SLOT_NUM];
    };
    using packet_gc_view_equip = pakcet_view_equip;

    struct land_packet_element {
        uint32_t id;
        int32_t x, y;
        int32_t width, height;
        uint32_t guild_id;
    };

    struct packet_land_list {
        uint8_t header_value;
        uint16_t size;
    };
    using packet_gc_land_list = packet_land_list;

    enum {
        CREATE_TARGET_TYPE_NONE,
        CREATE_TARGET_TYPE_LOCATION,
        CREATE_TARGET_TYPE_CHARACTER,
    };

    struct packet_gc_target_create_new {
        uint8_t header_value;
        int32_t id;
        char target_name[32 + 1];
        uint32_t vid;
        uint8_t type_value;
    };

    struct packet_gc_target_update {
        uint8_t header_value;
        int32_t id;
        int32_t x, y;
    };

    struct packet_gc_target_delete {
        uint8_t header_value;
        int32_t id;
    };

    struct packet_affect_element {
        uint32_t type_value;
        uint8_t point_idx_apply_on;
        int32_t apply_value;
        uint32_t flag;
        int32_t duration;
        int32_t sp_cost;
    };

    struct packet_gc_affect_add {
        uint8_t header_value;
        packet_affect_element elem;
    };

    struct packet_gc_affect_remove {
        uint8_t header_value;
        uint32_t type_value;
        uint8_t apply_on;
    };

    struct packet_mall_open {
        uint8_t header_value;
        uint8_t size;
    };
    using packet_gc_mall_open = packet_mall_open;

    struct packet_lover_info {
        uint8_t header_value;
        char name_value[CHARACTER_NAME_MAX_LEN + 1];
        uint8_t love_point;
    };
    using packet_gc_lover_info = packet_lover_info;

    struct packet_love_point_update {
        uint8_t header_value;
        uint8_t love_point;
    };
    using packet_gc_love_point_update = packet_love_point_update;

    struct packet_dig_motion {
        uint8_t header_value;
        uint32_t vid;
        uint32_t target_vid;
        uint8_t count;
    };
    using packet_gc_dig_motion = packet_dig_motion;

    // Used in both directions; only the data differs.

    struct packet_key_agreement {
        static const int MAX_DATA_LEN = 256;
        uint8_t header_value;
        uint16_t agreed_length;
        uint16_t data_length;
        uint8_t data_value[MAX_DATA_LEN];
    };

    struct packet_key_agreement_completed {
        uint8_t header_value;
        uint8_t data_value[3]; // unused
    };

    struct packet_gc_specific_effect {
        uint8_t header_value;
        uint32_t vid;
        char effect_file[128];
    };

    enum dragon_soul_refine_window_refine_type {
        DragonSoulRefineWindow_UPGRADE,
        DragonSoulRefineWindow_IMPROVEMENT,
        DragonSoulRefineWindow_REFINE,
    };

    enum packet_cg_dragon_soul_sub_header_type {
        DS_SUB_HEADER_OPEN,
        DS_SUB_HEADER_CLOSE,
        DS_SUB_HEADER_DO_UPGRADE,
        DS_SUB_HEADER_DO_IMPROVEMENT,
        DS_SUB_HEADER_DO_REFINE,
        DS_SUB_HEADER_REFINE_FAIL,
        DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE,
        DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL,
        DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY,
        DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL,
        DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL,
        DS_SUB_HEADER_REFINE_SUCCEED,
    };

    struct packet_cg_dragon_soul_refine {
        packet_cg_dragon_soul_refine() : header_value(HEADER_CG_DRAGON_SOUL_REFINE) {}
        uint8_t header_value;
        uint8_t sub_type;
        item_pos item_grid[DS_REFINE_WINDOW_MAX_NUM];
    };

    struct packet_gc_dragon_soul_refine {
        packet_gc_dragon_soul_refine() : header_value(HEADER_GC_DRAGON_SOUL_REFINE) {}
        uint8_t header_value;
        uint8_t sub_type;
        item_pos pos;
    };

    struct channel_status {
        short port;
        uint8_t status;
    };

#pragma pack(pop)

    // Wire sizes; the server pins the same packets, a change here breaks the protocol.
    static_assert(sizeof(packet_cg_mark_login) == 9);
    static_assert(sizeof(packet_cg_mark_upload) == 773);
    static_assert(sizeof(packet_cg_mark_idx_list) == 1);
    static_assert(sizeof(packet_cg_mark_crc_list) == 322);
    static_assert(sizeof(packet_gc_mark_idx_list) == 7);
    static_assert(sizeof(packet_gc_mark_block) == 10);
    static_assert(sizeof(packet_cg_symbol_upload) == 7);
    static_assert(sizeof(packet_cg_symbol_crc) == 13);
    static_assert(sizeof(packet_gc_guild_symbol_data) == 7);
    static_assert(sizeof(packet_cg_login) == 49);
    static_assert(sizeof(packet_cg_login2) == 52);
    static_assert(sizeof(packet_cg_login3) == 65);
    static_assert(sizeof(packet_cg_select_character) == 2);
    static_assert(sizeof(packet_cg_attack) == 8);
    static_assert(sizeof(packet_cg_chat) == 4);
    static_assert(sizeof(packet_cg_whisper) == 28);
    static_assert(sizeof(packet_cg_enter_front_game) == 1);
    static_assert(sizeof(packet_cg_item_use) == 4);
    static_assert(sizeof(packet_cg_item_use_to_item) == 7);
    static_assert(sizeof(packet_cg_item_drop) == 8);
    static_assert(sizeof(packet_cg_item_drop2) == 9);
    static_assert(sizeof(packet_cg_item_move) == 8);
    static_assert(sizeof(packet_cg_item_pick_up) == 5);
    static_assert(sizeof(packet_cg_quick_slot_add) == 4);
    static_assert(sizeof(packet_cg_quick_slot_del) == 2);
    static_assert(sizeof(packet_cg_quick_slot_swap) == 3);
    static_assert(sizeof(packet_cg_on_click) == 5);
    static_assert(sizeof(packet_cg_shop) == 2);
    static_assert(sizeof(packet_cg_exchange) == 10);
    static_assert(sizeof(packet_cg_script_answer) == 2);
    static_assert(sizeof(packet_cg_script_button) == 5);
    static_assert(sizeof(packet_cg_target) == 5);
    static_assert(sizeof(packet_cg_move) == 16);
    static_assert(sizeof(packet_cg_sync_position_element) == 12);
    static_assert(sizeof(packet_cg_sync_position) == 3);
    static_assert(sizeof(packet_cg_fly_targeting) == 13);
    static_assert(sizeof(packet_gc_fly_targeting) == 17);
    static_assert(sizeof(packet_cg_shoot) == 2);
    static_assert(sizeof(packet_gc_messenger) == 4);
    static_assert(sizeof(packet_gc_messenger_list_offline) == 2);
    static_assert(sizeof(packet_gc_messenger_list_online) == 2);
    static_assert(sizeof(packet_gc_messenger_login) == 1);
    static_assert(sizeof(packet_gc_messenger_logout) == 1);
    static_assert(sizeof(packet_cg_messenger) == 2);
    static_assert(sizeof(packet_cg_safebox_checkout) == 5);
    static_assert(sizeof(packet_cg_safebox_checkin) == 5);
    static_assert(sizeof(packet_cg_mall_checkout) == 5);
    static_assert(sizeof(packet_cg_use_skill) == 9);
    static_assert(sizeof(packet_cg_party_invite) == 5);
    static_assert(sizeof(packet_cg_party_invite_answer) == 6);
    static_assert(sizeof(packet_cg_party_remove) == 5);
    static_assert(sizeof(packet_cg_party_set_state) == 7);
    static_assert(sizeof(packet_gc_party_link) == 9);
    static_assert(sizeof(packet_gc_party_unlink) == 9);
    static_assert(sizeof(packet_cg_party_use_skill) == 6);
    static_assert(sizeof(packet_cg_guild) == 2);
    static_assert(sizeof(packet_cg_answer_make_guild) == 18);
    static_assert(sizeof(packet_gc_request_make_guild) == 5);
    static_assert(sizeof(packet_cg_give_item) == 9);
    static_assert(sizeof(packet_cg_hack) == 257);
    static_assert(sizeof(shop_item_table) == 13);
    static_assert(sizeof(packet_cg_my_shop) == 35);
    static_assert(sizeof(packet_cg_refine) == 3);
    static_assert(sizeof(packet_cg_change_name) == 27);
    static_assert(sizeof(packet_cg_client_version2) == 67);
    static_assert(sizeof(packet_cg_party_parameter) == 2);
    static_assert(sizeof(packet_cg_quest_input_string) == 66);
    static_assert(sizeof(packet_cg_quest_confirm) == 6);
    static_assert(sizeof(packet_cg_script_select_item) == 5);
    static_assert(sizeof(packet_gc_phase) == 2);
    static_assert(sizeof(packet_gc_blank_dynamic) == 3);
    static_assert(sizeof(packet_gc_handshake) == 13);
    static_assert(sizeof(dynamic_size_packet_header) == 3);
    static_assert(sizeof(simple_player_information) == 63);
    static_assert(sizeof(packet_gc_login_success4) == 329);
    static_assert(sizeof(packet_gc_login_failure) == 10);
    static_assert(sizeof(packet_cg_create_character) == 34);
    static_assert(sizeof(packet_gc_player_create_success) == 65);
    static_assert(sizeof(packet_gc_create_failure) == 2);
    static_assert(sizeof(packet_cg_destroy_character) == 10);
    static_assert(sizeof(packet_gc_destroy_character_success) == 2);
    static_assert(sizeof(packet_gc_character_additional_info) == 54);
    static_assert(sizeof(packet_gc_character_add) == 36);
    static_assert(sizeof(packet_gc_character_update) == 36);
    static_assert(sizeof(packet_gc_character_delete) == 5);
    static_assert(sizeof(packet_gc_chat) == 9);
    static_assert(sizeof(packet_gc_stun) == 5);
    static_assert(sizeof(packet_gc_dead) == 5);
    static_assert(sizeof(packet_gc_main_character2_empire) == 46);
    static_assert(sizeof(packet_gc_main_character3_bgm) == 71);
    static_assert(sizeof(packet_gc_main_character4_bgm_vol) == 75);
    static_assert(sizeof(packet_gc_points) == 1021);
    static_assert(sizeof(packet_gc_point_change) == 17);
    static_assert(sizeof(packet_gc_item_set) == 42);
    static_assert(sizeof(packet_gc_item_set2) == 51);
    static_assert(sizeof(packet_gc_item_del) == 2);
    static_assert(sizeof(packet_gc_item_update) == 38);
    static_assert(sizeof(packet_gc_item_ground_add) == 25);
    static_assert(sizeof(packet_gc_item_ground_del) == 5);
    static_assert(sizeof(packet_gc_item_ownership) == 30);
    static_assert(sizeof(packet_gc_quick_slot_add) == 4);
    static_assert(sizeof(packet_gc_quick_slot_del) == 2);
    static_assert(sizeof(packet_gc_quick_slot_swap) == 3);
    static_assert(sizeof(packet_gc_shop_start) == 1724);
    static_assert(sizeof(packet_gc_shop_update_item) == 44);
    static_assert(sizeof(packet_gc_shop) == 4);
    static_assert(sizeof(packet_gc_exchange) == 47);
    static_assert(sizeof(packet_gc_position) == 6);
    static_assert(sizeof(packet_gc_ping) == 1);
    static_assert(sizeof(packet_cg_pong) == 1);
    static_assert(sizeof(packet_gc_script) == 6);
    static_assert(sizeof(packet_gc_target) == 6);
    static_assert(sizeof(packet_gc_damage_info) == 10);
    static_assert(sizeof(packet_gc_move) == 24);
    static_assert(sizeof(packet_gc_quest_info) == 7);
    static_assert(sizeof(packet_gc_quest_confirm) == 74);
    static_assert(sizeof(packet_gcc2c) == 3);
    static_assert(sizeof(packet_gc_sync_position_element) == 12);
    static_assert(sizeof(packet_gc_sync_position) == 3);
    static_assert(sizeof(packet_gc_ownership) == 9);
    static_assert(sizeof(player_skill) == 6);
    static_assert(sizeof(packet_gc_skill_level_new) == 1531);
    static_assert(sizeof(packet_gc_create_fly) == 10);
    static_assert(sizeof(packet_gc_duel_start) == 3);
    static_assert(sizeof(packet_gcpvp) == 10);
    static_assert(sizeof(packet_gc_warp) == 15);
    static_assert(sizeof(packet_gc_party_invite) == 5);
    static_assert(sizeof(packet_gc_party_add) == 30);
    static_assert(sizeof(packet_gc_party_update) == 21);
    static_assert(sizeof(packet_gc_party_remove) == 5);
    static_assert(sizeof(packet_gc_safebox_wrong_password) == 1);
    static_assert(sizeof(packet_gc_safebox_size) == 2);
    static_assert(sizeof(packet_cg_empire) == 2);
    static_assert(sizeof(packet_gc_empire) == 2);
    static_assert(sizeof(packet_gc_fishing) == 7);
    static_assert(sizeof(packet_gc_party_parameter) == 2);
    static_assert(sizeof(packet_gc_guild) == 4);
    static_assert(sizeof(packet_gc_guild_sub_grade) == 10);
    static_assert(sizeof(packet_gc_guild_sub_member) == 13);
    static_assert(sizeof(packet_gc_guild_info) == 35);
    static_assert(sizeof(packet_gc_guild_war) == 10);
    static_assert(sizeof(packet_guild_war_point) == 12);
    static_assert(sizeof(packet_gc_dungeon) == 4);
    static_assert(sizeof(packet_gc_shop_sign) == 38);
    static_assert(sizeof(packet_gc_time) == 5);
    static_assert(sizeof(packet_gc_walk_mode) == 6);
    static_assert(sizeof(packet_gc_change_skill_group) == 2);
    static_assert(sizeof(refine_table) == 57);
    static_assert(sizeof(packet_gc_refine_information_new) == 60);
    static_assert(sizeof(packet_gc_special_effect) == 6);
    static_assert(sizeof(packet_gcnpc_position) == 5);
    static_assert(sizeof(packet_gc_change_name) == 30);
    static_assert(sizeof(packet_gc_auth_success) == 6);
    static_assert(sizeof(packet_gc_channel) == 2);
    static_assert(sizeof(equipment_item_set) == 38);
    static_assert(sizeof(packet_gc_view_equip) == 1221);
    static_assert(sizeof(land_packet_element) == 24);
    static_assert(sizeof(packet_gc_land_list) == 3);
    static_assert(sizeof(packet_gc_target_create_new) == 43);
    static_assert(sizeof(packet_gc_target_update) == 13);
    static_assert(sizeof(packet_gc_target_delete) == 5);
    static_assert(sizeof(packet_affect_element) == 21);
    static_assert(sizeof(packet_gc_affect_add) == 22);
    static_assert(sizeof(packet_gc_affect_remove) == 6);
    static_assert(sizeof(packet_gc_mall_open) == 2);
    static_assert(sizeof(packet_gc_lover_info) == 27);
    static_assert(sizeof(packet_gc_love_point_update) == 2);
    static_assert(sizeof(packet_gc_dig_motion) == 10);
    static_assert(sizeof(packet_gc_specific_effect) == 133);
    static_assert(sizeof(packet_cg_dragon_soul_refine) == 47);
    static_assert(sizeof(packet_gc_dragon_soul_refine) == 5);
    static_assert(sizeof(channel_status) == 3);
    static_assert(sizeof(refine_material) == 8);
    static_assert(sizeof(npc_position) == 34);
    static_assert(sizeof(packet_key_agreement) == 261);
    static_assert(sizeof(packet_key_agreement_completed) == 4);
}
