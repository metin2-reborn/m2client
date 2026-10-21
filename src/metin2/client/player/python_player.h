#pragma once

#include <metin2/client/app/abstract_player.h>
#include <metin2/client/protocol/packet.h>
#include <metin2/script/python_header.h>
#include <set>
#include "python_skill.h"

namespace metin2::inline client::inline characters {
    class instance_base;
}

namespace metin2::inline client::inline player {

    // Data and control for the main character (the one the player controls).

    class python_player : public abstract_player {
      public:
        enum {
            CATEGORY_NONE = 0,
            CATEGORY_ACTIVE = 1,
            CATEGORY_PASSIVE = 2,
            CATEGORY_MAX_NUM = 3,

            STATUS_INDEX_ST = 1,
            STATUS_INDEX_DX = 2,
            STATUS_INDEX_IQ = 3,
            STATUS_INDEX_HT = 4,
        };

        enum {
            MBT_LEFT,
            MBT_RIGHT,
            MBT_MIDDLE,
            MBT_NUM,
        };

        enum {
            MBF_SMART,
            MBF_MOVE,
            MBF_CAMERA,
            MBF_ATTACK,
            MBF_SKILL,
            MBF_AUTO,
        };

        enum {
            MBS_CLICK,
            MBS_PRESS,
        };

        enum mode {
            MODE_NONE,
            MODE_CLICK_POSITION,
            MODE_CLICK_ITEM,
            MODE_CLICK_ACTOR,
            MODE_USE_SKILL,
        };

        enum effect {
            EFFECT_PICK,
            EFFECT_NUM,
        };

        enum metin_socket_type {
            METIN_SOCKET_TYPE_NONE,
            METIN_SOCKET_TYPE_SILVER,
            METIN_SOCKET_TYPE_GOLD,
        };

        struct skill_instance {
            uint32_t index_value;
            int type_value;
            int grade;
            int level_value;
            float fcur_efficient_percentage;
            float fnext_efficient_percentage;
            bool is_cool_time;

            float cool_time;      // Used when a skill slot that is cooling down
            float last_used_time; // is put on the quick bar.
            bool active;
        };

        enum key_board_ud {
            KEYBOARD_UD_NONE,
            KEYBOARD_UD_UP,
            KEYBOARD_UD_DOWN,
        };

        enum key_board_lr {
            KEYBOARD_LR_NONE,
            KEYBOARD_LR_LEFT,
            KEYBOARD_LR_RIGHT,
        };

        enum {
            DIR_UP,
            DIR_DOWN,
            DIR_LEFT,
            DIR_RIGHT,
        };

        struct player_status {
            packet_item item_value[c_Inventory_Count];
            packet_item ds_item[c_DragonSoul_Inventory_Count];
            quick_slot quick_slot_value[QUICKSLOT_MAX_NUM];
            skill_instance skill[SKILL_MAX_NUM];
            int32_t point[POINT_MAX_NUM];
            int32_t quick_page_index;

            void set_point(uint32_t e_point, int32_t l_point);
            auto get_point(uint32_t point_value) -> int32_t;
        };

        struct party_member_info {
            party_member_info(uint32_t pid_value, const char *name) : vid(0), pid(pid_value), name_value(name) {}

            uint32_t vid;
            uint32_t pid;
            std::string name_value;
            uint8_t state_value;
            uint8_t hp_percentage;
            short affects_value[PARTY_AFFECT_SLOT_MAX_NUM];
        };

        enum party_role {
            PARTY_ROLE_NORMAL,
            PARTY_ROLE_LEADER,
            PARTY_ROLE_ATTACKER,
            PARTY_ROLE_TANKER,
            PARTY_ROLE_BUFFER,
            PARTY_ROLE_SKILL_MASTER,
            PARTY_ROLE_BERSERKER,
            PARTY_ROLE_DEFENDER,
            PARTY_ROLE_MAX_NUM,
        };

        enum {
            SKILL_NORMAL,
            SKILL_MASTER,
            SKILL_GRAND_MASTER,
            SKILL_PERFECT_MASTER,
        };

        struct auto_potion_info {
            auto_potion_info() : activated(false), current_amount(0), total_amount(0) {}

            bool activated;
            int32_t current_amount;
            int32_t total_amount;
            int32_t inventory_slot_index;
        };

        enum auto_potion_type {
            AUTO_POTION_TYPE_HP = 0,
            AUTO_POTION_TYPE_SP = 1,
            AUTO_POTION_TYPE_NUM
        };

