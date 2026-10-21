/* Copyright (C) John W. Ratcliff, 2001.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) John W. Ratcliff, 2001"
 */
#pragma once

#include "vector.h"

#include <metin2/math/math.h>

namespace metin2::inline geometry {

    enum view_state {
        VS_INSIDE,  // completely inside the frustum.
        VS_PARTIAL, // partially inside and partially outside the frustum.
        VS_OUTSIDE  // completely outside the frustum
    };

    class frustum {
      public:
        frustum() : using_sphere_(false), center_(0.0f, 0.0f, 0.0f), radius_(0.0f) {}

        void build_view_frustum(math::matrix &mat);
        void build_view_frustum2(math::matrix &mat, float near_value, float far_value, float fov, float aspect, const math::vector3 &v_camera, const math::vector3 &v_look);
        auto view_volume_test(const vector3d &center, const float radius) const -> view_state;

      private:
        bool using_sphere_;
        math::vector3 center_;
        float radius_;
        math::plane plane_[6];
    };
}
