#include "particle_system_data.h"
#include <metin2/core/random.h>
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include <set>
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

    auto particle_system_data::on_load_script(text_file_loader &text_file_loader) -> bool {
        {
            text_file_loader::goto_child goto_child(&text_file_loader, "emitterproperty");

            if (!text_file_loader.token_double_word("maxemissioncount", &emitter_property_value_.max_emission_count))
                return false;

            if (!text_file_loader.token_float("cyclelength", &emitter_property_value_.cycle_length)) {
                emitter_property_value_.cycle_length = 0.05f;
            }
            if (!text_file_loader.token_boolean("cycleloopenable", &emitter_property_value_.cycle_loop_flag)) {
                emitter_property_value_.cycle_loop_flag = false;
            }
            if (!text_file_loader.token_integer("loopcount", &emitter_property_value_.loop_count)) {
                emitter_property_value_.loop_count = 0;
            }

            if (!text_file_loader.token_byte("emittershape", &emitter_property_value_.emitter_shape))
                return false;

            if (!text_file_loader.token_byte("emitteradvancedtype", &emitter_property_value_.emitter_advanced_type)) {
                emitter_property_value_.emitter_advanced_type = emitter_property::EMITTER_ADVANCED_TYPE_FREE;
            }
            if (!text_file_loader.token_position("emittingsize", &emitter_property_value_.emitting_size)) {
                emitter_property_value_.emitting_size = math::vector3(0.0f, 0.0f, 0.0f);
            }
            if (!text_file_loader.token_float("emittingradius", &emitter_property_value_.emitting_radius)) {
                emitter_property_value_.emitting_radius = 0.0f;
            }

            if (!text_file_loader.token_boolean("emitteremitfromedgeflag", &emitter_property_value_.emit_from_edge_flag)) {
                emitter_property_value_.emit_from_edge_flag = false;
            }

            if (!text_file_loader.token_position("emittingdirection", &emitter_property_value_.emitting_direction)) {
                emitter_property_value_.emitting_direction = math::vector3(0.0f, 0.0f, 0.0f);
            }

            if (!get_token_time_event_float(text_file_loader, "timeeventemittingsize", &emitter_property_value_.time_event_emitting_size)) {
                emitter_property_value_.time_event_emitting_size.clear();
                time_event_type_float time_event_float;
                time_event_float.time = 0.0f;
                time_event_float.value = 0.0f;
                emitter_property_value_.time_event_emitting_size.push_back(time_event_float);
            }

            if (!get_token_time_event_float(text_file_loader, "timeeventemittingangularvelocity", &emitter_property_value_.time_event_emitting_angular_velocity)) {
                emitter_property_value_.time_event_emitting_angular_velocity.clear();
                time_event_type_float time_event_float;
                time_event_float.time = 0.0f;
                time_event_float.value = 0.0f;
                emitter_property_value_.time_event_emitting_angular_velocity.push_back(time_event_float);
            }

            if (!get_token_time_event_float(text_file_loader, "timeeventemittingdirectionx", &emitter_property_value_.time_event_emitting_direction_x)) {
                emitter_property_value_.time_event_emitting_direction_x.clear();
                time_event_type_float time_event_float;
                time_event_float.time = 0.0f;
                time_event_float.value = 0.0f;
                emitter_property_value_.time_event_emitting_direction_x.push_back(time_event_float);
            }
            if (!get_token_time_event_float(text_file_loader, "timeeventemittingdirectiony", &emitter_property_value_.time_event_emitting_direction_y)) {
                emitter_property_value_.time_event_emitting_direction_y.clear();
                time_event_type_float time_event_float;
                time_event_float.time = 0.0f;
                time_event_float.value = 0.0f;
                emitter_property_value_.time_event_emitting_direction_y.push_back(time_event_float);
            }
            if (!get_token_time_event_float(text_file_loader, "timeeventemittingdirectionz", &emitter_property_value_.time_event_emitting_direction_z)) {
                emitter_property_value_.time_event_emitting_direction_z.clear();
                time_event_type_float time_event_float;
                time_event_float.time = 0.0f;
                time_event_float.value = 0.0f;
                emitter_property_value_.time_event_emitting_direction_z.push_back(time_event_float);
            }
            if (!get_token_time_event_float(text_file_loader, "timeeventemittingvelocity", &emitter_property_value_.time_event_emitting_velocity)) {
                emitter_property_value_.time_event_emitting_velocity.clear();
                time_event_type_float time_event_float;
                time_event_float.time = 0.0f;
                time_event_float.value = 0.0f;
                emitter_property_value_.time_event_emitting_velocity.push_back(time_event_float);
            }
            if (!get_token_time_event_float(text_file_loader, "timeeventemissioncountpersecond", &emitter_property_value_.time_event_emission_count_per_second))
                return false;
            if (!get_token_time_event_float(text_file_loader, "timeeventlifetime", &emitter_property_value_.time_event_life_time))
                return false;
            if (!get_token_time_event_float(text_file_loader, "timeeventsizex", &emitter_property_value_.time_event_size_x))
                return false;
            if (!get_token_time_event_float(text_file_loader, "timeeventsizey", &emitter_property_value_.time_event_size_y))
                return false;
        }

        {
            text_file_loader::goto_child goto_child(&text_file_loader, "particleproperty");

            if (!text_file_loader.token_byte("srcblendtype", &particle_property_value_.src_blend_type)) {
                particle_property_value_.src_blend_type = gpu::blend::SRC_ALPHA;
            }
            if (!text_file_loader.token_byte("destblendtype", &particle_property_value_.dest_blend_type)) {
                particle_property_value_.dest_blend_type = gpu::blend::ONE;
            }
            if (!text_file_loader.token_byte("coloroperationtype", &particle_property_value_.color_operation_type)) {
                particle_property_value_.color_operation_type = gpu::texture_op::MODULATE;
            }

            if (!text_file_loader.token_byte("billboardtype", &particle_property_value_.billboard_type))
                return false;

            if (!text_file_loader.token_byte("rotationtype", &particle_property_value_.rotation_type))
                return false;
            if (!text_file_loader.token_float("rotationspeed", &particle_property_value_.rotation_speed))
                return false;
            if (!text_file_loader.token_word("rotationrandomstartingbegin", &particle_property_value_.rotation_random_starting_begin))
                return false;
            if (!text_file_loader.token_word("rotationrandomstartingend", &particle_property_value_.rotation_random_starting_end))
                return false;

            if (!text_file_loader.token_boolean("attachenable", &particle_property_value_.attach_flag)) {
                particle_property_value_.attach_flag = false;
            }
            if (!text_file_loader.token_boolean("stretchenable", &particle_property_value_.stretch_flag))
                return false;

            if (!text_file_loader.token_byte("texanitype", &particle_property_value_.tex_ani_type))
                return false;
            if (!text_file_loader.token_float("texanidelay", &particle_property_value_.tex_ani_delay))
                return false;
            if (!text_file_loader.token_boolean("texanirandomstartframeenable", &particle_property_value_.tex_ani_random_start_frame_flag))
                return false;

            float gravity;

            if (text_file_loader.token_float("gravity", &gravity)) {
                time_event_type_float f;
                f.time = 0.0f;
                f.value = gravity;
                particle_property_value_.time_event_gravity.push_back(f);
            } else if (!get_token_time_event_float(text_file_loader, "timeeventgravity", &particle_property_value_.time_event_gravity)) {
                particle_property_value_.time_event_gravity.clear();
            }

            float air_resistance;
            if (text_file_loader.token_float("airresistance", &air_resistance)) {
                time_event_type_float f;
                f.time = 0.0f;
                f.value = air_resistance;
                particle_property_value_.time_event_air_resistance.push_back(f);
            } else if (!get_token_time_event_float(text_file_loader, "timeeventairresistance", &particle_property_value_.time_event_air_resistance)) {
                particle_property_value_.time_event_air_resistance.clear();
            }

            if (!get_token_time_event_float(text_file_loader, "timeeventscalex", &particle_property_value_.time_event_scale_x))
                return false;
            if (!get_token_time_event_float(text_file_loader, "timeeventscaley", &particle_property_value_.time_event_scale_y))
                return false;

            time_event_table_float time_event_r;
            time_event_table_float time_event_b;
            time_event_table_float time_event_g;
            time_event_table_float time_event_a;
            if (!get_token_time_event_float(text_file_loader, "timeeventcolorred", &time_event_r))
                return false;
            if (!get_token_time_event_float(text_file_loader, "timeeventcolorgreen", &time_event_g))
                return false;
            if (!get_token_time_event_float(text_file_loader, "timeeventcolorblue", &time_event_b))
                return false;
            if (!get_token_time_event_float(text_file_loader, "timeeventalpha", &time_event_a))
                return false;

            particle_property_value_.time_event_color.clear();
            {
                std::set<float> times;
                for (const time_event_type_float &e : time_event_r)
                    times.insert(e.time);
                for (const time_event_type_float &e : time_event_g)
                    times.insert(e.time);
                for (const time_event_type_float &e : time_event_b)
                    times.insert(e.time);
                for (const time_event_type_float &e : time_event_a)
                    times.insert(e.time);
                std::set<float>::iterator it;
                for (it = times.begin(); it != times.end(); ++it) {
                    float time = *it;
                    float r, g, b, a;
                    get_time_event_blend_value<float>(time, time_event_r, &r);
                    get_time_event_blend_value<float>(time, time_event_g, &g);
                    get_time_event_blend_value<float>(time, time_event_b, &b);
                    get_time_event_blend_value<float>(time, time_event_a, &a);
                    time_event_type_color t;
                    t.time = time;
                    math::color c;
                    c.r = r;
                    c.g = g;
                    c.b = b;
                    c.a = a;
                    t.value.color_value = (uint32_t)c;
                    particle_property_value_.time_event_color.push_back(t);
                }
            }

            if (!get_token_time_event_float(text_file_loader, "timeeventrotation", &particle_property_value_.time_event_rotation))
                return false;

            token_vector *texture_vector;

            if (!text_file_loader.get_token_vector("texturefiles", &texture_vector))
                return false;

            for (uint32_t i = 0; i < texture_vector->size(); ++i) {
                std::string texture_file_name = texture_vector->at(i).c_str();

                if (!is_global_file_name(texture_file_name.c_str()))
                    texture_file_name = get_only_path_name(text_file_loader.file_name()) + texture_file_name;

                particle_property_value_.insert_texture(texture_file_name.c_str());
            }
        }

        return true;
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
