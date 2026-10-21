#include "vertex_layout.h"
#include "types.h"

namespace metin2::gpu {

    auto layout_from_fvf(uint32_t format, vertex_layout &layout) -> bool {
        layout = vertex_layout();
        int offset = 0;
        switch (format & fvf::POSITION_MASK) {
        case 0x002:
            layout.attributes.push_back({0, 0, 0, layout_attribute::kind::FLOAT, 3});
            offset = 12;
            break;
        case 0x004:
            layout.attributes.push_back({0, 0, 0, layout_attribute::kind::FLOAT, 4});
            offset = 16;
            layout.pre_transformed = true;
            break;
        case 0x006:
        case 0x008:
        case 0x00a:
        case 0x00c:
        case 0x00e:
            // positions followed by 1 to 5 blend weights, which the fixed-function pipeline ignores
            layout.attributes.push_back({0, 0, 0, layout_attribute::kind::FLOAT, 3});
            offset = 12 + 4 * static_cast<int>(((format & fvf::POSITION_MASK) - 0x004) / 2);
            break;
        default:
            return false;
        }
        if (format & fvf::NORMAL) {
            layout.attributes.push_back({1, 0, offset, layout_attribute::kind::FLOAT, 3});
            offset += 12;
            layout.has_normal = true;
        }
        if (format & fvf::PSIZE)
            offset += 4;
        if (format & fvf::DIFFUSE) {
            layout.attributes.push_back({2, 0, offset, layout_attribute::kind::COLOR, 4});
            offset += 4;
            layout.has_diffuse = true;
        }
        if (format & fvf::SPECULAR) {
            layout.attributes.push_back({3, 0, offset, layout_attribute::kind::COLOR, 4});
            offset += 4;
            layout.has_specular = true;
        }
        const uint32_t texture_count = (format & fvf::TEXCOUNT_MASK) >> TEXCOUNT_SHIFT;
        static constexpr int dimensions[4] = {2, 3, 4, 1};
        for (uint32_t i = 0; i < texture_count && i < 3; ++i) {
            const int dimension = dimensions[(format >> (16 + i * 2)) & 3];
            layout.attributes.push_back({4 + static_cast<int>(i), 0, offset, layout_attribute::kind::FLOAT, dimension});
            layout.texcoord_dimension[i] = dimension;
            offset += dimension * 4;
        }
        layout.stream_stride[0] = static_cast<uint32_t>(offset);
        return true;
    }

    auto layout_from_elements(std::span<const vertex_element> elements, vertex_layout &layout) -> bool {
        layout = vertex_layout();
        int offsets[4] = {0, 0, 0, 0};
        for (const vertex_element &element : elements) {
            if (element.stream > 3)
                return false;
            layout_attribute::kind data = layout_attribute::kind::FLOAT;
            int components = 0;
            int bytes = 0;
            switch (element.type) {
            case vertex_type::FLOAT1:
                components = 1;
                bytes = 4;
                break;
            case vertex_type::FLOAT2:
                components = 2;
                bytes = 8;
                break;
            case vertex_type::FLOAT3:
                components = 3;
                bytes = 12;
                break;
            case vertex_type::FLOAT4:
                components = 4;
                bytes = 16;
                break;
            case vertex_type::COLOR:
                data = layout_attribute::kind::COLOR;
                components = 4;
                bytes = 4;
                break;
            case vertex_type::UBYTE4:
                data = layout_attribute::kind::UBYTE;
                components = 4;
                bytes = 4;
                break;
            case vertex_type::SHORT2:
                data = layout_attribute::kind::SHORT;
                components = 2;
                bytes = 4;
                break;
            case vertex_type::SHORT4:
                data = layout_attribute::kind::SHORT;
                components = 4;
                bytes = 8;
                break;
            }
            int slot = 0;
            switch (element.usage) {
            case vertex_usage::POSITION:
                slot = 0;
                layout.pre_transformed = components == 4;
                break;
            case vertex_usage::NORMAL:
                slot = 1;
                layout.has_normal = true;
                break;
            case vertex_usage::DIFFUSE:
                slot = 2;
                layout.has_diffuse = true;
                break;
            case vertex_usage::SPECULAR:
                slot = 3;
                layout.has_specular = true;
                break;
            case vertex_usage::TEXCOORD0:
            case vertex_usage::TEXCOORD1:
            case vertex_usage::TEXCOORD2:
                {
                    const int set = static_cast<int>(element.usage) - static_cast<int>(vertex_usage::TEXCOORD0);
                    slot = 4 + set;
                    layout.texcoord_dimension[set] = components;
                    break;
                }
            }
            layout.attributes.push_back({slot, element.stream, offsets[element.stream], data, components});
            offsets[element.stream] += bytes;
        }
        for (int i = 0; i < 4; ++i)
            layout.stream_stride[i] = static_cast<uint32_t>(offsets[i]);
        return !layout.attributes.empty();
    }
}
