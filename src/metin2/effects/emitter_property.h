#pragma once

#include "type.h"

namespace metin2::inline effects {

    class emitter_property {
        friend class particle_system_data;
        friend class particle_system_instance;

      public:
        enum {
            EMITTER_SHAPE_POINT,
            EMITTER_SHAPE_ELLIPSE,
            EMITTER_SHAPE_SQUARE,
            EMITTER_SHAPE_SPHERE,
        };

        enum {
            EMITTER_ADVANCED_TYPE_FREE,
            EMITTER_ADVANCED_TYPE_OUTER,
            EMITTER_ADVANCED_TYPE_INNER,
        };

      public:
        emitter_property();
        virtual ~emitter_property();

        void clear();

        auto get_max_emission_count() -> uint32_t {
            return max_emission_count;
        }

        auto get_cycle_length() -> float {
            return cycle_length;
        }

        auto is_cycle_loop() -> bool {
            return cycle_loop_flag;
        }

        auto get_loop_count() -> int {
            return loop_count;
        }

        auto get_emitter_shape() -> uint8_t;
        auto get_emitter_advanced_type() -> uint8_t;
        auto is_emit_from_edge() -> bool;

        void get_emitting_size(float time, float *value);
        void emitting_angular_velocity(float time, float *value);

        void emitting_direction_x(float time, float *value);
        void emitting_direction_y(float time, float *value);
        void emitting_direction_z(float time, float *value);
        void emitting_velocity(float time, float *value);
        void emission_count_per_second(float time, float *value);
        void particle_life_time(float time, float *value);
        void particle_size_x(float time, float *value);
        void particle_size_y(float time, float *value);

        uint32_t max_emission_count;

        float cycle_length;
        bool cycle_loop_flag;
        int loop_count;

        uint8_t emitter_shape;
        uint8_t emitter_advanced_type;
        bool emit_from_edge_flag;
        math::vector3 emitting_size;
        float emitting_radius;

        math::vector3 emitting_direction;

        time_event_table_float time_event_emitting_size;
        time_event_table_float time_event_emitting_angular_velocity;
        time_event_table_float time_event_emitting_direction_x;
        time_event_table_float time_event_emitting_direction_y;
        time_event_table_float time_event_emitting_direction_z;
        time_event_table_float time_event_emitting_velocity;
        time_event_table_float time_event_emission_count_per_second;
        time_event_table_float time_event_life_time;
        time_event_table_float time_event_size_x;
        time_event_table_float time_event_size_y;
    };
}
