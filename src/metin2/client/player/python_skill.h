#pragma once

#include <metin2/poly/poly.h>
#include <metin2/world/items/item_data.h>

namespace metin2::inline client::inline characters {
    class instance_base;
}

namespace metin2::inline client::inline player {

    class python_skill {
      public:
        enum {
            SKILL_TYPE_NONE,
            SKILL_TYPE_ACTIVE,
            SKILL_TYPE_SUPPORT,
            SKILL_TYPE_GUILD,
            SKILL_TYPE_HORSE,
            SKILL_TYPE_MAX_NUM,
        };

        enum skill_table_token_type {
            TABLE_TOKEN_TYPE_VNUM,
            TABLE_TOKEN_TYPE_NAME,
            TABLE_TOKEN_TYPE_TYPE,
            TABLE_TOKEN_TYPE_LEVEL_STEP,
            TABLE_TOKEN_TYPE_MAX_LEVEL,
            TABLE_TOKEN_TYPE_LEVEL_LIMIT,
            TABLE_TOKEN_TYPE_POINT_ON,
            TABLE_TOKEN_TYPE_POINT_POLY,
            TABLE_TOKEN_TYPE_SP_COST_POLY,
            TABLE_TOKEN_TYPE_DURATION_POLY,
            TABLE_TOKEN_TYPE_DURATION_SP_COST_POLY,
            TABLE_TOKEN_TYPE_COOLDOWN_POLY,
            TABLE_TOKEN_TYPE_MASTER_BONUS_POLY,
            TABLE_TOKEN_TYPE_ATTACK_GRADE_POLY,
            TABLE_TOKEN_TYPE_FLAG,
            TABLE_TOKEN_TYPE_AFFECT_FLAG,
            TABLE_TOKEN_TYPE_POINT_ON2,
            TABLE_TOKEN_TYPE_POINT_POLY2,
            TABLE_TOKEN_TYPE_DURATION_POLY2,
            TABLE_TOKEN_TYPE_AFFECT_FLAG2,
            TABLE_TOKEN_TYPE_PREREQUISITE_SKILL_VNUM,
            TABLE_TOKEN_TYPE_PREREQUISITE_SKILL_LEVEL,
            TABLE_TOKEN_TYPE_SKILL_TYPE,
            TABLE_TOKEN_TYPE_MAX_HIT,
            TABLE_TOKEN_TYPE_SPLASH_AROUND_DAMAGE_ADJUST_POLY,
            TABLE_TOKEN_TYPE_TARGET_RANGE,
            TABLE_TOKEN_TYPE_SPLASH_RANGE,
            TABLE_TOKEN_TYPE_MAX_NUM,
        };

        enum skill_desc_token_type {
            DESC_TOKEN_TYPE_VNUM,
            DESC_TOKEN_TYPE_JOB,
            DESC_TOKEN_TYPE_NAME1,
            DESC_TOKEN_TYPE_NAME2,
            DESC_TOKEN_TYPE_NAME3,
            DESC_TOKEN_TYPE_DESCRIPTION,
            DESC_TOKEN_TYPE_CONDITION1,
            DESC_TOKEN_TYPE_CONDITION2,
            DESC_TOKEN_TYPE_CONDITION3,
            DESC_TOKEN_TYPE_CONDITION4,
            DESC_TOKEN_TYPE_ATTRIBUTE,
            DESC_TOKEN_TYPE_WEAPON,
            DESC_TOKEN_TYPE_ICON_NAME,
            DESC_TOKEN_TYPE_MOTION_INDEX,
            DESC_TOKEN_TYPE_MOTION_INDEX_GRADE_NUM,
            DESC_TOKEN_TYPE_TARGET_COUNT_FORMULA,
            DESC_TOKEN_TYPE_MOTION_LOOP_COUNT_FORMULA,
            DESC_TOKEN_TYPE_AFFECT_DESCRIPTION_1,
            DESC_TOKEN_TYPE_AFFECT_MIN_1,
            DESC_TOKEN_TYPE_AFFECT_MAX_1,
            DESC_TOKEN_TYPE_AFFECT_DESCRIPTION_2,
            DESC_TOKEN_TYPE_AFFECT_MIN_2,
            DESC_TOKEN_TYPE_AFFECT_MAX_2,
            DESC_TOKEN_TYPE_AFFECT_DESCRIPTION_3,
            DESC_TOKEN_TYPE_AFFECT_MIN_3,
            DESC_TOKEN_TYPE_AFFECT_MAX_3,
            DESC_TOKEN_TYPE_LEVEL_LIMIT,
            DESC_TOKEN_TYPE_MAX_LEVEL,
            DESC_TOKEN_TYPE_MAX_NUM,

