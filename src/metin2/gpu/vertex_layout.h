#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace metin2::gpu {

    // A vertex input described element by element, for vertices that span several streams.
    enum class vertex_usage : uint8_t {
        POSITION,
        NORMAL,
        DIFFUSE,
        SPECULAR,
        TEXCOORD0,
        TEXCOORD1,
        TEXCOORD2,
    };

    enum class vertex_type : uint8_t {
        FLOAT1,
        FLOAT2,
        FLOAT3,
        FLOAT4,
        COLOR, // 0xAARRGGBB, normalized to 0..1
        UBYTE4,
        SHORT2,
        SHORT4,
    };

    struct vertex_element {
        uint8_t stream;
        vertex_usage usage;
        vertex_type type;
    };

    // Where each attribute of the fixed-function shader comes from. Slots: 0 position, 1 normal,
    // 2 diffuse, 3 specular, 4-6 texture coordinates 0-2.
    struct layout_attribute {
        enum class kind : uint8_t {
            FLOAT,
            COLOR, // 4 bytes B, G, R, A, normalized
            UBYTE,
            SHORT,
        };

        int slot;
        int stream;
        int offset;
        kind data;
        int components;
    };

    struct vertex_layout {
        std::vector<layout_attribute> attributes;
        uint32_t stream_stride[4] = {0, 0, 0, 0};
        bool pre_transformed = false; // positions are screen x, y, z and 1/w
        bool has_diffuse = false;
        bool has_specular = false;
        bool has_normal = false;
        int texcoord_dimension[3] = {0, 0, 0};
    };

    // From a vertex format (gpu::fvf bits); false for formats without a position.
    auto layout_from_fvf(uint32_t format, vertex_layout &layout) -> bool;
    auto layout_from_elements(std::span<const vertex_element> elements, vertex_layout &layout) -> bool;
}
