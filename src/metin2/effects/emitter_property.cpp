#include "emitter_property.h"

namespace metin2::inline effects {

    auto emitter_property::get_emitter_shape() -> uint8_t {
        return emitter_shape;
    }
    auto emitter_property::get_emitter_advanced_type() -> uint8_t {
        return emitter_advanced_type;
    }
    auto emitter_property::is_emit_from_edge() -> bool {
        return emit_from_edge_flag;
    }

    void emitter_property::get_emitting_size(float time, float *value) {
        get_time_event_blend_value(time, time_event_emitting_size, value);
    }
    void emitter_property::emitting_angular_velocity(float time, float *value) {
        get_time_event_blend_value(time, time_event_emitting_angular_velocity, value);
    }
    void emitter_property::emitting_direction_x(float time, float *value) {
        get_time_event_blend_value(time, time_event_emitting_direction_x, value);
    }
    void emitter_property::emitting_direction_y(float time, float *value) {
        get_time_event_blend_value(time, time_event_emitting_direction_y, value);
    }
    void emitter_property::emitting_direction_z(float time, float *value) {
        get_time_event_blend_value(time, time_event_emitting_direction_z, value);
    }
    void emitter_property::emitting_velocity(float time, float *value) {
        get_time_event_blend_value(time, time_event_emitting_velocity, value);
    }
    void emitter_property::emission_count_per_second(float time, float *value) {
        get_time_event_blend_value(time, time_event_emission_count_per_second, value);
    }
    void emitter_property::particle_life_time(float time, float *value) {
        get_time_event_blend_value(time, time_event_life_time, value);
    }
    void emitter_property::particle_size_x(float time, float *value) {
        get_time_event_blend_value(time, time_event_size_x, value);
    }
    void emitter_property::particle_size_y(float time, float *value) {
        get_time_event_blend_value(time, time_event_size_y, value);
    }

    void emitter_property::clear() {
        max_emission_count = 0;

        cycle_length = 0.0f;
        cycle_loop_flag = false;
        loop_count = 0;

        emitter_shape = EMITTER_SHAPE_POINT;
        emitter_advanced_type = EMITTER_ADVANCED_TYPE_FREE;
        emit_from_edge_flag = false;
        emitting_size = math::vector3(0.0f, 0.0f, 0.0f);
        emitting_radius = 0.0f;

        emitting_direction = math::vector3(0.0f, 0.0f, 0.0f);

        time_event_emitting_size.clear();
        time_event_emitting_direction_x.clear();
        time_event_emitting_direction_y.clear();
        time_event_emitting_direction_z.clear();
        time_event_emitting_velocity.clear();
        time_event_emission_count_per_second.clear();
        time_event_life_time.clear();
        time_event_size_x.clear();
        time_event_size_y.clear();
        time_event_emitting_angular_velocity.clear();
    }

    emitter_property::emitter_property() {
        clear();
    }
    emitter_property::~emitter_property() = default;
}
