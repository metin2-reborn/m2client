//	CSpeedTreeWrapper Class
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

#include "speed_tree_wrapper.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>

#include <metin2/core/file_name.h>
#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>
#include "speed_tree_forest_direct_x8.h"
#include "vertex_shaders.h"

namespace metin2::inline trees {

    bool speed_tree_wrapper::self_shadow_on = true;

    speed_tree_wrapper::speed_tree_wrapper() : instance_of_(nullptr) {
        pos_[0] = pos_[1] = pos_[2] = 0.0f;
        memset(bounding_box_, 0, sizeof(bounding_box_));
    }

    void speed_tree_wrapper::on_render_pc_blocker() {
        get_speed_tree_forest_direct_x8().update_compund_matrix(get_camera_manager().current_camera()->eye(), view_, proj_);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);

        uint32_t lighting = STATEMANAGER.render_state(gpu::state::LIGHTING);
        uint32_t fog_enable = STATEMANAGER.render_state(gpu::state::FOG_ENABLE);
        uint32_t alpha_blend_enable = STATEMANAGER.render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.save_render_state(gpu::state::COLOR_VERTEX, true);
        STATEMANAGER.set_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::CW);
        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, false);

        STATEMANAGER.set_vertex_shader(SPEEDTREE_BRANCH_VERTEX_FORMAT);

        {
            gpu::texture *texture;

            if ((texture = branch_image_instance_.texture_reference().gpu_texture()))
                STATEMANAGER.set_texture(0, texture);

            if (branch_vertex_buffer_) {
                STATEMANAGER.set_stream_source(0, branch_vertex_buffer_.get(), sizeof(fvf_branch_vertex));
                STATEMANAGER.set_indices(branch_index_buffer_.get(), 0);
            }
        }

        render_branches();

        STATEMANAGER.set_texture(0, composite_image_instance_.texture_reference().gpu_texture());
        STATEMANAGER.set_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);

        {
            if (!composite_image_instance_.is_empty())
                STATEMANAGER.set_texture(0, composite_image_instance_.texture_reference().gpu_texture());

            if (frond_vertex_buffer_) {
                STATEMANAGER.set_stream_source(0, frond_vertex_buffer_.get(), sizeof(fvf_branch_vertex));
                STATEMANAGER.set_indices(frond_index_buffer_.get(), 0);
            }
        }
        render_fronds();

        STATEMANAGER.set_vertex_shader(SPEEDTREE_LEAF_VERTEX_FORMAT);

        {
            if (!composite_image_instance_.is_empty())
                STATEMANAGER.set_texture(0, composite_image_instance_.texture_reference().gpu_texture());
        }
        render_leaves();

        STATEMANAGER.restore_render_state(gpu::state::COLOR_VERTEX);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_FUNC);
        STATEMANAGER.set_render_state(gpu::state::ALPHA_BLEND_ENABLE, alpha_blend_enable);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, lighting);
        STATEMANAGER.set_render_state(gpu::state::FOG_ENABLE, fog_enable);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
    }

    void speed_tree_wrapper::on_render() {
        get_speed_tree_forest_direct_x8().update_compund_matrix(get_camera_manager().current_camera()->eye(), view_, proj_);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);

        STATEMANAGER.save_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.save_render_state(gpu::state::COLOR_VERTEX, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::CW);
        STATEMANAGER.save_render_state(gpu::state::FOG_ENABLE, false);

        STATEMANAGER.set_vertex_shader(SPEEDTREE_BRANCH_VERTEX_FORMAT);

        setup_branch_for_tree_type();
        render_branches();

        STATEMANAGER.set_texture(0, composite_image_instance_.texture_reference().gpu_texture());
        STATEMANAGER.set_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);

        setup_frond_for_tree_type();
        render_fronds();

        STATEMANAGER.set_vertex_shader(SPEEDTREE_LEAF_VERTEX_FORMAT);

        setup_leaf_for_tree_type();
        render_leaves();

        STATEMANAGER.restore_render_state(gpu::state::LIGHTING);
        STATEMANAGER.restore_render_state(gpu::state::COLOR_VERTEX);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_FUNC);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
        STATEMANAGER.restore_render_state(gpu::state::FOG_ENABLE);
    }

    speed_tree_wrapper::~speed_tree_wrapper() {
        clear();
    }

    auto speed_tree_wrapper::load_tree(const char *spt_file, std::span<const uint8_t> data) -> bool {
        auto tree = spt_tree::load(data);
        if (!tree) {
            metin2::log::error("Cannot load tree {}: {}", spt_file, tree.error());
            return false;
        }

        tree_ = std::make_shared<spt_tree>(std::move(*tree));
        memcpy(bounding_box_, tree_->bounds(), sizeof(bounding_box_));

        // The textures are .dds files next to the .spt.
        std::string spt_path = spt_file;
        const std::string directory = file_name_helper::path(spt_path);
        auto texture_file = [&](std::string name) { return directory + file_name_helper::no_extension(name) + ".dds"; };

        load_texture(texture_file(tree_->bark_texture()).c_str(), branch_image_instance_);

        if (!tree_->shadow_texture().empty())
            load_texture(texture_file(tree_->shadow_texture()).c_str(), shadow_image_instance_);

        if (!tree_->composite_texture().empty())
            load_texture(texture_file(tree_->composite_texture()).c_str(), composite_image_instance_);

        setup_buffers();
        return true;
    }

    void speed_tree_wrapper::setup_buffers(void) {
        setup_mesh_buffers(tree_->branches(), branch_vertex_buffer_, branch_index_buffer_);
        setup_mesh_buffers(tree_->fronds(), frond_vertex_buffer_, frond_index_buffer_);
        setup_leaf_buffers();
    }

    void speed_tree_wrapper::setup_mesh_buffers(const spt_tree::mesh &mesh, std::shared_ptr<gpu::vertex_buffer> &vertex_buffer, std::shared_ptr<gpu::index_buffer> &index_buffer) {
        static_assert(sizeof(spt_tree::vertex) == sizeof(fvf_branch_vertex), "the generated vertices are uploaded as they are");

        if (mesh.vertices.size() < 2 || mesh.strip.empty())
            return;

        void *data = nullptr;
        vertex_buffer = device_->create_vertex_buffer(uint32_t(mesh.vertices.size() * sizeof(fvf_branch_vertex)), gpu::usage::WRITE_ONLY);
        vertex_buffer->lock(0, 0, &data);
        memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(fvf_branch_vertex));
        vertex_buffer->unlock();

        index_buffer = device_->create_index_buffer(uint32_t(mesh.strip.size() * sizeof(uint16_t)), gpu::usage::WRITE_ONLY, gpu::format::INDEX16);
        index_buffer->lock(0, 0, &data);
        memcpy(data, mesh.strip.data(), mesh.strip.size() * sizeof(uint16_t));
        index_buffer->unlock();
    }

    void speed_tree_wrapper::setup_leaf_buffers(void) {
        const int corner_indices[6] = {0, 1, 2, 0, 2, 3};
        const std::vector<spt_tree::leaf> &leaves = tree_->leaves();

        if (leaves.empty())
            return;

        fvf_leaf_vertex *vertex = nullptr;
        leaf_vertex_buffer_ = device_->create_vertex_buffer(uint32_t(leaves.size() * 6 * sizeof(fvf_leaf_vertex)), gpu::usage::DYNAMIC);
        leaf_vertex_buffer_->lock(0, 0, reinterpret_cast<void **>(&vertex), gpu::lock::DISCARD | gpu::lock::NO_SYSLOCK);
        for (const spt_tree::leaf &leaf : leaves) {
            const spt_tree::leaf_map &map = tree_->leaf_maps()[leaf.map];
            for (int corner : corner_indices) { // 6 verts == 2 triangles
                vertex->v_position = math::vector3(leaf.center[0], leaf.center[1], leaf.center[2]);
                vertex->diffuse_color = leaf.color;
                vertex->tex_coords[0] = map.tex_coords[corner][0];
                vertex->tex_coords[1] = map.tex_coords[corner][1];
                ++vertex;
            }
        }
        leaf_vertex_buffer_->unlock();
    }

    auto speed_tree_wrapper::make_instance() -> speed_tree_wrapper * {
        auto *instance = new speed_tree_wrapper;

        instance->tree_ = tree_;

        instance->composite_image_instance_.set_image_pointer(composite_image_instance_.graphic_image_pointer());
        instance->branch_image_instance_.set_image_pointer(branch_image_instance_.graphic_image_pointer());

        if (!shadow_image_instance_.is_empty())
            instance->shadow_image_instance_.set_image_pointer(shadow_image_instance_.graphic_image_pointer());

        instance->branch_index_buffer_ = branch_index_buffer_;
        instance->branch_vertex_buffer_ = branch_vertex_buffer_;

        instance->frond_index_buffer_ = frond_index_buffer_;
        instance->frond_vertex_buffer_ = frond_vertex_buffer_;

        instance->leaf_vertex_buffer_ = leaf_vertex_buffer_;

        memcpy(instance->pos_, pos_, 3 * sizeof(float));
        memcpy(instance->bounding_box_, bounding_box_, 6 * sizeof(float));
        instance->instance_of_ = this;
        v_instances_.push_back(instance);

        return instance;
    }

    auto speed_tree_wrapper::instances(unsigned int &count) -> speed_tree_wrapper ** {
        count = static_cast<unsigned int>(v_instances_.size());
        if (count)
            return &(v_instances_[0]);
        else
            return nullptr;
    }

    void speed_tree_wrapper::delete_instance(speed_tree_wrapper *instance) {
        auto itor = v_instances_.begin();

        while (itor != v_instances_.end()) {
            if (*itor == instance) {
                itor = v_instances_.erase(itor);
            } else
                ++itor;
        }
        delete instance;
    }

    void speed_tree_wrapper::setup_branch_for_tree_type(void) const {

        gpu::texture *texture;

        if ((texture = branch_image_instance_.texture_reference().gpu_texture()))
            STATEMANAGER.set_texture(0, texture);

        if (self_shadow_on && (texture = shadow_image_instance_.texture_reference().gpu_texture()))
            STATEMANAGER.set_texture(1, texture);
        else
            STATEMANAGER.set_texture(1, nullptr);

        if (branch_vertex_buffer_) {
            STATEMANAGER.set_stream_source(0, branch_vertex_buffer_.get(), sizeof(fvf_branch_vertex));
            STATEMANAGER.set_indices(branch_index_buffer_.get(), 0);
        }
    }

    void speed_tree_wrapper::render_branches(void) const {
        render_mesh(tree_->branches(), branch_vertex_buffer_.get());
    }

    void speed_tree_wrapper::setup_frond_for_tree_type(void) const {

        if (!composite_image_instance_.is_empty())
            STATEMANAGER.set_texture(0, composite_image_instance_.texture_reference().gpu_texture());

        gpu::texture *texture;

        if ((texture = shadow_image_instance_.texture_reference().gpu_texture()))
            STATEMANAGER.set_texture(1, texture);

        if (frond_vertex_buffer_) {
            STATEMANAGER.set_stream_source(0, frond_vertex_buffer_.get(), sizeof(fvf_branch_vertex));
            STATEMANAGER.set_indices(frond_index_buffer_.get(), 0);
        }
    }

    void speed_tree_wrapper::render_fronds(void) const {
        render_mesh(tree_->fronds(), frond_vertex_buffer_.get());
    }

    void speed_tree_wrapper::render_mesh(const spt_tree::mesh &mesh, const gpu::vertex_buffer *vertex_buffer) const {
        if (!vertex_buffer)
            return;

        position_tree();

        STATEMANAGER.set_render_state(gpu::state::ALPHA_REF, spt_tree::alpha_test_value);

        const uint32_t triangles = uint32_t(mesh.strip.size() - 2);
        face_count_ += triangles;
        STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_STRIP, 0, uint32_t(mesh.vertices.size()), 0, triangles);
    }

    void speed_tree_wrapper::setup_leaf_for_tree_type(void) const {

        if (!composite_image_instance_.is_empty())
            STATEMANAGER.set_texture(0, composite_image_instance_.texture_reference().gpu_texture());

        STATEMANAGER.set_texture(1, nullptr);
    }

    void speed_tree_wrapper::render_leaves(void) const {
        if (!leaf_vertex_buffer_)
            return;

        // The leaf quads face the camera: six vertices (two triangles) per leaf.
        tree_->face_leaves(get_speed_tree_forest_direct_x8().camera_direction());

        const std::vector<spt_tree::leaf> &leaves = tree_->leaves();
        fvf_leaf_vertex *vertex = nullptr;
        if (leaf_vertex_buffer_->lock(0, 0, reinterpret_cast<void **>(&vertex), gpu::lock::DISCARD | gpu::lock::NO_SYSLOCK)) {
            // The leaves sway with the environment's wind: each leaf drifts a little, out of step with
            // its neighbours, so the crown moves without the trunk.
            const float time = get_timer().current_second();
            const float sway = 40.0f * get_speed_tree_forest_direct_x8().wind_strength();
            const int corner_indices[6] = {0, 1, 2, 0, 2, 3};
            for (const spt_tree::leaf &leaf : leaves) {
                const spt_tree::leaf_map &map = tree_->leaf_maps()[leaf.map];
                const float phase = (pos_[0] + leaf.center[0]) * 0.011f + (pos_[1] + leaf.center[1]) * 0.007f;
                const float dx = sway * std::sin(time * 1.3f + phase);
                const float dy = sway * 0.5f * std::sin(time * 0.9f + phase * 1.7f);
                for (int corner : corner_indices) {
                    vertex->v_position = math::vector3(map.corners[corner][0] + leaf.center[0] + dx, map.corners[corner][1] + leaf.center[1] + dy, map.corners[corner][2] + leaf.center[2]);
                    ++vertex;
                }
            }
            leaf_vertex_buffer_->unlock();
        }

        position_tree();

        STATEMANAGER.set_stream_source(0, leaf_vertex_buffer_.get(), sizeof(fvf_leaf_vertex));
        STATEMANAGER.set_render_state(gpu::state::ALPHA_REF, spt_tree::alpha_test_value);

        face_count_ += uint32_t(leaves.size() * 2);
        STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_LIST, 0, uint32_t(leaves.size() * 2));
    }

    void speed_tree_wrapper::position_tree(void) const {
        math::matrix translation = math::matrix_translation(pos_[0], pos_[1], pos_[2]);

        STATEMANAGER.set_transform(gpu::transform::WORLD, &translation);
    }

    auto speed_tree_wrapper::load_texture(const char *filename, graphic_image_instance &image) -> bool {
        resource *resource = get_resource_manager().resource_pointer(filename);
        image.set_image_pointer(static_cast<graphic_image *>(resource));

        if (image.is_empty())
            return false;

        return true;
    }

    void speed_tree_wrapper::set_position(float x, float y, float z) {
        pos_[0] = x;
        pos_[1] = y;
        pos_[2] = z;
        graphic_object_instance::set_position(x, y, z);
    }

    auto speed_tree_wrapper::bounding_sphere(math::vector3 &center, float &radius) -> bool {
        float x, y, z;

        x = bounding_box_[3] - bounding_box_[0];
        y = bounding_box_[4] - bounding_box_[1];
        z = bounding_box_[5] - bounding_box_[2];

        center.x = 0.0f;
        center.y = 0.0f;
        center.z = z * 0.5f;

        radius = sqrtf(x * x + y * y + z * z) * 0.5f * 0.9f; // a little tighter than the bounding box

        center += math::vector3(pos_[0], pos_[1], pos_[2]);

        return true;
    }

    void speed_tree_wrapper::calculate_b_box() {
        float x, y, z;

        x = bounding_box_[3] - bounding_box_[0];
        y = bounding_box_[4] - bounding_box_[1];
        z = bounding_box_[5] - bounding_box_[2];

        b_box_min_.x = -x / 2.0f;
        b_box_min_.y = -y / 2.0f;
        b_box_min_.z = 0.0f;
        b_box_max_.x = x / 2.0f;
        b_box_max_.y = y / 2.0f;
        b_box_max_.z = z;

        tb_box_[0] = math::vector4(b_box_min_.x, b_box_min_.y, b_box_min_.z, 1.0f);
        tb_box_[1] = math::vector4(b_box_min_.x, b_box_max_.y, b_box_min_.z, 1.0f);
        tb_box_[2] = math::vector4(b_box_max_.x, b_box_min_.y, b_box_min_.z, 1.0f);
        tb_box_[3] = math::vector4(b_box_max_.x, b_box_max_.y, b_box_min_.z, 1.0f);
        tb_box_[4] = math::vector4(b_box_min_.x, b_box_min_.y, b_box_max_.z, 1.0f);
        tb_box_[5] = math::vector4(b_box_min_.x, b_box_max_.y, b_box_max_.z, 1.0f);
        tb_box_[6] = math::vector4(b_box_max_.x, b_box_min_.y, b_box_max_.z, 1.0f);
        tb_box_[7] = math::vector4(b_box_max_.x, b_box_max_.y, b_box_max_.z, 1.0f);

        const math::matrix &transform = get_transform();

        for (uint32_t i = 0; i < 8; ++i) {
            tb_box_[i] = math::transform(tb_box_[i], transform);
            if (0 == i) {
                tb_box_min_.x = tb_box_[i].x;
                tb_box_min_.y = tb_box_[i].y;
                tb_box_min_.z = tb_box_[i].z;
                tb_box_max_.x = tb_box_[i].x;
                tb_box_max_.y = tb_box_[i].y;
                tb_box_max_.z = tb_box_[i].z;
            } else {
                if (tb_box_min_.x > tb_box_[i].x)
                    tb_box_min_.x = tb_box_[i].x;
                if (tb_box_max_.x < tb_box_[i].x)
                    tb_box_max_.x = tb_box_[i].x;
                if (tb_box_min_.y > tb_box_[i].y)
                    tb_box_min_.y = tb_box_[i].y;
                if (tb_box_max_.y < tb_box_[i].y)
                    tb_box_max_.y = tb_box_[i].y;
                if (tb_box_min_.z > tb_box_[i].z)
                    tb_box_min_.z = tb_box_[i].z;
                if (tb_box_max_.z < tb_box_[i].z)
                    tb_box_max_.z = tb_box_[i].z;
            }
        }
    }

    auto speed_tree_wrapper::get_position() -> const float * {
        return pos_;
    }

    void speed_tree_wrapper::on_update_collision_data(const static_collision_data_vector * /*pscdVector*/) {
        math::matrix mat;
        mat = math::matrix_translation(pos_[0], pos_[1], pos_[2]);

        for (const spt_tree::cylinder &cylinder : tree_->collision()) {
            static_collision_data collision_data = {};
            collision_data.type_value = COLLISION_TYPE_CYLINDER;
            collision_data.position_value = math::vector3(cylinder.position[0], cylinder.position[1], cylinder.position[2]);
            collision_data.dimensions[0] = cylinder.radius;
            collision_data.dimensions[1] = cylinder.height;
            add_collision(&collision_data, &mat);
        }
    }
}
