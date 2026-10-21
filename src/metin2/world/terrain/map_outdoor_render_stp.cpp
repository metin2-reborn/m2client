#include "map_outdoor.h"
#include "terrain_patch.h"
#include "terrain_quadtree.h"

#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/state_manager.h>

namespace metin2::inline world::inline terrain {

    struct software_transform_patch_splat_vertex {
        math::vector4 position_value;
        uint32_t diffuse_value;
        uint32_t specular_value;
        math::vector2 tex1;
        math::vector2 tex2;
    };

    void map_outdoor::render_terrain_render_software_transform_patch() {
        software_transform_patch_render_state tprs;

        uint32_t fog_enable = STATEMANAGER.render_state(gpu::state::FOG_ENABLE);

        software_transform_patch_apply_render_state();

        software_transform_patch_build_pipeline(tprs);

        std::pair<float, int32_t> fog_far(tprs.fog_far_distance + 800.0f, 0);
        std::pair<float, int32_t> fog_near(tprs.fog_near_distance - 3200.0f, 0);

        auto far_it = std::upper_bound(patch_vector_.begin(), patch_vector_.end(), fog_far);
        auto near_it = std::upper_bound(patch_vector_.begin(), patch_vector_.end(), fog_near);

        uint16_t primitive_count;
        gpu::primitive primitive_type;

        uint8_t c_urrent_lod_level = 0;

        float lod_level1_distance = no_fog_distance();
        float lod_level2_distance = fog_distance();

        select_index_buffer(0, &primitive_count, &primitive_type);

        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZRHW | gpu::fvf::DIFFUSE | gpu::fvf::SPECULAR | gpu::fvf::TEX2);

        auto it = patch_vector_.begin();

        for (; it != near_it; ++it) {
            if (c_urrent_lod_level == 0 && lod_level1_distance <= it->first) {
                c_urrent_lod_level = 1;
                select_index_buffer(1, &primitive_count, &primitive_type);
            } else if (c_urrent_lod_level == 1 && lod_level2_distance <= it->first) {
                c_urrent_lod_level = 2;
                select_index_buffer(2, &primitive_count, &primitive_type);
            }

            software_transform_patch_render_patch_splat(tprs, it->second, primitive_count, primitive_type, false);
            if (rendered_splat_num_ >= splat_limit_)
                break;
        }

