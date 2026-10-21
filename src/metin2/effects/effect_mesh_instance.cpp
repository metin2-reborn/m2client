#include "effect_mesh_instance.h"
#include <metin2/core/timer.h>
#include <metin2/geometry/grp_math.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/graphics/state_manager.h>

namespace metin2::inline effects {

    dynamic_pool<effect_mesh_instance> effect_mesh_instance::pool_value;

    void effect_mesh_instance::destroy_system() {
        pool_value.destroy();
    }

    auto effect_mesh_instance::new_value() -> effect_mesh_instance * {
        return pool_value.alloc();
    }

    void effect_mesh_instance::delete_value(effect_mesh_instance *mesh_instance) {
        mesh_instance->destroy();
        pool_value.free(mesh_instance);
    }

    auto effect_mesh_instance::is_active() -> bool {
        if (!effect_element_base_instance::is_active())
            return false;

        if (!mesh_frame_controller_.is_active())
            return false;

        for (uint32_t j = 0; j < texture_instance_vector_.size(); ++j) {
            int current_frame = mesh_frame_controller_.current_frame();
            if (texture_instance_vector_[j].texture_frame_controller.is_active(current_frame))
                return true;
        }

        return false;
    }

    auto effect_mesh_instance::on_update(float elapsed_time) -> bool {
        if (!is_active())
            return false;

        if (mesh_frame_controller_.is_active())
            mesh_frame_controller_.update(elapsed_time);

        for (uint32_t j = 0; j < texture_instance_vector_.size(); ++j) {
            int current_frame = mesh_frame_controller_.current_frame();
            if (texture_instance_vector_[j].texture_frame_controller.is_active(current_frame))
                texture_instance_vector_[j].texture_frame_controller.update(elapsed_time);
        }

        return true;
    }

    void effect_mesh_instance::on_render() {
        if (!is_active())
            return;

        effect_mesh *effect_mesh = ro_mesh_.pointer();

        for (uint32_t i = 0; i < effect_mesh->mesh_count(); ++i) {
            assert(i < texture_instance_vector_.size());

            frame_controller &texture_frame_controller = texture_instance_vector_[i].texture_frame_controller;
            if (!texture_frame_controller.is_active(mesh_frame_controller_.current_frame()))
                continue;

            int billboard_type = mesh_script_->billboard_type(i);

            math::matrix world;
            world = math::matrix_identity();

            switch (billboard_type) {
            case MESH_BILLBOARD_TYPE_ALL:
                {
                    math::matrix temp;
                    temp = math::matrix_rotation_x(90.0f);
                    math::inverse((screen::view_matrix()), world);

                    world = temp * world;
                }
                break;

            case MESH_BILLBOARD_TYPE_Y:
                {
                    math::matrix temp;
                    temp = math::matrix_identity();

                    math::inverse((screen::view_matrix()), temp);
                    world._11 = temp._11;
                    world._12 = temp._12;
                    world._21 = temp._21;
                    world._22 = temp._22;
                }
                break;

            case MESH_BILLBOARD_TYPE_MOVE:
                {
                    math::vector3 position;
                    mesh_script_->get_position(local_time_, position);
                    math::vector3 last_position;
                    mesh_script_->get_position(local_time_ - get_timer().elapsed_second(), last_position);
                    position -= last_position;
                    if (math::length_sq(position) > 0.001f) {
                        position = math::normalize(position);
                        math::quaternion q = safe_rotation_normalized_arc(math::vector3(0.0f, -1.0f, 0.0f), position);
                        world = math::matrix_rotation_quaternion(q);
                    }
                }
                break;
            }

            if (!mesh_script_->is_blending_enable(i)) {
                STATEMANAGER.set_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);
            } else {
                int blending_src_type = mesh_script_->blending_src_type(i);
                int blending_dest_type = mesh_script_->blending_dest_type(i);
                STATEMANAGER.set_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
                STATEMANAGER.set_render_state(gpu::state::SRC_BLEND, blending_src_type);
                STATEMANAGER.set_render_state(gpu::state::DEST_BLEND, blending_dest_type);
            }

            math::vector3 position;
            mesh_script_->get_position(local_time_, position);
            world._41 = position.x;
            world._42 = position.y;
            world._43 = position.z;
            world = world * *mc_pmat_local_;
            STATEMANAGER.set_transform(gpu::transform::WORLD, &world);

            uint8_t type;
            math::color color(1.0f, 1.0f, 1.0f, 1.0f);
            if (mesh_script_->color_operation_type(i, &type))
                STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, type);
            mesh_script_->color_factor(i, &color);

            time_event_table_float *table_alpha;

