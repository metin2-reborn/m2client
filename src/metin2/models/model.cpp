#include "model.h"
#include <metin2/gpu/types.h>
#include "mesh.h"

namespace metin2::inline models {

    auto model::get_material_palette() const -> const material_palette & {
        return mtrl_pal_;
    }

    auto model::mesh_node_list(mesh::type mesh_type, material::type mtrl_type) const -> const model::mesh_node * {
        return mesh_node_lists_[mesh_type][mtrl_type];
    }

    auto model::mesh_pointer(int mesh_value) -> mesh * {
        assert(check_mesh_index(mesh_value));
        assert(meshs_ != nullptr);

        return meshs_.get() + mesh_value;
    }

    auto model::mesh_pointer(int mesh_value) const -> const mesh * {
        assert(check_mesh_index(mesh_value));
        assert(meshs_ != nullptr);

        return meshs_.get() + mesh_value;
    }

    auto model::can_deform_pnt_vertices() const -> bool {
        return can_deform_pn_vertices_;
    }

    void model::deform_pnt_vertices(void *dst_base_vertices, math::matrix *bone_matrices, const std::vector<granny_mesh_binding *> &rvct_pgrn_mesh_binding) const {
        int mesh_count_value = mesh_count();

        for (int i_mesh = 0; i_mesh < mesh_count_value; ++i_mesh) {
            assert(i_mesh < rvct_pgrn_mesh_binding.size());

            mesh &r_mesh = meshs_[i_mesh];
            if (r_mesh.can_deform_pnt_vertices())
                r_mesh.deform_pnt_vertices(dst_base_vertices, bone_matrices, rvct_pgrn_mesh_binding[i_mesh]);
        }
    }

    auto model::deform_vertex_count() const -> int {
        return deform_vtx_count_;
    }

    auto model::vertex_count() const -> int {
        return vtx_count_;
    }

    auto model::mesh_count() const -> int {
        return pgrn_model_ ? pgrn_model_->MeshBindingCount : 0;
    }

    auto model::granny_model_pointer() -> granny_model * {
        return pgrn_model_;
    }

    auto model::gpu_index_buffer() const -> gpu::index_buffer * {
        return idx_buf_.gpu_buffer();
    }

    auto model::gpu_pnt_vertex_buffer() const -> gpu::vertex_buffer * {
        return pnt_vtx_buf_.gpu_buffer();
    }

    auto model::load_indices() -> bool {
        if (idx_count_ <= 0)
            return true;

        if (!idx_buf_.create(idx_count_, gpu::format::INDEX16))
            return false;

        void *indices;

        if (!idx_buf_.lock((void **)&indices))
            return false;

        for (int m = 0; m < pgrn_model_->MeshBindingCount; ++m) {
            mesh &mesh = meshs_[m];
            mesh.load_indices(indices);
        }

        idx_buf_.unlock();
        return true;
    }

    auto model::load_meshs() -> bool {
        assert(meshs_ == nullptr);
        assert(pgrn_model_ != nullptr);

        if (pgrn_model_->MeshBindingCount <= 0)
            return true;

        int vtx_rigid_pos = 0;
        int vtx_deform_pos = 0;
        int vtx_pos = 0;
        int idx_pos = 0;

        int diffuse_pnt_mesh_node_count = 0;
        int blend_pnt_mesh_node_count = 0;

        int mesh_count_value = mesh_count();
        meshs_ = std::make_unique<mesh[]>(mesh_count_value);

        fv_f_ = 0;
        unsigned tex_coord_count = 0;

        for (int m = 0; m < mesh_count_value; ++m) {
            mesh &mesh_value = meshs_[m];
            granny_mesh *pgrn_mesh = pgrn_model_->MeshBindings[m].Mesh;

            if (GrannyMeshIsRigid(pgrn_mesh)) {
                if (!mesh_value.create_from_granny_mesh_pointer(pgrn_mesh, vtx_rigid_pos, idx_pos, mtrl_pal_))
                    return false;

                vtx_rigid_pos += GrannyGetMeshVertexCount(pgrn_mesh);
            } else {
                if (!mesh_value.create_from_granny_mesh_pointer(pgrn_mesh, vtx_deform_pos, idx_pos, mtrl_pal_))
                    return false;

                vtx_deform_pos += GrannyGetMeshVertexCount(pgrn_mesh);
                can_deform_pn_vertices_ |= mesh_value.can_deform_pnt_vertices();
            }
            have_blend_thing_ |= mesh_value.have_blend_thing();

            // The vertex format of the rigid vertex buffer follows the members of the mesh vertex type.
            for (const granny_data_type_definition *member = pgrn_mesh->PrimaryVertexData->VertexType; member->Type != GrannyEndMember; ++member) {
                if (!member->Name || !*member->Name)
                    continue;
                if (0 == strcmp(member->Name, GrannyVertexPositionName))
                    fv_f_ |= gpu::fvf::XYZ;
                else if (0 == strcmp(member->Name, GrannyVertexNormalName))
                    fv_f_ |= gpu::fvf::NORMAL;
                else if (0 == strcmp(member->Name, GrannyVertexTextureCoordinatesName "0"))
                    tex_coord_count = std::max(tex_coord_count, 1u);
                else if (0 == strcmp(member->Name, GrannyVertexTextureCoordinatesName "1"))
                    tex_coord_count = std::max(tex_coord_count, 2u);
            }

            vtx_pos += GrannyGetMeshVertexCount(pgrn_mesh);
            idx_pos += GrannyGetMeshIndexCount(pgrn_mesh);

            if (mesh_value.tri_group_node_list(material::TYPE_DIFFUSE_PNT))
                ++diffuse_pnt_mesh_node_count;

            if (mesh_value.tri_group_node_list(material::TYPE_BLEND_PNT))
                ++blend_pnt_mesh_node_count;
        }

        mesh_node_capacity_ = diffuse_pnt_mesh_node_count + blend_pnt_mesh_node_count;
        mesh_nodes_ = std::make_unique<mesh_node[]>(mesh_node_capacity_);

        for (int n = 0; n < mesh_count_value; ++n) {
            mesh &mesh_value = meshs_[n];
            granny_mesh *pgrn_mesh = pgrn_model_->MeshBindings[n].Mesh;

            mesh::type mesh_type = GrannyMeshIsRigid(pgrn_mesh) ? mesh::TYPE_RIGID : mesh::TYPE_DEFORM;

            if (mesh_value.tri_group_node_list(material::TYPE_DIFFUSE_PNT))
                append_mesh_node(mesh_type, material::TYPE_DIFFUSE_PNT, n);

            if (mesh_value.tri_group_node_list(material::TYPE_BLEND_PNT))
                append_mesh_node(mesh_type, material::TYPE_BLEND_PNT, n);
        }

        fv_f_ |= tex_coord_count >= 2 ? gpu::fvf::TEX2 : gpu::fvf::TEX1;

        // Dungeon blocks carry two texture coordinate sets (TPNT2Vertex).
        if ((gpu::fvf::XYZ | gpu::fvf::NORMAL | gpu::fvf::TEX2) == fv_f_) {
            for (int n = 0; n < mesh_count_value; ++n) {
                mesh &mesh_value = meshs_[n];
                mesh_value.set_pnt2_mesh();
            }
        }

        rigid_vtx_count_ = vtx_rigid_pos;
        deform_vtx_count_ = vtx_deform_pos;

        vtx_count_ = vtx_pos;
        idx_count_ = idx_pos;
        return true;
    }

