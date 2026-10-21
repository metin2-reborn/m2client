#pragma once

#include <metin2/gpu/types.h>
#include <metin2/graphics/sky_box.h>
#include <metin2/sound/sound_manager.h>

namespace metin2::inline world::inline maps {
    class property;

    namespace property_types {
        enum property_type {
            PROPERTY_TYPE_NONE,
            PROPERTY_TYPE_TREE,
            PROPERTY_TYPE_BUILDING,
            PROPERTY_TYPE_EFFECT,
            PROPERTY_TYPE_AMBIENCE,
            PROPERTY_TYPE_DUNGEON_BLOCK,
            PROPERTY_TYPE_MAX_NUM,
        };

        const char c_szPropertyTypeName[PROPERTY_TYPE_MAX_NUM][32] =
            {
                "None",
                "Tree",
                "Building",
                "Effect",
                "Ambience",
                "DungeonBlock"};

        auto get_property_type(const char *type_name) -> uint32_t;

        struct property_building {
            std::string name_value;
            std::string file_name;
            std::string attribute_data_file_name;
            bool is_shadow_flag;
        };

        struct property_effect {
            std::string name_value;
            std::string file_name;
        };

        enum ambience_play_type {
            AMBIENCE_PLAY_TYPE_ONCE,
            AMBIENCE_PLAY_TYPE_STEP,
            AMBIENCE_PLAY_TYPE_LOOP,
            AMBIENCE_PLAY_TYPE_MAX_NUM,
        };

        const char c_szPlayTypeName[AMBIENCE_PLAY_TYPE_MAX_NUM][8] =
            {
                "ONCE",
                "STEP",
                "LOOP",
        };

        struct property_ambience {
            std::string name_value;
            std::string play_type;

            float play_interval;
            float play_interval_variation;
            float max_volume_area_percentage;
            std::vector<std::string> ambience_sound_vector;
        };

        struct property_dungeon_block {
            std::string name_value;
            std::string file_name;
            std::string attribute_data_file_name;
        };

        auto property_building_string_to_data(property *property, property_building *data) -> bool;
        auto property_effect_string_to_data(property *property, property_effect *data) -> bool;
        auto property_ambience_string_to_data(property *property, property_ambience *data) -> bool;
        auto property_dungeon_block_string_to_data(property *property, property_dungeon_block *data) -> bool;
    };

    enum {
        ENV_DIRLIGHT_BACKGROUND,
        ENV_DIRLIGHT_CHARACTER,
        ENV_DIRLIGHT_NUM
    };

    struct environment_data {
        bool dir_lights_enable[ENV_DIRLIGHT_NUM];
        gpu::light dir_lights[ENV_DIRLIGHT_NUM];

        gpu::material material_value;

        bool fog_enable;
        bool density_fog;

        float fog_near_distance;
        float fog_far_distance;

        auto get_fog_near_distance() const -> float;
        auto get_fog_far_distance() const -> float;

        math::color fog_color;

        bool filtering_enable;
        math::color filtering_color;
        uint8_t filtering_alpha_src;
        uint8_t filtering_alpha_dest;

        float wind_strength;
        float wind_random;

        math::vector3 sky_box_scale;
        bool sky_box_texture_render_mode;

        uint8_t sky_box_gradient_level_upper;
        uint8_t sky_box_gradient_level_lower;

        std::string sky_box_face_file_name[6]; // Order: front, back, left, right, top, bottom.

        math::vector2 cloud_scale;
        float cloud_height;
        math::vector2 cloud_texture_scale;
        math::vector2 cloud_speed;
        std::string cloud_texture_file_name;
        gradient_color cloud_gradient_color;

        std::vector<gradient_color> sky_box_gradient_color_vector;

        bool lens_flare_enable;
        math::color lens_flare_brightness_color;
        float lens_flare_max_brightness;

        bool main_flare_enable;
        std::string main_flare_texture_file_name;
        float main_flare_size;

        bool reserve; // The settings are not changed from outside.
    };

    using environment_data_map = std::map<uint32_t, environment_data *>;

    struct screen_position {
        int x;
        int y;
    };

    using pixel_position = math::vector3;

    auto s_pixel_position_calculate_distance_sq3d(const pixel_position &p_pos_left, const pixel_position &p_pos_right) -> float;

    struct cell_position {
        cell_position() {
            this->x = 0;
            this->y = 0;
        }

        cell_position(int x_value, int y_value) {
            this->x = x_value;
            this->y = y_value;
        }

        int x, y;
    };

    const int c_Section_xAttributeCellSize = 50;
    const int c_Section_yAttributeCellSize = 50;
    const int c_Section_xCellSize = 100;
    const int c_Section_yCellSize = 100;
}
