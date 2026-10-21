#pragma once

#include <cstdint>

namespace metin2::gpu {

    constexpr auto four_cc(char a, char b, char c, char d) -> uint32_t {
        return static_cast<uint32_t>(static_cast<uint8_t>(a)) | (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8) |
               (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) | (static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
    }

    // Pixel, depth and index formats. Colour channels are named from the most significant bit:
    // A8R8G8B8 is 0xAARRGGBB, stored B, G, R, A in memory.
    enum class format : uint32_t {
        UNKNOWN = 0,
        R8G8B8 = 20,
        A8R8G8B8 = 21,
        X8R8G8B8 = 22,
        R5G6B5 = 23,
        X1R5G5B5 = 24,
        A1R5G5B5 = 25,
        A4R4G4B4 = 26,
        A8 = 28,
        X4R4G4B4 = 30,
        DXT1 = four_cc('D', 'X', 'T', '1'),
        DXT2 = four_cc('D', 'X', 'T', '2'),
        DXT3 = four_cc('D', 'X', 'T', '3'),
        DXT4 = four_cc('D', 'X', 'T', '4'),
        DXT5 = four_cc('D', 'X', 'T', '5'),
        D16_LOCKABLE = 70,
        D32 = 71,
        D15S1 = 73,
        D24S8 = 75,
        D24X8 = 77,
        D24X4S4 = 79,
        D16 = 80,
        INDEX16 = 101,
        INDEX32 = 102,
    };

    struct format_info {
        format fmt;
        uint32_t bytes_per_pixel; // 0 for block-compressed formats
        uint32_t block_bytes;     // bytes per 4x4 block of a compressed format
        bool compressed;
        bool depth;
        bool alpha_one; // the X formats: alpha reads as 1
        bool has_alpha;
    };

    // nullptr for formats no backend supports.
    auto get_format_info(format fmt) -> const format_info *;
    auto format_pitch(const format_info &info, uint32_t width) -> uint32_t;
    auto format_level_size(const format_info &info, uint32_t width, uint32_t height) -> uint32_t;
    auto format_is_renderable(format fmt) -> bool;

    // Between RGBA bytes (R, G, B, A in memory) and an uncompressed colour format.
    void convert_from_rgba8(const format_info &info, const uint8_t *rgba, uint32_t width, uint32_t height, uint8_t *out, uint32_t out_pitch);
    void convert_to_rgba8(const format_info &info, const uint8_t *src, uint32_t src_pitch, uint32_t width, uint32_t height, uint8_t *rgba);

    struct usage {
        enum value : uint32_t {
            RENDER_TARGET = 0x001,
            DEPTH_STENCIL = 0x002,
            WRITE_ONLY = 0x008,
            DYNAMIC = 0x200,
        };
    };

    struct lock {
        enum value : uint32_t {
            READ_ONLY = 0x0010,
            NO_SYSLOCK = 0x0800,
            DISCARD = 0x2000,
            NO_DIRTY_UPDATE = 0x8000,
        };
    };

    struct clear {
        enum value : uint32_t {
            TARGET = 1,
            Z_BUFFER = 2,
            STENCIL = 4,
        };
    };
}
