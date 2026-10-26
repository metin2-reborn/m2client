#include "effect_instance.h"
#include "particle_system_instance.h"
#include "simple_light_instance.h"

#include <metin2/core/stl.h>
#include <metin2/core/timer.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/sound/sound_manager.h>

#include <functional>
#include <metin2/gpu/types.h>

namespace metin2::inline effects {

    dynamic_pool<effect_instance> effect_instance::pool_value;

    auto effect_instance::less_render_order(effect_instance *eft_inst) -> bool {
        return (eft_data_ < eft_inst->eft_data_);
    }

    auto effect_instance::new_value() -> effect_instance * {
        effect_instance *eft_inst = pool_value.alloc();
        return eft_inst;
    }

    void effect_instance::delete_value(effect_instance *eft_inst) {
        eft_inst->clear();
        pool_value.free(eft_inst);
    }

    void effect_instance::destroy_system() {
        pool_value.destroy();

        particle_system_instance::destroy_system();
        effect_mesh_instance::destroy_system();
        light_instance::destroy_system();
    }

    void effect_instance::update_sound() {
        // The cues are frames of a fixed rate; every frame the elapsed time has reached is played
        // once, so the schedule is the same at any frame rate.
        sound_time_ += get_timer().elapsed_second();
        const auto reached = static_cast<uint32_t>(sound_time_ * sound_types::CUE_FRAMES_PER_SECOND);
        if (sound_instance_vector_value_) {
            sound_manager &snd_mgr = get_sound_manager();
            for (; next_sound_frame_ <= reached; ++next_sound_frame_) {
                snd_mgr.update_sound_instance(global_._41, global_._42, global_._43, next_sound_frame_, sound_instance_vector_value_);
            }
        } else {
            next_sound_frame_ = reached + 1;
        }
    }

    struct effect_updator_fn {
        bool is_alive;
        float elapsed_time;
        effect_updator_fn(float elapsed_time_value)
            : is_alive(false), elapsed_time(elapsed_time_value) {
        }
        void operator()(effect_element_base_instance *instance) {
            if (instance->update(elapsed_time))
                is_alive = true;
        }
    };

    void effect_instance::on_update() {
        transform();

        effect_updator_fn f(get_timer().current_second() - last_time_);
        f = std::for_each(particle_instance_vector_.begin(), particle_instance_vector_.end(), f);
        f = std::for_each(mesh_instance_vector_.begin(), mesh_instance_vector_.end(), f);
        f = std::for_each(light_instance_vector_.begin(), light_instance_vector_.end(), f);
        is_alive_ = f.is_alive;

        last_time_ = get_timer().current_second();
    }

