#pragma once

#include "vector.h"

namespace metin2::math {
    struct quaternion {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f; // identity rotation by default

        constexpr quaternion() = default;
        constexpr quaternion(float fx, float fy, float fz, float fw) : x(fx), y(fy), z(fz), w(fw) {}

        explicit quaternion(const float *values) : x(values[0]), y(values[1]), z(values[2]), w(values[3]) {}

        operator float *() { return &x; }
        operator const float *() const { return &x; }

        auto operator+=(const quaternion &q) -> quaternion & {
            x += q.x;
            y += q.y;
            z += q.z;
            w += q.w;
            return *this;
        }

        auto operator-=(const quaternion &q) -> quaternion & {
            x -= q.x;
            y -= q.y;
            z -= q.z;
            w -= q.w;
            return *this;
        }

        auto operator*=(const quaternion &q) -> quaternion &;
        auto operator*=(float f) -> quaternion & {
            x *= f;
            y *= f;
            z *= f;
            w *= f;
            return *this;
        }

        auto operator/=(float f) -> quaternion & {
            const float inv = 1.0f / f;
            x *= inv;
            y *= inv;
            z *= inv;
            w *= inv;
            return *this;
        }

        auto operator+() const -> quaternion { return *this; }
        auto operator-() const -> quaternion { return {-x, -y, -z, -w}; }
        auto operator+(const quaternion &q) const -> quaternion { return {x + q.x, y + q.y, z + q.z, w + q.w}; }
        auto operator-(const quaternion &q) const -> quaternion { return {x - q.x, y - q.y, z - q.z, w - q.w}; }

        // a * b rotates by a, then by b (the Direct3D order).
        auto operator*(const quaternion &q) const -> quaternion;
        auto operator*(float f) const -> quaternion { return {x * f, y * f, z * f, w * f}; }

        auto operator/(float f) const -> quaternion {
            const float inv = 1.0f / f;
            return {x * inv, y * inv, z * inv, w * inv};
        }

        friend auto operator*(float f, const quaternion &q) -> quaternion { return q * f; }

        auto operator==(const quaternion &q) const -> bool = default;
    };

    auto conjugate(const quaternion &q) -> quaternion;
    auto quaternion_rotation_axis(const vector3 &axis, float angle) -> quaternion;
    auto quaternion_rotation_yaw_pitch_roll(float yaw, float pitch, float roll) -> quaternion;
}
