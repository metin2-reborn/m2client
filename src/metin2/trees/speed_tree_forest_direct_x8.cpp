//	CSpeedTreeForestDirectX8 Class
//	(c) 2003 IDV, Inc.
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//		IDV, Inc.
//		1233 Washington St. Suite 610
//		Columbia, SC 29201
//		Voice: (803) 799-1699
//		Fax:   (803) 931-0320
//		Web:   http://www.idvinc.com

#include "speed_tree_forest_direct_x8.h"

#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/state_manager.h>
#include "vertex_shaders.h"

namespace metin2::inline trees {

    speed_tree_forest_direct_x8::speed_tree_forest_direct_x8() = default;

    speed_tree_forest_direct_x8::~speed_tree_forest_direct_x8() = default;

    void speed_tree_forest_direct_x8::update_compund_matrix(const math::vector3 & /*eye_vec*/, const math::matrix &view, const math::matrix &proj) {
        const math::matrix blend_shader = view * proj;

        camera_direction_[0] = blend_shader.m[0][2];
        camera_direction_[1] = blend_shader.m[1][2];
        camera_direction_[2] = blend_shader.m[2][2];
    }

    void speed_tree_forest_direct_x8::render(uint32_t render_bit_vector) {
        if (main_tree_map_.empty())
            return;

        if (!(render_bit_vector & Forest_RenderToShadow) && !(render_bit_vector & Forest_RenderToMiniMap))
            update_compund_matrix(get_camera_manager().current_camera()->eye(), view_, proj_);

        uint32_t light_state = STATEMANAGER.render_state(gpu::state::LIGHTING);
        uint32_t color_vertex_state = STATEMANAGER.render_state(gpu::state::COLOR_VERTEX);
        uint32_t fog_vertex_mode = STATEMANAGER.render_state(gpu::state::FOG_VERTEX_MODE);

        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.set_render_state(gpu::state::COLOR_VERTEX, true);

        tree_map::iterator itor;
        unsigned int count;

        if (render_bit_vector & Forest_RenderToShadow) {
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);
        } else {
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
            STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::MIP_FILTER, gpu::filter::LINEAR);

            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);
        }

        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::CW);

        // Branches and fronds share the branch vertex format.
        STATEMANAGER.set_vertex_shader(SPEEDTREE_BRANCH_VERTEX_FORMAT);

        if (render_bit_vector & Forest_RenderBranches) {
            itor = main_tree_map_.begin();

            while (itor != main_tree_map_.end()) {
                speed_tree_wrapper *main_tree = (itor++)->second;
                speed_tree_wrapper **instances = main_tree->instances(count);

                main_tree->setup_branch_for_tree_type();

                for (unsigned int i = 0; i < count; ++i)
                    if (instances[i]->is_show())
                        instances[i]->render_branches();
            }
        }

        STATEMANAGER.set_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);

        if (render_bit_vector & Forest_RenderFronds) {
            itor = main_tree_map_.begin();

            while (itor != main_tree_map_.end()) {
                speed_tree_wrapper *main_tree = (itor++)->second;
                speed_tree_wrapper **instances = main_tree->instances(count);

                main_tree->setup_frond_for_tree_type();

                for (unsigned int i = 0; i < count; ++i)
                    if (instances[i]->is_show())
                        instances[i]->render_fronds();
            }
        }

        if (render_bit_vector & Forest_RenderLeaves) {
            STATEMANAGER.set_vertex_shader(SPEEDTREE_LEAF_VERTEX_FORMAT);

            if (render_bit_vector & Forest_RenderToShadow || render_bit_vector & Forest_RenderToMiniMap) {
                STATEMANAGER.set_render_state(gpu::state::ALPHA_FUNC, gpu::compare::NOT_EQUAL);
                STATEMANAGER.save_render_state(gpu::state::ALPHA_REF, 0x00000000);
            }

            itor = main_tree_map_.begin();

            while (itor != main_tree_map_.end()) {
                speed_tree_wrapper *main_tree = (itor++)->second;
                speed_tree_wrapper **instances = main_tree->instances(count);

                main_tree->setup_leaf_for_tree_type();

                for (unsigned int i = 0; i < count; ++i)
                    if (instances[i]->is_show())
                        instances[i]->render_leaves();
            }

            if (render_bit_vector & Forest_RenderToShadow || render_bit_vector & Forest_RenderToMiniMap) {
                STATEMANAGER.set_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER);
                STATEMANAGER.restore_render_state(gpu::state::ALPHA_REF);
            }
        }

        STATEMANAGER.set_render_state(gpu::state::LIGHTING, light_state);
        STATEMANAGER.set_render_state(gpu::state::COLOR_VERTEX, color_vertex_state);
        STATEMANAGER.set_render_state(gpu::state::FOG_VERTEX_MODE, fog_vertex_mode);

        // Texture stage 1 carries the self shadow; its COLOROP and ALPHAOP have to be disabled
        // or whatever renders next comes out wrong (possibly black).
        if (!(render_bit_vector & Forest_RenderToShadow)) {
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
            STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        }

        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_FUNC);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
    }
}
