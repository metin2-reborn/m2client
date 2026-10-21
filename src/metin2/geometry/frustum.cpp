/* Copyright (C) John W. Ratcliff, 2001.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) John W. Ratcliff, 2001"
 */

#include "frustum.h"

#include <cmath>

namespace metin2::inline geometry {

    auto frustum::view_volume_test(const vector3d &c_v3_center, const float radius) const -> view_state {
        if (using_sphere_) {
            math::vector3 v(
                c_v3_center.x - center_.x,
                c_v3_center.y - center_.y,
                c_v3_center.z - center_.z);

            if ((radius + radius_) * (radius + radius_) < math::length_sq(v)) {
                return VS_OUTSIDE;
            }
        }

        const int count = 6;

        math::vector3 center = c_v3_center;

        int i;

        float distance[count];
        for (i = 0; i < count; i++) {
            distance[i] = math::plane_dot_coord(plane_[i], center);
            if (distance[i] <= -radius)
                return VS_OUTSIDE;
        }

        for (i = 0; i < count; i++) {
            if (distance[i] <= radius)
                return VS_PARTIAL;
        }

        return VS_INSIDE;
    }

    void frustum::build_view_frustum(math::matrix &mat) {
        using_sphere_ = false;
        plane_[0] = math::plane(mat._13, mat._23, mat._33, mat._43);
        plane_[1] = math::plane(mat._14 - mat._13, mat._24 - mat._23, mat._34 - mat._33, mat._44 - mat._43);
        plane_[2] = math::plane(mat._14 + mat._11, mat._24 + mat._21, mat._34 + mat._31, mat._44 + mat._41);
        plane_[3] = math::plane(mat._14 - mat._11, mat._24 - mat._21, mat._34 - mat._31, mat._44 - mat._41);
        plane_[4] = math::plane(mat._14 + mat._12, mat._24 + mat._22, mat._34 + mat._32, mat._44 + mat._42);
        plane_[5] = math::plane(mat._14 - mat._12, mat._24 - mat._22, mat._34 - mat._32, mat._44 - mat._42);

        for (int i = 0; i < 6; i++)
            plane_[i] = math::plane_normalize(plane_[i]);
    }

    void frustum::build_view_frustum2(math::matrix &mat, float near_value, float far_value, float fov, float aspect, const math::vector3 &v_camera, const math::vector3 &v_look) {
        float view_len = far_value - near_value;
        float h = view_len * tanf(fov * 0.5f);
        float w = h * aspect;
        math::vector3 p(0.0f, 0.0f, near_value + view_len * 0.5f);
        math::vector3 q(w, h, view_len);
        math::vector3 pq = p - q;
        radius_ = math::length(pq);
        center_ = v_camera + v_look * (near_value + view_len * 0.5f);
        build_view_frustum(mat);
        using_sphere_ = true;
    }
}
