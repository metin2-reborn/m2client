#pragma once

#include <metin2/models/thing_instance.h>
#include <metin2/trees/speed_tree_wrapper.h>
#include "dungeon_block.h"
#include "map_type.h"

#include <string_view>

namespace metin2::inline world::inline terrain {
    class map_outdoor;
}
namespace metin2::inline effects {
    class effect_instance;
}

namespace metin2::inline world::inline maps {
    class property;

    class area {
      public:
        enum {
            ROTATION_STEP_COUNT = 24,
            ROTATION_STEP_AMOUNT = 360 / ROTATION_STEP_COUNT,
            YAW_STEP_COUNT = 24,
            YAW_STEP_AMOUNT = 360 / YAW_STEP_COUNT,
            PITCH_STEP_COUNT = YAW_STEP_COUNT,
            PITCH_STEP_AMOUNT = YAW_STEP_AMOUNT,
        };

        using object_position = math::vector3;

        struct object_data {
            object_position position_value;
            uint32_t crc;
            uint8_t portal_id[PORTAL_ID_MAX_NUM];

            float yaw;
            float pitch;
            float roll;
            float height_bias;

            uint32_t range;
            float max_volume_area_percentage;

            void initialize_rotation();
        };

        struct object_data_comp {
            auto operator()(object_data &lhs, object_data &rhs) const -> bool {
                return lhs.crc < rhs.crc;
            }
        };

        using object_data_vector = std::vector<object_data>;
        using object_data_iterator = object_data_vector::iterator;

        struct ambience_instance : public screen {
            float fx, fy, fz;
            uint32_t range;
            float max_volume_area_percentage;
            int play_sound_index;
            float next_play_time;
            property_types::property_ambience ambience_data;

            ambience_instance();
            ~ambience_instance() override = default;
            void update_impl(float fx_center, float fy_center, float fz_center);
            void (ambience_instance::*update_fn_value)(float x, float y, float z);
            void update_once_sound(float fx_center, float fy_center, float fz_center);
            void update_step_sound(float fx_center, float fy_center, float fz_center);
            void update_loop_sound(float fx_center, float fy_center, float fz_center);
            void render();
            auto picking() -> bool;
            auto volume_from_distance(float distance) -> float;
        };

        struct object_instance {
            uint32_t type_value;
            attribute_instance *attribute_instance_value;

            speed_tree_wrapper *tree;

            bool is_shadow_flag;
            graphic_thing_instance *thing_instance;

            uint32_t effect_id;
            uint32_t effect_instance_index;

            ambience_instance *ambience_instance_value;

            dungeon_block *dungeon_block_value;

            void clear() {
                if (tree)
                    tree->clear();

                if (thing_instance)
                    thing_instance->clear();

                if (attribute_instance_value)
                    attribute_instance_value->clear();

                if (ambience_instance_value)
                    ambience_instance_value->clear();

                if (dungeon_block_value)
                    dungeon_block_value->clear();

                initialize_impl();
            }

            void set_tree(float x, float y, float z, uint32_t tree_crc, const char *tree_name);

            void initialize_impl() {
                type_value = property_types::PROPERTY_TYPE_NONE;

                effect_id = 0xffffffff;
                effect_instance_index = 0xffffffff;
                is_shadow_flag = false;

                tree = nullptr;
                thing_instance = nullptr;
                attribute_instance_value = nullptr;
                ambience_instance_value = nullptr;
                dungeon_block_value = nullptr;
            }

            object_instance() {
                initialize_impl();
            }
            virtual ~object_instance() = default;
        };

        using tree_instance_vector = std::vector<speed_tree_wrapper *>;
        using thing_instance_vector = std::vector<graphic_thing_instance *>;
        using dungeon_block_instance_vector = std::vector<dungeon_block *>;
        using object_instance_vector = std::vector<object_instance *>;
        using ambience_instance_vector = std::vector<ambience_instance *>;
        using graphic_thing_instance_crc_map = std::map<graphic_thing_instance *, uint32_t>;
        using showing_portal_id_set = std::set<int>;

        struct crc_with_number {
            uint32_t crc;
            uint32_t number;
        };

        using crc_with_number_vector = std::vector<crc_with_number>;

        struct find_if_crc_fn {

            find_if_crc_fn(uint32_t crc_value) { crc = crc_value; }

            auto operator()(crc_with_number &crc_with_number) -> bool {
                if (crc_with_number.crc == crc)
                    return true;
                return false;
            }