    void effect_instance::on_render() {
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, false);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::TEX1);
        std::for_each(particle_instance_vector_.begin(), particle_instance_vector_.end(), std::mem_fn(&effect_element_base_instance::render));
        std::for_each(mesh_instance_vector_.begin(), mesh_instance_vector_.end(), std::mem_fn(&effect_element_base_instance::render));

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MIN_FILTER);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MAG_FILTER);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
    }

    void effect_instance::set_global_matrix(const math::matrix &global) {
        global_ = global;
    }

    auto effect_instance::is_alive() -> bool {
        return is_alive_;
    }

    void effect_instance::set_active() {
        std::for_each(
            particle_instance_vector_.begin(),
            particle_instance_vector_.end(),
            std::mem_fn(&effect_element_base_instance::set_active));
        std::for_each(
            mesh_instance_vector_.begin(),
            mesh_instance_vector_.end(),
            std::mem_fn(&effect_element_base_instance::set_active));
        std::for_each(
            light_instance_vector_.begin(),
            light_instance_vector_.end(),
            std::mem_fn(&effect_element_base_instance::set_active));
    }

    void effect_instance::set_deactive() {
        std::for_each(
            particle_instance_vector_.begin(),
            particle_instance_vector_.end(),
            std::mem_fn(&effect_element_base_instance::set_deactive));
        std::for_each(
            mesh_instance_vector_.begin(),
            mesh_instance_vector_.end(),
            std::mem_fn(&effect_element_base_instance::set_deactive));
        std::for_each(
            light_instance_vector_.begin(),
            light_instance_vector_.end(),
            std::mem_fn(&effect_element_base_instance::set_deactive));
    }

    void effect_instance::set_particle_data(particle_system_data *data) {
        particle_system_instance *instance = particle_system_instance::new_value();
        instance->set_data_pointer(data);
        instance->set_local_matrix_pointer(&global_);

        particle_instance_vector_.push_back(instance);
    }
    void effect_instance::set_mesh_data(effect_mesh_script *mesh) {
        effect_mesh_instance *mesh_instance = effect_mesh_instance::new_value();
        mesh_instance->set_data_pointer(mesh);
        mesh_instance->set_local_matrix_pointer(&global_);

        mesh_instance_vector_.push_back(mesh_instance);
    }

    void effect_instance::set_light_data(light_data *data) {
        light_instance *instance = light_instance::new_value();
        instance->set_data_pointer(data);
        instance->set_local_matrix_pointer(&global_);

        light_instance_vector_.push_back(instance);
    }

    void effect_instance::set_effect_data_pointer(effect_data *effect_data) {
        is_alive_ = true;

        eft_data_ = effect_data;

        last_time_ = get_timer().current_second();
        bounding_sphere_radius_ = effect_data->bounding_sphere_radius();
        bounding_sphere_position_ = effect_data->bounding_sphere_position();

        if (bounding_sphere_radius_ > 0.0f)
            graphic_object_instance::register_bounding_sphere();

        uint32_t i;

        for (i = 0; i < effect_data->particle_count(); ++i) {
            particle_system_data *particle = effect_data->particle_pointer(i);

            set_particle_data(particle);
        }

        for (i = 0; i < effect_data->mesh_count(); ++i) {
            effect_mesh_script *mesh = effect_data->mesh_pointer(i);

            set_mesh_data(mesh);
        }

        for (i = 0; i < effect_data->light_count(); ++i) {
            light_data *light = effect_data->light_pointer(i);

            set_light_data(light);
        }

        sound_instance_vector_value_ = effect_data->get_sound_instance_vector();
    }

    auto effect_instance::bounding_sphere(math::vector3 &center, float &radius) -> bool {
        center.x = global_._41 + bounding_sphere_position_.x;
        center.y = global_._42 + bounding_sphere_position_.y;
        center.z = global_._43 + bounding_sphere_position_.z;
        radius = bounding_sphere_radius_;
        return true;
    }

    void effect_instance::clear() {
        if (!particle_instance_vector_.empty()) {
            std::for_each(particle_instance_vector_.begin(), particle_instance_vector_.end(), particle_system_instance::delete_value);
            particle_instance_vector_.clear();
        }

        if (!mesh_instance_vector_.empty()) {
            std::for_each(mesh_instance_vector_.begin(), mesh_instance_vector_.end(), effect_mesh_instance::delete_value);
            mesh_instance_vector_.clear();
        }

        if (!light_instance_vector_.empty()) {
            std::for_each(light_instance_vector_.begin(), light_instance_vector_.end(), light_instance::delete_value);
            light_instance_vector_.clear();
        }

        initialize_impl();
    }

    void effect_instance::initialize_impl() {
        is_alive_ = false;
        sound_time_ = 0.0f;
        next_sound_frame_ = 0;
        sound_instance_vector_value_ = nullptr;
        bounding_sphere_radius_ = 0.0f;
        bounding_sphere_position_.x = bounding_sphere_position_.y = bounding_sphere_position_.z = 0.0f;

        eft_data_ = nullptr;

        global_ = math::matrix_identity();
    }

    effect_instance::effect_instance() {
        initialize_impl();
    }
    effect_instance::~effect_instance() {
        assert(particle_instance_vector_.empty());
        assert(mesh_instance_vector_.empty());
        assert(light_instance_vector_.empty());
    }
}