      public:
        python_player(void);
        ~python_player(void);

        void pick_close_item();

        void set_game_window(PyObject *ppy_object);

        void set_observer_mode(bool is_enable) override;
        auto is_observer_mode() -> bool;

        void set_quick_camera_mode(bool is_enable);

        void set_attack_key_state(bool is_press);

        void new_get_main_actor_position(pixel_position *p_pos_actor);

        auto register_effect(uint32_t eid, const char *eft_file_name, bool is_cache) -> bool;

        auto new_set_mouse_state(int mb_type, int mb_state) -> bool;
        auto new_set_mouse_func(int mb_type, int mb_func) -> bool;
        void new_set_mouse_middle_button_state(int mb_state);

        void new_set_auto_camera_rotation_speed(float rot_spd);
        void new_reset_camera_rotation();

        void new_set_single_dir_key_state(int dir_key, bool is_press);
        void new_set_single_dik_key_state(int dik_key, bool is_press);
        void new_set_multi_dir_key_state(bool is_left, bool is_right, bool is_up, bool is_down);

        void new_attack();
        void new_fishing();
        auto new_cancel_fishing() -> bool;

        auto new_move_to_dest_pixel_position_direction(const pixel_position &p_pos_dst) -> bool;
        auto new_move_to_mouse_screen_direction() -> bool;
        auto new_move_to_direction(float dir_rot) -> bool;
        void new_stop();

        auto new_is_empty_reserved_delay_time(float elapsedtime) -> bool;

        void set_dungeon_destination_position(int ix, int iy);
        void alarm_have_to_go();

        auto new_find_actor_ptr(uint32_t vid) -> instance_base *;
        auto new_get_main_actor_ptr() -> instance_base * override;

        void clear();
        void clear_skill_dict();
        void new_clear_skill_data(bool all = false);

        void update();

        auto play_time() -> uint32_t;
        void set_play_time(uint32_t play_time);

        void set_main_character_index(int index) override;

        auto main_character_index() -> uint32_t override;
        auto is_main_character_index(uint32_t index) -> bool override;
        auto guild_id() -> uint32_t;
        void notify_deleting_character_instance(uint32_t vid) override;
        void notify_character_dead(uint32_t vid) override;
        void notify_character_update(uint32_t vid) override;
        void notify_dead_main_character();
        void notify_change_pk_mode() override;

        auto get_name() -> const char * override;
        void set_name(const char *name);

        void set_race(uint32_t race) override;
        auto race() -> uint32_t;

        void set_weapon_power(uint32_t min_power, uint32_t max_power, uint32_t min_magic_power, uint32_t max_magic_power, uint32_t add_power) override;
        void set_status(uint32_t type, int32_t value);
        auto status(uint32_t type) -> int override;

        void set_item_data(item_pos cell, const packet_item &item_inst) override;
        auto get_item_data(item_pos cell) const -> const packet_item *;
        void set_item_count(item_pos cell, uint8_t count) override;
        void set_item_metin_socket(item_pos cell, uint32_t metin_socket_index, uint32_t metin_number) override;
        void set_item_attribute(item_pos cell, uint32_t attr_index, uint8_t type, short value) override;
        auto item_index(item_pos cell) -> uint32_t override;
        auto item_flags(item_pos cell) -> uint32_t override;
        auto item_count(item_pos cell) -> uint32_t override;
        auto item_count_by_vnum(uint32_t vnum) -> uint32_t;
        auto item_metin_socket(item_pos cell, uint32_t metin_socket_index) -> uint32_t;
        void item_attribute(item_pos cell, uint32_t attr_slot_index, uint8_t *type, short *value);
        void send_click_item_packet(uint32_t iid);

        void request_add_local_quick_slot(uint32_t local_slot_index, uint32_t wnd_type, uint32_t wnd_item_pos);
        void request_add_to_empty_local_quick_slot(uint32_t wnd_type, uint32_t wnd_item_pos);
        void request_move_global_quick_slot_to_local_quick_slot(uint32_t global_src_slot_index, uint32_t local_dst_slot_index);
        void request_delete_global_quick_slot(uint32_t global_slot_index);
        void request_use_local_quick_slot(uint32_t local_slot_index);
        auto local_quick_slot_index_to_global_quick_slot_index(uint32_t local_slot_index) -> uint32_t;

