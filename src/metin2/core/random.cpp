#include "random.h"

#include <sys/random.h>
#include <utility>

namespace metin2::random {
    auto engine() -> std::mt19937 & {
        static std::mt19937 instance{std::random_device{}()};
        return instance;
    }

    auto number(int from, int to) -> int {
        if (from > to)
            std::swap(from, to);
        return std::uniform_int_distribution<int>(from, to)(engine());
    }

    auto fnumber(float from, float to) -> float {
        if (from == to)
            return from;
        return std::uniform_real_distribution<float>(std::min(from, to), std::max(from, to))(engine());
    }

    auto fnumber(double from, double to) -> double {
        if (from == to)
            return from;
        return std::uniform_real_distribution<double>(std::min(from, to), std::max(from, to))(engine());
    }

    auto prob(int percent) -> bool {
        return number(1, 100) <= percent;
    }

    auto secure_u32() -> uint32_t {
        uint32_t value = 0;
        if (getrandom(&value, sizeof(value), 0) != sizeof(value))
            value = static_cast<uint32_t>(std::random_device{}());
        return value;
    }
}
