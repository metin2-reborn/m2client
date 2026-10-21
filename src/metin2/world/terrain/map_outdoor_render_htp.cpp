#include "map_outdoor.h"

#include <metin2/gpu/types.h>
#include <metin2/graphics/state_manager.h>

namespace metin2::inline world::inline terrain {

    void map_outdoor::render_terrain_render_hardware_transform_patch() {
        uint32_t fog_color;
        float fog_far_distance;
        float fog_near_distance;
        if (mc_environment_data_) {
            fog_color = mc_environment_data_->fog_color;
            fog_near_distance = mc_environment_data_->get_fog_near_distance();
            fog_far_distance = mc_environment_data_->get_fog_far_distance();
        } else {
            fog_color = 0xffffffff;
            fog_near_distance = 5000.0f;
            fog_far_distance = 10000.0f;
        }

        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX, gpu::texcoord::CAMERA_SPACE_POSITION);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX, gpu::texcoord::CAMERA_SPACE_POSITION);
        STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_REF, 0x00000000);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER);

        STATEMANAGER.save_render_state(gpu::state::TEXTURE_FACTOR, fog_color);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);

        speed_tree_wrapper::self_shadow_on = true;
        STATEMANAGER.set_best_filtering(0);
        STATEMANAGER.set_best_filtering(1);

        world_for_common_use_._41 = 0.0f;
        world_for_common_use_._42 = 0.0f;
        STATEMANAGER.set_transform(gpu::transform::WORLD, &world_for_common_use_);

        STATEMANAGER.save_transform(gpu::transform::TEXTURE0, &world_for_common_use_);
        STATEMANAGER.save_transform(gpu::transform::TEXTURE1, &world_for_common_use_);

        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::NORMAL);

        rendered_splat_num_sq_sum_ = 0;
        rendered_patch_num_ = 0;
        rendered_splat_num_ = 0;
        rendered_texture_num_vector_.clear();

        std::pair<float, int32_t> fog_far(fog_far_distance + 1600.0f, 0);
        std::pair<float, int32_t> fog_near(fog_near_distance - 3200.0f, 0);

        auto far_it = std::upper_bound(patch_vector_.begin(), patch_vector_.end(), fog_far);
        auto near_it = std::upper_bound(patch_vector_.begin(), patch_vector_.end(), fog_near);

        uint16_t primitive_count;
        gpu::primitive primitive_type;

        uint8_t c_urrent_lod_level = 0;

        float lod_level1_distance = no_fog_distance();
        float lod_level2_distance = fog_distance();

        select_index_buffer(0, &primitive_count, &primitive_type);

        uint32_t fog_enable = STATEMANAGER.render_state(gpu::state::FOG_ENABLE);
        auto it = patch_vector_.begin();

        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, false);

        for (; it != near_it; ++it) {
            if (c_urrent_lod_level == 0 && lod_level1_distance <= it->first) {
                c_urrent_lod_level = 1;
                select_index_buffer(1, &primitive_count, &primitive_type);
            } else if (c_urrent_lod_level == 1 && lod_level2_distance <= it->first) {
                c_urrent_lod_level = 2;
                select_index_buffer(2, &primitive_count, &primitive_type);
            }

            hardware_transform_patch_render_patch_splat(it->second, primitive_count, primitive_type);
            if (rendered_splat_num_ >= splat_limit_)
                break;
        }

        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, fog_enable);

        if (rendered_splat_num_ < splat_limit_) {
            for (it = near_it; it != far_it; ++it) {
                if (c_urrent_lod_level == 0 && lod_level1_distance <= it->first) {
                    c_urrent_lod_level = 1;
                    select_index_buffer(1, &primitive_count, &primitive_type);
                } else if (c_urrent_lod_level == 1 && lod_level2_distance <= it->first) {
                    c_urrent_lod_level = 2;
                    select_index_buffer(2, &primitive_count, &primitive_type);
                }

                hardware_transform_patch_render_patch_splat(it->second, primitive_count, primitive_type);

                if (rendered_splat_num_ >= splat_limit_)
                    break;
            }
        }

        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, false);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);

        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, false);

        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, false);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        if (rendered_splat_num_ < splat_limit_) {
            for (it = far_it; it != patch_vector_.end(); ++it) {
                if (c_urrent_lod_level == 0 && lod_level1_distance <= it->first) {
                    c_urrent_lod_level = 1;
                    select_index_buffer(1, &primitive_count, &primitive_type);
                } else if (c_urrent_lod_level == 1 && lod_level2_distance <= it->first) {
                    c_urrent_lod_level = 2;
                    select_index_buffer(2, &primitive_count, &primitive_type);
                }

                hardware_transform_patch_render_patch_none(it->second, primitive_count, primitive_type);

                if (rendered_splat_num_ >= splat_limit_)
                    break;
            }
        }

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, fog_enable);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, true);

        std::sort(rendered_texture_num_vector_.begin(), rendered_texture_num_vector_.end());

        STATEMANAGER.restore_render_state(gpu::state::TEXTURE_FACTOR);

        STATEMANAGER.restore_transform(gpu::transform::TEXTURE0);
        STATEMANAGER.restore_transform(gpu::transform::TEXTURE1);

        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX);
        STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);

        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_REF);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_FUNC);
    }

    void map_outdoor::hardware_transform_patch_render_patch_splat(int32_t patchnum, uint16_t primitive_count, gpu::primitive primitive_type) {
        assert(nullptr != terrain_patch_proxy_list_ && "__HardwareTransformPatch_RenderPatchSplat");
        terrain_patch_proxy *terrain_patch_proxy = &terrain_patch_proxy_list_[patchnum];

        if (!terrain_patch_proxy->is_used())
            return;

        int32_t patch_num = terrain_patch_proxy->patch_num();
        if (patch_num < 0)
            return;

        uint8_t terrain_num = terrain_patch_proxy->terrain_num();
        if (0xFF == terrain_num)
            return;

        terrain_area *terrain;
        if (!terrain_pointer(terrain_num, &terrain))
            return;

        uint32_t fog_color;
        if (mc_environment_data_)
            fog_color = mc_environment_data_->fog_color;
        else
            fog_color = 0xffffffff;

        uint16_t coord_x, coord_y;
        terrain->coordinate(&coord_x, &coord_y);

        terrain_splat_patch &terrain_splat_patch = terrain->get_terrain_splat_patch();

        math::matrix tex_transform, splat_alpha_tex_transform, splat_color_tex_transform;
        world_for_common_use_._41 = -(float)(coord_x * terrain_impl::TERRAIN_XSIZE);
        world_for_common_use_._42 = (float)(coord_y * terrain_impl::TERRAIN_YSIZE);
        tex_transform = view_inverse_ * world_for_common_use_;
        splat_alpha_tex_transform = tex_transform * splat_alpha_;
        STATEMANAGER.set_transform(gpu::transform::TEXTURE1, &splat_alpha_tex_transform);

        math::matrix tiling;
        tiling = math::matrix_scaling(1.0f / 640.0f, -1.0f / 640.0f, 0.0f);
        tiling._41 = 0.0f;
        tiling._42 = 0.0f;

        splat_color_tex_transform = view_inverse_ * tiling;
        STATEMANAGER.set_transform(gpu::transform::TEXTURE0, &splat_color_tex_transform);

        graphic_vertex_buffer *vb = terrain_patch_proxy->hardware_transform_patch_get_vertex_buffer_ptr();
        if (!vb)
            return;

        STATEMANAGER.set_stream_source(0, vb->gpu_buffer(), patch_terrain_vertex_size_);

        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);

        int prev_rendered_splat_num = rendered_splat_num_;

        bool is_first = true;
        for (uint32_t j = 1; j < terrain->num_textures(); ++j) {
            terain_splat &splat = terrain_splat_patch.splats[j];

            if (!splat.active)
                continue;

            if (terrain_splat_patch.patch_tile_count[patch_num][j] == 0)
                continue;

            const terrain_texture &texture = texture_set_value_.texture(j);

            splat_color_tex_transform = view_inverse_ * texture.transform;
            STATEMANAGER.set_transform(gpu::transform::TEXTURE0, &splat_color_tex_transform);
            if (is_first) {
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
                STATEMANAGER.set_texture(0, texture.gpu_texture);
                STATEMANAGER.set_texture(1, splat.gpu_texture);
                STATEMANAGER.draw_indexed_primitive(primitive_type, 0, patch_terrain_vertex_count_, 0, primitive_count);
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
                is_first = false;
            } else {
                STATEMANAGER.set_texture(0, texture.gpu_texture);
                STATEMANAGER.set_texture(1, splat.gpu_texture);
                STATEMANAGER.draw_indexed_primitive(primitive_type, 0, patch_terrain_vertex_count_, 0, primitive_count);
            }

            auto iterator = std::find(rendered_texture_num_vector_.begin(), rendered_texture_num_vector_.end(), (int)j);
            if (iterator == rendered_texture_num_vector_.end())
                rendered_texture_num_vector_.push_back(j);
            ++rendered_splat_num_;
            if (rendered_splat_num_ >= splat_limit_)
                break;
        }

        if (draw_shadow_) {
            STATEMANAGER.set_render_state(gpu::state::LIGHTING, true);

            STATEMANAGER.set_render_state(gpu::state::FOG_COLOR, 0xFFFFFFFF);
            STATEMANAGER.set_render_state(gpu::state::SRC_BLEND, gpu::blend::ZERO);
            STATEMANAGER.set_render_state(gpu::state::DEST_BLEND, gpu::blend::SRC_COLOR);

            math::matrix shadow_tex_transform;
            shadow_tex_transform = tex_transform * static_shadow_;

            STATEMANAGER.set_transform(gpu::transform::TEXTURE0, &shadow_tex_transform);
            STATEMANAGER.set_texture(0, terrain->shadow_texture());

            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::CURRENT);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);

            if (draw_chr_shadow_) {
                STATEMANAGER.set_transform(gpu::transform::TEXTURE1, &dynamic_shadow_);

                STATEMANAGER.set_texture(1, character_shadow_target_.texture());
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);
            } else {
                STATEMANAGER.set_texture(1, nullptr);
            }

            face_count_ += primitive_count;
            STATEMANAGER.draw_indexed_primitive(primitive_type, 0, patch_terrain_vertex_count_, 0, primitive_count);
            ++rendered_splat_num_;

            if (draw_chr_shadow_) {
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
            }

            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);

            STATEMANAGER.set_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
            STATEMANAGER.set_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);
            STATEMANAGER.set_render_state(gpu::state::FOG_COLOR, fog_color);

            STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);
        }
        ++rendered_patch_num_;

        int cur_rendered_splat_num = rendered_splat_num_ - prev_rendered_splat_num;

        rendered_splat_num_sq_sum_ += cur_rendered_splat_num * cur_rendered_splat_num;
    }

    void map_outdoor::hardware_transform_patch_render_patch_none(int32_t patchnum, uint16_t primitive_count, gpu::primitive primitive_type) {
        assert(nullptr != terrain_patch_proxy_list_ && "__HardwareTransformPatch_RenderPatchNone");
        terrain_patch_proxy *terrain_patch_proxy = &terrain_patch_proxy_list_[patchnum];

        if (!terrain_patch_proxy->is_used())
            return;

        graphic_vertex_buffer *vb = terrain_patch_proxy->hardware_transform_patch_get_vertex_buffer_ptr();
        if (!vb)
            return;

        STATEMANAGER.set_stream_source(0, vb->gpu_buffer(), patch_terrain_vertex_size_);
        STATEMANAGER.draw_indexed_primitive(primitive_type, 0, patch_terrain_vertex_count_, 0, primitive_count);
    }
}