        void global_quick_slot_data(uint32_t global_slot_index, uint32_t *wnd_type, uint32_t *wnd_item_pos);
        void local_quick_slot_data(uint32_t slot_pos, uint32_t *wnd_type, uint32_t *wnd_item_pos);

        auto is_belt_inventory_slot(item_pos cell) -> bool;
        auto is_equipment_slot(item_pos slot_index) -> bool;

        auto is_equip_item_in_slot(item_pos slot_index) -> bool override;

        auto quick_page() -> int;
        void set_quick_page(int page_index);
        void add_quick_slot(int quickslot_index, char icon_type, char icon_position) override;
        void delete_quick_slot(int quickslot_index) override;
        void move_quick_slot(int source, int target) override;

        void set_skill(uint32_t slot_index, uint32_t skill_index);
        auto skill_slot_index(uint32_t skill_index, uint32_t *slot_index) -> bool;
        auto skill_index(uint32_t slot_index) -> int;
        auto skill_grade(uint32_t slot_index) -> int;
        auto skill_level(uint32_t slot_index) -> int;
        auto skill_current_efficient_percentage(uint32_t slot_index) -> float;
        auto skill_next_efficient_percentage(uint32_t slot_index) -> float;
        void set_skill_level(uint32_t slot_index, uint32_t skill_level);
        void set_skill_level_impl(uint32_t skill_index, uint32_t skill_grade, uint32_t skill_level);
        auto is_toggle_skill(uint32_t slot_index) -> bool;
        void click_skill_slot(uint32_t slot_index);
        void change_current_skill_number_only(uint32_t slot_index);
        auto find_skill_slot_index_by_skill_index(uint32_t skill_index, uint32_t *skill_slot_index) -> bool;

        void set_skill_cool_time(uint32_t skill_index);
        void end_skill_cool_time(uint32_t skill_index);

        auto skill_cool_time(uint32_t slot_index) -> float;
        auto skill_elapsed_cool_time(uint32_t slot_index) -> float;
        auto is_skill_active(uint32_t slot_index) -> bool;
        auto is_skill_cool_time(uint32_t slot_index) -> bool;
        void use_guild_skill(uint32_t skill_slot_index);
        auto affect_index_to_skill_index(uint32_t affect_index, uint32_t *skill_index) -> bool;

        void set_affect(uint32_t affect);
        void reset_affect(uint32_t affect);
        void clear_affects();

        void set_target(uint32_t vid, bool force_change = true) override;
        void open_character_menu(uint32_t victim_actor_id);
        auto get_target_vid() -> uint32_t;

        void exit_party();
        void append_party_member(uint32_t pid, const char *name);
        void link_party_member(uint32_t pid, uint32_t vid);
        void unlink_party_member(uint32_t pid);
        void update_party_member_info(uint32_t pid, uint8_t state, uint8_t hp_percentage);
        void update_party_member_affect(uint32_t pid, uint8_t affect_slot_index, short affect_number);
        void remove_party_member(uint32_t pid);
        auto is_party_member_by_vid(uint32_t vid) -> bool override;
        auto is_party_member_by_name(const char *name) -> bool;
        auto party_member_ptr(uint32_t pid, party_member_info **party_member_info_value) -> bool;
        auto party_member_pid_to_vid(uint32_t pid, uint32_t *vid) -> bool;
        auto party_member_vid_to_pid(uint32_t vid, uint32_t *pid) -> bool override;
        auto is_same_party_member(uint32_t vid1, uint32_t vid2) -> bool override;

        void remember_challenge_instance(uint32_t vid);
        void remember_revenge_instance(uint32_t vid);
        void remember_cant_fight_instance(uint32_t vid);
        void forget_instance(uint32_t vid);
        auto is_challenge_instance(uint32_t vid) -> bool;
        auto is_revenge_instance(uint32_t vid) -> bool;
        auto is_cant_fight_instance(uint32_t vid) -> bool;

        void open_private_shop();
        void close_private_shop();
        auto is_open_private_shop() -> bool;

        void start_stamina_consume(uint32_t consume_per_sec, uint32_t current_stamina) override;
        void stop_stamina_consume(uint32_t current_stamina) override;

        auto pk_mode() -> uint32_t;

        void set_combo_skill_flag(bool flag) override;

        void set_movable_ground_distance(float distance);

        void act_emotion(uint32_t emotion_id);
        void start_emotion_process() override;
        void end_emotion_process() override;

