#pragma once

#include "game_type.h"

namespace metin2::inline client::inline characters {
    class instance_base;
}

namespace metin2::inline client::inline app {

    class abstract_player {
      public:
        abstract_player() = default;
        virtual ~abstract_player() = default;

        virtual auto main_character_index() -> uint32_t = 0;
        virtual void set_main_character_index(int index) = 0;
        virtual auto is_main_character_index(uint32_t index) -> bool = 0;

        virtual auto status(uint32_t type) -> int = 0;

        virtual auto get_name() -> const char * = 0;

        virtual void set_race(uint32_t race) = 0;

        virtual void start_stamina_consume(uint32_t consume_per_sec, uint32_t current_stamina) = 0;
        virtual void stop_stamina_consume(uint32_t current_stamina) = 0;

        virtual auto is_party_member_by_vid(uint32_t vid) -> bool = 0;
        virtual auto party_member_vid_to_pid(uint32_t vid, uint32_t *pid) -> bool = 0;
        virtual auto is_same_party_member(uint32_t vid1, uint32_t vid2) -> bool = 0;

        virtual void set_item_data(item_pos item_pos, const packet_item &item_inst) = 0;
        virtual void set_item_count(item_pos item_pos, uint8_t count) = 0;
        virtual void set_item_metin_socket(item_pos item_pos, uint32_t metin_socket_index, uint32_t metin_number) = 0;
        virtual void set_item_attribute(item_pos item_pos, uint32_t attr_index, uint8_t type, short value) = 0;

        virtual auto item_index(item_pos item_pos) -> uint32_t = 0;
        virtual auto item_flags(item_pos item_pos) -> uint32_t = 0;
        virtual auto item_count(item_pos item_pos) -> uint32_t = 0;

        virtual auto is_equip_item_in_slot(item_pos item_pos) -> bool = 0;

        virtual void add_quick_slot(int quickslot_index, char icon_type, char icon_position) = 0;
        virtual void delete_quick_slot(int quickslot_index) = 0;
        virtual void move_quick_slot(int source, int target) = 0;

        virtual void set_weapon_power(uint32_t min_power, uint32_t max_power, uint32_t min_magic_power, uint32_t max_magic_power, uint32_t add_power) = 0;

        virtual void set_target(uint32_t vid, bool force_change = true) = 0;
        virtual void notify_character_update(uint32_t vid) = 0;
        virtual void notify_character_dead(uint32_t vid) = 0;
        virtual void notify_deleting_character_instance(uint32_t vid) = 0;
        virtual void notify_change_pk_mode() = 0;

        virtual void set_observer_mode(bool is_enable) = 0;
        virtual void set_mobile_flag(bool flag) = 0;
        virtual void set_combo_skill_flag(bool flag) = 0;

        virtual void start_emotion_process() = 0;
        virtual void end_emotion_process() = 0;

        virtual auto new_get_main_actor_ptr() -> instance_base * = 0;
    };

    auto get_abstract_player() -> abstract_player &;
}
