#include "mesh.h"
#include "material.h"
#include "model.h"

namespace metin2::inline models {

    granny_data_type_definition granny_pnt3322_vertex_type[5] =
        {
            {GrannyReal32Member, GrannyVertexPositionName, nullptr, 3},
            {GrannyReal32Member, GrannyVertexNormalName, nullptr, 3},
            {GrannyReal32Member, GrannyVertexTextureCoordinatesName "0", nullptr, 2},
            {GrannyReal32Member, GrannyVertexTextureCoordinatesName "1", nullptr, 2},
            {GrannyEndMember}
    };

    static_assert(sizeof(pnt_vertex) == 32, "TPNTVertex must match GrannyPNT332VertexType");
    static_assert(sizeof(tpnt2_vertex) == 40, "TPNT2Vertex must match GrannyPNT3322VertexType");

    void mesh::load_indices(void *dst_base_indices) {
        const granny_mesh *pgrn_mesh = granny_mesh_pointer();

        index *dst_indices = ((index *)dst_base_indices) + idx_base_pos_;
        GrannyCopyMeshIndices(pgrn_mesh, sizeof(index), dst_indices);
    }

    void mesh::load_vertices(void *dst_base_vertices) {
        const granny_mesh *pgrn_mesh = granny_mesh_pointer();

        if (!GrannyMeshIsRigid(pgrn_mesh)) {
            return;
        }

        pnt_vertex *dst_vertices = ((pnt_vertex *)dst_base_vertices) + vtx_base_pos_;
        GrannyCopyMeshVertices(pgrn_mesh, pgrn_mesh_type_, dst_vertices);
    }

    void mesh::deform_pnt_vertices(void *dst_base_vertices, math::matrix *bone_matrices, granny_mesh_binding *pgrn_mesh_binding) const {
        assert(dst_base_vertices != nullptr);
        assert(bone_matrices != nullptr);
        assert(pgrn_mesh_deformer_ != nullptr);

        const granny_mesh *pgrn_mesh = granny_mesh_pointer();

        auto *src_vertices = (pnt_vertex *)GrannyGetMeshVertices(pgrn_mesh);
        pnt_vertex *dst_vertices = ((pnt_vertex *)dst_base_vertices) + vtx_base_pos_;

        int vtx_count = GrannyGetMeshVertexCount(pgrn_mesh);

        int *bone_indices = (int *)GrannyGetMeshBindingToBoneIndices(pgrn_mesh_binding);

        GrannyDeformVertices(
            pgrn_mesh_deformer_,
            bone_indices,
            (float *)bone_matrices,
            vtx_count,
            src_vertices,
            dst_vertices);
    }

    auto mesh::can_deform_pnt_vertices() const -> bool {
        return can_deform_pnt_vertex_;
    }

    auto mesh::granny_mesh_pointer() const -> const granny_mesh * {
        return pgrn_mesh_;
    }

    auto mesh::tri_group_node_list(material::type mtrl_type) const -> const mesh::tri_group_node * {
        return tri_group_node_lists_[mtrl_type];
    }

    auto mesh::vertex_count() const -> int {
        assert(pgrn_mesh_ != nullptr);
        return GrannyGetMeshVertexCount(pgrn_mesh_);
    }

    auto mesh::vertex_base_position() const -> int {
        return vtx_base_pos_;
    }

    auto mesh::is_empty() const -> bool {
        if (pgrn_mesh_) {
            return false;
        }

        return true;
    }

    auto mesh::create_from_granny_mesh_pointer(granny_mesh *pgrn_mesh, int vtx_base_pos, int idx_base_pos, material_palette &mtrl_pal) -> bool {
        assert(is_empty());

        pgrn_mesh_ = pgrn_mesh;
        vtx_base_pos_ = vtx_base_pos;
        idx_base_pos_ = idx_base_pos;

        if (!GrannyMeshIsRigid(pgrn_mesh_)) {
            can_deform_pnt_vertex_ = true;

            granny_data_type_definition *pgrn_input_type = GrannyGetMeshVertexType(pgrn_mesh_);
            granny_data_type_definition *pgrn_output_type = pgrn_mesh_type_;

            pgrn_mesh_deformer_ = GrannyNewMeshDeformer(pgrn_input_type, pgrn_output_type, GrannyDeformPositionNormal, GrannyAllowUncopiedTail);
            assert(pgrn_mesh_deformer_ != nullptr && "Cannot create mesh deformer");
        }

        if (!load_materials(mtrl_pal)) {
            return false;
        }

        if (!load_tri_group_node_list(mtrl_pal)) {
            return false;
        }

        return true;
    }

