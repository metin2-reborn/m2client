#include "particle_system_data.h"
#include <metin2/core/random.h>
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include <set>
#include <span>
#include "effect_update_decorator.h"
#include "particle_instance.h"

namespace metin2::inline effects {

    dynamic_pool<particle_system_data> particle_system_data::pool_value;

    void particle_system_data::destroy_system() {
        pool_value.destroy();
    }

    auto particle_system_data::new_value() -> particle_system_data * {
        return pool_value.alloc();
    }

    void particle_system_data::delete_value(particle_system_data *data) {
        data->clear();
        pool_value.free(data);
    }

    auto particle_system_data::emitter_property_pointer() -> emitter_property * {
        return &emitter_property_value_;
    }
    auto particle_system_data::particle_property_pointer() -> particle_property * {
        return &particle_property_value_;
    }

    namespace {
        constexpr std::string_view emitter_shape_names[] = {"Point", "Ellipse", "Square", "Sphere"};
        constexpr std::string_view emitter_advanced_type_names[] = {"Free", "Outer", "Inner"};
        constexpr std::string_view billboard_names[] = {"None", "All", "Y", "Lie", "TwoFace", "ThreeFace"};
        constexpr std::string_view rotation_names[] = {"None", "TimeEvent", "Clockwise", "CounterClockwise", "RandomDirection"};
        constexpr std::string_view texture_animation_names[] = {"None", "Clockwise", "CounterClockwise", "RandomFrame", "RandomDirection"};

        auto read_enum(asset_table &table, std::string_view key, std::span<const std::string_view> names, uint8_t &value, bool required) -> bool {
            int index = value;
            const bool ok = required ? table.required_enum(key, names, index) : table.optional_enum(key, names, index);
            value = static_cast<uint8_t>(index);
            return ok;
        }

        // A missing table means a constant 0.
        auto read_time_events_or_zero(asset_table &table, std::string_view key, time_event_table_float &events) -> bool {
            events.clear();
            time_event_type_float &zero = events.emplace_back();
            zero.time = 0.0f;
            zero.value = 0.0f;
            return read_time_events(table, key, events);
        }

        auto load_emitter(asset_table &table, emitter_property &emitter) -> bool {
            emitter.cycle_length = 0.05f;
            emitter.cycle_loop_flag = false;
            emitter.loop_count = 0;
            emitter.emitter_advanced_type = emitter_property::EMITTER_ADVANCED_TYPE_FREE;
            emitter.emitting_size = math::vector3(0.0f, 0.0f, 0.0f);
            emitter.emitting_radius = 0.0f;
            emitter.emit_from_edge_flag = false;
            emitter.emitting_direction = math::vector3(0.0f, 0.0f, 0.0f);

            bool ok = table.required("max_emission_count", emitter.max_emission_count);
            ok &= table.optional("cycle_length", emitter.cycle_length);
            ok &= table.optional("cycle_loop", emitter.cycle_loop_flag);
            ok &= table.optional("loop_count", emitter.loop_count);
            ok &= read_enum(table, "shape", emitter_shape_names, emitter.emitter_shape, true);
            ok &= read_enum(table, "advanced_type", emitter_advanced_type_names, emitter.emitter_advanced_type, false);
            ok &= table.optional("size", emitter.emitting_size);
            ok &= table.optional("radius", emitter.emitting_radius);
            ok &= table.optional("emit_from_edge", emitter.emit_from_edge_flag);
            ok &= table.optional("direction", emitter.emitting_direction);
            ok &= read_time_events_or_zero(table, "size_events", emitter.time_event_emitting_size);
            ok &= read_time_events_or_zero(table, "angular_velocity_events", emitter.time_event_emitting_angular_velocity);
            ok &= read_time_events_or_zero(table, "direction_x_events", emitter.time_event_emitting_direction_x);
            ok &= read_time_events_or_zero(table, "direction_y_events", emitter.time_event_emitting_direction_y);
            ok &= read_time_events_or_zero(table, "direction_z_events", emitter.time_event_emitting_direction_z);
            ok &= read_time_events_or_zero(table, "velocity_events", emitter.time_event_emitting_velocity);
            ok &= read_required_time_events(table, "emission_rate_events", emitter.time_event_emission_count_per_second);
            ok &= read_required_time_events(table, "life_time_events", emitter.time_event_life_time);
            ok &= read_required_time_events(table, "particle_size_x_events", emitter.time_event_size_x);
            ok &= read_required_time_events(table, "particle_size_y_events", emitter.time_event_size_y);
            return ok;
        }

