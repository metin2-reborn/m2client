#include "particle_system_instance.h"
#include <metin2/core/random.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/state_manager.h>
#include "particle_instance.h"
#include "particle_system_data.h"

namespace metin2::inline effects {

    dynamic_pool<particle_system_instance> particle_system_instance::pool_value;

    using namespace update_decorators;

    void particle_system_instance::destroy_system() {
        pool_value.destroy();

        particle_instance::destroy_system();
    }

    auto particle_system_instance::new_value() -> particle_system_instance * {
        return pool_value.alloc();
    }

    void particle_system_instance::delete_value(particle_system_instance *ps_inst) {
        ps_inst->destroy();
        pool_value.free(ps_inst);
    }

    auto particle_system_instance::emission_count() -> uint32_t {
        return current_emission_count_;
    }

    void particle_system_instance::create_particles(float elapsed_time) {
        float emission_count_value;
        emitter_property_value_->emission_count_per_second(local_time_, &emission_count_value);

        float creating_value = emission_count_value * (elapsed_time / 1.0f) + emission_residue_;
        int creating_count = int(creating_value);
        emission_residue_ = creating_value - creating_count;

        int icur_emission_count = emission_count();
        int max_emission_count = int(emitter_property_value_->get_max_emission_count());
        int next_emission_count = int(icur_emission_count + creating_count);
        creating_count -= std::max(0, next_emission_count - max_emission_count);

        float life_time = 0.0f;
        float emitting_size = 0.0f;
        math::vector3 v3_time_position;
        math::vector3 v3_velocity;
        float f_velocity = 0.0f;
        math::vector2 half_size;
        float lie_rotation = 0;
        if (creating_count) {
            emitter_property_value_->particle_life_time(local_time_, &life_time);
            if (life_time == 0.0f) {
                return;
            }

            emitter_property_value_->get_emitting_size(local_time_, &emitting_size);

            data_value_->get_position(local_time_, v3_time_position);

            emitter_property_value_->emitting_direction_x(local_time_, &v3_velocity.x);
            emitter_property_value_->emitting_direction_y(local_time_, &v3_velocity.y);
            emitter_property_value_->emitting_direction_z(local_time_, &v3_velocity.z);

            emitter_property_value_->emitting_velocity(local_time_, &f_velocity);

            emitter_property_value_->particle_size_x(local_time_, &half_size.x);
            emitter_property_value_->particle_size_y(local_time_, &half_size.y);

            if (BILLBOARD_TYPE_LIE == particle_property_value_->billboard_type && mc_pmat_local_) {
                float fsx = mc_pmat_local_->_32;
                float fcx = sqrtf(1.0f - fsx * fsx);

                if (fcx >= 0.00001f)
                    lie_rotation = math::to_degree(atan2f(-mc_pmat_local_->_12, mc_pmat_local_->_22));
            }
        }

        particle_instance *first_instance = nullptr;

        for (int i = 0; i < creating_count; ++i) {
            particle_instance *instance_value;

            instance_value = particle_instance::new_value();
            instance_value->particle_property_value_ = particle_property_value_;
            instance_value->emitter_property_value_ = emitter_property_value_;

            instance_value->life_time_ = life_time;
            instance_value->last_life_time_ = life_time;

            switch (emitter_property_value_->get_emitter_shape()) {
            case emitter_property::EMITTER_SHAPE_POINT:
                instance_value->position_value_.x = 0.0f;
                instance_value->position_value_.y = 0.0f;
                instance_value->position_value_.z = 0.0f;
                break;

            case emitter_property::EMITTER_SHAPE_ELLIPSE:
                instance_value->position_value_.x = metin2::random::fnumber(-500.0f, 500.0f);
                instance_value->position_value_.y = metin2::random::fnumber(-500.0f, 500.0f);
                instance_value->position_value_.z = 0.0f;
                instance_value->position_value_ = math::normalize(instance_value->position_value_);

                if (emitter_property_value_->is_emit_from_edge()) {
                    instance_value->position_value_ *= (emitter_property_value_->emitting_radius + emitting_size);
                } else {
                    instance_value->position_value_ *= (metin2::random::fnumber(0.0f, emitter_property_value_->emitting_radius) + emitting_size);
                }
                break;

            case emitter_property::EMITTER_SHAPE_SQUARE:
                instance_value->position_value_.x = (metin2::random::fnumber(-emitter_property_value_->emitting_size.x / 2.0f, emitter_property_value_->emitting_size.x / 2.0f) + emitting_size);
                instance_value->position_value_.y = (metin2::random::fnumber(-emitter_property_value_->emitting_size.y / 2.0f, emitter_property_value_->emitting_size.y / 2.0f) + emitting_size);
                instance_value->position_value_.z = (metin2::random::fnumber(-emitter_property_value_->emitting_size.z / 2.0f, emitter_property_value_->emitting_size.z / 2.0f) + emitting_size);
                break;

            case emitter_property::EMITTER_SHAPE_SPHERE:
                instance_value->position_value_.x = metin2::random::fnumber(-500.0f, 500.0f);
                instance_value->position_value_.y = metin2::random::fnumber(-500.0f, 500.0f);
                instance_value->position_value_.z = metin2::random::fnumber(-500.0f, 500.0f);
                instance_value->position_value_ = math::normalize(instance_value->position_value_);

                if (emitter_property_value_->is_emit_from_edge()) {
                    instance_value->position_value_ *= (emitter_property_value_->emitting_radius + emitting_size);
                } else {
                    instance_value->position_value_ *= (metin2::random::fnumber(0.0f, emitter_property_value_->emitting_radius) + emitting_size);
                }
                break;
            }

            math::vector3 time_position_value = v3_time_position;

            instance_value->position_value_ += time_position_value;

            if (mc_pmat_local_ && !particle_property_value_->attach_flag) {
                instance_value->position_value_ = math::transform_coord(instance_value->position_value_, *mc_pmat_local_);
                time_position_value = math::transform_coord(time_position_value, *mc_pmat_local_);
            }
            instance_value->start_position_ = time_position_value;

            instance_value->velocity_.x = 0.0f;
            instance_value->velocity_.y = 0.0f;
            instance_value->velocity_.z = 0.0f;

            if (emitter_property::EMITTER_ADVANCED_TYPE_INNER == emitter_property_value_->get_emitter_advanced_type()) {
                auto diff_vector = (instance_value->position_value_ - time_position_value);
                instance_value->velocity_ = math::normalize(diff_vector);
                instance_value->velocity_ *= -100.0f;
            } else if (emitter_property::EMITTER_ADVANCED_TYPE_OUTER == emitter_property_value_->get_emitter_advanced_type()) {
                if (emitter_property_value_->get_emitter_shape() == emitter_property::EMITTER_SHAPE_POINT) {
                    instance_value->velocity_.x = metin2::random::fnumber(-100.0f, 100.0f);
                    instance_value->velocity_.y = metin2::random::fnumber(-100.0f, 100.0f);
                    instance_value->velocity_.z = metin2::random::fnumber(-100.0f, 100.0f);
                } else {
                    auto diff_vector = (instance_value->position_value_ - time_position_value);
                    instance_value->velocity_ = math::normalize(diff_vector);
                    instance_value->velocity_ *= 100.0f;
                }
            }

            math::vector3 velocity = v3_velocity;
            if (mc_pmat_local_ && !particle_property_value_->attach_flag) {
                velocity = math::transform_normal(velocity, *mc_pmat_local_);
            }

            instance_value->velocity_ += velocity;
            if (emitter_property_value_->emitting_direction.x > 0.0f)
                instance_value->velocity_.x += metin2::random::fnumber(-emitter_property_value_->emitting_direction.x / 2.0f, emitter_property_value_->emitting_direction.x / 2.0f) * 1000.0f;
            if (emitter_property_value_->emitting_direction.y > 0.0f)
                instance_value->velocity_.y += metin2::random::fnumber(-emitter_property_value_->emitting_direction.y / 2.0f, emitter_property_value_->emitting_direction.y / 2.0f) * 1000.0f;
            if (emitter_property_value_->emitting_direction.z > 0.0f)
                instance_value->velocity_.z += metin2::random::fnumber(-emitter_property_value_->emitting_direction.z / 2.0f, emitter_property_value_->emitting_direction.z / 2.0f) * 1000.0f;

            instance_value->velocity_ *= f_velocity;

            instance_value->half_size_ = half_size;

            instance_value->rotation_ = particle_property_value_->rotation_random_starting_begin;
            instance_value->rotation_ = metin2::random::fnumber(float(particle_property_value_->rotation_random_starting_begin), float(particle_property_value_->rotation_random_starting_end));
            // For the lie type, apply the local matrix rotation to the random value.
            if (BILLBOARD_TYPE_LIE == particle_property_value_->billboard_type && mc_pmat_local_) {
                instance_value->rotation_ += lie_rotation;
            }

            instance_value->frame_index_ = 0;
            instance_value->texture_animation_type_ = particle_property_value_->texture_animation_type();

            if (particle_property_value_->texture_animation_frame_count() > 1) {
                if (particle_property::TEXTURE_ANIMATION_TYPE_RANDOM_DIRECTION == particle_property_value_->texture_animation_type()) {
                    if (metin2::random::number(0, 1)) {
                        instance_value->frame_index_ = 0;
                        instance_value->texture_animation_type_ = particle_property::TEXTURE_ANIMATION_TYPE_CW;
                    } else {
                        instance_value->frame_index_ = particle_property_value_->texture_animation_frame_count() - 1;
                        instance_value->texture_animation_type_ = particle_property::TEXTURE_ANIMATION_TYPE_CCW;
                    }
                }
                if (particle_property_value_->tex_ani_random_start_frame_flag) {
                    instance_value->frame_index_ = metin2::random::number(0, particle_property_value_->texture_animation_frame_count() - 1);
                }
            }

            {
                instance_value->last_position_ = instance_value->position_value_ - (instance_value->velocity_ * elapsed_time);
                instance_value->scale_.x = particle_property_value_->time_event_scale_x.empty() ? 1.0f : particle_property_value_->time_event_scale_x.front().value;
                instance_value->scale_.y = particle_property_value_->time_event_scale_y.empty() ? 1.0f : particle_property_value_->time_event_scale_y.front().value;
                if (!particle_property_value_->time_event_color.empty())
                    instance_value->dc_color_ = particle_property_value_->time_event_color.front().value;
            }

            if (!first_instance) {
                data_value_->build_decorator(instance_value);
                first_instance = instance_value;
            } else {
                instance_value->decorator_ = first_instance->decorator_->clone(first_instance, instance_value);
            }

            particle_instance_list_vector_value_[instance_value->frame_index_].push_back(instance_value);
            current_emission_count_++;
        }
    }

