#pragma once

#include <metin2/models/thing.h>
#include "game_type.h"

namespace metin2::inline graphics {
    class attribute_data;
}

namespace metin2::inline world::inline actors {

    class race_motion_data;

#define COMBO_KEY                               uint32_t
#define MAKE_COMBO_KEY(motion_mode, combo_type) ((uint32_t(motion_mode) << 16) | (uint32_t(combo_type)))
#define COMBO_KEY_GET_MOTION_MODE(key)          (uint16_t(uint32_t(key) >> 16 & 0xFFFF))
#define COMBO_KEY_GET_COMBO_TYPE(key)           (uint16_t(uint32_t(key) & 0xFFFF))

    class race_data {
      public:
        enum parts {
            // Indices are shared with the server; keep ECharacterEquipmentPart in sync.
            // Adding a part changes the packet size, so coordinate with the server side first.
            PART_MAIN,
            PART_WEAPON,
            PART_HEAD,
            PART_WEAPON_LEFT,
            PART_HAIR,

            PART_MAX_NUM,
        };

        enum {
            SMOKE_NUM = 4,
        };

        using graphic_thing_map = std::map<uint16_t, graphic_thing *>;
        using attaching_bone_name_map = std::map<uint32_t, std::string>;

        struct motion {
            uint8_t percentage;
            graphic_thing *motion_value;
            race_motion_data *motion_data;
        };
        using motion_vector = std::vector<motion>;
        using motion_vector_map = std::map<uint16_t, motion_vector>;

        struct motion_mode_data {
            uint16_t motion_mode_index;

            motion_vector_map motion_vector_map_value;

            motion_mode_data() = default;
            virtual ~motion_mode_data() = default;
        };
        using motion_mode_data_map = std::map<uint16_t, motion_mode_data *>;
        using motion_mode_data_iterator = motion_mode_data_map::iterator;

        struct model_data {
            race_types::attaching_data_vector attaching_data_vector_value;
        };
        using model_data_map = std::map<uint32_t, model_data>;
        using model_data_map_iterator = model_data_map::iterator;

        using motion_data_map = std::map<uint32_t, race_motion_data *>;

        using combo_index_vector = std::vector<uint32_t>;
        struct combo_attack_data {
            combo_index_vector combo_index_vector_value;
        };
        using combo_data = combo_attack_data;
        using normal_attack_index_map = std::map<uint32_t, uint32_t>;
        using combo_attack_data_map = std::map<COMBO_KEY, combo_data>;
        using combo_attack_data_iterator = combo_attack_data_map::iterator;

        struct skin {
            int part_value;

            std::string src_file_name;
            std::string dst_file_name;

            skin() {
                part_value = 0;
            }
            skin(const skin &skin) {
                copy(skin);
            }
            void operator=(const skin &skin) {
                copy(skin);
            }
            void copy(const skin &skin) {
                part_value = skin.part_value;
                src_file_name = skin.src_file_name;
                dst_file_name = skin.dst_file_name;
            }
        };

        struct hair {
            std::string model_file_name;
            std::vector<skin> vct_skin;
        };

        struct shape {
            std::string model_file_name;
            std::vector<skin> vct_skin;
        };

        // A [[motion]] of the race script, registered in the general mode when the race loads.
        struct motion_entry {
            uint16_t index;
            std::string file_name;
            uint8_t percentage;
        };

      public:
        static auto new_value() -> race_data *;
        static void delete_value(race_data *race_data);
        static void create_system(uint32_t capacity, uint32_t mot_mode_capacity);
        static void destroy_system();

      public:
        race_data();
        virtual ~race_data();

        void destroy();

        auto motion_list() const -> const std::vector<motion_entry> & { return motion_list_; }
        auto base_model_thing() -> graphic_thing *;
        auto lod_model_thing() -> graphic_thing *;
        auto attribute_data_ptr() -> attribute_data *;
        auto attaching_bone_name(uint32_t part_index, const char **bone_name) -> bool;
        auto create_motion_mode_iterator(motion_mode_data_iterator &itor) -> bool;
        auto next_motion_mode_iterator(motion_mode_data_iterator &itor) -> bool;

        auto motion_key(uint16_t motion_mode_index, uint16_t motion_index, MOTION_KEY *motion_key) -> bool;

