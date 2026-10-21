#pragma once

#include <metin2/client/characters/instance_base.h>

namespace metin2::inline client::inline characters {
    class python_character_manager;
}

namespace metin2::inline client::inline protocol {

    struct network_actor_data {
        std::string name_value;

        affect_flag_container affect_flags;

        uint8_t type_value;
        uint32_t vid;
        uint32_t state_flags;
        uint32_t empire_id;
        uint32_t race;
        uint32_t mov_spd;
        uint32_t atk_spd;
        float rot;
        int32_t cur_x;
        int32_t cur_y;
        int32_t src_x;
        int32_t src_y;
        int32_t dst_x;
        int32_t dst_y;

        uint32_t server_src_time;
        uint32_t client_src_time;
        uint32_t duration;

        uint32_t armor;
        uint32_t weapon;
        uint32_t hair_value;

        uint32_t owner_vid;

        short alignment;
        uint8_t pk_mode;
        uint32_t mount_vnum;

        uint32_t guild_id;
        uint32_t level_value;

        network_actor_data();

        void set_dst_position(uint32_t server_time_value, int32_t dst_x_value, int32_t dst_y_value, uint32_t duration_value);
        void set_position(int32_t pos_x, int32_t pos_y);
        void update_position();

        network_actor_data(const network_actor_data &src);
        void operator=(const network_actor_data &src);
        void copy_impl(const network_actor_data &src);
    };

    struct network_move_actor_data {
        uint32_t vid;
        uint32_t time;
        int32_t pos_x;
        int32_t pos_y;
        float rot;
        uint32_t func;
        uint32_t arg_value;
        uint32_t duration;

        network_move_actor_data() {
            vid = 0;
            time = 0;
            rot = 0.0f;
            pos_x = 0;
            pos_y = 0;
            func = 0;
            arg_value = 0;
            duration = 0;
        }
    };

    struct network_update_actor_data {
        uint32_t vid;
        uint32_t guild_id;
        uint32_t armor;
        uint32_t weapon;
        uint32_t hair_value;
        uint32_t mov_spd;
        uint32_t atk_spd;
        short alignment;
        uint8_t pk_mode;
        uint32_t mount_vnum;
        uint32_t state_flags; // only used on creation
        affect_flag_container affect_flags;

        network_update_actor_data() {
            guild_id = 0;
            vid = 0;
            armor = 0;
            weapon = 0;
            hair_value = 0;
            mov_spd = 0;
            atk_spd = 0;
            alignment = 0;
            pk_mode = 0;
            mount_vnum = 0;
            state_flags = 0;
            affect_flags.clear();
        }
    };

    class network_actor_manager : public reference_object {
      public:
        network_actor_manager();
        ~network_actor_manager() override;

        void destroy();

        void set_main_actor_vid(uint32_t vid);

        void remove_actor(uint32_t vid);
        void append_actor(const network_actor_data &net_actor_data_value);
        void update_actor(const network_update_actor_data &net_update_actor_data);
        void move_actor(const network_move_actor_data &net_move_actor_data);

        void sync_actor(uint32_t vid, int32_t pos_x, int32_t pos_y);
        void set_actor_owner(uint32_t owner_vid, uint32_t victim_vid);

        void update();

      protected:
        void old_update();

        void update_main_actor();

        auto is_visible_pos(int32_t pos_x, int32_t pos_y) -> bool;
        auto is_visible_actor(const network_actor_data &net_actor_data_value) -> bool;
        auto is_main_actor_vid(uint32_t vid) -> bool;

        void remove_all_ground_items();
        void remove_dynamic_actors();
        void remove_character_manager_actor(network_actor_data &net_actor_data_value);

        auto append_character_manager_actor(network_actor_data &net_actor_data_value) -> instance_base *;
        auto find_actor(network_actor_data &net_actor_data_value) -> instance_base *;
        auto find_actor(network_actor_data &net_actor_data_value, int32_t dst_x, int32_t dst_y) -> instance_base *;

        auto character_manager() -> python_character_manager &;

      protected:
        uint32_t main_vid_;

        int32_t main_pos_x_;
        int32_t main_pos_y_;

        std::map<uint32_t, network_actor_data> net_actor_dict_;
    };
}
