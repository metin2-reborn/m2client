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

#include <cmath>

#include <metin2/math/math.h>

namespace metin2::inline geometry {

    class vector3d : public math::vector3 {
      public:
        vector3d(void) = default; // leaves the coordinates uninitialized

        vector3d(const vector3d &a)
            : math::vector3() {
            x = a.x;
            y = a.y;
            z = a.z;
        };

        vector3d(float a, float b, float c) {
            x = a;
            y = b;
            z = c;
        };

        auto operator==(const vector3d &a) const -> bool {
            if (a.x == x && a.y == y && a.z == z) return true;
            return false;
        };

        auto operator!=(const vector3d &a) const -> bool {
            if (a.x != x || a.y != y || a.z != z) return true;
            return false;
        };

        auto operator=(const vector3d &a) -> vector3d & {
            x = a.x;
            y = a.y;
            z = a.z;
            return (*this);
        };

        auto operator+(const vector3d &a) const -> vector3d {
            vector3d sum(x + a.x, y + a.y, z + a.z);
            return (sum);
        };

        auto operator-(const vector3d &a) const -> vector3d {
            vector3d diff(x - a.x, y - a.y, z - a.z);
            return (diff);
        };

        auto operator*(const float s) const -> vector3d {
            vector3d scaled(x * s, y * s, z * s);
            return (scaled);
        };

        auto operator/(const float s) const -> vector3d {
            float r = 1.0f / s;
            vector3d scaled(x * r, y * r, z * r);
            return (scaled);
        };

        void operator+=(const vector3d a) {
            x += a.x;
            y += a.y;
            z += a.z;
        };
        void operator-=(const vector3d a) {
            x -= a.x;
            y -= a.y;
            z -= a.z;
        };
        void operator*=(const float s) {
            x *= s;
            y *= s;
            z *= s;
        }

        auto operator-(void) const -> vector3d {
            vector3d negated(-x, -y, -z);
            return (negated);
        };

        void set(float a, float b, float c) {
            x = a;
            y = b;
            z = c;
        };

        auto length(void) const -> float {
            return float(sqrtf(x * x + y * y + z * z));
        };

        auto distance_sq(const vector3d &a) const -> float {
            float dx = a.x - x;
            float dy = a.y - y;
            float dz = a.z - z;
            return dx * dx + dy * dy + dz * dz;
        };

        auto distance(const vector3d &a) const -> float {
            return sqrtf(distance_sq(a));
        }

        auto length2(void) const -> float {
            return x * x + y * y + z * z;
        };

        auto normalize(void) -> float // returns the length before normalizing
        {
            float l = length();
            if (l != 0) {
                x /= l;
                y /= l;
                z /= l;
            } else {
                x = y = z = 0;
            }
            return l;
        };

        auto dot(const vector3d &a) const -> float {
            return (x * a.x + y * a.y + z * a.z);
        };
    };

    inline auto operator*(float s, const vector3d &v) -> vector3d {
        vector3d scaled(v.x * s, v.y * s, v.z * s);
        return (scaled);
    };
}
