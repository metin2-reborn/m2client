#include "map_type.h"
#include <metin2/core/file_name.h>
#include <metin2/core/parse.h>
#include "property.h"

namespace metin2::inline world::inline maps {

    auto environment_data::get_fog_near_distance() const -> float {
        return fog_near_distance;
    }

    auto environment_data::get_fog_far_distance() const -> float {
        return fog_far_distance;
    }

    auto s_pixel_position_calculate_distance_sq3d(const pixel_position &p_pos_left, const pixel_position &p_pos_right) -> float {
        float dx = p_pos_left.x - p_pos_right.x;
        float dy = p_pos_left.y - p_pos_right.y;
        float dz = p_pos_left.z - p_pos_right.z;

        return dx * dx + dy * dy + dz * dz;
    }

    namespace property_types {

        auto get_property_type(const char *type_name) -> uint32_t {
            for (uint32_t i = 0; i < PROPERTY_TYPE_MAX_NUM; ++i) {
                if (!strcmp(c_szPropertyTypeName[i], type_name))
                    return i;
            }

            return PROPERTY_TYPE_NONE;
        }

        auto integer_number_to_string(int number) -> const char * {
            static char string[16 + 1];
            snprintf(string, sizeof(string), "%d", number);
            return string;
        }

        auto float_number_to_string(float number) -> const char * {
            static char string[16 + 1];
            snprintf(string, sizeof(string), "%f", number);
            return string;
        }

        auto property_building_string_to_data(property *property, property_building *data) -> bool {
            const char *property_type_value;
            const char *property_name;

            if (!property->string("PropertyType", &property_type_value))
                return false;

            if (!property->string("PropertyName", &property_name))
                return false;

            if (strcmp(property_type_value, "Building"))
                return false;

            data->name_value = property_name;

            const char *building_name;
            if (!property->string("BuildingFile", &building_name))
                return false;

            data->file_name = building_name;
            data->attribute_data_file_name = file_name_helper::no_extension(data->file_name) + ".mdatr";

            const char *shadow_flag;
            if (!property->string("ShadowFlag", &shadow_flag)) {
                data->is_shadow_flag = false;
            } else {
                data->is_shadow_flag = metin2::to_number<int>(shadow_flag);
            }

            return true;
        }

        auto property_effect_string_to_data(property *property, property_effect *data) -> bool {
            const char *property_type_value;
            const char *property_name;

            if (!property->string("PropertyType", &property_type_value))
                return false;

            if (!property->string("PropertyName", &property_name))
                return false;

            if (strcmp(property_type_value, "Effect"))
                return false;

            data->name_value = property_name;

            const char *effect_name_value;
            if (!property->string("EffectFile", &effect_name_value))
                return false;

            data->file_name = effect_name_value;

            return true;
        }

        auto property_ambience_string_to_data(property *property, property_ambience *data) -> bool {
            const char *property_type_value;
            const char *property_name;

            if (!property->string("PropertyType", &property_type_value))
                return false;

            if (!property->string("PropertyName", &property_name))
                return false;

            if (strcmp(property_type_value, "Ambience"))
                return false;

            data->name_value = property_name;

            const char *play_type;
            const char *play_interval;
            const char *play_interval_variation;
            const char *max_volume_area_percentage = nullptr;
            token_vector ambience_sound_vector;
            if (!property->string("PlayType", &play_type))
                return false;
            if (!property->string("PlayInterval", &play_interval))
                return false;
            if (!property->string("PlayIntervalVariation", &play_interval_variation))
                return false;
            if (!property->string("MaxVolumeAreaPercentage", &max_volume_area_percentage)) {
                data->max_volume_area_percentage = 0.0f;
            }
            if (!property->vector("AmbienceSoundVector", ambience_sound_vector))
                return false;

            data->play_type = play_type;
            data->play_interval = metin2::to_number<double>(play_interval);
            data->play_interval_variation = metin2::to_number<double>(play_interval_variation);
            if (max_volume_area_percentage)
                data->max_volume_area_percentage = metin2::to_number<double>(max_volume_area_percentage);
            for (auto itor = ambience_sound_vector.begin(); itor != ambience_sound_vector.end(); ++itor)
                data->ambience_sound_vector.push_back(*itor);

            return true;
        }

        auto property_dungeon_block_string_to_data(property *property, property_dungeon_block *data) -> bool {
            const char *property_type_value;
            const char *property_name;

            if (!property->string("PropertyType", &property_type_value))
                return false;

            if (!property->string("PropertyName", &property_name))
                return false;

            if (strcmp(property_type_value, "DungeonBlock"))
                return false;

            data->name_value = property_name;

            const char *dungeon_block_file_name = nullptr;
            if (!property->string("dungeonblockfile", &dungeon_block_file_name))
                return false;

            data->file_name = dungeon_block_file_name;
            data->attribute_data_file_name = file_name_helper::no_extension(data->file_name) + std::string(".mdatr");

            return true;
        }

    };
}
