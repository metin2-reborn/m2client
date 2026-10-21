#include "types.h"

namespace metin2::gpu {

    auto vertex_size(uint32_t format) -> uint32_t {
        uint32_t size = 0;
        switch (format & fvf::POSITION_MASK) {
        case 0x002:
            size += 12;
            break;
        case 0x004:
        case 0x006:
            size += 16;
            break;
        case 0x008:
            size += 20;
            break;
        case 0x00a:
            size += 24;
            break;
        case 0x00c:
            size += 28;
            break;
        case 0x00e:
            size += 32;
            break;
        default:
            break;
        }
        if (format & fvf::NORMAL)
            size += 12;
        if (format & fvf::PSIZE)
            size += 4;
        if (format & fvf::DIFFUSE)
            size += 4;
        if (format & fvf::SPECULAR)
            size += 4;
        // Each texture coordinate set has two format bits from bit 16: 0 is 2 floats, 1 is 3, 2 is 4,
        // 3 is 1.
        const uint32_t texture_count = (format & fvf::TEXCOUNT_MASK) >> TEXCOUNT_SHIFT;
        static constexpr uint32_t floats[4] = {2, 3, 4, 1};
        for (uint32_t i = 0; i < texture_count; ++i)
            size += 4 * floats[(format >> (i * 2 + 16)) & 3];
        return size;
    }
}