            float alpha = 1.0f;
            if (mesh_script_->time_table_alpha_pointer(i, &table_alpha) && !table_alpha->empty())
                get_time_event_blend_value(local_time_, *table_alpha, &alpha);

            effect_mesh::effect_mesh_data *mesh_data = effect_mesh->mesh_data_pointer(i);

            assert(mesh_frame_controller_.current_frame() < mesh_data->effect_frame_data_vector.size());
            effect_mesh::effect_frame_data &frame_data = mesh_data->effect_frame_data_vector[mesh_frame_controller_.current_frame()];

            uint32_t dwcur_texture_frame = texture_frame_controller.current_frame();
            if (dwcur_texture_frame < texture_instance_vector_[i].texture_instance_vector.size()) {
                graphic_image_instance *image_instance = texture_instance_vector_[i].texture_instance_vector[dwcur_texture_frame];
                STATEMANAGER.set_texture(0, image_instance->texture_pointer()->gpu_texture());
            }

            color.a = alpha * frame_data.visibility;
            STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, uint32_t(color));
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::TEX1);
            STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_LIST,
                frame_data.index_count / 3,
                &frame_data.pdt_vertex_vector[0],
                sizeof(pt_vertex));
        }
    }

    void effect_mesh_instance::on_set_data_pointer(effect_element_base *element) {
        auto *mesh = (effect_mesh_script *)element;
        mesh_script_ = mesh;

        const char *mesh_file_name = mesh->mesh_file_name();

        effect_mesh_value_ = (effect_mesh *)get_resource_manager().resource_pointer(mesh_file_name);

        if (!effect_mesh_value_)
            return;

        ro_mesh_.set_pointer(effect_mesh_value_);

        mesh_frame_controller_.clear();
        mesh_frame_controller_.set_max_frame(ro_mesh_.pointer()->frame_count());
        mesh_frame_controller_.set_frame_time(mesh->mesh_animation_frame_delay());
        mesh_frame_controller_.set_loop_flag(mesh->is_mesh_animation_loop());
        mesh_frame_controller_.set_loop_count(mesh->mesh_animation_loop_count());
        mesh_frame_controller_.set_start_frame(0);

        texture_instance_vector_.clear();
        texture_instance_vector_.resize(effect_mesh_value_->mesh_count());
        for (uint32_t j = 0; j < texture_instance_vector_.size(); ++j) {
            effect_mesh_script::mesh_data *mesh_data;
            if (!mesh_script_->mesh_data_pointer(j, &mesh_data))
                continue;

            effect_mesh *eft_mesh = ro_mesh_.pointer();

            if (!eft_mesh)
                continue;

            std::vector<graphic_image *> *p_texture_vector = eft_mesh->texture_vector_pointer(j);
            if (!p_texture_vector)
                continue;

            std::vector<graphic_image *> &r_texture_vector = *p_texture_vector;

            frame_controller &frame_controller = texture_instance_vector_[j].texture_frame_controller;
            frame_controller.clear();
            frame_controller.set_max_frame(r_texture_vector.size());
            frame_controller.set_frame_time(mesh_data->texture_animation_frame_delay);
            frame_controller.set_loop_flag(mesh_data->texture_animation_loop_enable);
            frame_controller.set_start_frame(mesh_data->texture_animation_start_frame);

            std::vector<graphic_image_instance *> &image_instance_vector = texture_instance_vector_[j].texture_instance_vector;
            image_instance_vector.clear();
            image_instance_vector.reserve(r_texture_vector.size());
            for (auto itor = r_texture_vector.begin(); itor != r_texture_vector.end(); ++itor) {
                graphic_image *image = *itor;
                graphic_image_instance *image_instance = graphic_image_instance::pool_value.alloc();
                image_instance->set_image_pointer(image);
                image_instance_vector.push_back(image_instance);
            }
        }
    }

    void c_effect_mesh_instance_delete_image_instance(graphic_image_instance *instance) {
        graphic_image_instance::pool_value.free(instance);
    }

    void c_effect_mesh_instance_delete_texture_instance(effect_mesh_instance::texture_instance &instance) {
        std::vector<graphic_image_instance *> &vector = instance.texture_instance_vector;
        for_each(vector.begin(), vector.end(), c_effect_mesh_instance_delete_image_instance);
        vector.clear();
    }

    void effect_mesh_instance::on_initialize() {
    }

    void effect_mesh_instance::on_destroy() {
        for_each(texture_instance_vector_.begin(), texture_instance_vector_.end(), c_effect_mesh_instance_delete_texture_instance);
        texture_instance_vector_.clear();
        ro_mesh_.set_pointer(nullptr);
    }

    effect_mesh_instance::effect_mesh_instance() {
        initialize();
    }

    effect_mesh_instance::~effect_mesh_instance() {
        destroy();
    }
}