        auto toggle_cool_time() -> bool;
        auto toggle_level_limit() -> bool;

        __inline auto get_auto_potion_info(int type) const -> const auto_potion_info & { return auto_potion_info_value_[type]; }
        __inline auto get_auto_potion_info(int type) -> auto_potion_info & { return auto_potion_info_value_[type]; }

      protected:
        auto ref_local_quick_slot(int slot_index) -> quick_slot &;
        auto ref_global_quick_slot(int slot_index) -> quick_slot &;

        auto level_atk() -> uint32_t;
        auto stat_atk() -> uint32_t;
        auto weapon_atk(uint32_t weapon_power) -> uint32_t;
        auto total_atk(uint32_t weapon_power, uint32_t refine_bonus) -> uint32_t;
        auto race_stat() -> uint32_t;
        auto hit_rate() -> uint32_t;
        auto evade_rate() -> uint32_t;

        void update_battle_status();

        void deactivate_skill_slot(uint32_t slot_index);
        void activate_skill_slot(uint32_t slot_index);

        void on_press_smart(instance_base &inst_main, bool is_auto);
        void on_click_smart(instance_base &inst_main, bool is_auto);

        void on_press_item(instance_base &inst_main, uint32_t picked_item_id);
        void on_press_actor(instance_base &inst_main, uint32_t picked_actor_id, bool is_auto);
        void on_press_ground(instance_base &inst_main, const pixel_position &p_pos_picked_ground);
        void on_press_screen(instance_base &inst_main);

        void on_click_actor(instance_base &inst_main, uint32_t picked_actor_id, bool is_auto);
        void on_click_item(instance_base &inst_main, uint32_t picked_item_id);
        void on_click_ground(instance_base &inst_main, const pixel_position &p_pos_picked_ground);

        auto is_movable_ground_distance(instance_base &inst_main, const pixel_position &p_pos_picked_ground) -> bool;

        auto picked_actor_ptr(instance_base **inst_picked) -> bool;

        auto picked_actor_id(uint32_t *actor_id) -> bool;
        auto picked_item_id_impl(uint32_t *item_id) -> bool;
        auto picked_ground_pos(pixel_position *p_pos_picked) -> bool;

        void clear_reserved_action();
        void reserve_click_item(uint32_t item_id);
        void reserve_click_actor(uint32_t actor_id);
        void reserve_click_ground(const pixel_position &p_pos_picked_ground);
        void reserve_use_skill(uint32_t actor_id, uint32_t skill_slot_index, uint32_t range);

        void reserve_process_click_actor();

        void show_picked_effect(const pixel_position &p_pos_picked_ground);
        void send_click_actor_packet(instance_base &inst_victim);

        void clear_auto_attack_target_actor_id();
        void set_auto_attack_target_actor_id(uint32_t actor_id);

        void new_show_effect(int eid, pixel_position p_pos_dst);

        void new_set_mouse_smart_state(int mbs, bool is_auto);
        void new_set_mouse_move_state(int mbs);
        void new_set_mouse_camera_state(int mbs);
        void new_get_mouse_dir_rotation(float scr_x, float scr_y, float *dir_rot);
        void new_get_multi_key_dir_rotation(bool is_left, bool is_right, bool is_up, bool is_down, float *dir_rot);

        auto degree_from_direction(int ud, int lr) -> float;
        auto degree_from_position(int ix, int iy, int half_width, int half_height) -> float;

        void new_refresh_mouse_walking_direction();

        auto can_shot(instance_base &inst_main, instance_base &inst_target) -> bool;
        auto can_use_skill_impl() -> bool;

        auto can_move_impl() -> bool;

        auto can_attack_impl() -> bool;

        auto check_skill_usable(uint32_t slot_index) -> bool;
        void use_current_skill();
        auto use_skill(uint32_t slot_index) -> bool;
        auto check_special_skill(uint32_t skill_index) -> bool;

        auto check_rest_skill_cool_time(uint32_t skill_slot_index) -> bool;
        auto check_short_life(skill_instance &skill_inst, python_skill::skill_data &skill_data) -> bool;
        auto check_short_mana(skill_instance &skill_inst, python_skill::skill_data &skill_data) -> bool;
        auto check_short_arrow(skill_instance &skill_inst, python_skill::skill_data &skill_data) -> bool;
        auto check_dash_affect(instance_base &inst_main) -> bool;

