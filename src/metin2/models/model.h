#pragma once

#include <memory>

#include <metin2/graphics/grp_index_buffer.h>
#include <metin2/graphics/grp_vertex_buffer.h>

#include "mesh.h"

namespace metin2::inline models {

    class model : public reference_object {
      public:
        struct mesh_node {
            int mesh_index;
            const mesh *mesh_value;
            mesh_node *next_mesh_node;
        };

      public:
        model();
        ~model() override;

        auto is_empty() const -> bool;
        auto create_from_granny_model_pointer(granny_model *pgrn_model) -> bool;
        auto create_device_objects() -> bool;
        void destroy_device_objects();
        void destroy();

        auto deform_vertex_count() const -> int;
        auto vertex_count() const -> int;

        auto can_deform_pnt_vertices() const -> bool;
        void deform_pnt_vertices(void *dst_base_vertices, math::matrix *bone_matrices, const std::vector<granny_mesh_binding *> &rvct_pgrn_mesh_binding) const;

        auto mesh_count() const -> int;
        auto mesh_pointer(int mesh_value) -> mesh *;
        auto granny_model_pointer() -> granny_model *;
        auto mesh_pointer(int mesh_value) const -> const mesh *;

        auto gpu_pnt_vertex_buffer() const -> gpu::vertex_buffer *;
        auto gpu_index_buffer() const -> gpu::index_buffer *;

        auto mesh_node_list(mesh::type mesh_type, material::type mtrl_type) const -> const model::mesh_node *;

        auto get_material_palette() const -> const material_palette &;

      protected:
        auto load_meshs() -> bool;
        auto load_vertices() -> bool;
        auto load_indices() -> bool;
        void initialize();

        auto check_mesh_index(int index) const -> bool;
        void append_mesh_node(mesh::type mesh_type, material::type mtrl_type, int mesh);

      protected:
        granny_model *pgrn_model_;

        std::unique_ptr<mesh[]> meshs_;

        graphic_vertex_buffer pnt_vtx_buf_; // for rigid mesh
        graphic_index_buffer idx_buf_;

        std::unique_ptr<mesh_node[]> mesh_nodes_;
        mesh_node *mesh_node_lists_[mesh::TYPE_MAX_NUM][material::TYPE_MAX_NUM];

        int deform_vtx_count_;
        int rigid_vtx_count_;
        int vtx_count_;
        int idx_count_;

        int mesh_node_size_;
        int mesh_node_capacity_;

        bool can_deform_pn_vertices_;

        material_palette mtrl_pal_;

      private:
        bool have_blend_thing_;

      public:
        auto have_blend_thing() -> bool { return have_blend_thing_; }

      protected:
        uint32_t fv_f_;
    };
}
