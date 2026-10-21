#include "mesh.h"

#include <cmath>
#include <cstring>
#include <numbers>
#include <vector>

namespace metin2::gpu {

    namespace {
        struct mesh_vertex {
            float x, y, z;
            float nx, ny, nz;
        };

        auto upload(device &dev, const std::vector<mesh_vertex> &vertices, const std::vector<uint16_t> &indices) -> mesh {
            mesh m;
            m.vertex_count = static_cast<uint32_t>(vertices.size());
            m.face_count = static_cast<uint32_t>(indices.size() / 3);
            m.vertices = dev.create_vertex_buffer(static_cast<uint32_t>(vertices.size() * sizeof(mesh_vertex)), usage::WRITE_ONLY);
            m.indices = dev.create_index_buffer(static_cast<uint32_t>(indices.size() * sizeof(uint16_t)), usage::WRITE_ONLY, format::INDEX16);
            void *data = nullptr;
            if (m.vertices && m.vertices->lock(0, 0, &data)) {
                memcpy(data, vertices.data(), vertices.size() * sizeof(mesh_vertex));
                m.vertices->unlock();
            }
            if (m.indices && m.indices->lock(0, 0, &data)) {
                memcpy(data, indices.data(), indices.size() * sizeof(uint16_t));
                m.indices->unlock();
            }
            return m;
        }

        // Two triangles per quad of a (rows + 1) x (columns + 1) vertex grid.
        auto grid_indices(uint32_t rows, uint32_t columns) -> std::vector<uint16_t> {
            std::vector<uint16_t> indices;
            for (uint32_t i = 0; i < rows; ++i)
                for (uint32_t j = 0; j < columns; ++j) {
                    const auto a = static_cast<uint16_t>(i * (columns + 1) + j);
                    const auto b = static_cast<uint16_t>(a + columns + 1);
                    indices.insert(indices.end(), {a, b, static_cast<uint16_t>(a + 1), static_cast<uint16_t>(a + 1), b, static_cast<uint16_t>(b + 1)});
                }
            return indices;
        }
    }

    auto make_sphere(device &dev, float radius, uint32_t slices, uint32_t stacks) -> mesh {
        std::vector<mesh_vertex> vertices;
        for (uint32_t i = 0; i <= stacks; ++i) {
            const float phi = std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(stacks);
            for (uint32_t j = 0; j <= slices; ++j) {
                const float theta = 2.0f * std::numbers::pi_v<float> * static_cast<float>(j) / static_cast<float>(slices);
                const float nx = sinf(phi) * cosf(theta);
                const float ny = sinf(phi) * sinf(theta);
                const float nz = cosf(phi);
                vertices.push_back({nx * radius, ny * radius, nz * radius, nx, ny, nz});
            }
        }
        return upload(dev, vertices, grid_indices(stacks, slices));
    }

    auto make_cylinder(device &dev, float radius1, float radius2, float length, uint32_t slices, uint32_t stacks) -> mesh {
        std::vector<mesh_vertex> vertices;
        for (uint32_t i = 0; i <= stacks; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(stacks);
            const float z = -length / 2.0f + t * length;
            const float r = radius1 + (radius2 - radius1) * t;
            for (uint32_t j = 0; j <= slices; ++j) {
                const float theta = 2.0f * std::numbers::pi_v<float> * static_cast<float>(j) / static_cast<float>(slices);
                const float nx = cosf(theta);
                const float ny = sinf(theta);
                vertices.push_back({nx * r, ny * r, z, nx, ny, 0.0f});
            }
        }
        return upload(dev, vertices, grid_indices(stacks, slices));
    }
}