            CONDITION_COLUMN_COUNT = 3,
            AFFECT_COLUMN_COUNT = 3,
            AFFECT_STEP_COUNT = 3,
        };

        enum {
            SKILL_ATTRIBUTE_NEED_TARGET = (1 << 0),
            SKILL_ATTRIBUTE_TOGGLE = (1 << 1),
            SKILL_ATTRIBUTE_WEAPON_LIMITATION = (1 << 2),
            SKILL_ATTRIBUTE_MELEE_ATTACK = (1 << 3),
            SKILL_ATTRIBUTE_USE_HP = (1 << 4),
            SKILL_ATTRIBUTE_CAN_CHANGE_DIRECTION = (1 << 5),
            SKILL_ATTRIBUTE_STANDING_SKILL = (1 << 6),
            SKILL_ATTRIBUTE_ONLY_FOR_ALLIANCE = (1 << 7),
            SKILL_ATTRIBUTE_CAN_USE_FOR_ME = (1 << 8),
            SKILL_ATTRIBUTE_NEED_CORPSE = (1 << 9),
            SKILL_ATTRIBUTE_FAN_RANGE = (1 << 10),
            SKILL_ATTRIBUTE_CAN_USE_IF_NOT_ENOUGH = (1 << 11),
            SKILL_ATTRIBUTE_NEED_EMPTY_BOTTLE = (1 << 12),
            SKILL_ATTRIBUTE_NEED_POISON_BOTTLE = (1 << 13),
            SKILL_ATTRIBUTE_ATTACK_SKILL = (1 << 14),
            SKILL_ATTRIBUTE_TIME_INCREASE_SKILL = (1 << 15), // Only used by Jeungjisul.
            SKILL_ATTRIBUTE_CHARGE_ATTACK = (1 << 16),
            SKILL_ATTRIBUTE_PASSIVE = (1 << 17),
            SKILL_ATTRIBUTE_CANNOT_LEVEL_UP = (1 << 18),
            SKILL_ATTRIBUTE_ONLY_FOR_GUILD_WAR = (1 << 19),
            SKILL_ATTRIBUTE_MOVING_SKILL = (1 << 20),
            SKILL_ATTRIBUTE_HORSE_SKILL = (1 << 21),
            SKILL_ATTRIBUTE_CIRCLE_RANGE = (1 << 22),
            SKILL_ATTRIBUTE_SEARCH_TARGET = (1 << 23),
        };

        enum {
            SKILL_NEED_WEAPON_SWORD = (1 << item_data::WEAPON_SWORD),
            SKILL_NEED_WEAPON_DAGGER = (1 << item_data::WEAPON_DAGGER),
            SKILL_NEED_WEAPON_BOW = (1 << item_data::WEAPON_BOW),
            SKILL_NEED_WEAPON_TWO_HANDED = (1 << item_data::WEAPON_TWO_HANDED),
            SKILL_NEED_WEAPON_DOUBLE_SWORD = (1 << item_data::WEAPON_DAGGER),
            SKILL_NEED_WEAPON_BELL = (1 << item_data::WEAPON_BELL),
            SKILL_NEED_WEAPON_FAN = (1 << item_data::WEAPON_FAN),
            SKILL_NEED_WEAPON_ARROW = (1 << item_data::WEAPON_ARROW),
            SKILL_NEED_WEAPON_EMPTY_HAND = (1 << item_data::WEAPON_NONE),
        };

        enum {
            VALUE_TYPE_FREE,
            VALUE_TYPE_MIN,
            VALUE_TYPE_MAX,
        };

        enum {
            SKILL_GRADE_COUNT = 3,
            SKILL_EFFECT_COUNT = 4, // Beginner, intermediate and advanced, plus an extra effect at master level.
            SKILL_GRADE_STEP_COUNT = 20,
            SKILL_GRADEGAP = 25,
        };

