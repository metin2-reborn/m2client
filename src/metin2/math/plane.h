#pragma once

#include "vector.h"

namespace metin2::math {

    // a * x + b * y + c * z + d = 0
    struct plane {
        float a = 0.0f;
        float b = 0.0f;
        float c = 0.0f;
        float d = 0.0f;

        constexpr plane() = default;
        constexpr plane(float fa, float fb, float fc, float fd) : a(fa), b(fb), c(fc), d(fd) {}

        operator float *() { return &a; }
        operator const float *() const { return &a; }

        auto operator+() const -> plane { return *this; }
        auto operator-() const -> plane { return {-a, -b, -c, -d}; }
        auto operator==(const plane &p) const -> bool = default;
    };

    // Signed distance of the point to a normalized plane.
    auto plane_dot_coord(const plane &p, const vector3 &v) -> float;
    auto plane_normalize(const plane &p) -> plane;
}
