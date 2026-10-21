#pragma once

// Vectors, matrices, quaternions, planes and colours in the Direct3D conventions the game was written
// for: row vectors (v' = v * M), right-handed view and projection matrices.

#include "color.h"
#include "matrix.h"
#include "plane.h"
#include "quaternion.h"
#include "vector.h"

namespace metin2::math {

    inline constexpr float PI = 3.141592654f;

    constexpr auto to_radian(float degree) -> float {
        return degree * (PI / 180.0f);
    }

    constexpr auto to_degree(float radian) -> float {
        return radian * (180.0f / PI);
    }
}
