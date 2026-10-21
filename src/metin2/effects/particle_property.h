#pragma once

#include <metin2/graphics/grp_image_instance.h>
#include <vector>

#include "type.h"

namespace metin2::inline effects {

    class particle_property {
        friend class particle_system_data;
        friend class particle_system_instance;

      public:
        enum {
            ROTATION_TYPE_NONE,
            ROTATION_TYPE_TIME_EVENT,
            ROTATION_TYPE_CW,
            ROTATION_TYPE_CCW,
            ROTATION_TYPE_RANDOM_DIRECTION,
        };

        enum {
            TEXTURE_ANIMATION_TYPE_NONE,
            TEXTURE_ANIMATION_TYPE_CW,
            TEXTURE_ANIMATION_TYPE_CCW,
            TEXTURE_ANIMATION_TYPE_RANDOM_FRAME,
            TEXTURE_ANIMATION_TYPE_RANDOM_DIRECTION,
        };

      public:
        particle_property();
        virtual ~particle_property();

        void clear();

        void insert_texture(const char *file_name);
        auto set_texture(const char *file_name) -> bool;

        inline auto texture_animation_type() -> uint8_t {
            return tex_ani_type;
        }

        inline auto texture_animation_frame_count() -> uint32_t {
            return image_vector.size();
        }

        inline auto texture_animation_frame_delay() -> float {
            return tex_ani_delay;
        }

        uint8_t tex_ani_type;
        float tex_ani_delay;
        bool tex_ani_random_start_frame_flag;

        uint8_t src_blend_type;
        uint8_t dest_blend_type;
        uint8_t color_operation_type;
        uint8_t billboard_type;

        uint8_t rotation_type;
        float rotation_speed;
        uint16_t rotation_random_starting_begin;
        uint16_t rotation_random_starting_end;

        bool attach_flag;
        bool stretch_flag;

        time_event_table_float time_event_gravity;
        time_event_table_float time_event_air_resistance;

        time_event_table_float time_event_scale_x;
        time_event_table_float time_event_scale_y;
        time_event_table_color time_event_color;
        time_event_table_float time_event_rotation;

        std::vector<graphic_image *> image_vector;

        auto operator=(const particle_property &particle_property_value) -> particle_property &;

        // Pre-transformed variables.
        math::vector3 z_axis;
    };
}
