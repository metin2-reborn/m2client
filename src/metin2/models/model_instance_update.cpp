#include <metin2/log/log.h>
#include "model.h"
#include "model_instance.h"

namespace metin2::inline models {

    void model_instance::update(uint32_t ani_fps) {
        if (!ani_fps)
            return;

        const auto cur_update_frame = (uint32_t)(local_time() * static_cast<float>(ANIFPS_MAX));
        const uint32_t anifps_step = ANIFPS_MAX / ani_fps;
        if (cur_update_frame > anifps_step && cur_update_frame / anifps_step == old_update_frame_ / anifps_step)
            return;

        old_update_frame_ = cur_update_frame;

        GrannyFreeCompletedModelControls(pgrn_model_instance_); // Completed controls that are not freed pile up and end in a black screen.

        GrannySetModelClock(pgrn_model_instance_, local_time());
    }

    void model_instance::update_local_time(float elapsed_time) {
        local_time_ += elapsed_time;
    }

    void model_instance::update_transform(math::matrix *matrix, float seconds_elapsed) {
        if (!pgrn_model_instance_) {
            metin2::log::error("Cannot update the transform: no Granny model instance");
            return;
        }
        GrannyUpdateModelMatrix(pgrn_model_instance_, seconds_elapsed, (const float *)matrix, (float *)matrix, false);
    }

    void model_instance::deform(const math::matrix *world_matrix) {
        if (is_empty())
            return;

        update_world_pose();
        update_world_matrices(world_matrix);

        if (model_value_->can_deform_pnt_vertices()) {
            graphic_vertex_buffer &deformable_vertex_buffer = deformable_vertex_buffer_ref();
            pnt_vertex *pnt_vertices;
            if (deformable_vertex_buffer.lock_range(model_value_->deform_vertex_count(), (void **)&pnt_vertices)) {
                deform_pnt_vertices(pnt_vertices);
                deformable_vertex_buffer.unlock();
            } else {
                metin2::log::error("Cannot lock the dynamic deform vertex buffer");
            }
        }
    }

    class local_pose_cache {
      public:
        local_pose_cache() {
            pgrn_local_pose_ = nullptr;
            bone_count_ = 0;
        }

        virtual ~local_pose_cache() {
            if (pgrn_local_pose_)
                GrannyFreeLocalPose(pgrn_local_pose_);
        }

        auto get(int bone_count) -> granny_local_pose * {
            if (pgrn_local_pose_) {
                if (bone_count_ >= bone_count)
                    return pgrn_local_pose_;

                GrannyFreeLocalPose(pgrn_local_pose_);
            }

            bone_count_ = bone_count;
            pgrn_local_pose_ = GrannyNewLocalPose(bone_count_);
            return pgrn_local_pose_;
        }

      private:
        granny_local_pose *pgrn_local_pose_;
        int bone_count_;
    };

    void model_instance::update_world_pose() {
        // Weapons (parent bone index != 0) and LOD models update their world pose; hair does not.

        if (skeleton_inst_)
            if (*skeleton_inst_ != this)
                return;

        static local_pose_cache shared_local_pose;

        granny_skeleton *pgrn_skeleton = GrannyGetSourceSkeleton(pgrn_model_instance_);
        granny_local_pose *pgrn_local_pose = shared_local_pose.get(pgrn_skeleton->BoneCount);

        const float *attach_bone_matrix = (mc_parent_instance_) ? mc_parent_instance_->bone_matrix_pointer(parent_bone_index_) : nullptr;

        GrannySampleModelAnimationsAccelerated(pgrn_model_instance_, pgrn_skeleton->BoneCount, attach_bone_matrix, pgrn_local_pose, world_pose_ptr());
        GrannyFreeCompletedModelControls(pgrn_model_instance_);
    }

    void model_instance::update_world_matrices(const math::matrix *c_p_world_matrix) {
        if (!mesh_matrices_)
            return;

        assert(model_value_ != nullptr);

        int mesh_count = model_value_->mesh_count();

        granny_matrix_4x4 *pgrn_mat_composite_buffer = GrannyGetWorldPoseComposite4x4Array(world_pose_ptr());
        auto *bone_matrices = (math::matrix *)pgrn_mat_composite_buffer;

        for (int i = 0; i < mesh_count; ++i) {
            math::matrix &r_world_matrix = mesh_matrices_[i];

            const mesh *mesh = model_value_->mesh_pointer(i);

            int *bone_indices = mesh_bone_indices(i);

            if (mesh->can_deform_pnt_vertices()) {
                r_world_matrix = *c_p_world_matrix;
            } else {
                int bone = *bone_indices;
                r_world_matrix = bone_matrices[bone] * *c_p_world_matrix;
            }
        }
    }

    void model_instance::deform_pnt_vertices(void *dest) {
        assert(model_value_ != nullptr);
        assert(model_value_->can_deform_pnt_vertices());

        model_value_->deform_pnt_vertices(dest, (math::matrix *)GrannyGetWorldPoseComposite4x4Array(world_pose_ptr()), vct_pgrn_mesh_binding_);
    }
}
