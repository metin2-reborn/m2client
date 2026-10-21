#pragma once

#include <cstdint>
#include <random>

// One random engine for the whole client, seeded once from std::random_device.
namespace metin2::random {
    auto engine() -> std::mt19937 &;

    // Uniform on [from, to]; the bounds may come in either order.
    auto number(int from, int to) -> int;
    // Uniform on [from, to).
    auto fnumber(float from, float to) -> float;
    auto fnumber(double from, double to) -> double;
    // True with the given chance in percent.
    auto prob(int percent) -> bool;

    // From the operating system's generator, for keys.
    auto secure_u32() -> uint32_t;
}
