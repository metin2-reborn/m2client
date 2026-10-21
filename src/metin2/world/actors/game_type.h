#pragma once

#include <string>

#include <metin2/core/text_file_loader.h>
#include <metin2/geometry/collision_data.h>

namespace metin2::inline client::inline app {
    extern std::string image_path;
}
namespace metin2::inline world::inline actors {

    extern float game_fps;

    extern std::string resource_path;

#define MOTION_KEY                                uint32_t
#define MAKE_MOTION_KEY(mode, index)              (((uint32_t)(((uint8_t)(mode)) << 24) | ((uint32_t)((uint16_t)(index))) << 8) | ((uint32_t)((uint8_t)(0))))
#define MAKE_RANDOM_MOTION_KEY(mode, index, type) (((uint32_t)(((uint8_t)(mode)) << 24) | ((uint32_t)((uint16_t)(index))) << 8) | ((uint32_t)((uint8_t)(type))))
#define GET_MOTION_MODE(key)                      ((uint8_t)(((uint32_t)(key) >> 24) & 0xFF))
#define GET_MOTION_INDEX(key)                     ((uint16_t)(((uint32_t)(key) >> 8) & 0xFFFF))
#define GET_MOTION_SUB_INDEX(key)                 ((uint8_t)(((uint32_t)(key)) & 0xFF))

    namespace race_types {
        enum jobs {
            JOB_WARRIOR,
            JOB_ASSASSIN,
            JOB_SURA,
            JOB_SHAMAN,
            JOB_MAX_NUM
        };

        enum attack_type {
            ATTACK_TYPE_SPLASH,
            ATTACK_TYPE_SNIPE,
        };
        enum hit_type {
            HIT_TYPE_NONE,
            HIT_TYPE_GREAT,
            HIT_TYPE_GOOD,
        };
        enum motion_type {
            MOTION_TYPE_NONE,
            MOTION_TYPE_NORMAL,
            MOTION_TYPE_COMBO,
            MOTION_TYPE_SKILL,
        };

        using hit_time_position = dynamic_sphere_instance;
        using hit_time_position_map = std::map<float, hit_time_position>;

        struct attack_data {
            int attack_type_value;
            int hitting_type;
            float invisible_time;
            float external_force;
            float stiffen_time;
            int hit_limit_count;
        };

        struct hit_data {
            float attack_start_time;
            float attack_end_time;

            float weapon_length;
            std::string bone_name;

            hit_time_position_map hit_position;

            auto load(text_file_loader &text_file_loader) -> bool;
        };

        using hit_data_container = std::vector<hit_data>;

        struct motion_attack_data : public attack_data {
            int motion_type_value;

            hit_data_container hit_data_container_value;
        };

        enum collision_type {
            COLLISION_TYPE_NONE,

            COLLISION_TYPE_BODY,
            COLLISION_TYPE_ATTACKING,
            COLLISION_TYPE_DEFENDING,
            COLLISION_TYPE_SPLASH,
        };
        enum collision_shape {
            COLLISION_SHAPE_SPHERE,
            COLLISION_SHAPE_CYLINDER,
        };

        struct collision_data {
            int collision_type_value;
            sphere_collision_instance_vector sphere_data_vector;

            collision_data() = default;
            virtual ~collision_data() = default;
        };

        struct attaching_effect_data {
            std::string file_name;
            math::vector3 position_value;
            math::vector3 rotation;

            attaching_effect_data() = default;
            virtual ~attaching_effect_data() = default;
        };

        struct attaching_object_data {
            std::string file_name;

            attaching_object_data() = default;
            virtual ~attaching_object_data() = default;
        };

        using collision_data_vector = std::vector<collision_data>;

        void destroy_system();

        auto load_attack_data(text_file_loader &text_file_loader, attack_data *data) -> bool;
        auto load_motion_attack_data(text_file_loader &text_file_loader, motion_attack_data *data) -> bool;
        auto load_collision_data(text_file_loader &text_file_loader, collision_data *collision_data_value) -> bool;
        auto load_effect_data(text_file_loader &text_file_loader, attaching_effect_data *effect_data) -> bool;
        auto load_object_data(text_file_loader &text_file_loader, attaching_object_data *object_data) -> bool;

        enum attaching_data_type {
            ATTACHING_DATA_TYPE_NONE,

            ATTACHING_DATA_TYPE_COLLISION_DATA,
            ATTACHING_DATA_TYPE_EFFECT,
            ATTACHING_DATA_TYPE_OBJECT,

            ATTACHING_DATA_TYPE_MAX_NUM,
        };

        struct attaching_data {
            attaching_data()
                : type_value(0), is_attaching(false), attaching_model_index(0), collision_data_value(nullptr), effect_data_value(nullptr), object_data_value(nullptr) {}
            uint32_t type_value;

            bool is_attaching;
            uint32_t attaching_model_index;
            std::string attaching_bone_name;

            collision_data *collision_data_value;
            attaching_effect_data *effect_data_value;
            attaching_object_data *object_data_value;
        };

        using attaching_data_vector = std::vector<attaching_data>;

        auto load_attaching_data(text_file_loader &text_file_loader, attaching_data_vector *attaching_data_vector_value) -> bool;

        extern dynamic_pool<collision_data> collision_data_pool;
        extern dynamic_pool<attaching_effect_data> effect_data_pool;
        extern dynamic_pool<attaching_object_data> object_data_pool;
    };
}