        // The colour table holds every time of the four channel tables, each channel blended at it.
        void merge_colors(time_event_table_float &red, time_event_table_float &green, time_event_table_float &blue, time_event_table_float &alpha, time_event_table_color &colors) {
            std::set<float> times;
            for (const time_event_table_float *channel : {&red, &green, &blue, &alpha}) {
                for (const time_event_type_float &e : *channel) {
                    times.insert(e.time);
                }
            }
            colors.clear();
            for (float time : times) {
                math::color c;
                get_time_event_blend_value<float>(time, red, &c.r);
                get_time_event_blend_value<float>(time, green, &c.g);
                get_time_event_blend_value<float>(time, blue, &c.b);
                get_time_event_blend_value<float>(time, alpha, &c.a);
                time_event_type_color t;
                t.time = time;
                t.value.color_value = static_cast<uint32_t>(c);
                colors.push_back(t);
            }
        }
    }

    auto particle_system_data::on_load_script(asset_table &table) -> bool {
        const toml::table *emitter = table.table("emitter");
        const toml::table *sprite = table.table("sprite");
        if (!emitter || !sprite) {
            table.error("", "needs an [particle.emitter] and a [particle.sprite] table");
            return false;
        }

        asset_table emitter_table(*emitter, table.file_name());
        bool ok = load_emitter(emitter_table, emitter_property_value_);
        ok &= emitter_table.finish();

        asset_table t(*sprite, table.file_name());
        particle_property &p = particle_property_value_;
        p.src_blend_type = gpu::blend::SRC_ALPHA;
        p.dest_blend_type = gpu::blend::ONE;
        p.color_operation_type = gpu::texture_op::MODULATE;
        p.rotation_random_starting_begin = 0;
        p.rotation_random_starting_end = 0;
        p.attach_flag = false;
        p.tex_ani_random_start_frame_flag = false;
        ok &= t.optional("src_blend", p.src_blend_type);
        ok &= t.optional("dest_blend", p.dest_blend_type);
        ok &= t.optional("color_operation", p.color_operation_type);
        ok &= read_enum(t, "billboard", billboard_names, p.billboard_type, true);
        ok &= read_enum(t, "rotation", rotation_names, p.rotation_type, true);
        ok &= t.required("rotation_speed", p.rotation_speed);
        if (const toml::array *start = t.array("rotation_random_start")) {
            const auto begin = start->size() == 2 ? (*start)[0].value_exact<int64_t>() : std::nullopt;
            const auto end = start->size() == 2 ? (*start)[1].value_exact<int64_t>() : std::nullopt;
            if (!begin || !end || *begin < 0 || *end < 0 || *begin > 0xffff || *end > 0xffff) {
                t.error("rotation_random_start", "must be two whole numbers from 0 to 65535");
                ok = false;
            } else {
                p.rotation_random_starting_begin = static_cast<uint16_t>(*begin);
                p.rotation_random_starting_end = static_cast<uint16_t>(*end);
            }
        }
        ok &= t.optional("attach", p.attach_flag);
        ok &= t.required("stretch", p.stretch_flag);
        ok &= read_enum(t, "texture_animation", texture_animation_names, p.tex_ani_type, true);
        ok &= t.required("texture_animation_delay", p.tex_ani_delay);
        ok &= t.optional("texture_animation_random_start", p.tex_ani_random_start_frame_flag);

        p.time_event_gravity.clear();
        p.time_event_air_resistance.clear();
        ok &= read_time_events(t, "gravity_events", p.time_event_gravity);
        ok &= read_time_events(t, "air_resistance_events", p.time_event_air_resistance);
        ok &= read_required_time_events(t, "scale_x_events", p.time_event_scale_x);
        ok &= read_required_time_events(t, "scale_y_events", p.time_event_scale_y);

        time_event_table_float red;
        time_event_table_float green;
        time_event_table_float blue;
        time_event_table_float alpha;
        ok &= read_required_time_events(t, "red_events", red);
        ok &= read_required_time_events(t, "green_events", green);
        ok &= read_required_time_events(t, "blue_events", blue);
        ok &= read_required_time_events(t, "alpha_events", alpha);
        merge_colors(red, green, blue, alpha, p.time_event_color);

        ok &= read_required_time_events(t, "rotation_events", p.time_event_rotation);

        const toml::array *textures = t.array("textures");
        if (!textures) {
            t.error("textures", "is missing");
            ok = false;
        } else {
            // A name without an archive is next to the script.
            const std::string folder = get_only_path_name(std::string(table.file_name()).c_str());
            for (const toml::node &node : *textures) {
                const auto name = node.value_exact<std::string>();
                if (!name) {
                    t.error("textures", "must be file names");
                    ok = false;
                    continue;
                }
                p.insert_texture((is_global_file_name(name->c_str()) ? *name : folder + *name).c_str());
            }
        }

        return t.finish() && ok;
    }

    void particle_system_data::change_texture(const char *file_name) {
        particle_property_value_.set_texture(file_name);
    }

