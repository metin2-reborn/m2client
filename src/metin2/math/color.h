#pragma once

#include <cstdint>

namespace metin2::math {

    // Floating point RGBA; converts to and from 0xAARRGGBB.
    struct color {
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        float a = 0.0f;

        constexpr color() = default;
        constexpr color(float fr, float fg, float fb, float fa) : r(fr), g(fg), b(fb), a(fa) {}
        color(uint32_t argb);
        explicit color(const float *values) : r(values[0]), g(values[1]), b(values[2]), a(values[3]) {}

        operator uint32_t() const;
        operator float *() { return &r; }
        operator const float *() const { return &r; }

        auto operator+=(const color &c) -> color & {
            r += c.r;
            g += c.g;
            b += c.b;
            a += c.a;
            return *this;
        }
        auto operator-=(const color &c) -> color & {
            r -= c.r;
            g -= c.g;
            b -= c.b;
            a -= c.a;
            return *this;
        }
        auto operator*=(float f) -> color & {
            r *= f;
            g *= f;
            b *= f;
            a *= f;
            return *this;
        }
        auto operator/=(float f) -> color & {
            const float inv = 1.0f / f;
            r *= inv;
            g *= inv;
            b *= inv;
            a *= inv;
            return *this;
        }
        auto operator+() const -> color { return *this; }
        auto operator-() const -> color { return {-r, -g, -b, -a}; }
        auto operator+(const color &c) const -> color { return {r + c.r, g + c.g, b + c.b, a + c.a}; }
        auto operator-(const color &c) const -> color { return {r - c.r, g - c.g, b - c.b, a - c.a}; }
        auto operator*(float f) const -> color { return {r * f, g * f, b * f, a * f}; }
        auto operator/(float f) const -> color {
            const float inv = 1.0f / f;
            return {r * inv, g * inv, b * inv, a * inv};
        }
        friend auto operator*(float f, const color &c) -> color { return c * f; }
        auto operator==(const color &c) const -> bool = default;
    };

    // Component-wise product.
    auto color_modulate(const color &a, const color &b) -> color;
}