    auto particle_system_instance::on_update(float elapsed_time) -> bool {
        bool make_particle = true;

        if (local_time_ >= emitter_property_value_->get_cycle_length()) {
            if (emitter_property_value_->is_cycle_loop() && --loop_count_ != 0) {
                if (loop_count_ < 0)
                    loop_count_ = 0;
                local_time_ = local_time_ - emitter_property_value_->get_cycle_length();
            } else {
                make_particle = false;
                loop_count_ = 1;
                if (emission_count() == 0)
                    return false;
            }
        }

        int frame_index;
        int frame_count = particle_property_value_->texture_animation_frame_count();

        float angular_velocity;
        emitter_property_value_->emitting_angular_velocity(local_time_, &angular_velocity);

        if (angular_velocity && !particle_property_value_->attach_flag) {
            auto diff_vector = math::vector3(0.0f, 0.0f, 1.0f);
            particle_property_value_->z_axis = math::transform_normal(diff_vector, *mc_pmat_local_);
        }

        for (frame_index = 0; frame_index < frame_count; frame_index++) {
            auto itor = particle_instance_list_vector_value_[frame_index].begin();
            for (; itor != particle_instance_list_vector_value_[frame_index].end();) {
                particle_instance *instance = *itor;

                if (!instance->update(elapsed_time, angular_velocity)) {
                    instance->delete_this();

                    itor = particle_instance_list_vector_value_[frame_index].erase(itor);
                    current_emission_count_--;
                } else {
                    if (instance->frame_index_ != frame_index) {
                        particle_instance_list_vector_value_[frame_count + instance->frame_index_].push_back(*itor);
                        itor = particle_instance_list_vector_value_[frame_index].erase(itor);
                    } else
                        ++itor;
                }
            }
        }
        if (is_active() && make_particle)
            create_particles(elapsed_time);

        for (frame_index = 0; frame_index < frame_count; ++frame_index) {
            particle_instance_list_vector_value_[frame_index].splice(particle_instance_list_vector_value_[frame_index].end(), particle_instance_list_vector_value_[frame_index + frame_count]);
            particle_instance_list_vector_value_[frame_index + frame_count].clear();
        }

        return true;
    }

