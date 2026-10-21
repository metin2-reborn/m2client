#pragma once

#include <metin2/core/func_object.h>
#include <metin2/net/net_packet_header_map.h>
#include <metin2/net/net_stream.h>

#include <metin2/client/social/insult_checker.h>

#include <deque>
#include <metin2/script/python_header.h>
#include "packet.h"

namespace metin2::inline client::inline characters {
    class instance_base;
}

namespace metin2::inline client::inline protocol {

    class network_actor_manager;
    struct network_actor_data;
    struct network_update_actor_data;

    class python_network_stream : public network_stream {
      public:
        enum {
            SERVER_COMMAND_LOG_OUT = 0,
            SERVER_COMMAND_RETURN_TO_SELECT_CHARACTER = 1,
            SERVER_COMMAND_QUIT = 2,

            MAX_ACCOUNT_PLAYER
        };

        enum {
            ERROR_NONE,
            ERROR_UNKNOWN,
            ERROR_CONNECT_MARK_SERVER,
            ERROR_LOAD_MARK,
            ERROR_MARK_WIDTH,
            ERROR_MARK_HEIGHT,

            ERROR_MARK_UPLOAD_NEED_RECONNECT,
            ERROR_MARK_CHECK_NEED_RECONNECT,
        };

        enum {
            ACCOUNT_CHARACTER_SLOT_ID,
            ACCOUNT_CHARACTER_SLOT_NAME,
            ACCOUNT_CHARACTER_SLOT_RACE,
            ACCOUNT_CHARACTER_SLOT_LEVEL,
            ACCOUNT_CHARACTER_SLOT_STR,
            ACCOUNT_CHARACTER_SLOT_DEX,
            ACCOUNT_CHARACTER_SLOT_HTH,
            ACCOUNT_CHARACTER_SLOT_INT,
            ACCOUNT_CHARACTER_SLOT_PLAYTIME,
            ACCOUNT_CHARACTER_SLOT_FORM,
            ACCOUNT_CHARACTER_SLOT_ADDR,
            ACCOUNT_CHARACTER_SLOT_PORT,
            ACCOUNT_CHARACTER_SLOT_GUILD_ID,
            ACCOUNT_CHARACTER_SLOT_GUILD_NAME,
            ACCOUNT_CHARACTER_SLOT_CHANGE_NAME_FLAG,
            ACCOUNT_CHARACTER_SLOT_HAIR,
        };

        enum {
            PHASE_WINDOW_LOGO,
            PHASE_WINDOW_LOGIN,
            PHASE_WINDOW_SELECT,
            PHASE_WINDOW_CREATE,
            PHASE_WINDOW_LOAD,
            PHASE_WINDOW_GAME,
            PHASE_WINDOW_EMPIRE,
            PHASE_WINDOW_NUM,
        };

      public:
        python_network_stream();
        ~python_network_stream() override;

        void start_game();
        void warp(int32_t global_x, int32_t global_y);

        void notify_hack(const char *msg);

        void send_emoticon(uint32_t emoticon);

        void exit_application();
        void exit_game();
        void log_out_game();
        void absolute_exit_game();
        void absolute_exit_application();

        void enable_chat_insult_filter(bool is_enable);
        auto is_chat_insult_in(const char *msg) -> bool;
        auto is_insult_in(const char *msg) -> bool;

        auto guild_id() -> uint32_t;

        auto upload_mark(const char *image_file_name) -> uint32_t;
        auto upload_symbol(const char *image_file_name) -> uint32_t;

        auto load_insult_list(const char *insult_list_file_name) -> bool;

        auto account_character_slot_datau(uint32_t slot, uint32_t type) -> uint32_t;
        auto account_character_slot_dataz(uint32_t slot, uint32_t type) -> const char *;

        auto field_music_file_name() -> const char *;
        auto field_music_volume() -> float;

        auto is_selected_empire() -> bool;

        void toggle_game_debug_info();

        void set_mark_server(const char *addr, uint32_t port);
        void connect_login_server(const char *addr, uint32_t port);
        void connect_game_server(uint32_t chr_slot);