            uint32_t crc;
        };

        struct rc_num_comp {
            auto operator()(crc_with_number &lhs, crc_with_number &rhs) const -> bool {
                return lhs.number > rhs.number;
            }
        };

      public:
        area();
        virtual ~area();

        void collect_rendering_object(std::vector<graphic_thing_instance *> &vct_opaque_thing_inst);
        void collect_blend_rendering_object(std::vector<graphic_thing_instance *> &vct_blend_thing_inst);

        void set_map_out_door(map_outdoor *owner_outdoor_map);
        void clear();

        auto load(const char *path_name) -> bool;
        // Parses an area.toml ([asset] type "Area"): the placed objects and the ambience sounds.
        static auto load_config(std::string_view text, std::string_view name, object_data_vector &objects, object_data_vector &ambiences) -> bool;

        auto object_data_count() -> uint32_t;
        auto object_data_pointer(uint32_t index, const object_data **object_data_value) const -> bool;

        auto object_instance_count() const -> uint32_t;
        auto object_instance_pointer(const uint32_t &index, const object_instance **object_instance_value) const -> bool;

        void enable_portal(bool flag = true);
        void clear_portal();
        void add_showing_portal_id(int num);
        void refresh_portal();

        void update();
        void update_around_ambience(float x, float y, float z);

        void render();
        void render_effect();
        void render_collision();
        void render_dungeon();
        void refresh();

        void set_coordinate(const uint16_t &us_coord_x, const uint16_t &us_coord_y);
        void coordinate(uint16_t *us_coord_x, uint16_t *us_coord_y);

        auto debug_get_rendered_crc_num() -> uint32_t;
        auto debug_get_rendered_crc_with_num_vector() -> crc_with_number_vector &;
        auto debug_get_rendered_grapphic_thing_instance_num() -> uint32_t;

        auto owner() -> map_outdoor * { return owner_outdoor_map_; }

      protected:
        auto check_object_index(uint32_t index) const -> bool;

        void load_build_object_instances();

        void update_ani_thing_list();
        void update_effect_list();

        void load_attribute(object_instance *object_instance_value, const char *attribute_file_name);

      protected:
        void clear_destroy_object_instance(object_instance *object_instance_value);

        void set_object_instance(object_instance *object_instance_value, const object_data *data);
        void set_object_instance_set_tree(object_instance *object_instance_value, const object_data *data, const property *property);
        void set_object_instance_set_building(object_instance *object_instance_value, const object_data *data, const property *property);
        void set_object_instance_set_effect(object_instance *object_instance_value, const object_data *data, const property *property);
        void set_object_instance_set_ambience(object_instance *object_instance_value, const object_data *data, const property *property);
        void set_object_instance_set_dungeon_block(object_instance *object_instance_value, const object_data *data, const property *property);

      protected:
        object_data_vector object_data_vector_value_;

        object_instance_vector object_instance_vector_value_;

        tree_instance_vector tree_clone_instace_vector_;
        thing_instance_vector thing_clone_instace_vector_;
        dungeon_block_instance_vector dungeon_block_clone_instance_vector_;
        thing_instance_vector ani_thing_clone_instance_vector_;
        thing_instance_vector shadow_thing_clone_instace_vector_;
        ambience_instance_vector ambience_clone_instance_vector_;
        graphic_thing_instance_crc_map graphic_thing_instance_crc_map_value_;
        crc_with_number_vector rendered_thing_instance_crc_with_number_vector_;
        thing_instance_vector rendered_grapphic_thing_instance_vector_;

        // Area coordinates within the whole seamless map, both 0 to 999.
        uint16_t x_;
        uint16_t y_;

        map_outdoor *owner_outdoor_map_;

      protected:
        static dynamic_pool<object_instance> object_instance_pool_;
        static dynamic_pool<attribute_instance> attribute_instance_pool_;
        static dynamic_pool<ambience_instance> ambience_instance_pool_;
        static dynamic_pool<dungeon_block> dungeon_block_instance_pool_;

        using effect_instance_map = std::map<uint32_t, effect_instance *>;
        using effect_instance_iterator = effect_instance_map::iterator;
        effect_instance_map effect_instance_map_value_;

        bool portal_enable_;
        showing_portal_id_set set_showing_portal_id_;

      public:
        static void destroy_system();

        static auto new_value() -> area *;
        static void delete_value(area *area);

        static dynamic_pool<area> pool_value;
    };
}