    void particle_system_data::on_clear() {
        emitter_property_value_.clear();
        particle_property_value_.clear();
    }

    auto particle_system_data::on_is_data() -> bool {
        return true;
    }

    void particle_system_data::build_decorator(particle_instance *instance) {
        using namespace update_decorators;

        instance->decorator_ = new null_decorator;

        if (particle_property_value_.time_event_air_resistance.size() > 1) {
            instance->decorator_ = instance->decorator_->add_chain_front(new air_resistance_decorator);
            instance->decorator_ = instance->decorator_->add_chain_front(
                new air_resistance_value_decorator(particle_property_value_.time_event_air_resistance, &instance->air_resistance_));
        } else if (particle_property_value_.time_event_air_resistance.size() == 1) {
            instance->air_resistance_ = particle_property_value_.time_event_air_resistance[0].value;
            instance->decorator_ = instance->decorator_->add_chain_front(new air_resistance_decorator);
        }

        if (particle_property_value_.time_event_gravity.size() > 1) {
            instance->decorator_ = instance->decorator_->add_chain_front(new gravity_decorator);
            instance->decorator_ = instance->decorator_->add_chain_front(
                new gravity_value_decorator(particle_property_value_.time_event_gravity, &instance->gravity_));
        } else if (particle_property_value_.time_event_gravity.size() == 1) {
            instance->gravity_ = particle_property_value_.time_event_gravity[0].value;
            instance->decorator_ = instance->decorator_->add_chain_front(new gravity_decorator);
        }
        instance->decorator_ = instance->decorator_->add_chain_front(
            new color_all_decorator(particle_property_value_.time_event_color, &instance->dc_color_));

        instance->decorator_ = instance->decorator_->add_chain_front(
            new scale_value_decorator(particle_property_value_.time_event_scale_x, &instance->scale_.x));
        instance->decorator_ = instance->decorator_->add_chain_front(
            new scale_value_decorator(particle_property_value_.time_event_scale_y, &instance->scale_.y));

        if (particle_property_value_.texture_animation_frame_count() > 1 && particle_property_value_.texture_animation_frame_delay() > 1e-6) {
            switch (instance->texture_animation_type_) {
            case particle_property::TEXTURE_ANIMATION_TYPE_CW:
                instance->decorator_ = instance->decorator_->add_chain_front(
                    new texture_animation_cw_decorator(particle_property_value_.texture_animation_frame_delay(), particle_property_value_.texture_animation_frame_count(), &instance->frame_index_));
                break;
            case particle_property::TEXTURE_ANIMATION_TYPE_CCW:
                instance->decorator_ = instance->decorator_->add_chain_front(
                    new texture_animation_ccw_decorator(particle_property_value_.texture_animation_frame_delay(), particle_property_value_.texture_animation_frame_count(), &instance->frame_index_));
                break;
            case particle_property::TEXTURE_ANIMATION_TYPE_RANDOM_FRAME:
                instance->decorator_ = instance->decorator_->add_chain_front(
                    new texture_animation_random_decorator(particle_property_value_.texture_animation_frame_delay(), particle_property_value_.texture_animation_frame_count(), &instance->frame_index_));
                break;
            }
        }

        uint8_t rotation_type = particle_property_value_.rotation_type;

        if (particle_property_value_.rotation_speed == 0.0f && rotation_type != particle_property::ROTATION_TYPE_TIME_EVENT) {
            rotation_type = particle_property::ROTATION_TYPE_NONE;
        } else if (rotation_type == particle_property::ROTATION_TYPE_RANDOM_DIRECTION) {
            rotation_type = metin2::random::number(0, 1) ? particle_property::ROTATION_TYPE_CW : particle_property::ROTATION_TYPE_CCW;
        }

        switch (rotation_type) {
        case particle_property::ROTATION_TYPE_TIME_EVENT:
            instance->decorator_ = instance->decorator_->add_chain_front(
                new rotation_decorator());
            instance->decorator_ = instance->decorator_->add_chain_front(
                new rotation_speed_value_decorator(particle_property_value_.time_event_rotation, &instance->rotation_speed_));
            break;

        case particle_property::ROTATION_TYPE_CW:
            instance->rotation_speed_ = particle_property_value_.rotation_speed;
            instance->decorator_ = instance->decorator_->add_chain_front(
                new rotation_decorator());
            break;

        case particle_property::ROTATION_TYPE_CCW:
            instance->rotation_speed_ = -particle_property_value_.rotation_speed;
            instance->decorator_ = instance->decorator_->add_chain_front(
                new rotation_decorator());
            break;
        }

        instance->decorator_ = instance->decorator_->add_chain_front(new header_decorator);
    }

    particle_system_data::particle_system_data() = default;
    particle_system_data::~particle_system_data() = default;
}
