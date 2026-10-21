#include <metin2/graphics/state_manager.h>

#include <metin2/gpu/types.h>
#include "actor_instance.h"

namespace metin2::inline world::inline actors {

    bool actor_instance::is_dir_line_ = false;

    auto actor_instance::is_dir_line() -> bool {
        return is_dir_line_;
    }

    void actor_instance::show_direction_line(bool is_visible) {
        is_dir_line_ = is_visible;
    }

    void actor_instance::set_material_color(uint32_t color) {
        if (horse_)
            horse_->set_material_color(color);

        mtrl_color_ &= 0xff000000;
        mtrl_color_ |= (color & 0x00ffffff);
    }

    void actor_instance::on_render() {
        gpu::material mtrl;
        STATEMANAGER.get_material(&mtrl);

        mtrl.diffuse = math::color(mtrl_color_);
        STATEMANAGER.set_material(&mtrl);

        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);

        switch (render_mode_value_) {
        case RENDER_MODE_NORMAL:
            begin_diffuse_render();
            render_with_one_texture();
            end_diffuse_render();
            begin_opacity_render();
            blend_render_with_one_texture();
            end_opacity_render();
            break;
        case RENDER_MODE_BLEND:
            if (alpha_value_ == 1.0f) {
                begin_diffuse_render();
                render_with_one_texture();
                end_diffuse_render();
                begin_opacity_render();
                blend_render_with_one_texture();
                end_opacity_render();
            } else if (alpha_value_ > 0.0f) {
                begin_blend_render();
                render_with_one_texture();
                blend_render_with_one_texture();
                end_blend_render();
            }
            break;
        case RENDER_MODE_ADD:
            begin_add_render();
            render_with_one_texture();
            blend_render_with_one_texture();
            end_add_render();
            break;
        case RENDER_MODE_MODULATE:
            begin_modulate_render();
            render_with_one_texture();
            blend_render_with_one_texture();
            end_modulate_render();
            break;
        }

        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);

        mtrl.diffuse = math::color(0xffffffff);
        STATEMANAGER.set_material(&mtrl);

        if (is_dir_line_) {
            math::vector3 current_position(x_, y_, z_);

            math::vector3 look_direction(0.0f, -1.0f, 0.0f);
            math::matrix look_matrix;
            look_matrix = math::matrix_rotation_z(math::to_radian(rotation()));
            look_direction = math::transform_coord(look_direction, look_matrix);
            look_direction = look_direction * 200.0f;
            look_direction = look_direction + current_position;

            math::vector3 advance_direction(0.0f, -1.0f, 0.0f);
            math::matrix advance_matrix;
            advance_matrix = math::matrix_rotation_z(math::to_radian(advancing_rotation()));
            advance_direction = math::transform_coord(advance_direction, advance_matrix);
            advance_direction = advance_direction * 200.0f;
            advance_direction = advance_direction + current_position;

            static screen screen;

            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::DIFFUSE);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
            STATEMANAGER.save_render_state(gpu::state::Z_ENABLE, false);
            STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);

            screen.set_diffuse_color(1.0f, 1.0f, 0.0f);
            screen.render_line3d(current_position.x, current_position.y, current_position.z, advance_direction.x, advance_direction.y, advance_direction.z);

            screen.set_diffuse_color(0.0f, 1.0f, 1.0f);
            screen.render_line3d(current_position.x, current_position.y, current_position.z, look_direction.x, look_direction.y, look_direction.z);

            STATEMANAGER.set_render_state(gpu::state::LIGHTING, true);
            STATEMANAGER.restore_render_state(gpu::state::Z_ENABLE);

            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG1);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_OP);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_OP);
            STATEMANAGER.restore_vertex_shader();
        }
    }

    void actor_instance::begin_diffuse_render() {
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);
    }

    void actor_instance::end_diffuse_render() {
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
    }

    void actor_instance::begin_opacity_render() {
        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_REF, 0);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);
    }

    void actor_instance::end_opacity_render() {
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_REF);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_FUNC);
    }

    void actor_instance::begin_blend_render() {
        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, math::color(1.0f, 1.0f, 1.0f, alpha_value_));
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG2);
    }

    void actor_instance::end_blend_render() {
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
    }

    void actor_instance::begin_add_render() {
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, add_color_);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::ADD);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);
    }

    void actor_instance::end_add_render() {
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
    }

    void actor_instance::restore_render_mode() {
        // TODO: restore the model's own default render mode (such as specular) instead of always
        // using normal.
        render_mode_value_ = RENDER_MODE_NORMAL;
        if (blend_alpha_value_.is_blending) {
            blend_alpha_value_.old_render_mode = render_mode_value_;
        }
    }

    void actor_instance::set_add_render_mode() {
        render_mode_value_ = RENDER_MODE_ADD;
        if (blend_alpha_value_.is_blending) {
            blend_alpha_value_.old_render_mode = render_mode_value_;
        }
    }

    void actor_instance::set_render_mode(int render_mode_value) {
        render_mode_value_ = render_mode_value;
        if (blend_alpha_value_.is_blending) {
            blend_alpha_value_.old_render_mode = render_mode_value;
        }
    }

    void actor_instance::set_add_color(const math::color &color) {
        add_color_ = color;
        add_color_.a = 1.0f;
    }

    void actor_instance::begin_modulate_render() {
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, add_color_);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);
    }

    void actor_instance::end_modulate_render() {
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
    }

    void actor_instance::set_modulate_render_mode() {
        render_mode_value_ = RENDER_MODE_MODULATE;
        if (blend_alpha_value_.is_blending) {
            blend_alpha_value_.old_render_mode = render_mode_value_;
        }
    }

    void actor_instance::render_collision_data() {
        static screen screen;

        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
        if (attribute_instance_value_) {
            for (uint32_t col = 0; col < collision_instance_count(); ++col) {
                base_collision_instance *p_instance = collision_instance_data(col);
                p_instance->render();
            }
        }

        STATEMANAGER.set_render_state(gpu::state::Z_ENABLE, false);
        screen.set_color_operation();
        screen.set_diffuse_color(1.0f, 0.0f, 0.0f);
        collision_point_instance_list::iterator itor;
        screen.set_diffuse_color(1.0f, (is_show()) ? 1.0f : 0.0f, 0.0f);
        math::vector3 center;
        float r;
        bounding_sphere(center, r);
        screen.render_circle3d(center.x, center.y, center.z, r);

        screen.set_diffuse_color(0.0f, 0.0f, 1.0f);
        itor = defending_point_instance_list_.begin();
        for (; itor != defending_point_instance_list_.end(); ++itor) {
            const collision_point_instance &c_r_instance = *itor;
            for (uint32_t i = 0; i < c_r_instance.sphere_instance_vector.size(); ++i) {
                const dynamic_sphere_instance &sphere_instance = c_r_instance.sphere_instance_vector[i];
                screen.render_circle3d(sphere_instance.position_value.x,
                    sphere_instance.position_value.y,
                    sphere_instance.position_value.z,
                    sphere_instance.radius);
            }
        }

        screen.set_diffuse_color(0.0f, 1.0f, 0.0f);
        itor = body_point_instance_list_.begin();
        for (; itor != body_point_instance_list_.end(); ++itor) {
            const collision_point_instance &instance = *itor;
            for (uint32_t i = 0; i < instance.sphere_instance_vector.size(); ++i) {
                const dynamic_sphere_instance &sphere_instance = instance.sphere_instance_vector[i];
                screen.render_circle3d(sphere_instance.position_value.x,
                    sphere_instance.position_value.y,
                    sphere_instance.position_value.z,
                    sphere_instance.radius);
            }
        }

        screen.set_diffuse_color(1.0f, 0.0f, 0.0f);
        {
            auto itor = splash_area_value_.sphere_instance_vector.begin();
            for (; itor != splash_area_value_.sphere_instance_vector.end(); ++itor) {
                const dynamic_sphere_instance &instance = *itor;
                screen.render_circle3d(instance.position_value.x,
                    instance.position_value.y,
                    instance.position_value.z,
                    instance.radius);
            }
        }

        STATEMANAGER.set_render_state(gpu::state::Z_ENABLE, true);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, true);
    }

    void actor_instance::render_to_shadow_map() {
        if (RENDER_MODE_BLEND == render_mode_value_)
            if (alpha_value() < 0.5f)
                return;

        graphic_thing_instance::render_to_shadow_map();

        if (horse_)
            horse_->render_to_shadow_map();
    }
}
