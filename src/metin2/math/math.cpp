#include "math.h"

#include <cmath>

namespace metin2::math {
    auto length(const vector2 &v) -> float {
        return sqrtf(v.x * v.x + v.y * v.y);
    }

    auto dot(const vector2 &a, const vector2 &b) -> float {
        return a.x * b.x + a.y * b.y;
    }

    auto ccw(const vector2 &a, const vector2 &b) -> float {
        return a.x * b.y - a.y * b.x;
    }

    auto normalize(const vector2 &v) -> vector2 {
        const auto len = length(v);
        if (len > 0.0f) {
            return {
                v.x / len,
                v.y / len,
            };
        }

        return v;
    }

    auto length(const vector3 &v) -> float {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    auto length_sq(const vector3 &v) -> float {
        return v.x * v.x + v.y * v.y + v.z * v.z;
    }

    auto dot(const vector3 &a, const vector3 &b) -> float {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    auto cross(const vector3 &a, const vector3 &b) -> vector3 {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

    auto normalize(const vector3 &v) -> vector3 {
        const auto len = length(v);
        if (len > 0.0f) {
            const float inv = 1.0f / len;
            return {
                v.x * inv,
                v.y * inv,
                v.z * inv,
            };
        }

        return v;
    }

    auto lerp(const vector3 &a, const vector3 &b, float s) -> vector3 {
        return {a.x + s * (b.x - a.x), a.y + s * (b.y - a.y), a.z + s * (b.z - a.z)};
    }

    auto matrix::operator*=(const matrix &other) -> matrix & {
        *this = *this * other;
        return *this;
    }

    auto matrix::operator+=(const matrix &other) -> matrix & {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                m[i][j] += other.m[i][j];
            }
        }

        return *this;
    }

    auto matrix::operator-=(const matrix &other) -> matrix & {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                m[i][j] -= other.m[i][j];
            }
        }

