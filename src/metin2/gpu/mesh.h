#pragma once

#include "device.h"

#include <memory>

namespace metin2::gpu {

    // An indexed triangle list of positions and normals (fvf::XYZ | fvf::NORMAL, 16-bit indices).
    struct mesh {
        static constexpr uint32_t VERTEX_FORMAT = fvf::XYZ | fvf::NORMAL;
        static constexpr uint32_t VERTEX_SIZE = 24;

        std::shared_ptr<vertex_buffer> vertices;
        std::shared_ptr<index_buffer> indices;
        uint32_t vertex_count = 0;
        uint32_t face_count = 0;
    };

    // Centred on the origin, the poles on the z axis.
    auto make_sphere(device &dev, float radius, uint32_t slices, uint32_t stacks) -> mesh;
    // Along the z axis from -length / 2 to length / 2, without caps.
    auto make_cylinder(device &dev, float radius1, float radius2, float length, uint32_t slices, uint32_t stacks) -> mesh;
}