        void send_use_skill(uint32_t skill_slot_index, uint32_t target_vid);
        void run_cool_time(uint32_t skill_slot_index);

        auto skill_type(uint32_t skill_slot_index) -> uint8_t;

        auto is_reserved_use_skill(uint32_t skill_slot_index) -> bool;
        auto skill_target_range(python_skill::skill_data &skill_data) -> uint32_t;
        auto is_using_charge_skill() -> bool;

        auto process_enemy_skill_target_range(instance_base &inst_main, instance_base &inst_target, python_skill::skill_data &skill_data, uint32_t skill_slot_index) -> bool;

        auto has_enough_arrow() -> bool;
        auto has_item(uint32_t item_id) -> bool;
        auto pickable_distance() -> uint32_t;

        auto target_actor_ptr() -> instance_base *;
        auto target_vid() -> uint32_t;
        void set_target_vid(uint32_t vid);
        auto is_same_target_vid(uint32_t vid) -> bool;
        auto is_target() -> bool;
        auto change_target_to_picked_instance() -> bool;

        auto alive_target_instance_ptr() -> instance_base *;
        auto dead_target_instance_ptr() -> instance_base *;

        auto is_right_button_skill_mode() -> bool;

        void update_auto_attack();
        void update_notify_guild_area_event();

        auto is_processing_emotion() -> bool;

      protected:
        PyObject *ppy_game_window_;

        std::map<uint32_t, uint32_t> skill_slot_dict_;
        std::string name_value_;
        uint32_t main_character_index_;
        uint32_t race_;
        uint32_t weapon_min_power_;
        uint32_t weapon_max_power_;
        uint32_t weapon_min_magic_power_;
        uint32_t weapon_max_magic_power_;
        uint32_t weapon_add_power_;

        uint32_t sending_target_vid_;
        float target_update_time_;

        uint32_t auto_attack_target_vid_;

        mode reserved_mode_;
        float reserved_delay_time_;

        float mov_dir_rot_;

        bool is_up_;
        bool is_down_;
        bool is_left_;
        bool is_right_;
        bool is_atk_key_;
        bool is_dir_key_;
        bool is_cmr_rot_;
        bool is_smt_mov_;
        bool is_dir_mov_;

        float cmr_rot_spd_;

        player_status player_status_value_;

        uint32_t combo_old_;
        uint32_t vid_reserved_;
        uint32_t iid_reserved_;

        uint32_t dwcur_skill_slot_index_;
        uint32_t skill_slot_index_reserved_;
        uint32_t skill_range_reserved_;

        pixel_position p_pos_inst_prev_;
        pixel_position p_pos_reserved_;

        bool bis_processing_emotion_;

        bool is_dest_position_;
        int ix_dest_pos_;
        int iy_dest_pos_;
        int last_alarm_time_;

        std::map<uint32_t, party_member_info> party_member_map_;

        std::set<uint32_t> challenge_instance_set_;
        std::set<uint32_t> revenge_instance_set_;
        std::set<uint32_t> cant_fight_instance_set_;

        bool is_open_private_shop_;
        bool is_observer_mode_;

        bool is_consuming_stamina_;
        float current_stamina_;
        float consume_stamina_per_sec_;

        uint32_t in_guild_area_id_;

        bool sys_is_cool_time_;
        bool sys_is_level_limit_;

      protected:
        pixel_position moving_cursor_position_;
        float moving_cursor_setting_time_;
        uint32_t effect_value_[EFFECT_NUM];

        uint32_t vid_picked_;
        uint32_t iid_picked_;
        int mbf_button_[MBT_NUM];

        uint32_t target_vid_;
        uint32_t target_end_time_;
        uint32_t play_time_;

        auto_potion_info auto_potion_info_value_[AUTO_POTION_TYPE_NUM];

      protected:
        float movable_ground_distance_;

      private:
        std::map<uint32_t, uint32_t> map_affect_index_to_skill_index_;
    };

    inline auto get_python_player() -> python_player & {
        static python_player instance;
        return instance;
    }

    extern const int c_iFastestSendingCount;
    extern const int c_iSlowestSendingCount;
    extern const float c_fFastestSendingDelay;
    extern const float c_fSlowestSendingDelay;
    extern const float c_fRotatingStepTime;

    extern const float c_fComboDistance;
    extern const float c_fPickupDistance;
    extern const float c_fClickDistance;
}