    namespace particle_renderers {
        struct two_side_renderer {
            const math::matrix *pmat;
            two_side_renderer(const math::matrix *pmat_value = nullptr)
                : pmat(pmat_value) {
            }

            inline void operator()(particle_instance *instance) {
                instance->transform(pmat, math::to_radian(-30.0f));
                STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, instance->particle_mesh_pointer(), sizeof(pt_vertex));

                instance->transform(pmat, math::to_radian(+30.0f));
                STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, instance->particle_mesh_pointer(), sizeof(pt_vertex));
            }
        };

        struct three_side_renderer {
            const math::matrix *pmat;
            three_side_renderer(const math::matrix *pmat_value = nullptr)
                : pmat(pmat_value) {
            }

            inline void operator()(particle_instance *instance) {
                instance->transform(pmat);
                STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, instance->particle_mesh_pointer(), sizeof(pt_vertex));
                instance->transform(pmat, math::to_radian(-60.0f));
                STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, instance->particle_mesh_pointer(), sizeof(pt_vertex));
                instance->transform(pmat, math::to_radian(+60.0f));
                STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, instance->particle_mesh_pointer(), sizeof(pt_vertex));
            }
        };

        struct normal_renderer {
            inline void operator()(particle_instance *instance) {
                instance->transform();
                STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, instance->particle_mesh_pointer(), sizeof(pt_vertex));
            }
        };
        struct attach_renderer {
            const math::matrix *pmat;
            attach_renderer(const math::matrix *pmat_value)
                : pmat(pmat_value) {
            }

            inline void operator()(particle_instance *instance) {
                instance->transform(pmat);
                STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, instance->particle_mesh_pointer(), sizeof(pt_vertex));
            }
        };
    }

    void particle_system_instance::on_render() {
        screen::identity();
        STATEMANAGER.set_render_state(gpu::state::SRC_BLEND, particle_property_value_->src_blend_type);
        STATEMANAGER.set_render_state(gpu::state::DEST_BLEND, particle_property_value_->dest_blend_type);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, particle_property_value_->color_operation_type);
        if (particle_property_value_->billboard_type < BILLBOARD_TYPE_2FACE) {
            if (!particle_property_value_->attach_flag) {
                auto obj = particle_renderers::normal_renderer();
                for_each_particle_rendering(obj);
            } else {
                auto obj = particle_renderers::attach_renderer(mc_pmat_local_);
                for_each_particle_rendering(obj);
            }
        } else if (particle_property_value_->billboard_type == BILLBOARD_TYPE_2FACE) {
            if (!particle_property_value_->attach_flag) {
                auto obj = particle_renderers::two_side_renderer();
                for_each_particle_rendering(obj);
            } else {
                auto obj = particle_renderers::two_side_renderer(mc_pmat_local_);
                for_each_particle_rendering(obj);
            }
        } else if (particle_property_value_->billboard_type == BILLBOARD_TYPE_3FACE) {
            if (!particle_property_value_->attach_flag) {
                auto obj = particle_renderers::three_side_renderer();
                for_each_particle_rendering(obj);
            } else {
                auto obj = particle_renderers::three_side_renderer(mc_pmat_local_);
                for_each_particle_rendering(obj);
            }
        }
    }

    void particle_system_instance::on_set_data_pointer(effect_element_base *element) {
        data_value_ = (particle_system_data *)element;

        current_emission_count_ = 0;
        particle_property_value_ = data_value_->particle_property_pointer();
        emitter_property_value_ = data_value_->emitter_property_pointer();
        loop_count_ = emitter_property_value_->get_loop_count();
        particle_instance_list_vector_value_.resize(particle_property_value_->texture_animation_frame_count() * 2 + 2);

        assert(vct_img_inst_.empty());
        vct_img_inst_.reserve(particle_property_value_->image_vector.size());
        for (uint32_t i = 0; i < particle_property_value_->image_vector.size(); ++i) {
            graphic_image *image = particle_property_value_->image_vector[i];

            graphic_image_instance *img_inst_new = graphic_image_instance::new_value();
            img_inst_new->set_image_pointer(image);
            vct_img_inst_.push_back(img_inst_new);
        }
    }

    void particle_system_instance::on_initialize() {
        current_emission_count_ = 0;
        loop_count_ = 0;
        emission_residue_ = 0.0f;
    }

    void particle_system_instance::on_destroy() {
        particle_instance_list_vector::iterator i;
        for (i = particle_instance_list_vector_value_.begin(); i != particle_instance_list_vector_value_.end(); ++i) {
            particle_instance_list &lst_particle_inst = *i;

            particle_instance_list::iterator j;
            for (j = lst_particle_inst.begin(); j != lst_particle_inst.end(); ++j) {
                particle_instance *particle_inst = *j;
                particle_inst->delete_this();
            }

            lst_particle_inst.clear();
        }
        particle_instance_list_vector_value_.clear();

        std::for_each(vct_img_inst_.begin(), vct_img_inst_.end(), graphic_image_instance::delete_value);
        vct_img_inst_.clear();
    }

    particle_system_instance::particle_system_instance() {
        initialize();
    }

    particle_system_instance::~particle_system_instance() {
        assert(particle_instance_list_vector_value_.empty());
        assert(vct_img_inst_.empty());
    }
}
