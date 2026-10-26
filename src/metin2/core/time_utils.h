#pragma once

#include <ctime>

namespace metin2::inline core {

    // The local calendar time of t; localtime_r and localtime_s differ in name and argument order.
    inline auto local_time(std::time_t t) -> std::tm {
        std::tm result{};
#ifdef _WIN32
        localtime_s(&result, &t);
#else
        localtime_r(&t, &result);
#endif
        return result;
    }
}
