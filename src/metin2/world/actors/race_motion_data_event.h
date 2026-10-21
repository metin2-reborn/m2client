#pragma once

#include <metin2/core/crc32.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/world/flying/flying_object_manager.h>
#include "game_type.h"

namespace metin2::inline world::inline actors {

    namespace motion_events {
        struct motion_event_data {
            int type_value;
            uint32_t frame;
            float starting_time;
            float duration_time;

            motion_event_data() : frame(0), starting_time(0.0f), duration_time(0.0f) {}
            virtual ~motion_event_data() = default;

            virtual auto load(text_file_loader &text_file_loader) -> bool { return true; }
        };

        struct motion_event_data_screen_waving : public motion_event_data {
            int power;
            int affecting_range;

            motion_event_data_screen_waving() = default;
            ~motion_event_data_screen_waving() override = default;

            auto load(text_file_loader &text_file_loader) -> bool override {
                if (!text_file_loader.token_float("duringtime", &duration_time))
                    return false;
                if (!text_file_loader.token_integer("power", &power))
                    return false;
                if (!text_file_loader.token_integer("affectingrange", &affecting_range))
                    affecting_range = 0;

                return true;
            }
        };

        struct motion_event_data_screen_flashing : public motion_event_data {
            math::color flashing_color;

            motion_event_data_screen_flashing() = default;
            ~motion_event_data_screen_flashing() override = default;

            auto load(text_file_loader &text_file_loader) -> bool override {
                return true;
            }
        };

        struct motion_event_data_effect : public motion_event_data {
            bool is_attaching;
            bool is_following;
            bool is_independent;
            std::string attaching_bone_name;
            math::vector3 effect_position_value;

            uint32_t effect_index;
            std::string effect_file_name;

            motion_event_data_effect() = default;
            ~motion_event_data_effect() override = default;

            auto load(text_file_loader &text_file_loader) -> bool override {
                if (!text_file_loader.token_boolean("independentflag", &is_independent))
                    is_independent = false;
                if (!text_file_loader.token_boolean("attachingenable", &is_attaching))
                    return false;
                if (!text_file_loader.token_string("attachingbonename", &attaching_bone_name))
                    return false;
                if (!text_file_loader.token_string("effectfilename", &effect_file_name))
                    return false;
                if (!text_file_loader.token_position("effectposition", &effect_position_value))
                    return false;
                if (!text_file_loader.token_boolean("followingenable", &is_following)) {
                    is_following = false;
                }
                effect_index = get_case_crc32(effect_file_name.c_str(), effect_file_name.length());
                get_effect_manager().register_effect(effect_file_name.c_str());

                return true;
            }
        };

        struct motion_event_data_effect_to_target : public motion_event_data {
            uint32_t effect_index;

            std::string effect_file_name;
            math::vector3 effect_position_value;
            bool is_following;
            bool is_fishing_effect;

            motion_event_data_effect_to_target() = default;
            ~motion_event_data_effect_to_target() override = default;

            auto load(text_file_loader &text_file_loader) -> bool override {
                if (!text_file_loader.token_string("effectfilename", &effect_file_name))
                    return false;
                if (!text_file_loader.token_position("effectposition", &effect_position_value))
                    return false;
                if (!text_file_loader.token_boolean("followingenable", &is_following)) {
                    is_following = false;
                }
                if (!text_file_loader.token_boolean("fishingeffectflag", &is_fishing_effect)) {
                    is_fishing_effect = false;
                }
                effect_index = get_case_crc32(effect_file_name.c_str(), effect_file_name.length());
                get_effect_manager().register_effect(effect_file_name.c_str());

                return true;
            }
        };

        struct motion_event_data_fly : public motion_event_data {
            bool is_attaching;
            std::string attaching_bone_name;
            math::vector3 fly_position;

            uint32_t fly_index;
            std::string fly_file_name;

            motion_event_data_fly() = default;
            ~motion_event_data_fly() override = default;

            auto load(text_file_loader &text_file_loader) -> bool override {
                if (!text_file_loader.token_boolean("attachingenable", &is_attaching))
                    return false;
                if (!text_file_loader.token_string("attachingbonename", &attaching_bone_name))
                    return false;
                if (!text_file_loader.token_string("flyfilename", &fly_file_name))
                    return false;
                if (!text_file_loader.token_position("flyposition", &fly_position))
                    return false;
                fly_index = get_case_crc32(fly_file_name.c_str(), fly_file_name.length());

                get_flying_manager().register_flying_data(fly_file_name.c_str());

                return true;
            }
        };

        struct motion_event_data_attack : public motion_event_data {
            race_types::collision_data collision_data_value;
            race_types::attack_data attack_data_value;
            bool is_enable_hit_process;

            motion_event_data_attack() = default;
            ~motion_event_data_attack() override = default;

            auto load(text_file_loader &text_file_loader) -> bool override {
                if (!text_file_loader.token_float("duringtime", &duration_time))
                    return false;

                if (!text_file_loader.token_boolean("enablehitprocess", &is_enable_hit_process)) {
                    is_enable_hit_process = true;
                }

                if (!race_types::load_attack_data(text_file_loader, &attack_data_value))
                    return false;

                if (!race_types::load_collision_data(text_file_loader, &collision_data_value))
                    return false;

                return true;
            }
        };
        using motion_event_data_attacking = motion_event_data_attack;

        struct motion_event_data_sound : public motion_event_data {
            std::string sound_file_name;

            motion_event_data_sound() = default;
            ~motion_event_data_sound() override = default;

            auto load(text_file_loader &text_file_loader) -> bool override {
                if (!text_file_loader.token_string("soundfilename", &sound_file_name))
                    return false;

                return true;
            }
        };

        struct motion_event_data_character_show : public motion_event_data {
            motion_event_data_character_show() = default;
            ~motion_event_data_character_show() override = default;
        };

        struct motion_event_data_character_hide : public motion_event_data {
            motion_event_data_character_hide() = default;
            ~motion_event_data_character_hide() override = default;
        };

        struct motion_event_data_warp : public motion_event_data {
            motion_event_data_warp() = default;
            ~motion_event_data_warp() override = default;
        };
        using motion_warp_event_data = motion_event_data_warp;
    };
}
