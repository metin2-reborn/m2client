#include "model.h"
#include "model_instance.h"

namespace metin2::inline models {

    void model_instance::make_bound_box(bound_box *bound_box,
        const float *mat,
        const float *obb_min,
        const float *obb_max,
        math::vector3 *vt_min,
        math::vector3 *vt_max) {
        bound_box->sx = obb_min[0] * mat[0] + obb_min[1] * mat[4] + obb_min[2] * mat[8] + mat[12];
        bound_box->sy = obb_min[0] * mat[1] + obb_min[1] * mat[5] + obb_min[2] * mat[9] + mat[13];
        bound_box->sz = obb_min[0] * mat[2] + obb_min[1] * mat[6] + obb_min[2] * mat[10] + mat[14];

        bound_box->ex = obb_max[0] * mat[0] + obb_max[1] * mat[4] + obb_max[2] * mat[8] + mat[12];
        bound_box->ey = obb_max[0] * mat[1] + obb_max[1] * mat[5] + obb_max[2] * mat[9] + mat[13];
        bound_box->ez = obb_max[0] * mat[2] + obb_max[1] * mat[6] + obb_max[2] * mat[10] + mat[14];

        vt_min->x = std::min(vt_min->x, bound_box->sx);
        vt_min->x = std::min(vt_min->x, bound_box->ex);
        vt_min->y = std::min(vt_min->y, bound_box->sy);
        vt_min->y = std::min(vt_min->y, bound_box->ey);
        vt_min->z = std::min(vt_min->z, bound_box->sz);
        vt_min->z = std::min(vt_min->z, bound_box->ez);

        vt_max->x = std::max(vt_max->x, bound_box->sx);
        vt_max->x = std::max(vt_max->x, bound_box->ex);
        vt_max->y = std::max(vt_max->y, bound_box->sy);
        vt_max->y = std::max(vt_max->y, bound_box->ey);
        vt_max->z = std::max(vt_max->z, bound_box->sz);
        vt_max->z = std::max(vt_max->z, bound_box->ez);
    }

    auto model_instance::intersect(const math::matrix *matrix,
        float * /*pu*/, float * /*pv*/, float *pt) -> bool {
        if (!pgrn_model_instance_)
            return false;

        float u, v, t;
        *pt = 100000000.0f;

        const float limit = 10000000.0f;
        math::vector3 vt_min, vt_max;
        vt_min.x = vt_min.y = vt_min.z = limit;
        vt_max.x = vt_max.y = vt_max.z = -limit;

        bound_box bound_box;

        int mesh_count = model_value_->mesh_count();

        for (int m = 0; m < mesh_count; ++m) {
            const granny_mesh *pgrn_mesh = model_value_->granny_model_pointer()->MeshBindings[m].Mesh;

            for (int b = 0; b < pgrn_mesh->BoneBindingCount; ++b) {
                const granny_bone_binding &rgrn_bone_binding = pgrn_mesh->BoneBindings[b];

                float *transform = GrannyGetWorldPose4x4(world_pose_ptr(), mesh_bone_indices(m)[b]);

                make_bound_box(&bound_box,
                    transform,
                    rgrn_bone_binding.OBBMin,
                    rgrn_bone_binding.OBBMax,
                    &vt_min,
                    &vt_max);
            }
        }

        return intersect_cube(matrix,
            vt_min.x, vt_min.y, vt_min.z,
            vt_max.x, vt_max.y, vt_max.z,
            vt_pick_ray_orig_, vt_pick_ray_dir_,
            &u, &v, &t);
    }

    void model_instance::get_bound_box(math::vector3 *vt_min, math::vector3 *vt_max) {
        if (!pgrn_model_instance_)
            return;

        bound_box bound_box;

        vt_min->x = vt_min->y = vt_min->z = +100000.0f;
        vt_max->x = vt_max->y = vt_max->z = -100000.0f;

        int mesh_count = model_value_->mesh_count();
        for (int m = 0; m < mesh_count; ++m) {
            const granny_mesh *pgrn_mesh = model_value_->granny_model_pointer()->MeshBindings[m].Mesh;

            int *bone_indices = mesh_bone_indices(m);
            for (int b = 0; b < pgrn_mesh->BoneBindingCount; ++b) {
                const granny_bone_binding &rgrn_bone_binding = pgrn_mesh->BoneBindings[b];

                make_bound_box(&bound_box,
                    GrannyGetWorldPose4x4(world_pose_ptr(), bone_indices[b]),
                    rgrn_bone_binding.OBBMin, rgrn_bone_binding.OBBMax, vt_min, vt_max);
            }
        }
    }
}