        void set_login_info(const char *id, const char *password);
        void set_login_key(uint32_t login_key);
        void clear_login_info(void);

        void set_handler(PyObject *handler);
        void set_phase_window(uint32_t e_phase_wnd, PyObject *po_phase_wnd);
        void clear_phase_window(uint32_t e_phase_wnd, PyObject *po_phase_wnd);
        void set_server_command_parser_window(PyObject *phase_wnd);

        auto send_sync_position_element_packet(uint32_t victim_vid, uint32_t victim_x, uint32_t victim_y) -> bool;

        auto send_attack_packet(uint32_t mot_attack, uint32_t vid_victim) -> bool;
        auto send_character_state_packet(const pixel_position &p_pos_dst, float dst_rot, uint32_t func, uint32_t arg) -> bool;
        auto send_use_skill_packet(uint32_t skill_index, uint32_t target_vid = 0) -> bool;
        auto send_target_packet(uint32_t vid) -> bool;

        auto send_item_use_packet(item_pos pos) -> bool;
        auto send_item_use_to_item_packet(item_pos source_pos, item_pos target_pos) -> bool;
        auto send_item_drop_packet(item_pos pos, uint32_t elk) -> bool;
        auto send_item_drop_packet_new(item_pos pos, uint32_t elk, uint32_t count) -> bool;
        auto send_item_move_packet(item_pos pos, item_pos change_pos, uint8_t num) -> bool;
        auto send_item_pick_up_packet(uint32_t vid) -> bool;

        auto send_quick_slot_add_packet(uint8_t wpos, uint8_t type, uint8_t pos) -> bool;
        auto send_quick_slot_del_packet(uint8_t wpos) -> bool;
        auto send_quick_slot_move_packet(uint8_t wpos, uint8_t change_pos) -> bool;

        auto send_shop_end_packet() -> bool;
        auto send_shop_buy_packet(uint8_t count) -> bool;
        auto send_shop_sell_packet(uint8_t slot) -> bool;
        auto send_shop_sell_packet_new(uint8_t slot, uint8_t count) -> bool;

        auto send_exchange_start_packet(uint32_t vid) -> bool;
        auto send_exchange_item_add_packet(item_pos item_pos, uint8_t display_pos) -> bool;
        auto send_exchange_elk_add_packet(uint32_t elk) -> bool;
        auto send_exchange_item_del_packet(uint8_t pos) -> bool;
        auto send_exchange_accept_packet() -> bool;
        auto send_exchange_exit_packet() -> bool;

        auto send_script_answer_packet(int answer) -> bool;
        auto send_script_button_packet(unsigned int index) -> bool;
        auto send_answer_make_guild_packet(const char *name) -> bool;
        auto send_quest_input_string_packet(const char *string) -> bool;
        auto send_quest_confirm_packet(uint8_t answer, uint32_t pid) -> bool;

        auto send_on_click_packet(uint32_t vid) -> bool;

        auto send_fly_targeting_packet(uint32_t target_vid, const pixel_position &p_pos_target) -> bool;
        auto send_add_fly_targeting_packet(uint32_t target_vid, const pixel_position &p_pos_target) -> bool;
        auto send_shoot_packet(uint32_t skill) -> bool;

        auto client_command(const char *command) -> bool;
        void server_command(char *command);

        void register_emoticon_string(const char *emoticon_string);

        auto send_party_invite_packet(uint32_t vid) -> bool;
        auto send_party_invite_answer_packet(uint32_t leader_vid, uint8_t accept) -> bool;
        auto send_party_remove_packet(uint32_t pid) -> bool;
        auto send_party_set_state_packet(uint32_t vid, uint8_t state, uint8_t flag) -> bool;
        auto send_party_use_skill_packet(uint8_t skill_index, uint32_t vid) -> bool;
        auto send_party_parameter_packet(uint8_t distribute_mode) -> bool;

        auto send_safe_box_money_packet(uint8_t state, uint32_t money) -> bool;
        auto send_safe_box_checkin_packet(item_pos inventory_pos, uint8_t safe_box_pos) -> bool;
        auto send_safe_box_checkout_packet(uint8_t safe_box_pos, item_pos inventory_pos) -> bool;
        auto send_safe_box_item_move_packet(uint8_t source_pos, uint8_t target_pos, uint8_t count) -> bool;

