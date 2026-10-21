#include "format.h"

#include <cstring>

namespace metin2::gpu {

    namespace {
        // clang-format off
        constexpr format_info formats[] = {
            //  format               bpp blk compressed depth  alpha_one has_alpha
            {format::A8R8G8B8,      4,  0,  false,     false, false,    true},
            {format::X8R8G8B8,      4,  0,  false,     false, true,     false},
            {format::R8G8B8,        3,  0,  false,     false, false,    false},
            {format::R5G6B5,        2,  0,  false,     false, false,    false},
            {format::X1R5G5B5,      2,  0,  false,     false, true,     false},
            {format::A1R5G5B5,      2,  0,  false,     false, false,    true},
            {format::A4R4G4B4,      2,  0,  false,     false, false,    true},
            {format::X4R4G4B4,      2,  0,  false,     false, true,     false},
            {format::A8,            1,  0,  false,     false, false,    true},
            {format::DXT1,          0,  8,  true,      false, false,    true},
            {format::DXT2,          0,  16, true,      false, false,    true},
            {format::DXT3,          0,  16, true,      false, false,    true},
            {format::DXT4,          0,  16, true,      false, false,    true},
            {format::DXT5,          0,  16, true,      false, false,    true},
            {format::D16,           2,  0,  false,     true,  false,    false},
            {format::D16_LOCKABLE,  2,  0,  false,     true,  false,    false},
            {format::D15S1,         4,  0,  false,     true,  false,    false},
            {format::D24S8,         4,  0,  false,     true,  false,    false},
            {format::D24X8,         4,  0,  false,     true,  false,    false},
            {format::D24X4S4,       4,  0,  false,     true,  false,    false},
            {format::D32,           4,  0,  false,     true,  false,    false},
        };
        // clang-format on
    }

    auto get_format_info(format fmt) -> const format_info * {
        for (const format_info &info : formats)
            if (info.fmt == fmt)
                return &info;
        return nullptr;
    }

    auto format_pitch(const format_info &info, uint32_t width) -> uint32_t {
        if (info.compressed)
            return ((width + 3) / 4) * info.block_bytes;
        return width * info.bytes_per_pixel;
    }

    auto format_level_size(const format_info &info, uint32_t width, uint32_t height) -> uint32_t {
        if (info.compressed)
            return ((width + 3) / 4) * ((height + 3) / 4) * info.block_bytes;
        return width * height * info.bytes_per_pixel;
    }

    auto format_is_renderable(format fmt) -> bool {
        switch (fmt) {
        case format::A8R8G8B8:
        case format::X8R8G8B8:
        case format::R5G6B5:
        case format::A4R4G4B4:
        case format::A1R5G5B5:
        case format::X1R5G5B5:
        case format::R8G8B8:
            return true;
        default:
            return false;
        }
    }

    void convert_from_rgba8(const format_info &info, const uint8_t *rgba, uint32_t width, uint32_t height, uint8_t *out, uint32_t out_pitch) {
        for (uint32_t y = 0; y < height; ++y) {
            const uint8_t *s = rgba + static_cast<size_t>(y) * width * 4;
            uint8_t *d = out + static_cast<size_t>(y) * out_pitch;
            for (uint32_t x = 0; x < width; ++x, s += 4) {
                const uint32_t r = s[0];
                const uint32_t g = s[1];
                const uint32_t b = s[2];
                const uint32_t a = s[3];
                switch (info.fmt) {
                case format::A8R8G8B8:
                case format::X8R8G8B8:
                    {
                        const uint32_t v = (a << 24) | (r << 16) | (g << 8) | b;
                        memcpy(d, &v, 4);
                        d += 4;
                        break;
                    }
                case format::R8G8B8:
                    d[0] = static_cast<uint8_t>(b);
                    d[1] = static_cast<uint8_t>(g);
                    d[2] = static_cast<uint8_t>(r);
                    d += 3;
                    break;
                case format::R5G6B5:
                    {
                        const auto v = static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
                        memcpy(d, &v, 2);
                        d += 2;
                        break;
                    }
                case format::X1R5G5B5:
                case format::A1R5G5B5:
                    {
                        const auto v = static_cast<uint16_t>(((a >> 7) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
                        memcpy(d, &v, 2);
                        d += 2;
                        break;
                    }
                case format::A4R4G4B4:
                case format::X4R4G4B4:
                    {
                        const auto v = static_cast<uint16_t>(((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4));
                        memcpy(d, &v, 2);
                        d += 2;
                        break;
                    }
                case format::A8:
                    *d++ = static_cast<uint8_t>(a);
                    break;
                default:
                    break;
                }
            }
        }
    }

    void convert_to_rgba8(const format_info &info, const uint8_t *src, uint32_t src_pitch, uint32_t width, uint32_t height, uint8_t *rgba) {
        for (uint32_t y = 0; y < height; ++y) {
            const uint8_t *s = src + static_cast<size_t>(y) * src_pitch;
            uint8_t *d = rgba + static_cast<size_t>(y) * width * 4;
            for (uint32_t x = 0; x < width; ++x, d += 4) {
                uint32_t r = 0;
                uint32_t g = 0;
                uint32_t b = 0;
                uint32_t a = 255;
                switch (info.fmt) {
                case format::A8R8G8B8:
                case format::X8R8G8B8:
                    {
                        uint32_t v;
                        memcpy(&v, s, 4);
                        s += 4;
                        a = info.alpha_one ? 255 : (v >> 24);
                        r = (v >> 16) & 255;
                        g = (v >> 8) & 255;
                        b = v & 255;
                        break;
                    }
                case format::R8G8B8:
                    b = s[0];
                    g = s[1];
                    r = s[2];
                    s += 3;
                    break;
                case format::R5G6B5:
                    {
                        uint16_t v;
                        memcpy(&v, s, 2);
                        s += 2;
                        r = ((v >> 11) & 31) * 255 / 31;
                        g = ((v >> 5) & 63) * 255 / 63;
                        b = (v & 31) * 255 / 31;
                        break;
                    }
                case format::X1R5G5B5:
                case format::A1R5G5B5:
                    {
                        uint16_t v;
                        memcpy(&v, s, 2);
                        s += 2;
                        a = info.alpha_one ? 255 : ((v >> 15) ? 255 : 0);
                        r = ((v >> 10) & 31) * 255 / 31;
                        g = ((v >> 5) & 31) * 255 / 31;
                        b = (v & 31) * 255 / 31;
                        break;
                    }
                case format::A4R4G4B4:
                case format::X4R4G4B4:
                    {
                        uint16_t v;
                        memcpy(&v, s, 2);
                        s += 2;
                        a = info.alpha_one ? 255 : ((v >> 12) & 15) * 17;
                        r = ((v >> 8) & 15) * 17;
                        g = ((v >> 4) & 15) * 17;
                        b = (v & 15) * 17;
                        break;
                    }
                case format::A8:
                    a = *s++;
                    r = g = b = 255;
                    break;
                default:
                    break;
                }
                d[0] = static_cast<uint8_t>(r);
                d[1] = static_cast<uint8_t>(g);
                d[2] = static_cast<uint8_t>(b);
                d[3] = static_cast<uint8_t>(a);
            }
        }
    }
}
