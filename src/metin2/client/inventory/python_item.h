#pragma once

// Updates and renders the items dropped on the ground and manages each item's data and icon
// image instances.

#include <metin2/models/thing_instance.h>
#include <metin2/world/maps/map_type.h>

namespace metin2::inline world::inline items {
    class item_data;
}

namespace metin2::inline client::inline inventory {

    class python_item {
      public:
        enum {
            INVALID_ID = 0xffffffff,
        };

        enum {
            VNUM_MONEY = 1,
        };

        enum {
            USESOUND_NONE,
            USESOUND_DEFAULT,
            USESOUND_ARMOR,
            USESOUND_WEAPON,
            USESOUND_BOW,
            USESOUND_ACCESSORY,
            USESOUND_POTION,
            USESOUND_PORTAL,
            USESOUND_NUM,
        };

        enum {
            DROPSOUND_DEFAULT,
            DROPSOUND_ARMOR,
            DROPSOUND_WEAPON,
            DROPSOUND_BOW,
            DROPSOUND_ACCESSORY,
            DROPSOUND_NUM
        };

        struct ground_item_instance {
            uint32_t virtual_number;
            math::vector3 end_position;

            math::vector3 rotation_axis;
            math::quaternion end;
            math::vector3 center;
            graphic_thing_instance thing_instance;
            uint32_t start_time;
            uint32_t end_time;

            uint32_t drop_sound_type;

            bool anim_ended;
            auto update() -> bool;
            void clear();

            uint32_t effect_instance_index;
            std::string ownership;

            static void play_drop_sound_impl(uint32_t item_type, const math::vector3 &pos);
            static std::string drop_sound_file_name[DROPSOUND_NUM];

            ground_item_instance() = default;
            virtual ~ground_item_instance() = default;
        };

        using ground_item_instance_map = std::map<uint32_t, ground_item_instance *>;

      public:
        python_item(void);
        ~python_item(void);

        void destroy();
        void create();

        void play_use_sound(uint32_t item_id);
        void play_drop_sound(uint32_t item_id);
        void play_use_potion_sound();

        void set_use_sound_file_name(uint32_t item_type, const std::string &file_name);
        void set_drop_sound_file_name(uint32_t item_type, const std::string &file_name);

        void get_info(std::string *info);

        void delete_all_items();

        void render();
        void update(int mouse_x, int mouse_y);

        void create_item(uint32_t virtual_id, uint32_t virtual_number, float x, float y, float z, bool drop = true);
        void delete_item(uint32_t virtual_id);
        void set_ownership(uint32_t vid, const char *name);
        auto ownership(uint32_t vid, const char **name) -> bool;

        auto ground_item_position(uint32_t virtual_id, pixel_position *position) -> bool;

        auto picked_item_id(uint32_t *picked_item_id) -> bool;

        auto close_item(const pixel_position &pixel_position, uint32_t *item_id, uint32_t distance = 300) -> bool;

        auto virtual_number_of_ground_item(uint32_t vid) -> uint32_t;

      protected:
        auto pick_impl(int mouse_x, int mouse_y) -> uint32_t;

        auto use_sound_type(const item_data &item_data) -> uint32_t;
        auto drop_sound_type(const item_data &item_data) -> uint32_t;

      protected:
        ground_item_instance_map ground_item_instance_map_value_;
        dynamic_pool<ground_item_instance> ground_item_instance_pool_;

        uint32_t drop_item_effect_id_;
        uint32_t picked_item_id_;

        int mouse_x_;
        int mouse_y_;

        std::string use_sound_file_name_[USESOUND_NUM];
    };

    inline auto get_python_item() -> python_item & {
        static python_item instance;
        return instance;
    }
}
