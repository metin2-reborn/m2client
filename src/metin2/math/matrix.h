#pragma once

#include "vector.h"

#include <cstring>
#include <vector>

namespace metin2::math {

    // 4x4 matrix for row vectors (v' = v * M): the translation is in the fourth row.
    struct matrix {
        union {
            struct {
                float _11, _12, _13, _14;
                float _21, _22, _23, _24;
                float _31, _32, _33, _34;
                float _41, _42, _43, _44;
            };
            float m[4][4];
        };

        // All zero, not identity (see matrix_identity()).
        matrix() { std::memset(m, 0, sizeof(m)); }
        explicit matrix(const float *values) { std::memcpy(m, values, sizeof(m)); }
        matrix(float f11, float f12, float f13, float f14, float f21, float f22, float f23, float f24,
            float f31, float f32, float f33, float f34, float f41, float f42, float f43, float f44)
            : _11(f11), _12(f12), _13(f13), _14(f14), _21(f21), _22(f22), _23(f23), _24(f24),
              _31(f31), _32(f32), _33(f33), _34(f34), _41(f41), _42(f42), _43(f43), _44(f44) {}

        auto operator()(unsigned row, unsigned column) -> float & { return m[row][column]; }
        auto operator()(unsigned row, unsigned column) const -> float { return m[row][column]; }
        operator float *() { return &_11; }
        operator const float *() const { return &_11; }

        auto operator*=(const matrix &other) -> matrix &;
        auto operator+=(const matrix &other) -> matrix &;
        auto operator-=(const matrix &other) -> matrix &;
        auto operator*=(float f) -> matrix &;
        auto operator/=(float f) -> matrix &;
        auto operator+() const -> matrix { return *this; }
        auto operator-() const -> matrix;
        auto operator*(const matrix &other) const -> matrix;
        auto operator+(const matrix &other) const -> matrix;
        auto operator-(const matrix &other) const -> matrix;
        auto operator*(float f) const -> matrix;
        auto operator/(float f) const -> matrix;
        friend auto operator*(float f, const matrix &mat) -> matrix { return mat * f; }
        auto operator==(const matrix &other) const -> bool { return std::memcmp(m, other.m, sizeof(m)) == 0; }
    };

    struct quaternion;

    auto matrix_identity() -> matrix;
    auto matrix_translation(float x, float y, float z) -> matrix;
    auto matrix_scaling(float x, float y, float z) -> matrix;
    auto matrix_rotation_x(float angle) -> matrix;
    auto matrix_rotation_z(float angle) -> matrix;
    auto matrix_rotation_axis(const vector3 &axis, float angle) -> matrix;
    auto matrix_rotation_quaternion(const quaternion &q) -> matrix;
    auto matrix_rotation_yaw_pitch_roll(float yaw, float pitch, float roll) -> matrix;
    auto matrix_look_at_rh(const vector3 &eye, const vector3 &at, const vector3 &up) -> matrix;
    auto matrix_perspective_fov_rh(float fov_y, float aspect, float z_near, float z_far) -> matrix;
    auto matrix_ortho_rh(float width, float height, float z_near, float z_far) -> matrix;
    auto matrix_ortho_off_center_rh(float left, float right, float bottom, float top, float z_near, float z_far) -> matrix;

    auto transpose(const matrix &mat) -> matrix;
    auto determinant(const matrix &mat) -> float;
    // False (and the matrix untouched) when the matrix is singular.
    auto inverse(const matrix &mat, matrix &result, float *determinant_value = nullptr) -> bool;

    auto transform(const vector3 &v, const matrix &mat) -> vector4; // (x, y, z, 1) * M
    auto transform(const vector4 &v, const matrix &mat) -> vector4;
    auto transform_coord(const vector3 &v, const matrix &mat) -> vector3;  // divided by w
    auto transform_normal(const vector3 &v, const matrix &mat) -> vector3; // no translation

    struct viewport_rect {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float min_z = 0.0f;
        float max_z = 1.0f;
    };

    // World position to screen coordinates; any of the matrices may be null.
    auto project(const vector3 &v, const viewport_rect &viewport, const matrix *projection, const matrix *view, const matrix *world) -> vector3;

    // A stack of matrices whose top starts as identity.
    class matrix_stack {
      public:
        matrix_stack();

        void push();
        void pop(); // the last matrix stays
        void load_identity();
        void load(const matrix &mat);
        void multiply(const matrix &mat);       // top = top * mat
        void multiply_local(const matrix &mat); // top = mat * top
        auto top() -> matrix &;

      private:
        std::vector<matrix> stack_;
    };
}
