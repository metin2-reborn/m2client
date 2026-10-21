#pragma once

#include <memory>

#include "material.h"

namespace metin2::inline models {

    extern granny_data_type_definition granny_pnt3322_vertex_type[5];

    class mesh {
      public:
        enum type {
            TYPE_RIGID,
            TYPE_DEFORM,
            TYPE_MAX_NUM
        };

        struct tri_group_node {
            tri_group_node *next_tri_group_node;
            int idx_pos;
            int tri_count;
            uint32_t mtrl_index;
        };

      public:
        mesh();

        virtual ~mesh();

        auto is_empty() const -> bool;
        auto create_from_granny_mesh_pointer(granny_mesh *pgrn_mesh, int vtx_base_pos, int idx_base_pos, material_palette &mtrl_pal) -> bool;
        void load_indices(void *dst_base_indices);
        void load_vertices(void *dst_base_vertices);
        void destroy();

        void set_pnt2_mesh();

        void deform_pnt_vertices(void *dst_base_vertices, math::matrix *bone_matrices, granny_mesh_binding *pgrn_mesh_binding) const;
        auto can_deform_pnt_vertices() const -> bool;

        auto vertex_count() const -> int;

        auto vertex_base_position() const -> int;

        auto granny_mesh_pointer() const -> const granny_mesh *;
        auto tri_group_node_list(material::type mtrl_type) const -> const mesh::tri_group_node *;

      protected:
        void initialize();

        auto load_materials(material_palette &mtrl_pal) -> bool;
        auto load_tri_group_node_list(material_palette &mtrl_pal) -> bool;

      protected:
        granny_data_type_definition *pgrn_mesh_type_;
        granny_mesh *pgrn_mesh_;

        granny_mesh_deformer *pgrn_mesh_deformer_;

        std::vector<uint32_t> mtrl_index_vector_;

        std::unique_ptr<tri_group_node[]> tri_group_nodes_;
        tri_group_node *tri_group_node_lists_[material::TYPE_MAX_NUM];

        int vtx_base_pos_;
        int idx_base_pos_;

        bool can_deform_pnt_vertex_;

      private:
        bool have_blend_thing_;

      public:
        auto have_blend_thing() -> bool { return have_blend_thing_; }
    };
}
