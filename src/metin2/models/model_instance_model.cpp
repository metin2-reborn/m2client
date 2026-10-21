#include "model.h"
#include <metin2/gpu/types.h>
#include "model_instance.h"

namespace metin2::inline models {

    void model_instance::clear() {
        mtrl_pal_.clear();

        destroy_device_objects();
        destroy_mesh_binding_vector();
        destroy_mesh_matrices();
        destroy_model_instance();
        destroy_world_pose();

        initialize_impl();
    }

    void model_instance::set_main_model_pointer(model *model, graphic_vertex_buffer *shared_deformable_vertex_buffer) {
        set_linked_model_pointer(model, shared_deformable_vertex_buffer, nullptr);
    }

    void model_instance::set_linked_model_pointer(model *model, graphic_vertex_buffer *shared_deformable_vertex_buffer, model_instance **skeleton_inst) {
        clear();

        model_value_ = model;

        model_value_->add_reference();

        if (shared_deformable_vertex_buffer)
            set_shared_deformable_vertex_buffer(shared_deformable_vertex_buffer);
        else
            create_dynamic_vertex_buffer();

        create_model_instance();

        if (skeleton_inst && *skeleton_inst) {
            skeleton_inst_ = skeleton_inst;
            create_world_pose(*skeleton_inst);
            create_mesh_binding_vector(*skeleton_inst);
        } else {
            create_world_pose(nullptr);
            create_mesh_binding_vector(nullptr);
        }

        create_mesh_matrices();

        reset_local_time();

        mtrl_pal_.copy(model->get_material_palette());
    }

    auto model_instance::world_pose_ptr() const -> granny_world_pose * {
        if (pgrn_world_pose_real_)
            return pgrn_world_pose_real_;

        if (skeleton_inst_ && *skeleton_inst_)
            return (*skeleton_inst_)->pgrn_world_pose_real_;

        assert(skeleton_inst_ != nullptr && "__GetWorldPosePtr - NO HAVE SKELETON");
        return nullptr;
    }

    auto model_instance::mesh_bone_indices(unsigned int mesh_binding) const -> int * {
        assert(mesh_binding < vct_pgrn_mesh_binding_.size());
        return (int *)GrannyGetMeshBindingToBoneIndices(vct_pgrn_mesh_binding_[mesh_binding]);
    }

    auto model_instance::create_mesh_binding_vector(model_instance *dst_model_inst) -> bool {
        assert(vct_pgrn_mesh_binding_.empty());

        if (!model_value_)
            return false;

        granny_model *pgrn_model = model_value_->granny_model_pointer();
        if (!pgrn_model)
            return false;

        granny_skeleton *pgrn_dst_skeleton = pgrn_model->Skeleton;
        if (dst_model_inst && dst_model_inst->model_value_ && dst_model_inst->model_value_->granny_model_pointer())
            pgrn_dst_skeleton = dst_model_inst->model_value_->granny_model_pointer()->Skeleton;

        vct_pgrn_mesh_binding_.reserve(pgrn_model->MeshBindingCount);

        granny_int32 mesh_binding;
        for (mesh_binding = 0; mesh_binding != pgrn_model->MeshBindingCount; ++mesh_binding)
            vct_pgrn_mesh_binding_.push_back(GrannyNewMeshBinding(pgrn_model->MeshBindings[mesh_binding].Mesh, pgrn_model->Skeleton, pgrn_dst_skeleton));

        return true;
    }

    void model_instance::destroy_mesh_binding_vector() {
        std::for_each(vct_pgrn_mesh_binding_.begin(), vct_pgrn_mesh_binding_.end(), GrannyFreeMeshBinding);
        vct_pgrn_mesh_binding_.clear();
    }

