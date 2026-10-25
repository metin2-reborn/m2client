#pragma once

#include <metin2/core/crc32.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/files/eter_file_manager.h>
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

            // Reads the keys of the event's type; register_assets() then loads the files it names.
            virtual auto load(asset_table &table) -> bool { return true; }
            virtual void register_assets() {}
        };

        struct motion_event_data_screen_waving : public motion_event_data {
            int power;
            int affecting_range;

            motion_event_data_screen_waving() = default;
            ~motion_event_data_screen_waving() override = default;

            auto load(asset_table &table) -> bool override {
                affecting_range = 0;
                bool ok = table.required("duration", duration_time);
                ok &= table.required("power", power);
                ok &= table.optional("range", affecting_range);
                return ok;
            }
        };

        struct motion_event_data_screen_flashing : public motion_event_data {
            math::color flashing_color;

            motion_event_data_screen_flashing() = default;
            ~motion_event_data_screen_flashing() override = default;
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

            auto load(asset_table &table) -> bool override {
                is_independent = false;
                is_following = false;
                bool ok = table.required("effect", effect_file_name);
                ok &= table.required("position", effect_position_value);
                ok &= table.required("bone", attaching_bone_name);
                ok &= table.required("attach", is_attaching);
                ok &= table.optional("follow", is_following);
                ok &= table.optional("independent", is_independent);
                effect_file_name = eter_file_manager::canonical_name(effect_file_name);
                effect_index = get_case_crc32(effect_file_name.c_str(), effect_file_name.length());
                return ok;
            }

            void register_assets() override {
                get_effect_manager().register_effect(effect_file_name.c_str());
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

            auto load(asset_table &table) -> bool override {
                is_following = false;
                is_fishing_effect = false;
                bool ok = table.required("effect", effect_file_name);
                ok &= table.required("position", effect_position_value);
                ok &= table.optional("follow", is_following);
                ok &= table.optional("fishing", is_fishing_effect);
                effect_file_name = eter_file_manager::canonical_name(effect_file_name);
                effect_index = get_case_crc32(effect_file_name.c_str(), effect_file_name.length());
                return ok;
            }

            void register_assets() override {
                get_effect_manager().register_effect(effect_file_name.c_str());
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

            auto load(asset_table &table) -> bool override {
                bool ok = table.required("fly", fly_file_name);
                ok &= table.required("position", fly_position);
                ok &= table.required("bone", attaching_bone_name);
                ok &= table.required("attach", is_attaching);
                fly_file_name = eter_file_manager::canonical_name(fly_file_name);
                fly_index = get_case_crc32(fly_file_name.c_str(), fly_file_name.length());
                return ok;
            }

            void register_assets() override {
                get_flying_manager().register_flying_data(fly_file_name.c_str());
            }
        };

        struct motion_event_data_attack : public motion_event_data {
            race_types::collision_data collision_data_value;
            race_types::attack_data attack_data_value;
            bool is_enable_hit_process;

            motion_event_data_attack() = default;
            ~motion_event_data_attack() override = default;

            auto load(asset_table &table) -> bool override {
                is_enable_hit_process = true;
                bool ok = table.required("duration", duration_time);
                ok &= table.optional("hit_process", is_enable_hit_process);
                ok &= race_types::load_attack_data(table, "attack_type", &attack_data_value);
                ok &= race_types::load_collision_data(table, &collision_data_value);
                return ok;
            }
        };
        using motion_event_data_attacking = motion_event_data_attack;

        struct motion_event_data_sound : public motion_event_data {
            std::string sound_file_name;

            motion_event_data_sound() = default;
            ~motion_event_data_sound() override = default;

            auto load(asset_table &table) -> bool override {
                return table.required("sound", sound_file_name);
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