    auto mesh::load_tri_group_node_list(material_palette &mtrl_pal) -> bool {
        assert(pgrn_mesh_ != nullptr);
        assert(tri_group_nodes_ == nullptr);

        int mtrl_count = pgrn_mesh_->MaterialBindingCount;
        if (mtrl_count <= 0) { // Some map meshes have no material bindings; this crashed on the second floor of the Cave of the Thousand.
            return true;
        }

        int group_node_count = GrannyGetMeshTriangleGroupCount(pgrn_mesh_);
        if (group_node_count <= 0) {
            return true;
        }

        tri_group_nodes_ = std::make_unique<tri_group_node[]>(group_node_count);

        const granny_tri_material_group *pgrn_tri_groups = GrannyGetMeshTriangleGroups(pgrn_mesh_);

        for (int g = 0; g < group_node_count; ++g) {
            const granny_tri_material_group &rgrn_tri_group = pgrn_tri_groups[g];
            tri_group_node *tri_group_node_value = tri_group_nodes_.get() + g;

            tri_group_node_value->idx_pos = idx_base_pos_ + rgrn_tri_group.TriFirst * 3;
            tri_group_node_value->tri_count = rgrn_tri_group.TriCount;

            int i_mtrl = rgrn_tri_group.MaterialIndex;
            if (i_mtrl < 0 || i_mtrl >= mtrl_count) {
                tri_group_node_value->mtrl_index = 0;
            } else {
                tri_group_node_value->mtrl_index = mtrl_index_vector_[i_mtrl];
            }

            const material &rk_mtrl = mtrl_pal.material_ref(tri_group_node_value->mtrl_index);
            tri_group_node_value->next_tri_group_node = tri_group_node_lists_[rk_mtrl.get_type()];
            tri_group_node_lists_[rk_mtrl.get_type()] = tri_group_node_value;
        }

        return true;
    }

    auto mesh::load_materials(material_palette &mtrl_pal) -> bool {
        assert(pgrn_mesh_ != nullptr);

        if (pgrn_mesh_->MaterialBindingCount <= 0) {
            return true;
        }

        int mtrl_count = pgrn_mesh_->MaterialBindingCount;
        bool have_blend_thing = false;

        for (int m = 0; m < mtrl_count; ++m) {
            granny_material *pgrn_material = pgrn_mesh_->MaterialBindings[m].Material;
            uint32_t mtrl_index = mtrl_pal.register_material(pgrn_material);
            mtrl_index_vector_.push_back(mtrl_index);
            have_blend_thing |= mtrl_pal.material_ref(mtrl_index).get_type() == material::TYPE_BLEND_PNT;
        }
        have_blend_thing_ = have_blend_thing;

        return true;
    }

    void mesh::set_pnt2_mesh() {
        pgrn_mesh_type_ = granny_pnt3322_vertex_type;
    }

    void mesh::destroy() {
        if (tri_group_nodes_) {
            tri_group_nodes_.reset();
        }

        mtrl_index_vector_.clear();

        if (pgrn_mesh_deformer_) {
            GrannyFreeMeshDeformer(pgrn_mesh_deformer_);
        }

        initialize();
    }

    void mesh::initialize() {
        for (int r = 0; r < material::TYPE_MAX_NUM; ++r) {
            tri_group_node_lists_[r] = nullptr;
        }

        pgrn_mesh_type_ = GrannyPNT332VertexType;
        pgrn_mesh_ = nullptr;
        pgrn_mesh_deformer_ = nullptr;

        tri_group_nodes_.reset();

        vtx_base_pos_ = 0;
        idx_base_pos_ = 0;

        can_deform_pnt_vertex_ = false;
        have_blend_thing_ = false;
    }

    mesh::mesh() {
        initialize();
    }

    mesh::~mesh() {
        destroy();
    }
}