        auto motion_mode_data_pointer(uint16_t motion_mode, motion_mode_data **motion_mode_data_value) -> bool;
        auto motion_vector_pointer(uint16_t motion_mode, uint16_t motion_index, const motion_vector **motion_vector_value) -> bool;
        auto motion_data_pointer(uint16_t motion_mode, uint16_t motion_index, uint16_t motion_sub_index, race_motion_data **motion_data) -> bool;
        auto motion_data_pointer(uint32_t motion_key, race_motion_data **motion_data) -> bool;

        auto attaching_data_count() -> uint32_t;
        auto attaching_data_pointer(uint32_t index, const race_types::attaching_data **attaching_data) -> bool;
        auto collision_data_pointer(uint32_t index, const race_types::attaching_data **attaching_data) -> bool;

        auto is_tree() -> bool;
        auto tree_file_name() -> const char *;

        // Set up from the race script (TOML, [asset] type "Race") and register its effects.
        auto load_race_data(const char *file_name) -> bool;
        // Parses the text of a race script; registers nothing.
        auto load(std::string_view text, std::string_view name) -> bool;

        auto register_motion_data(uint16_t motion_mode, uint16_t motion_index, const char *file_name, uint8_t percentage = 100) -> graphic_thing *;

        // Set up from Python.
        void set_race(uint32_t race_index);
        void register_attaching_bone_name(uint32_t part_index, const char *bone_name);

        void register_motion_mode(uint16_t motion_mode_index);
        auto new_register_motion(race_motion_data *motion_data, uint16_t motion_mode_index, uint16_t motion_index, const char *file_name, uint8_t percentage = 100) -> graphic_thing *;
        auto set_motion_random_weight(uint16_t motion_mode_index, uint16_t motion_index, uint16_t motion_sub_index, uint8_t percentage) -> bool;

        void register_normal_attack(uint16_t motion_mode_index, uint16_t motion_index);
        auto normal_attack_index(uint16_t motion_mode_index, uint16_t *motion_index) -> bool;

        void reserve_combo_attack(uint16_t motion_mode_index, uint16_t combo_type, uint32_t combo_count);
        void register_combo_attack(uint16_t motion_mode_index, uint16_t combo_type, uint32_t combo_index, uint16_t motion_index);
        auto combo_data_pointer(uint16_t motion_mode_index, uint16_t combo_type, combo_data **combo_data_value) -> bool;

        void set_shape_model(uint32_t shape_value, const char *model_file_name);
        void append_shape_skin(uint32_t shape_value, uint32_t part, const char *src_file_name, const char *dst_file_name);

        void set_hair_skin(uint32_t hair_value, uint32_t part, const char *model_file_name, const char *src_file_name, const char *dst_file_name);

        auto smoke_effect_id(uint32_t smoke) -> uint32_t;

        auto smoke_bone() -> const std::string &;

        auto find_hair(uint32_t hair_value) -> hair *;
        auto find_shape(uint32_t shape_value) -> shape *;

      protected:
        void initialize_impl();

        auto motion_vector_pointer(uint16_t motion_mode, uint16_t motion_index, motion_vector **motion_vector_value) -> bool;

      protected:
        uint32_t race_index_;
        uint32_t smoke_effect_id_[SMOKE_NUM];
        std::string smoke_file_names_[SMOKE_NUM];

        graphic_thing *base_model_thing_;
        graphic_thing *lod_model_thing_;

        std::string base_model_file_name_;
        std::string tree_file_name_;
        std::string attribute_file_name_;
        std::vector<motion_entry> motion_list_;
        std::string smoke_bone_name_;

        model_data_map model_data_map_value_;
        motion_mode_data_map motion_mode_data_map_value_;
        attaching_bone_name_map attaching_bone_name_map_value_;
        combo_attack_data_map combo_attack_data_map_value_;
        normal_attack_index_map normal_attack_index_map_value_;

        std::map<uint32_t, hair> map_hair_key_hair_;
        std::map<uint32_t, shape> map_shape_key_shape_;

        race_types::attaching_data_vector attaching_data_vector_value_;

      protected:
        static dynamic_pool<motion_mode_data> motion_mode_data_pool_;
        static dynamic_pool<race_data> pool_value_;
    };
}