        return *this;
    }

    auto matrix::operator*=(float f) -> matrix & {
        for (auto &row : m) {
            for (float &value : row) {
                value *= f;
            }
        }

        return *this;
    }

    auto matrix::operator/=(float f) -> matrix & {
        return *this *= 1.0f / f;
    }

    auto matrix::operator-() const -> matrix {
        matrix r = *this;
        r *= -1.0f;
        return r;
    }

    auto matrix::operator*(const matrix &other) const -> matrix {
        matrix r;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                r.m[i][j] = m[i][0] * other.m[0][j] + m[i][1] * other.m[1][j] + m[i][2] * other.m[2][j] + m[i][3] * other.m[3][j];
            }
        }

        return r;
    }

    auto matrix::operator+(const matrix &other) const -> matrix {
        matrix r = *this;
        r += other;
        return r;
    }

    auto matrix::operator-(const matrix &other) const -> matrix {
        matrix r = *this;
        r -= other;
        return r;
    }

    auto matrix::operator*(float f) const -> matrix {
        matrix r = *this;
        r *= f;
        return r;
    }

    auto matrix::operator/(float f) const -> matrix {
        matrix r = *this;
        r /= f;
        return r;
    }

    auto matrix_identity() -> matrix {
        matrix r;

        r._11 = r._22 = r._33 = r._44 = 1.0f;

        return r;
    }

    auto matrix_translation(float x, float y, float z) -> matrix {
        matrix r = matrix_identity();

        r._41 = x;
        r._42 = y;
        r._43 = z;

        return r;
    }

    auto matrix_scaling(float x, float y, float z) -> matrix {
        matrix r = matrix_identity();

        r._11 = x;
        r._22 = y;
        r._33 = z;

        return r;
    }

    auto matrix_rotation_x(float angle) -> matrix {
        const auto s = sinf(angle);
        const auto c = cosf(angle);

        matrix r = matrix_identity();

        r._22 = c;
        r._23 = s;
        r._32 = -s;
        r._33 = c;

        return r;
    }

    auto matrix_rotation_z(float angle) -> matrix {
        const float s = sinf(angle);
        const float c = cosf(angle);

        matrix r = matrix_identity();

        r._11 = c;
        r._12 = s;
        r._21 = -s;
        r._22 = c;

        return r;
    }

    auto matrix_rotation_axis(const vector3 &axis, float angle) -> matrix {
        return matrix_rotation_quaternion(quaternion_rotation_axis(normalize(axis), angle));
    }

    auto matrix_rotation_quaternion(const quaternion &q) -> matrix {
        const float x = q.x;
        const float y = q.y;
        const float z = q.z;
        const float w = q.w;

        matrix r = matrix_identity();

        r._11 = 1.0f - 2.0f * (y * y + z * z);
        r._12 = 2.0f * (x * y + z * w);
        r._13 = 2.0f * (x * z - y * w);
        r._21 = 2.0f * (x * y - z * w);
        r._22 = 1.0f - 2.0f * (x * x + z * z);
        r._23 = 2.0f * (y * z + x * w);
        r._31 = 2.0f * (x * z + y * w);
        r._32 = 2.0f * (y * z - x * w);
        r._33 = 1.0f - 2.0f * (x * x + y * y);

        return r;
    }

    auto matrix_rotation_yaw_pitch_roll(float yaw, float pitch, float roll) -> matrix {
        return matrix_rotation_quaternion(quaternion_rotation_yaw_pitch_roll(yaw, pitch, roll));
    }

    auto matrix_look_at_rh(const vector3 &eye, const vector3 &at, const vector3 &up) -> matrix {
        const auto z_axis = normalize(eye - at);
        const auto x_axis = normalize(cross(up, z_axis));
        const auto y_axis = cross(z_axis, x_axis);

        matrix r = matrix_identity();

        r._11 = x_axis.x;
        r._12 = y_axis.x;
        r._13 = z_axis.x;
        r._21 = x_axis.y;
        r._22 = y_axis.y;
        r._23 = z_axis.y;
        r._31 = x_axis.z;
        r._32 = y_axis.z;
        r._33 = z_axis.z;
        r._41 = -dot(x_axis, eye);
        r._42 = -dot(y_axis, eye);
        r._43 = -dot(z_axis, eye);

        return r;
    }

    auto matrix_perspective_fov_rh(float fov_y, float aspect, float z_near, float z_far) -> matrix {
        const auto y_scale = 1.0f / tanf(fov_y * 0.5f);

        matrix r;

        r._11 = y_scale / aspect;
        r._22 = y_scale;
        r._33 = z_far / (z_near - z_far);
        r._34 = -1.0f;
        r._43 = z_near * z_far / (z_near - z_far);

        return r;
    }

    auto matrix_ortho_rh(float width, float height, float z_near, float z_far) -> matrix {
        matrix r = matrix_identity();

        r._11 = 2.0f / width;
        r._22 = 2.0f / height;
        r._33 = 1.0f / (z_near - z_far);
        r._43 = z_near / (z_near - z_far);

        return r;
    }

    auto matrix_ortho_off_center_rh(float left, float right, float bottom, float top, float z_near, float z_far) -> matrix {
        matrix r = matrix_identity();

        r._11 = 2.0f / (right - left);
        r._22 = 2.0f / (top - bottom);
        r._33 = 1.0f / (z_near - z_far);
        r._41 = (left + right) / (left - right);
        r._42 = (top + bottom) / (bottom - top);
        r._43 = z_near / (z_near - z_far);

        return r;
    }

    auto transpose(const matrix &mat) -> matrix {
        matrix r;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                r.m[i][j] = mat.m[j][i];
            }
        }

        return r;
    }

    namespace {
        struct minors {
            float s[6];
            float c[6];

            explicit minors(const matrix &mat) {
                const float *a = &mat._11;
                s[0] = a[0] * a[5] - a[4] * a[1];
                s[1] = a[0] * a[6] - a[4] * a[2];
                s[2] = a[0] * a[7] - a[4] * a[3];
                s[3] = a[1] * a[6] - a[5] * a[2];
                s[4] = a[1] * a[7] - a[5] * a[3];
                s[5] = a[2] * a[7] - a[6] * a[3];
                c[5] = a[10] * a[15] - a[14] * a[11];
                c[4] = a[9] * a[15] - a[13] * a[11];
                c[3] = a[9] * a[14] - a[13] * a[10];
                c[2] = a[8] * a[15] - a[12] * a[11];
                c[1] = a[8] * a[14] - a[12] * a[10];
                c[0] = a[8] * a[13] - a[12] * a[9];
            }

            auto determinant() const -> float {
                return s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] - s[4] * c[1] + s[5] * c[0];
            }
        };
    }

    auto determinant(const matrix &mat) -> float {
        return minors(mat).determinant();
    }

    auto inverse(const matrix &mat, matrix &result, float *determinant_value) -> bool {
        const minors mn(mat);
        const float *s = mn.s;
        const float *c = mn.c;
        const float det = mn.determinant();

        if (determinant_value) {
            *determinant_value = det;
        }

        if (det == 0.0f) {
            return false;
        }

        const float inv = 1.0f / det;
        const float *a = &mat._11;

        matrix r;

        float *b = &r._11;
        b[0] = (a[5] * c[5] - a[6] * c[4] + a[7] * c[3]) * inv;
        b[1] = (-a[1] * c[5] + a[2] * c[4] - a[3] * c[3]) * inv;
        b[2] = (a[13] * s[5] - a[14] * s[4] + a[15] * s[3]) * inv;
        b[3] = (-a[9] * s[5] + a[10] * s[4] - a[11] * s[3]) * inv;
        b[4] = (-a[4] * c[5] + a[6] * c[2] - a[7] * c[1]) * inv;
        b[5] = (a[0] * c[5] - a[2] * c[2] + a[3] * c[1]) * inv;
        b[6] = (-a[12] * s[5] + a[14] * s[2] - a[15] * s[1]) * inv;
        b[7] = (a[8] * s[5] - a[10] * s[2] + a[11] * s[1]) * inv;
        b[8] = (a[4] * c[4] - a[5] * c[2] + a[7] * c[0]) * inv;
        b[9] = (-a[0] * c[4] + a[1] * c[2] - a[3] * c[0]) * inv;
        b[10] = (a[12] * s[4] - a[13] * s[2] + a[15] * s[0]) * inv;
        b[11] = (-a[8] * s[4] + a[9] * s[2] - a[11] * s[0]) * inv;
        b[12] = (-a[4] * c[3] + a[5] * c[1] - a[6] * c[0]) * inv;
        b[13] = (a[0] * c[3] - a[1] * c[1] + a[2] * c[0]) * inv;
        b[14] = (-a[12] * s[3] + a[13] * s[1] - a[14] * s[0]) * inv;
        b[15] = (a[8] * s[3] - a[9] * s[1] + a[10] * s[0]) * inv;

        result = r;

        return true;
    }

    auto transform(const vector3 &v, const matrix &mat) -> vector4 {
        return {
            v.x * mat._11 + v.y * mat._21 + v.z * mat._31 + mat._41,
            v.x * mat._12 + v.y * mat._22 + v.z * mat._32 + mat._42,
            v.x * mat._13 + v.y * mat._23 + v.z * mat._33 + mat._43,
            v.x * mat._14 + v.y * mat._24 + v.z * mat._34 + mat._44,
        };
    }

    auto transform(const vector4 &v, const matrix &mat) -> vector4 {
        return {
            v.x * mat._11 + v.y * mat._21 + v.z * mat._31 + v.w * mat._41,
            v.x * mat._12 + v.y * mat._22 + v.z * mat._32 + v.w * mat._42,
            v.x * mat._13 + v.y * mat._23 + v.z * mat._33 + v.w * mat._43,
            v.x * mat._14 + v.y * mat._24 + v.z * mat._34 + v.w * mat._44,
        };
    }

    auto transform_coord(const vector3 &v, const matrix &mat) -> vector3 {
        const auto r = transform(v, mat);
        const auto inv = r.w != 0.0f ? 1.0f / r.w : 1.0f;

        return {r.x * inv, r.y * inv, r.z * inv};
    }

    auto transform_normal(const vector3 &v, const matrix &mat) -> vector3 {
        return {
            v.x * mat._11 + v.y * mat._21 + v.z * mat._31,
            v.x * mat._12 + v.y * mat._22 + v.z * mat._32,
            v.x * mat._13 + v.y * mat._23 + v.z * mat._33,
        };
    }

    auto project(const vector3 &v, const viewport_rect &viewport, const matrix *projection, const matrix *view, const matrix *world) -> vector3 {
        matrix mat = matrix_identity();

        if (world) mat = *world;
        if (view) mat = mat * *view;
        if (projection) mat = mat * *projection;

        const vector3 p = transform_coord(v, mat);
        return {
            viewport.x + (1.0f + p.x) * viewport.width * 0.5f,
            viewport.y + (1.0f - p.y) * viewport.height * 0.5f,
            viewport.min_z + p.z * (viewport.max_z - viewport.min_z),
        };
    }

    matrix_stack::matrix_stack() {
        stack_.push_back(matrix_identity());
    }

    void matrix_stack::push() {
        stack_.push_back(stack_.back());
    }

    void matrix_stack::pop() {
        if (stack_.size() > 1) {
            stack_.pop_back();
        }
    }

    void matrix_stack::load_identity() {
        stack_.back() = matrix_identity();
    }

    void matrix_stack::load(const matrix &mat) {
        stack_.back() = mat;
    }

    void matrix_stack::multiply(const matrix &mat) {
        stack_.back() = stack_.back() * mat;
    }

    void matrix_stack::multiply_local(const matrix &mat) {
        stack_.back() = mat * stack_.back();
    }

    auto matrix_stack::top() -> matrix & {
        return stack_.back();
    }

    auto quaternion::operator*=(const quaternion &q) -> quaternion & {
        *this = *this * q;
        return *this;
    }

    auto quaternion::operator*(const quaternion &b) const -> quaternion {
        const quaternion &a = *this;

        return {
            b.w * a.x + b.x * a.w + b.y * a.z - b.z * a.y,
            b.w * a.y - b.x * a.z + b.y * a.w + b.z * a.x,
            b.w * a.z + b.x * a.y - b.y * a.x + b.z * a.w,
            b.w * a.w - b.x * a.x - b.y * a.y - b.z * a.z};
    }

    auto conjugate(const quaternion &q) -> quaternion {
        return {-q.x, -q.y, -q.z, q.w};
    }

    auto quaternion_rotation_axis(const vector3 &axis, float angle) -> quaternion {
        const vector3 n = normalize(axis);
        const float s = sinf(angle * 0.5f);

        return {
            n.x * s,
            n.y * s,
            n.z * s,
            cosf(angle * 0.5f),
        };
    }

    auto quaternion_rotation_yaw_pitch_roll(float yaw, float pitch, float roll) -> quaternion {
        const float sy = sinf(yaw * 0.5f);
        const float cy = cosf(yaw * 0.5f);
        const float sp = sinf(pitch * 0.5f);
        const float cp = cosf(pitch * 0.5f);
        const float sr = sinf(roll * 0.5f);
        const float cr = cosf(roll * 0.5f);

        return {cy * sp * cr + sy * cp * sr, sy * cp * cr - cy * sp * sr, cy * cp * sr - sy * sp * cr, cy * cp * cr + sy * sp * sr};
    }

    auto plane_dot_coord(const plane &p, const vector3 &v) -> float {
        return p.a * v.x + p.b * v.y + p.c * v.z + p.d;
    }

    auto plane_normalize(const plane &p) -> plane {
        const auto len = sqrtf(p.a * p.a + p.b * p.b + p.c * p.c);
        if (len > 0.0f) {
            const float inv = 1.0f / len;
            return {p.a * inv, p.b * inv, p.c * inv, p.d * inv};
        }

        return p;
    }

    color::color(uint32_t argb) {
        const float f = 1.0f / 255.0f;

        r = f * static_cast<float>(static_cast<uint8_t>(argb >> 16));
        g = f * static_cast<float>(static_cast<uint8_t>(argb >> 8));
        b = f * static_cast<float>(static_cast<uint8_t>(argb));
        a = f * static_cast<float>(static_cast<uint8_t>(argb >> 24));
    }

    color::operator uint32_t() const {
        auto channel = [](float value) -> uint32_t {
            return value >= 1.0f ? 0xff : value <= 0.0f ? 0x00
                                                        : static_cast<uint32_t>(value * 255.0f + 0.5f);
        };

        return (channel(a) << 24) | (channel(r) << 16) | (channel(g) << 8) | channel(b);
    }

    auto color_modulate(const color &a, const color &b) -> color {
        return {a.r * b.r, a.g * b.g, a.b * b.b, a.a * b.a};
    }
}
