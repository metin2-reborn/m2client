#pragma once

#include <cstdint>

namespace metin2::inline core {

    // Edges in pixels; right and bottom are exclusive.
    struct rectangle {
        int32_t left = 0;
        int32_t top = 0;
        int32_t right = 0;
        int32_t bottom = 0;
    };
}