    void model_instance::create_world_pose(model_instance *skeleton_inst) {
        assert(pgrn_model_instance_ != nullptr);
        assert(pgrn_world_pose_real_ == nullptr);

        if (skeleton_inst)
            return;

        granny_skeleton *pgrn_skeleton = GrannyGetSourceSkeleton(pgrn_model_instance_);

        pgrn_world_pose_real_ = GrannyNewWorldPose(pgrn_skeleton->BoneCount);
    }

    void model_instance::destroy_world_pose() {
        if (!pgrn_world_pose_real_)
            return;

        GrannyFreeWorldPose(pgrn_world_pose_real_);
        pgrn_world_pose_real_ = nullptr;
    }

    void model_instance::create_model_instance() {
        assert(model_value_ != nullptr);
        assert(pgrn_model_instance_ == nullptr);

        const granny_model *pgrn_model = model_value_->granny_model_pointer();
        pgrn_model_instance_ = GrannyInstantiateModel(pgrn_model);
    }

    void model_instance::destroy_model_instance() {
        if (!pgrn_model_instance_)
            return;

        GrannyFreeModelInstance(pgrn_model_instance_);
        pgrn_model_instance_ = nullptr;
    }

    void model_instance::create_mesh_matrices() {
        assert(model_value_ != nullptr);

        if (model_value_->mesh_count() <= 0) // Some models (cameras, for example) have no meshes.
            return;

        int mesh_count = model_value_->mesh_count();
        mesh_matrices_ = std::make_unique<math::matrix[]>(mesh_count);
    }

    void model_instance::destroy_mesh_matrices() {
        if (!mesh_matrices_)
            return;

        mesh_matrices_.reset();
        mesh_matrices_.reset();
    }

    auto model_instance::vertex_count() -> uint32_t {
        if (!model_value_)
            return 0;

        return model_value_->vertex_count();
    }

    void model_instance::set_shared_deformable_vertex_buffer(graphic_vertex_buffer *shared_deformable_vertex_buffer) {
        shared_deformable_vertex_buffer_ = shared_deformable_vertex_buffer;
    }

    auto model_instance::deformable_vertex_buffer_ptr() -> gpu::vertex_buffer * {
        return deformable_vertex_buffer_ref().gpu_buffer();
    }

    auto model_instance::deformable_vertex_buffer_ref() -> graphic_vertex_buffer & {
        if (shared_deformable_vertex_buffer_)
            return *shared_deformable_vertex_buffer_;

        return local_deformable_vertex_buffer_;
    }

    void model_instance::create_dynamic_vertex_buffer() {
        assert(model_value_ != nullptr);
        assert(local_deformable_vertex_buffer_.is_empty());

        int vtx_count = model_value_->deform_vertex_count();

        if (0 != vtx_count) {
            if (!local_deformable_vertex_buffer_.create(vtx_count,
                    gpu::fvf::XYZ | gpu::fvf::NORMAL | gpu::fvf::TEX1,
                    gpu::usage::WRITE_ONLY))
                return;
        }
    }

    void model_instance::destroy_dynamic_vertex_buffer() {
        local_deformable_vertex_buffer_.destroy();
        shared_deformable_vertex_buffer_ = nullptr;
    }

    auto model_instance::bone_index_by_name(const char *bone_name, int *bone_index) const -> bool {
        assert(pgrn_model_instance_ != nullptr);

        granny_skeleton *pgrn_skeleton = GrannyGetSourceSkeleton(pgrn_model_instance_);

        if (!GrannyFindBoneByName(pgrn_skeleton, bone_name, bone_index))
            return false;

        return true;
    }

    auto model_instance::bone_matrix_pointer(int bone) const -> const float * {
        const float *bones = GrannyGetWorldPose4x4(world_pose_ptr(), bone);
        return bones;
    }

    auto model_instance::composite_bone_matrix_pointer(int bone) const -> const float * {
        // GrannyGetWorldPose4x4 may return wrong scale values: for speed, Granny does not fill every
        // element of the world pose matrices, so use the composite matrix here.
        return GrannyGetWorldPoseComposite4x4(world_pose_ptr(), bone);
    }
}