        auto send_mall_checkout_packet(uint8_t mall_pos, item_pos inventory_pos) -> bool;

        auto send_guild_add_member_packet(uint32_t vid) -> bool;
        auto send_guild_remove_member_packet(uint32_t pid) -> bool;
        auto send_guild_change_grade_name_packet(uint8_t grade_number, const char *name) -> bool;
        auto send_guild_change_grade_authority_packet(uint8_t grade_number, uint8_t authority) -> bool;
        auto send_guild_offer_packet(uint32_t experience) -> bool;
        auto send_guild_post_comment_packet(const char *message) -> bool;
        auto send_guild_delete_comment_packet(uint32_t index) -> bool;
        auto send_guild_refresh_comments_packet(uint32_t highest_index) -> bool;
        auto send_guild_change_member_grade_packet(uint32_t pid, uint8_t grade) -> bool;
        auto send_guild_use_skill_packet(uint32_t skill_id, uint32_t target_vid) -> bool;
        auto send_guild_change_member_general_packet(uint32_t pid, uint8_t flag) -> bool;
        auto send_guild_invite_answer_packet(uint32_t guild_id, uint8_t answer) -> bool;
        auto send_guild_charge_gsp_packet(uint32_t money) -> bool;
        auto send_guild_deposit_money_packet(uint32_t money) -> bool;
        auto send_guild_withdraw_money_packet(uint32_t money) -> bool;

        auto recv_mall_open_packet() -> bool;
        auto recv_mall_item_set_packet() -> bool;
        auto recv_mall_item_del_packet() -> bool;

        auto recv_lover_info_packet() -> bool;
        auto recv_love_point_update_packet() -> bool;

        auto recv_dig_motion_packet() -> bool;

        auto send_fishing_packet(int rotation) -> bool;
        auto send_give_item_packet(uint32_t target_vid, item_pos item_pos, int item_count) -> bool;

        auto send_build_private_shop_packet(const char *name, const std::vector<shop_item_table> &selling_item_stock) -> bool;

        auto send_refine_packet(uint8_t pos, uint8_t type) -> bool;
        auto send_select_item_packet(uint32_t item_pos) -> bool;

        auto send_client_version_packet() -> bool;

        auto send_dragon_soul_refine_packet(uint8_t refine_type, item_pos *pos) -> bool;

        auto recv_handshake_packet() -> bool;
        auto recv_handshake_ok_packet() -> bool;

        auto recv_key_agreement_packet() -> bool;
        auto recv_key_agreement_completed_packet() -> bool;

        auto main_actor_vid() -> uint32_t;
        auto main_actor_race() -> uint32_t;
        auto main_actor_empire() -> uint32_t;
        auto main_actor_skill_group() -> uint32_t;
        void set_empire_id(uint32_t empire_id);
        auto empire_id() -> uint32_t;
        void test_set_skill_group_fake(int index);

      public:
        void set_off_line_phase();
        void set_hand_shake_phase();
        void set_login_phase();
        void set_select_phase();
        void set_loading_phase();
        void set_game_phase();
        void close_phase();

        // Login Phase
        auto send_login_packet(const char *name, const char *password) -> bool;
        auto send_login_packet_new(const char *name, const char *password) -> bool;

        auto send_enter_game() -> bool;

        // Select Phase
        auto send_select_empire_packet(uint32_t empire_id) -> bool;
        auto send_select_character_packet(uint8_t account_index) -> bool;
        auto send_change_name_packet(uint8_t index, const char *name) -> bool;
        auto send_create_character_packet(uint8_t index, const char *name, uint8_t job, uint8_t shape, uint8_t stat1, uint8_t stat2, uint8_t stat3, uint8_t stat4) -> bool;
        auto send_destroy_character_packet(uint8_t index, const char *private_code) -> bool;

