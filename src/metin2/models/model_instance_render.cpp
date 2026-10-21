#include <metin2/gpu/types.h>
#include <metin2/graphics/state_manager.h>
#include "model.h"
#include "model_instance.h"

namespace metin2::inline models {

    void model_instance::deform_no_skin(const math::matrix *world_matrix) {
        if (is_empty())
            return;

        update_world_pose();
        update_world_matrices(world_matrix);
    }

    void model_instance::render_with_one_texture() {
        if (is_empty())
            return;

        STATEMANAGER.set_vertex_shader(pnt_vs_);

        gpu::vertex_buffer *deform_vertex_buffer = deformable_vertex_buffer_ptr();

        gpu::vertex_buffer *rigid_vertex_buffer = model_value_->gpu_pnt_vertex_buffer();

        if (deform_vertex_buffer) {
            STATEMANAGER.set_stream_source(0, deform_vertex_buffer, sizeof(pnt_vertex));
            render_mesh_node_list_with_one_texture(mesh::TYPE_DEFORM, material::TYPE_DIFFUSE_PNT);
        }
        if (rigid_vertex_buffer) {
            STATEMANAGER.set_stream_source(0, rigid_vertex_buffer, sizeof(pnt_vertex));
            render_mesh_node_list_with_one_texture(mesh::TYPE_RIGID, material::TYPE_DIFFUSE_PNT);
        }
    }

    void model_instance::blend_render_with_one_texture() {
        if (is_empty())
            return;

        gpu::vertex_buffer *deform_vertex_buffer = deformable_vertex_buffer_ptr();
        gpu::vertex_buffer *rigid_vertex_buffer = model_value_->gpu_pnt_vertex_buffer();

        STATEMANAGER.set_vertex_shader(pnt_vs_);

        if (deform_vertex_buffer) {
            STATEMANAGER.set_stream_source(0, deform_vertex_buffer, sizeof(pnt_vertex));
            render_mesh_node_list_with_one_texture(mesh::TYPE_DEFORM, material::TYPE_BLEND_PNT);
        }

        if (rigid_vertex_buffer) {
            STATEMANAGER.set_stream_source(0, rigid_vertex_buffer, sizeof(pnt_vertex));
            render_mesh_node_list_with_one_texture(mesh::TYPE_RIGID, material::TYPE_BLEND_PNT);
        }
    }

    void model_instance::render_without_texture() {
        if (is_empty())
            return;

        STATEMANAGER.set_vertex_shader(pnt_vs_);
        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture(1, nullptr);

        gpu::vertex_buffer *deform_vertex_buffer = deformable_vertex_buffer_ptr();
        gpu::vertex_buffer *rigid_vertex_buffer = model_value_->gpu_pnt_vertex_buffer();

        if (deform_vertex_buffer) {
            STATEMANAGER.set_stream_source(0, deform_vertex_buffer, sizeof(pnt_vertex));
            render_mesh_node_list_without_texture(mesh::TYPE_DEFORM, material::TYPE_DIFFUSE_PNT);
            render_mesh_node_list_without_texture(mesh::TYPE_DEFORM, material::TYPE_BLEND_PNT);
        }

        if (rigid_vertex_buffer) {
            STATEMANAGER.set_stream_source(0, rigid_vertex_buffer, sizeof(pnt_vertex));
            render_mesh_node_list_without_texture(mesh::TYPE_RIGID, material::TYPE_DIFFUSE_PNT);
            render_mesh_node_list_without_texture(mesh::TYPE_RIGID, material::TYPE_BLEND_PNT);
        }
    }

    void model_instance::render_mesh_node_list_with_one_texture(mesh::type mesh_type, material::type mtrl_type) {
        assert(model_value_ != nullptr);

        gpu::index_buffer *model_index_buffer = model_value_->gpu_index_buffer();
        assert(model_index_buffer != nullptr);

        const model::mesh_node *mesh_node = model_value_->mesh_node_list(mesh_type, mtrl_type);

        while (mesh_node) {
            const mesh *mesh = mesh_node->mesh_value;
            int vtx_mesh_base_pos = mesh->vertex_base_position();

            STATEMANAGER.set_indices(model_index_buffer, vtx_mesh_base_pos);
            STATEMANAGER.set_transform(gpu::transform::WORLD, &mesh_matrices_[mesh_node->mesh_index]);

            const mesh::tri_group_node *tri_group_node = mesh->tri_group_node_list(mtrl_type);
            int vtx_count = mesh->vertex_count();
            while (tri_group_node) {
                face_count_ += tri_group_node->tri_count;

                material &mtrl = mtrl_pal_.material_ref(tri_group_node->mtrl_index);
                mtrl.apply_render_state();
                STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, vtx_count, tri_group_node->idx_pos, tri_group_node->tri_count);
                mtrl.restore_render_state();

                tri_group_node = tri_group_node->next_tri_group_node;
            }

            mesh_node = mesh_node->next_mesh_node;
        }
    }

    void model_instance::render_mesh_node_list_with_two_texture(mesh::type mesh_type, material::type mtrl_type) {
        assert(model_value_ != nullptr);

        gpu::index_buffer *model_index_buffer = model_value_->gpu_index_buffer();
        assert(model_index_buffer != nullptr);

        const model::mesh_node *mesh_node = model_value_->mesh_node_list(mesh_type, mtrl_type);

        while (mesh_node) {
            const mesh *mesh = mesh_node->mesh_value;
            int vtx_mesh_base_pos = mesh->vertex_base_position();

            STATEMANAGER.set_indices(model_index_buffer, vtx_mesh_base_pos);
            STATEMANAGER.set_transform(gpu::transform::WORLD, &mesh_matrices_[mesh_node->mesh_index]);

            const mesh::tri_group_node *tri_group_node = mesh->tri_group_node_list(mtrl_type);
            int vtx_count = mesh->vertex_count();
            while (tri_group_node) {
                face_count_ += tri_group_node->tri_count;

                const material &mtrl = mtrl_pal_.material_ref(tri_group_node->mtrl_index);
                STATEMANAGER.set_texture(0, mtrl.gpu_texture(0));
                STATEMANAGER.set_texture(1, mtrl.gpu_texture(1));
                STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, vtx_count, tri_group_node->idx_pos, tri_group_node->tri_count);
                tri_group_node = tri_group_node->next_tri_group_node;
            }

            mesh_node = mesh_node->next_mesh_node;
        }
    }

    void model_instance::render_mesh_node_list_without_texture(mesh::type mesh_type, material::type mtrl_type) {
        assert(model_value_ != nullptr);

        gpu::index_buffer *model_index_buffer = model_value_->gpu_index_buffer();
        assert(model_index_buffer != nullptr);

        const model::mesh_node *mesh_node = model_value_->mesh_node_list(mesh_type, mtrl_type);

        while (mesh_node) {
            const mesh *mesh = mesh_node->mesh_value;
            int vtx_mesh_base_pos = mesh->vertex_base_position();

            STATEMANAGER.set_indices(model_index_buffer, vtx_mesh_base_pos);
            STATEMANAGER.set_transform(gpu::transform::WORLD, &mesh_matrices_[mesh_node->mesh_index]);

            const mesh::tri_group_node *tri_group_node = mesh->tri_group_node_list(mtrl_type);
            int vtx_count = mesh->vertex_count();

            while (tri_group_node) {
                face_count_ += tri_group_node->tri_count;
                STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, vtx_count, tri_group_node->idx_pos, tri_group_node->tri_count);
                tri_group_node = tri_group_node->next_tri_group_node;
            }

            mesh_node = mesh_node->next_mesh_node;
        }
    }
}