        if (rendered_splat_num_ < splat_limit_) {
            for (it = near_it; it != far_it; ++it) {
                if (c_urrent_lod_level == 0 && lod_level1_distance <= it->first) {
                    c_urrent_lod_level = 1;
                    select_index_buffer(1, &primitive_count, &primitive_type);
                } else if (c_urrent_lod_level == 1 && lod_level2_distance <= it->first) {
                    c_urrent_lod_level = 2;
                    select_index_buffer(2, &primitive_count, &primitive_type);
                }

                software_transform_patch_render_patch_splat(tprs, it->second, primitive_count, primitive_type, true);

                if (rendered_splat_num_ >= splat_limit_)
                    break;
            }
        }

        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture(1, nullptr);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZRHW);

        if (c_urrent_lod_level != 2) {
            c_urrent_lod_level = 2;
            select_index_buffer(2, &primitive_count, &primitive_type);
        }

        if (rendered_splat_num_ < splat_limit_) {
            for (it = far_it; it != patch_vector_.end(); ++it) {
                software_transform_patch_render_patch_none(tprs, it->second, primitive_count, primitive_type);

                if (rendered_splat_num_ >= splat_limit_)
                    break;
            }
        }

        software_transform_patch_restore_render_state(fog_enable);
    }

    void map_outdoor::software_transform_patch_render_patch_splat(software_transform_patch_render_state &tprs, int32_t patchnum, uint16_t primitive_count, gpu::primitive primitive_type, bool is_fog_enable) {
        assert(nullptr != terrain_patch_proxy_list_ && "CMapOutdoor::__SoftwareTransformPatch_RenderPatchSplat");

        terrain_patch_proxy *terrain_patch_proxy = &terrain_patch_proxy_list_[patchnum];

        if (!terrain_patch_proxy->is_used())
            return;

        bool is_dynamic_shadow = terrain_patch_proxy->is_in(tprs.player, 3000.0f);

        if (!draw_chr_shadow_)
            is_dynamic_shadow = false;

        int32_t patch_num = terrain_patch_proxy->patch_num();

        if (patch_num < 0)
            return;

        uint8_t terrain_num = terrain_patch_proxy->terrain_num();

        if (0xFF == terrain_num)
            return;

        terrain_area *terrain;
        if (!terrain_pointer(terrain_num, &terrain))
            return;

        uint16_t coord_x, coord_y;
        terrain->coordinate(&coord_x, &coord_y);

        terrain_splat_patch &terrain_splat_patch = terrain->get_terrain_splat_patch();

        software_transform_patch_tl_vertex trans_vertex[terrain_patch::TERRAIN_VERTEX_COUNT];
        if (!software_transform_patch_set_transform(tprs, trans_vertex, *terrain_patch_proxy, coord_x, coord_y, is_fog_enable, is_dynamic_shadow))
            return;

        if (!software_transform_patch_set_splat_stream(trans_vertex))
            return;

        if (is_fog_enable) {
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TFACTOR);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::BLEND_DIFFUSE_ALPHA);
        } else {
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        }

        int prev_rendered_splat_num = rendered_splat_num_;

        bool is_first = true;
        for (uint32_t j = 1; j < terrain->num_textures(); ++j) {
            terain_splat &splat = terrain_splat_patch.splats[j];

            if (!splat.active)
                continue;

            if (terrain_splat_patch.patch_tile_count[patch_num][j] == 0)
                continue;

            const terrain_texture &texture = texture_set_value_.texture(j);

            if (is_first) {
                STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG2);
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
            software_transform_patch_set_shadow_stream(trans_vertex);
            software_transform_patch_apply_static_shadow_render_state();

            if (is_dynamic_shadow)
                software_transform_patch_apply_dynamic_shadow_render_state();
            else
                software_transform_patch_apply_fog_shadow_render_state();

            if (is_fog_enable) {
                STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, true);
                STATEMANAGER.set_render_state(gpu::state::FOG_COLOR, 0xFFFFFFFF);
                STATEMANAGER.set_texture(0, terrain->shadow_texture());
                STATEMANAGER.draw_indexed_primitive(primitive_type, 0, patch_terrain_vertex_count_, 0, primitive_count);
                STATEMANAGER.set_render_state(gpu::state::FOG_COLOR, tprs.fog_color);
                STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, false);
            } else {
                STATEMANAGER.set_texture(0, terrain->shadow_texture());
                STATEMANAGER.draw_indexed_primitive(primitive_type, 0, patch_terrain_vertex_count_, 0, primitive_count);
            }

            if (is_dynamic_shadow)
                software_transform_patch_restore_dynamic_shadow_render_state();
            else
                software_transform_patch_restore_fog_shadow_render_state();

            face_count_ += primitive_count;
            ++rendered_splat_num_;

            software_transform_patch_restore_static_shadow_render_state();
        }

        ++rendered_patch_num_;

        int cur_rendered_splat_num = rendered_splat_num_ - prev_rendered_splat_num;

        rendered_splat_num_sq_sum_ += cur_rendered_splat_num * cur_rendered_splat_num;
    }

    void map_outdoor::software_transform_patch_render_patch_none(software_transform_patch_render_state &tprs, int32_t patchnum, uint16_t primitive_count, gpu::primitive primitive_type) {
        assert(nullptr != terrain_patch_proxy_list_ && "CMapOutdoor::__SoftwareTransformPatch_RenderPatchNone");

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

        uint16_t coord_x, coord_y;
        terrain->coordinate(&coord_x, &coord_y);

        software_transform_patch_source_vertex *src_vertex = terrain_patch_proxy->software_transform_patch_get_terrain_vertex_data_ptr();
        if (!src_vertex)
            return;

        float screen_half_width = tprs.screen_half_width;
        float screen_half_height = tprs.screen_half_height;

        math::matrix m4_frustum = tprs.m4_frustum;

        software_transform_patch_t_vertex trans_vertex[terrain_patch::TERRAIN_VERTEX_COUNT];

        auto *ak_position = (math::vector4 *)trans_vertex;
        math::vector4 *pk_position;
        for (uint32_t index = 0; index != terrain_patch::TERRAIN_VERTEX_COUNT; ++index) {
            pk_position = ak_position + index;
            *pk_position = math::transform(src_vertex[index].position_value, m4_frustum);
            pk_position->w = 1.0f / pk_position->w;
            pk_position->z *= pk_position->w;
            pk_position->y = (pk_position->y * pk_position->w - 1.0f) * screen_half_height;
            pk_position->x = (pk_position->x * pk_position->w + 1.0f) * screen_half_width;
        }

        gpu::vertex_buffer *vb = stpd_.vb_none[stpd_.none_pos++].get();
        stpd_.none_pos %= software_transform_patch_data::NONE_VB_NUM;
        if (!vb)
            return;

        uint32_t vb_size = sizeof(software_transform_patch_t_vertex) * terrain_patch::TERRAIN_VERTEX_COUNT;
        void *dst_vertex;
        if (!vb->lock(0, vb_size, &dst_vertex, gpu::lock::DISCARD))
            return;

        memcpy(dst_vertex, trans_vertex, vb_size);

        vb->unlock();

        STATEMANAGER.set_stream_source(0, vb, sizeof(software_transform_patch_t_vertex));
        STATEMANAGER.draw_indexed_primitive(primitive_type, 0, patch_terrain_vertex_count_, 0, primitive_count);
        face_count_ += primitive_count;
    }

    void map_outdoor::software_transform_patch_apply_static_shadow_render_state() {
        STATEMANAGER.set_render_state(gpu::state::SRC_BLEND, gpu::blend::ZERO);
        STATEMANAGER.set_render_state(gpu::state::DEST_BLEND, gpu::blend::SRC_COLOR);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);
    }

    void map_outdoor::software_transform_patch_apply_dynamic_shadow_render_state() {
        STATEMANAGER.set_texture(1, character_shadow_target_.texture());
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);
    }

    void map_outdoor::software_transform_patch_apply_fog_shadow_render_state() {
        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
    }
    void map_outdoor::software_transform_patch_restore_static_shadow_render_state() {
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);

        STATEMANAGER.set_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.set_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);
    }

    void map_outdoor::software_transform_patch_restore_dynamic_shadow_render_state() {
        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);
    }

    void map_outdoor::software_transform_patch_restore_fog_shadow_render_state() {
        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, false);

        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
    }

    void map_outdoor::software_transform_patch_apply_render_state() {
        uint32_t fog_color = 0xffffffff;
        if (mc_environment_data_)
            fog_color = mc_environment_data_->fog_color;

        STATEMANAGER.save_render_state(gpu::state::SOFTWARE_VERTEX_PROCESSING, false);

        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_REF, 0x00000000);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER);

        STATEMANAGER.save_render_state(gpu::state::TEXTURE_FACTOR, fog_color);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, false);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);
        STATEMANAGER.set_best_filtering(0);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);
        STATEMANAGER.set_best_filtering(1);

        speed_tree_wrapper::self_shadow_on = true;

        rendered_splat_num_sq_sum_ = 0;
        rendered_patch_num_ = 0;
        rendered_splat_num_ = 0;
        rendered_texture_num_vector_.clear();

        world_for_common_use_._41 = 0.0f;
        world_for_common_use_._42 = 0.0f;
        STATEMANAGER.set_transform(gpu::transform::WORLD, &world_for_common_use_);
    }

    void map_outdoor::software_transform_patch_restore_render_state(uint32_t fog_enable) {
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, true);

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

        std::sort(rendered_texture_num_vector_.begin(), rendered_texture_num_vector_.end());

        STATEMANAGER.restore_render_state(gpu::state::TEXTURE_FACTOR);

        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_REF);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_FUNC);

        STATEMANAGER.restore_render_state(gpu::state::SOFTWARE_VERTEX_PROCESSING);
    }

    void map_outdoor::software_transform_patch_build_pipeline(software_transform_patch_render_state &tprs) {
        memset(static_cast<void *>(&tprs), 0, sizeof(tprs));

        if (mc_environment_data_) {
            tprs.fog_color = mc_environment_data_->fog_color;
            tprs.fog_near_distance = mc_environment_data_->get_fog_near_distance();
            tprs.fog_far_distance = mc_environment_data_->get_fog_far_distance();
        } else {
            tprs.fog_color = 0xffffffff;
            tprs.fog_near_distance = 5000.0f;
            tprs.fog_far_distance = 10000.0f;
        }

        uint32_t screen_width;
        uint32_t screen_height;
        screen::back_buffer_size(&screen_width, &screen_height);

        tprs.screen_half_width = +float(screen_width) / 2.0f;
        tprs.screen_half_height = -float(screen_height) / 2.0f;

        STATEMANAGER.get_light(0, &tprs.light_value);
        STATEMANAGER.get_material(&tprs.mtrl);

        math::matrix m4_view;
        STATEMANAGER.get_transform(gpu::transform::VIEW, &m4_view);
        math::matrix m4_proj;
        STATEMANAGER.get_transform(gpu::transform::PROJECTION, &m4_proj);

        tprs.m4_frustum = m4_view * m4_proj;

        tprs.player.x = +player_.x;
        tprs.player.y = -player_.y;
        tprs.player.z = +player_.z;

        tprs.m4_proj = m4_proj;
        tprs.m4_dynamic_shadow = light_view_ * dynamic_shadow_scale_;

        math::vector3 fog_near_vector;
        const auto farvv = math::vector3(0.0f, 0.0f, -tprs.fog_near_distance);
        fog_near_vector = math::transform_coord(farvv, tprs.m4_proj);

        math::vector3 fog_far_vector;
        const auto nearvv = math::vector3(0.0f, 0.0f, -tprs.fog_far_distance);
        fog_far_vector = math::transform_coord(nearvv, tprs.m4_proj);

        float fog_near = fog_near_vector.z;
        float fog_far = fog_far_vector.z;
        float fog_len_inv = 1.0f / (fog_far - fog_near);

        tprs.fog_near_trans_z = fog_near;
        tprs.fog_far_trans_z = fog_far;
        tprs.fog_len_inv = fog_len_inv;
    }

    auto map_outdoor::software_transform_patch_set_transform(software_transform_patch_render_state &tprs, software_transform_patch_tl_vertex *trans_vertex, terrain_patch_proxy &terrain_patch_proxy, uint32_t terrain_x, uint32_t terrain_y, bool is_fog_enable, bool is_dynamic_shadow) -> bool {
        software_transform_patch_source_vertex *src_vertex = terrain_patch_proxy.software_transform_patch_get_terrain_vertex_data_ptr();
        if (!src_vertex)
            return false;

        terrain_patch_proxy.software_transform_patch_update_terrain_lighting(
            stpd_.light_version,
            tprs.light_value, tprs.mtrl);

        math::vector3 *src_position;

        float tile_pattern_x = +1 / 640.0f;
        float tile_pattern_y = -1 / 640.0f;

        float terrain_base_x = -(float)(terrain_x * terrain_impl::TERRAIN_XSIZE) + terrain_tex_coord_base_ * 12.30769f;
        float terrain_base_y = +(float)(terrain_y * terrain_impl::TERRAIN_YSIZE) + terrain_tex_coord_base_ * 12.30769f;

        float screen_half_width = tprs.screen_half_width;
        float screen_half_height = tprs.screen_half_height;

        float alpha_pattern_x = splat_alpha_._11;
        float alpha_pattern_y = splat_alpha_._22;
        float alpha_bias_x = splat_alpha_._41;
        float alpha_bias_y = splat_alpha_._42;
        float shadow_pattern_x = +terrain_tex_coord_base_ * ((float)terrain_impl::PATCH_XSIZE / static_cast<float>(terrain_impl::XSIZE));
        float shadow_pattern_y = -terrain_tex_coord_base_ * ((float)terrain_impl::PATCH_YSIZE / static_cast<float>(terrain_impl::YSIZE));

        math::matrix m4_frustum = tprs.m4_frustum;

        if (is_fog_enable) {
            float fog_cur;
            float fog_far = tprs.fog_far_trans_z;
            float fog_len_inv = tprs.fog_len_inv;

            float local_x;
            float local_y;

            software_transform_patch_tl_vertex work_vertex;
            for (uint32_t index = 0; index != terrain_patch::TERRAIN_VERTEX_COUNT; ++index) {
                src_position = &src_vertex[index].position_value;
                work_vertex.position_value = math::transform(*src_position, m4_frustum);
                local_x = src_position->x + terrain_base_x;
                local_y = src_position->y + terrain_base_y;
                work_vertex.position_value.w = 1.0f / work_vertex.position_value.w;
                work_vertex.position_value.x *= work_vertex.position_value.w;
                work_vertex.position_value.y *= work_vertex.position_value.w;
                work_vertex.position_value.z *= work_vertex.position_value.w;
                work_vertex.position_value.x = (work_vertex.position_value.x + 1.0f) * screen_half_width;
                work_vertex.position_value.y = (work_vertex.position_value.y - 1.0f) * screen_half_height;
                work_vertex.diffuse_value = src_vertex[index].diffuse_value;
                work_vertex.tex_tile.x = src_position->x * tile_pattern_x;
                work_vertex.tex_tile.y = src_position->y * tile_pattern_y;
                work_vertex.tex_alpha.x = local_x * alpha_pattern_x + alpha_bias_x;
                work_vertex.tex_alpha.y = local_y * alpha_pattern_y + alpha_bias_y;
                work_vertex.tex_static_shadow.x = local_x * shadow_pattern_x;
                work_vertex.tex_static_shadow.y = local_y * shadow_pattern_y;
                work_vertex.tex_dynamic_shadow.x = 0.0f;
                work_vertex.tex_dynamic_shadow.y = 0.0f;

                fog_cur = (fog_far - work_vertex.position_value.z) * fog_len_inv;
                if (fog_cur < 0.0f)
                    work_vertex.fog = work_vertex.diffuse_value = 0x0000000 | (work_vertex.diffuse_value & 0xffffff);
                else if (fog_cur > 1.0f)
                    work_vertex.fog = work_vertex.diffuse_value = 0xFF000000 | (work_vertex.diffuse_value & 0xffffff);
                else
                    work_vertex.fog = work_vertex.diffuse_value = uint8_t(255.0f * fog_cur) << 24 | (work_vertex.diffuse_value & 0xffffff);

                *(trans_vertex + index) = work_vertex;
            }
        } else {
            float local_x;
            float local_y;

            software_transform_patch_tl_vertex work_vertex;
            for (uint32_t index = 0; index != terrain_patch::TERRAIN_VERTEX_COUNT; ++index) {
                src_position = &src_vertex[index].position_value;
                work_vertex.position_value = math::transform(*src_position, m4_frustum);
                local_x = src_position->x + terrain_base_x;
                local_y = src_position->y + terrain_base_y;
                work_vertex.position_value.w = 1.0f / work_vertex.position_value.w;
                work_vertex.position_value.x *= work_vertex.position_value.w;
                work_vertex.position_value.y *= work_vertex.position_value.w;
                work_vertex.position_value.z *= work_vertex.position_value.w;
                work_vertex.position_value.x = (work_vertex.position_value.x + 1.0f) * screen_half_width;
                work_vertex.position_value.y = (work_vertex.position_value.y - 1.0f) * screen_half_height;
                work_vertex.diffuse_value = src_vertex[index].diffuse_value;
                work_vertex.fog = 0xffffffff;
                work_vertex.tex_tile.x = src_position->x * tile_pattern_x;
                work_vertex.tex_tile.y = src_position->y * tile_pattern_y;
                work_vertex.tex_alpha.x = local_x * alpha_pattern_x + alpha_bias_x;
                work_vertex.tex_alpha.y = local_y * alpha_pattern_y + alpha_bias_y;
                work_vertex.tex_static_shadow.x = local_x * shadow_pattern_x;
                work_vertex.tex_static_shadow.y = local_y * shadow_pattern_y;
                work_vertex.tex_dynamic_shadow.x = 0.0f;
                work_vertex.tex_dynamic_shadow.y = 0.0f;

                *(trans_vertex + index) = work_vertex;
            }
        }

        if (is_dynamic_shadow) {
            math::matrix m4_dynamic_shadow = tprs.m4_dynamic_shadow;

            math::vector3 shadow;
            for (uint32_t index = 0; index != terrain_patch::TERRAIN_VERTEX_COUNT; ++index) {
                shadow = math::transform_coord(src_vertex[index].position_value, m4_dynamic_shadow);
                trans_vertex[index].tex_dynamic_shadow.x = shadow.x;
                trans_vertex[index].tex_dynamic_shadow.y = shadow.y;
            }
        }

        return true;
    }

    auto map_outdoor::software_transform_patch_set_splat_stream(software_transform_patch_tl_vertex *src_vertex) -> bool {
        gpu::vertex_buffer *vb = stpd_.vb_splat[stpd_.splat_pos++].get();
        stpd_.splat_pos %= software_transform_patch_data::SPLAT_VB_NUM;
        if (!vb)
            return false;

        uint32_t vb_size = sizeof(software_transform_patch_splat_vertex) * terrain_patch::TERRAIN_VERTEX_COUNT;
        void *data;
        if (!vb->lock(0, vb_size, &data))
            return false;

        auto *dst_vertex = static_cast<software_transform_patch_splat_vertex *>(data);

        for (uint32_t index = 0; index != terrain_patch::TERRAIN_VERTEX_COUNT; ++index)
            *(dst_vertex + index) = *((software_transform_patch_splat_vertex *)(src_vertex + index));

        vb->unlock();

        STATEMANAGER.set_stream_source(0, vb, sizeof(software_transform_patch_splat_vertex));
        return true;
    }

    auto map_outdoor::software_transform_patch_set_shadow_stream(software_transform_patch_tl_vertex *ak_src_vertex) -> bool {
        gpu::vertex_buffer *vb = stpd_.vb_splat[stpd_.splat_pos++].get();
        stpd_.splat_pos %= software_transform_patch_data::SPLAT_VB_NUM;
        if (!vb)
            return false;

        uint32_t vb_size = sizeof(software_transform_patch_splat_vertex) * terrain_patch::TERRAIN_VERTEX_COUNT;
        void *data;
        if (!vb->lock(0, vb_size, &data))
            return false;

        auto *ak_dst_vertex = static_cast<software_transform_patch_splat_vertex *>(data);

        software_transform_patch_tl_vertex *pk_src_vertex;
        software_transform_patch_splat_vertex *pk_dst_vertex;
        for (uint32_t index = 0; index != terrain_patch::TERRAIN_VERTEX_COUNT; ++index) {
            pk_src_vertex = ak_src_vertex + index;
            pk_dst_vertex = ak_dst_vertex + index;
            pk_dst_vertex->position_value = pk_src_vertex->position_value;
            pk_dst_vertex->diffuse_value = pk_src_vertex->diffuse_value;
            pk_dst_vertex->specular_value = pk_src_vertex->fog;
            pk_dst_vertex->tex1 = pk_src_vertex->tex_static_shadow;
            pk_dst_vertex->tex2 = pk_src_vertex->tex_dynamic_shadow;
        }
        vb->unlock();

        STATEMANAGER.set_stream_source(0, vb, sizeof(software_transform_patch_splat_vertex));
        return true;
    }

    void map_outdoor::software_transform_patch_initialize() {
        for (auto &vb : stpd_.vb_splat)
            vb.reset();
        stpd_.splat_pos = 0;

        for (auto &vb : stpd_.vb_none)
            vb.reset();
        stpd_.none_pos = 0;
    }

    auto map_outdoor::software_transform_patch_create() -> bool {
        for (auto &vb : stpd_.vb_splat) {
            assert(!vb);
            vb = device_->create_vertex_buffer(sizeof(software_transform_patch_splat_vertex) * terrain_patch::TERRAIN_VERTEX_COUNT, gpu::usage::DYNAMIC | gpu::usage::WRITE_ONLY);
            if (!vb)
                return false;
        }

        for (auto &vb : stpd_.vb_none) {
            assert(!vb);
            vb = device_->create_vertex_buffer(sizeof(software_transform_patch_t_vertex) * terrain_patch::TERRAIN_VERTEX_COUNT, gpu::usage::DYNAMIC | gpu::usage::WRITE_ONLY);
            if (!vb)
                return false;
        }
        return true;
    }

    void map_outdoor::software_transform_patch_destroy() {
        software_transform_patch_initialize();
    }
}