    auto model::check_mesh_index(int index) const -> bool {
        if (index < 0)
            return false;
        if (index >= mesh_count())
            return false;

        return true;
    }

    void model::append_mesh_node(mesh::type mesh_type, material::type mtrl_type, int mesh) {
        assert(mesh_node_size_ < mesh_node_capacity_);

        mesh_node &mesh_node_value = mesh_nodes_[mesh_node_size_++];

        mesh_node_value.mesh_index = mesh;
        mesh_node_value.mesh_value = meshs_.get() + mesh;
        mesh_node_value.next_mesh_node = mesh_node_lists_[mesh_type][mtrl_type];
        mesh_node_lists_[mesh_type][mtrl_type] = &mesh_node_value;
    }

    auto model::create_from_granny_model_pointer(granny_model *pgrn_model) -> bool {
        assert(is_empty());

        pgrn_model_ = pgrn_model;

        if (!load_meshs())
            return false;

        if (!load_vertices())
            return false;

        if (!load_indices())
            return false;

        add_reference();

        return true;
    }

    auto model::create_device_objects() -> bool {
        if (rigid_vtx_count_ > 0)
            if (!pnt_vtx_buf_.create_device_objects())
                return false;

        if (idx_count_ > 0)
            if (!idx_buf_.create_device_objects())
                return false;

        return true;
    }

    void model::destroy_device_objects() {
        pnt_vtx_buf_.destroy_device_objects();
        idx_buf_.destroy_device_objects();
    }

    auto model::is_empty() const -> bool {
        if (pgrn_model_)
            return false;

        return true;
    }

    void model::destroy() {
        mtrl_pal_.clear();

        if (mesh_nodes_)
            mesh_nodes_.reset();

        if (meshs_)
            meshs_.reset();

        pnt_vtx_buf_.destroy();
        idx_buf_.destroy();

        initialize();
    }

    auto model::load_vertices() -> bool {
        if (rigid_vtx_count_ <= 0)
            return true;

        assert(meshs_ != nullptr);

        if (!pnt_vtx_buf_.create(rigid_vtx_count_, fv_f_, gpu::usage::WRITE_ONLY))
            return false;

        void *vertices;
        if (!pnt_vtx_buf_.lock(&vertices))
            return false;

        for (int m = 0; m < pgrn_model_->MeshBindingCount; ++m) {
            mesh &mesh = meshs_[m];
            mesh.load_vertices(vertices);
        }

        pnt_vtx_buf_.unlock();
        return true;
    }

    void model::initialize() {
        memset(mesh_node_lists_, 0, sizeof(mesh_node_lists_));

        pgrn_model_ = nullptr;
        meshs_.reset();
        mesh_nodes_.reset();

        mesh_node_size_ = 0;
        mesh_node_capacity_ = 0;

        rigid_vtx_count_ = 0;
        deform_vtx_count_ = 0;
        vtx_count_ = 0;
        idx_count_ = 0;

        can_deform_pn_vertices_ = false;

        fv_f_ = 0;
        have_blend_thing_ = false;
    }

    model::model() {
        initialize();
    }

    model::~model() {
        destroy();
    }
}
