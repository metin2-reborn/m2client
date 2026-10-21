#pragma once

namespace metin2::math {
    struct vector2 {
        float x = 0.0f;
        float y = 0.0f;

        constexpr vector2() = default;
        constexpr vector2(float fx, float fy) : x(fx), y(fy) {}

        explicit vector2(const float *values) : x(values[0]), y(values[1]) {}

        operator float *() { return &x; }

        operator const float *() const { return &x; }

        auto operator+=(const vector2 &v) -> vector2 & {
            x += v.x;
            y += v.y;
            return *this;
        }

        auto operator-=(const vector2 &v) -> vector2 & {
            x -= v.x;
            y -= v.y;
            return *this;
        }

        auto operator*=(float f) -> vector2 & {
            x *= f;
            y *= f;
            return *this;
        }

        auto operator/=(float f) -> vector2 & {
            x /= f;
            y /= f;
            return *this;
        }

        auto operator+() const -> vector2 { return *this; }
        auto operator-() const -> vector2 { return {-x, -y}; }
        auto operator+(const vector2 &v) const -> vector2 { return {x + v.x, y + v.y}; }
        auto operator-(const vector2 &v) const -> vector2 { return {x - v.x, y - v.y}; }
        auto operator*(float f) const -> vector2 { return {x * f, y * f}; }
        auto operator/(float f) const -> vector2 { return {x / f, y / f}; }

        friend auto operator*(float f, const vector2 &v) -> vector2 { return {f * v.x, f * v.y}; }

        auto operator==(const vector2 &v) const -> bool = default;
    };

    struct vector3 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        constexpr vector3() = default;
        constexpr vector3(float fx, float fy, float fz) : x(fx), y(fy), z(fz) {}

        explicit vector3(const float *values) : x(values[0]), y(values[1]), z(values[2]) {}

        operator float *() { return &x; }
        operator const float *() const { return &x; }

        auto operator+=(const vector3 &v) -> vector3 & {
            x += v.x;
            y += v.y;
            z += v.z;
            return *this;
        }

        auto operator-=(const vector3 &v) -> vector3 & {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            return *this;
        }

        auto operator*=(float f) -> vector3 & {
            x *= f;
            y *= f;
            z *= f;
            return *this;
        }

        auto operator/=(float f) -> vector3 & {
            const float inv = 1.0f / f;
            x *= inv;
            y *= inv;
            z *= inv;
            return *this;
        }

        auto operator+() const -> vector3 { return *this; }
        auto operator-() const -> vector3 { return {-x, -y, -z}; }
        auto operator+(const vector3 &v) const -> vector3 { return {x + v.x, y + v.y, z + v.z}; }
        auto operator-(const vector3 &v) const -> vector3 { return {x - v.x, y - v.y, z - v.z}; }
        auto operator*(float f) const -> vector3 { return {x * f, y * f, z * f}; }
        auto operator/(float f) const -> vector3 {
            const float inv = 1.0f / f;
            return {x * inv, y * inv, z * inv};
        }

        friend auto operator*(float f, const vector3 &v) -> vector3 { return {f * v.x, f * v.y, f * v.z}; }

        auto operator==(const vector3 &v) const -> bool = default;
    };

    struct vector4 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;

        constexpr vector4() = default;
        constexpr vector4(float fx, float fy, float fz, float fw) : x(fx), y(fy), z(fz), w(fw) {}

        explicit vector4(const float *values) : x(values[0]), y(values[1]), z(values[2]), w(values[3]) {}

        operator float *() { return &x; }
        operator const float *() const { return &x; }

        auto operator+=(const vector4 &v) -> vector4 & {
            x += v.x;
            y += v.y;
            z += v.z;
            w += v.w;
            return *this;
        }

        auto operator-=(const vector4 &v) -> vector4 & {
            x -= v.x;
            y -= v.y;
            z -= v.z;
            w -= v.w;
            return *this;
        }

        auto operator*=(float f) -> vector4 & {
            x *= f;
            y *= f;
            z *= f;
            w *= f;
            return *this;
        }

        auto operator/=(float f) -> vector4 & {
            const float inv = 1.0f / f;
            x *= inv;
            y *= inv;
            z *= inv;
            w *= inv;
            return *this;
        }

        auto operator+() const -> vector4 { return *this; }
        auto operator-() const -> vector4 { return {-x, -y, -z, -w}; }
        auto operator+(const vector4 &v) const -> vector4 { return {x + v.x, y + v.y, z + v.z, w + v.w}; }
        auto operator-(const vector4 &v) const -> vector4 { return {x - v.x, y - v.y, z - v.z, w - v.w}; }
        auto operator*(float f) const -> vector4 { return {x * f, y * f, z * f, w * f}; }
        auto operator/(float f) const -> vector4 {
            const float inv = 1.0f / f;
            return {x * inv, y * inv, z * inv, w * inv};
        }

        friend auto operator*(float f, const vector4 &v) -> vector4 { return {f * v.x, f * v.y, f * v.z, f * v.w}; }

        auto operator==(const vector4 &v) const -> bool = default;
    };

    auto length(const vector2 &v) -> float;
    auto dot(const vector2 &a, const vector2 &b) -> float;
    auto ccw(const vector2 &a, const vector2 &b) -> float;
    auto normalize(const vector2 &v) -> vector2;
    auto length(const vector3 &v) -> float;
    auto length_sq(const vector3 &v) -> float;
    auto dot(const vector3 &a, const vector3 &b) -> float;
    auto cross(const vector3 &a, const vector3 &b) -> vector3;
    auto normalize(const vector3 &v) -> vector3;
    auto lerp(const vector3 &a, const vector3 &b, float s) -> vector3;
}
