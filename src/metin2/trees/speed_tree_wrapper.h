//	SpeedTreeRTExample Class
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

#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <metin2/gpu/types.h>
#include <metin2/math/math.h>

#include <metin2/graphics/grp_image_instance.h>
#include <metin2/graphics/grp_object_instance.h>
#include "spt_tree.h"

namespace metin2::inline trees {

    class speed_tree_wrapper : public graphic_object_instance {
        enum {
            ID = TREE_OBJECT
        };
        auto get_type() const -> uint32_t override { return ID; }

      protected:
        void on_update_collision_data(const static_collision_data_vector *pscd_vector) override;
        void on_update_heigh_instance(attribute_instance *attribute_instance) override {}
        auto on_get_object_height(float x, float y, float *height) -> bool override { return false; }

      public:
        auto bounding_sphere(math::vector3 &center, float &radius) -> bool override;

      public:
        static bool self_shadow_on;

      public:
        virtual void set_position(float x, float y, float z);
        virtual void calculate_b_box();

        void on_render() override;
        // Only on_render draws here; normally speed_tree_forest renders all trees by part.
        void on_blend_render() override {}
        void on_render_to_shadow_map() override {}
        void on_render_shadow() override {}
        void on_render_pc_blocker() override;

      public:
        speed_tree_wrapper();
        ~speed_tree_wrapper() override;

        auto get_position() -> const float *;

        auto load_tree(const char *spt_file, std::span<const uint8_t> data) -> bool;

        void setup_branch_for_tree_type(void) const;
        void setup_frond_for_tree_type(void) const;
        void setup_leaf_for_tree_type(void) const;

        void render_branches(void) const;
        void render_fronds(void) const;
        void render_leaves(void) const;

        auto instances(unsigned int &count) -> speed_tree_wrapper **;
        auto instance_of(void) const -> speed_tree_wrapper * { return instance_of_; }
        auto make_instance() -> speed_tree_wrapper *;
        void delete_instance(speed_tree_wrapper *instance);

      private:
        void setup_buffers(void);
        void setup_mesh_buffers(const spt_tree::mesh &mesh, std::shared_ptr<gpu::vertex_buffer> &vertex_buffer, std::shared_ptr<gpu::index_buffer> &index_buffer);
        void setup_leaf_buffers(void);
        void render_mesh(const spt_tree::mesh &mesh, const gpu::vertex_buffer *vertex_buffer) const;
        void position_tree(void) const;
        static auto load_texture(const char *filename, graphic_image_instance &image) -> bool;

      private:
        // Instances share the main tree's geometry, textures and buffers.
        std::shared_ptr<spt_tree> tree_;
        std::vector<speed_tree_wrapper *> v_instances_;
        speed_tree_wrapper *instance_of_;

        std::shared_ptr<gpu::vertex_buffer> branch_vertex_buffer_;
        std::shared_ptr<gpu::index_buffer> branch_index_buffer_;

        std::shared_ptr<gpu::vertex_buffer> frond_vertex_buffer_;
        std::shared_ptr<gpu::index_buffer> frond_index_buffer_;

        std::shared_ptr<gpu::vertex_buffer> leaf_vertex_buffer_; // six vertices per leaf

        float pos_[3];
        float bounding_box_[6];

        graphic_image_instance branch_image_instance_;
        graphic_image_instance shadow_image_instance_; // empty unless the tree has a self shadow
        graphic_image_instance composite_image_instance_;
    };
}