        // Main Game Phase
        auto send_chat_packet(const char *chat, uint8_t type = CHAT_TYPE_TALKING) -> bool;
        auto send_whisper_packet(const char *name, const char *chat) -> bool;
        auto send_messenger_add_by_vid_packet(uint32_t vid) -> bool;
        auto send_messenger_add_by_name_packet(const char *name) -> bool;
        auto send_messenger_remove_packet(const char *key, const char *name) -> bool;

      protected:
        auto on_process() -> bool override;
        void off_line_phase();
        void hand_shake_phase();
        void login_phase();
        void select_phase();
        void loading_phase();
        void game_phase();

        auto is_not_ping() -> bool;

        void download_mark();
        void download_symbol(const std::vector<uint32_t> &vec_guild_id);

        void play_inventory_item_use_sound(item_pos slot_pos);
        void play_inventory_item_drop_sound(item_pos slot_pos);
        void play_safe_box_item_drop_sound(uint32_t slot_pos);
        void play_mall_item_drop_sound(uint32_t slot_pos);

        auto can_act_main_instance() -> bool;

        enum refresh_window_type {
            RefreshStatus = (1 << 0),
            RefreshAlignmentWindow = (1 << 1),
            RefreshCharacterWindow = (1 << 2),
            RefreshEquipmentWindow = (1 << 3),
            RefreshInventoryWindow = (1 << 4),
            RefreshExchangeWindow = (1 << 5),
            RefreshSkillWindow = (1 << 6),
            RefreshSafeboxWindow = (1 << 7),
            RefreshMessengerWindow = (1 << 8),
            RefreshGuildWindowInfoPage = (1 << 9),
            RefreshGuildWindowBoardPage = (1 << 10),
            RefreshGuildWindowMemberPage = (1 << 11),
            RefreshGuildWindowMemberPageGradeComboBox = (1 << 12),
            RefreshGuildWindowSkillPage = (1 << 13),
            RefreshGuildWindowGradePage = (1 << 14),
            RefreshTargetBoard = (1 << 15),
            RefreshMallWindow = (1 << 16),
        };

        void refresh_status();
        void refresh_alignment_window();
        void refresh_character_window();
        void refresh_equipment_window();
        void refresh_inventory_window();
        void refresh_exchange_window();
        void refresh_skill_window();
        void refresh_safebox_window();
        void refresh_messenger_window();
        void refresh_guild_window_info_page();
        void refresh_guild_window_board_page();
        void refresh_guild_window_member_page();
        void refresh_guild_window_member_page_grade_combo_box();
        void refresh_guild_window_skill_page();
        void refresh_guild_window_grade_page();
        void refresh_target_board_by_vid(uint32_t vid);
        void refresh_target_board_by_name(const char *name);
        void refresh_target_board();
        void refresh_mall_window();

        auto send_hack(const char *msg) -> bool;

      protected:
        // Common
        auto recv_error_packet(int header) -> bool;
        auto recv_ping_packet() -> bool;
        auto recv_default_packet(int header) -> bool;
        auto recv_phase_packet() -> bool;

        // Login Phase
        auto recv_login_success_packet4() -> bool;
        auto recv_login_failure_packet() -> bool;
        auto recv_empire_packet() -> bool;

        // Select Phase
        auto recv_player_create_success_packet() -> bool;
        auto recv_player_create_failure_packet() -> bool;
        auto recv_player_destroy_success_packet() -> bool;
        auto recv_player_destroy_failure_packet() -> bool;
        auto recv_player_points() -> bool;
        auto recv_change_name() -> bool;

        // Loading Phase
        auto recv_main_character2_empire() -> bool;
        auto recv_main_character3_bgm() -> bool;
        auto recv_main_character4_bgm_vol() -> bool;

        void set_field_music_file_name(const char *music_name);
        void set_field_music_file_info(const char *music_name, float vol);

        // Main Game Phase
        auto recv_warp_packet() -> bool;
        auto recv_pvp_packet() -> bool;
        auto recv_duel_start_packet() -> bool;
        auto recv_character_append_packet() -> bool;
        auto recv_character_additional_info() -> bool;
        auto recv_character_update_packet() -> bool;
        auto recv_character_delete_packet() -> bool;
        auto recv_chat_packet() -> bool;
        auto recv_owner_ship_packet() -> bool;
        auto recv_sync_position_packet() -> bool;
        auto recv_whisper_packet() -> bool;
        auto recv_point_change() -> bool;