        struct affect_data {
            std::string affect_description;
            std::string affect_min_formula;
            std::string affect_max_formula;
        };
        struct affect_data_new {
            std::string point_type;
            std::string point_poly;
        };
        struct require_stat_data {
            uint8_t point;
            uint8_t level_value;
        };
        struct grade_data {
            std::string name_value;
            graphic_image *image;
            uint16_t motion_index;
        };
        struct skill_data {
            static uint32_t MELEE_SKILL_TARGET_RANGE;
            skill_data();
            auto get_target_range() const -> uint32_t;
            auto can_change_direction() -> bool;
            auto is_fan_range() -> bool;
            auto is_circle_range() -> bool;
            auto is_auto_search_target() -> bool;
            auto is_need_target() -> bool;
            auto is_need_corpse() -> bool;
            auto is_toggle_skill() -> bool;
            auto is_use_hp_skill() -> bool;
            auto is_standing_skill() -> bool;
            auto can_use_weapon_type(uint32_t weapon_type) -> bool;
            auto is_only_for_alliance() -> bool;
            auto can_use_for_me() -> bool;
            auto can_use_if_not_enough() -> bool;
            auto is_need_empty_bottle() -> bool;
            auto is_need_poison_bottle() -> bool;
            auto is_need_bow() -> bool;
            auto is_attack_skill() -> bool;
            auto is_horse_skill() -> bool;
            auto is_moving_skill() -> bool;
            auto is_time_increase_skill() -> bool;
            auto is_charge_skill() -> bool;
            auto is_only_for_guild_war() -> bool;

            auto get_state(const char *state_name, int *state, int min_max_type = VALUE_TYPE_FREE) -> bool;
            auto process_formula(expression *poly, float skill_level = 0.0f, int min_max_type = VALUE_TYPE_FREE) -> float;
            auto affect_description(uint32_t index, float skill_level) -> const char *;
            auto skill_cool_time(float skill_point) -> uint32_t;
            auto need_sp(float skill_point) -> int;
            auto continuation_sp(float skill_point) -> uint32_t;
            auto motion_loop_count(float skill_point) -> uint32_t;
            auto target_count(float skill_point) -> uint32_t;
            auto get_duration(float skill_point) -> uint32_t;
            auto skill_motion_index(int grade = -1) -> uint32_t;
            auto get_max_level() -> uint8_t;
            auto get_level_up_point() -> uint8_t;
            auto is_can_use_skill() -> bool;
            auto is_no_motion() -> bool;

            auto get_name() const -> const std::string;
            auto get_type() const -> uint8_t;

            uint8_t type_value;
            uint32_t skill_index;
            uint8_t max_level;
            uint8_t level_up_point;
            uint8_t level_limit;
            bool no_motion;

            std::string name_value;
            std::string icon_file_name;
            std::string description;
            std::string motion_script_file_name;

            std::vector<std::string> condition_data_vector;
            std::vector<affect_data> affect_data_vector;
            std::vector<affect_data_new> affect_data_new_vector;
            std::vector<require_stat_data> require_stat_data_vector;
            std::string cool_time_formula;
            std::string target_count_formula;
            std::string motion_loop_count_formula;
            std::string need_sp_formula;
            std::string continuation_sp_formula;
            std::string duration;

            uint32_t skill_attribute;
            uint32_t need_weapon;
            uint32_t target_range;
            uint16_t motion_index;
            uint16_t motion_index_for_me;

            bool is_requirement;
            std::string require_skill_name;
            uint8_t require_skill_level;

            grade_data grade_data_value[SKILL_EFFECT_COUNT];

            graphic_image *image;

            static std::map<std::string, uint32_t> status_name_map;
            static std::map<std::string, uint32_t> new_min_status_name_map;
            static std::map<std::string, uint32_t> new_max_status_name_map;
            static uint32_t time_increase_skill_number;
        };

        using skill_data_map = std::map<uint32_t, skill_data>;
        using skill_motion_index_map = std::map<uint32_t, uint32_t>;
        using path_name_map = std::map<std::string, std::string>;

      public:
        python_skill();
        ~python_skill();

        void destroy();
        auto register_skill(uint32_t skill_index, const char *file_name) -> bool;
        auto register_skill_table(const char *file_name) -> bool;
        auto register_skill_desc(const char *file_name) -> bool;
        auto get_skill_data(uint32_t skill_index, skill_data **skill_data_value) -> bool;
        auto skill_data_by_name(const char *name, skill_data **skill_data_value) -> bool;

        void set_path_name(const char *file_name);

      protected:
        void register_grade_icon_image(skill_data &data, const char *header, const char *image_name);
        void register_normal_icon_image(skill_data &data, const char *header, const char *image_name);

      protected:
        skill_data_map skill_data_map_value_;

        std::string path_name_;

        std::map<std::string, uint32_t> skill_type_index_map_;
        std::map<std::string, uint32_t> skill_attribute_index_map_;
        std::map<std::string, uint32_t> skill_need_weapon_index_map_;
        std::map<std::string, uint32_t> skill_weapon_type_index_map_;
        path_name_map path_name_map_value_;
    };

    inline auto get_python_skill() -> python_skill & {
        static python_skill instance;
        return instance;
    }
}