        auto recv_stun_packet() -> bool;
        auto recv_dead_packet() -> bool;
        auto recv_character_move_packet() -> bool;

        auto recv_item_set_packet() -> bool;
        auto recv_item_set_packet2() -> bool;
        auto recv_item_update_packet() -> bool;
        auto recv_item_ground_add_packet() -> bool;
        auto recv_item_ground_del_packet() -> bool;
        auto recv_item_ownership() -> bool;

        auto recv_quick_slot_add_packet() -> bool;
        auto recv_quick_slot_del_packet() -> bool;
        auto recv_quick_slot_move_packet() -> bool;

        auto recv_character_position_packet() -> bool;

        auto recv_shop_packet() -> bool;
        auto recv_shop_sign_packet() -> bool;
        auto recv_exchange_packet() -> bool;

        auto recv_script_packet() -> bool;
        auto recv_quest_info_packet() -> bool;
        auto recv_quest_confirm_packet() -> bool;
        auto recv_request_make_guild() -> bool;

        auto recv_skill_level_new() -> bool;

        auto recv_target_packet() -> bool;
        auto recv_view_equip_packet() -> bool;
        auto recv_damage_info_packet() -> bool;

        auto recv_create_fly_packet() -> bool;
        auto recv_fly_targeting_packet() -> bool;
        auto recv_add_fly_targeting_packet() -> bool;

        auto recv_messenger() -> bool;

        auto recv_guild() -> bool;

        auto recv_party_invite() -> bool;
        auto recv_party_add() -> bool;
        auto recv_party_update() -> bool;
        auto recv_party_remove() -> bool;
        auto recv_party_link() -> bool;
        auto recv_party_unlink() -> bool;
        auto recv_party_parameter() -> bool;

        auto recv_safe_box_set_packet() -> bool;
        auto recv_safe_box_del_packet() -> bool;
        auto recv_safe_box_wrong_password_packet() -> bool;
        auto recv_safe_box_size_packet() -> bool;

        auto recv_fishing() -> bool;

        auto recv_dungeon() -> bool;

        auto recv_time_packet() -> bool;

        auto recv_walk_mode_packet() -> bool;

        auto recv_change_skill_group_packet() -> bool;

        auto recv_refine_information_packet_new() -> bool;

        auto recv_special_effect() -> bool;

        auto recv_specific_effect() -> bool;

        auto recv_dragon_soul_refine() -> bool;

        // Minimap
        auto recv_npc_list() -> bool;
        auto recv_land_packet() -> bool;
        auto recv_target_create_packet_new() -> bool;
        auto recv_target_update_packet() -> bool;
        auto recv_target_delete_packet() -> bool;

        auto recv_affect_add_packet() -> bool;
        auto recv_affect_remove_packet() -> bool;

        auto recv_channel_packet() -> bool;

      protected:
        auto parse_emoticon(const char *chat_msg, uint32_t *emoticon) -> bool;

        // These call into the Python handler.
        void on_connect_failure() override;
        void on_script_event_start(int skin, int index);

        void on_remote_disconnect() override;
        void on_disconnect() override;

        void set_game_online();
        void set_game_offline();

      protected:
        auto check_packet(packet_header *ret_header) -> bool;
        void finish_packet(bool is_processing_over);
        // Reads the rest of the current packet as text, truncated to fit and NUL-terminated.
        auto recv_packet_text(char *buffer_value, size_t size) -> bool;
        // Bytes of the current packet not read yet.
        auto packet_bytes_left() -> int;
        // Reads a string of the given length from the current packet (up to its first NUL).
        auto recv_string(int length, std::string &out) -> bool;
        void protocol_error(const char *format, int a, int b = 0);

        void initialize_game_phase();
        void initialize_mark_auth();
        void global_position_to_local_position_impl(int32_t &global_x, int32_t &global_y);
        void local_position_to_global_position_impl(int32_t &local_x, int32_t &local_y);

        auto is_player_attacking() -> bool;
        auto is_equip_item_in_slot_impl(item_pos cell) -> bool;

        void show_map_name(int32_t local_x, int32_t local_y);

        void leave_offline_phase() {}
        void leave_handshake_phase() {}
        void leave_login_phase() {}
        void leave_select_phase() {}
        void leave_loading_phase() {}
        void leave_game_phase();

        void clear_network_actor_manager();

        void clear_select_character_data();

        void recv_character_append_packet_impl(network_actor_data *net_actor_data_value);
        void recv_character_update_packet_impl(network_update_actor_data *net_update_actor_data);

        void filter_insult_impl(char *line, uint32_t line_len);

        void set_guild_id(uint32_t id);

      protected:
        packet_gc_handshake handshake_data_;
        uint32_t changing_phase_time_;
        uint32_t bindup_retry_count_;
        uint32_t main_actor_vid_;
        uint32_t main_actor_race_;
        uint32_t main_actor_empire_;
        uint32_t main_actor_skill_group_;
        bool is_game_online_;
        bool is_start_game_;

        uint32_t guild_id_;
        uint32_t empire_id_;
        uint32_t guild_creation_key_; // from the last guild name request

        struct server_time_sync {
            uint32_t change_server_time;
            uint32_t change_client_time;
        } server_time_sync_value_;

        void server_time_sync_initialize();

        uint32_t last_game_ping_time_;

        std::string id_;
        std::string password_;
        std::string last_command_;
        std::string phase_value_;

        int packet_header_value_ = 0;
        int packet_size_ = 0;
        int packet_rest_after_ = -1;
        bool is_protocol_error_ = false;
        uint32_t login_key_;

        std::string mark_ip_;

        func_object<python_network_stream> phase_process_func_;
        func_object<python_network_stream> phase_leave_func_;

        PyObject *handler_;
        PyObject *apo_phase_wnd_[PHASE_WINDOW_NUM];
        PyObject *ser_command_parser_wnd_;

        simple_player_information simple_player_infos_[PLAYER_PER_ACCOUNT4];
        uint32_t guild_ids_[PLAYER_PER_ACCOUNT4];
        std::string guild_name_[PLAYER_PER_ACCOUNT4];
        bool has_simple_player_info_;

        ref_ptr<network_actor_manager> rok_net_actor_mgr_;

        bool is_refresh_status_;
        bool is_refresh_character_wnd_;
        bool is_refresh_equipment_wnd_;
        bool is_refresh_inventory_wnd_;
        bool is_refresh_exchange_wnd_;
        bool is_refresh_skill_wnd_;
        bool is_refresh_safebox_wnd_;
        bool is_refresh_mall_wnd_;
        bool is_refresh_messenger_wnd_;
        bool is_refresh_guild_wnd_info_page_;
        bool is_refresh_guild_wnd_board_page_;
        bool is_refresh_guild_wnd_member_page_;
        bool is_refresh_guild_wnd_member_page_grade_combo_box_;
        bool is_refresh_guild_wnd_skill_page_;
        bool is_refresh_guild_wnd_grade_page_;

        std::vector<std::string> emoticon_string_vector_;

        struct mark_auth {
            network_address net_addr;
            uint32_t handle;
            uint32_t random_key;
        } mark_auth_value_;

        uint32_t selected_character_index_;

        insult_checker insult_checker_value_;

        bool is_enable_chat_insult_filter_;
        bool combo_skill_flag_;

        std::deque<std::string> que_hack_;

      private:
        struct direct_enter_mode {
            bool is_set;
            uint32_t chr_slot_index;
        } direct_enter_mode_value_;

        void direct_enter_mode_initialize();
        void direct_enter_mode_set(uint32_t chr_slot_index);
        auto direct_enter_mode_is_set() -> bool;

      public:
      private:
        struct betting_guild_war {
            uint32_t betting_money;
            uint32_t observer_count;
        } betting_guild_war_value_;

        instance_base *inst_target_;

        void betting_guild_war_initialize();
    };

    inline auto get_python_network_stream() -> python_network_stream & {
        static python_network_stream instance;
        return instance;
    }
}
