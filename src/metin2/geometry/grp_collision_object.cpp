#include "grp_collision_object.h"
#include <cmath>
#include <metin2/log/log.h>

namespace metin2::inline geometry {

    auto graphic_collision_object::intersect_cube(const math::matrix *world, float sx, float sy, float sz, float ex, float ey, float ez,
        math::vector3 &ray_original, math::vector3 &ray_direction, float *pu, float *pv, float *pt) -> bool {
        position pos_vertices[8];

        pos_vertices[0] = position(sx, sy, sz);
        pos_vertices[1] = position(ex, sy, sz);
        pos_vertices[2] = position(sx, ey, sz);
        pos_vertices[3] = position(ex, ey, sz);
        pos_vertices[4] = position(sx, sy, ez);
        pos_vertices[5] = position(ex, sy, ez);
        pos_vertices[6] = position(sx, ey, ez);
        pos_vertices[7] = position(ex, ey, ez);

        static const uint16_t fill_cube_indices[36] = {
            0,
            1,
            2,
            1,
            3,
            2,
            2,
            0,
            6,
            0,
            4,
            6,
            0,
            1,
            4,
            1,
            5,
            4,
            1,
            3,
            5,
            3,
            7,
            5,
            3,
            2,
            7,
            2,
            6,
            7,
            4,
            5,
            6,
            5,
            7,
            6,
        };

        return intersect_indexed_mesh(
            world,
            pos_vertices,
            sizeof(position),
            8,
            fill_cube_indices,
            36,
            ray_original,
            ray_direction,
            pu,
            pv,
            pt);
    }

    const int c_iLimitVertexCount = 1024;

    auto graphic_collision_object::intersect_indexed_mesh(const math::matrix *world, const void *vertices, int step, int vtx_count, const void *indices, int idx_count,
        math::vector3 &ray_original, math::vector3 &ray_direction, float *pu, float *pv, float *pt) -> bool {
        static math::vector3 position_array[c_iLimitVertexCount];
        static uint32_t position_count;

        if (vtx_count > c_iLimitVertexCount) {
            metin2::log::debug("Mesh has too many vertices for collision detection: {} (limit {})", vtx_count, c_iLimitVertexCount);
            return false;
        }

        position_count = 0;

        char *pcur_vtx = (char *)vertices;

        while (vtx_count--) {
            auto *pos = (float *)pcur_vtx;

            position_array[position_count++] = math::transform_coord(*((math::vector3 *)pos), *world);

            pcur_vtx += step;
        }

        auto *pcur_idx = (uint16_t *)indices;

        int tri_count = idx_count / 3;
        while (tri_count--) {
            if (intersect_triangle(ray_original, ray_direction,
                    position_array[pcur_idx[0]],
                    position_array[pcur_idx[1]],
                    position_array[pcur_idx[2]],
                    pu, pv, pt)) {
                return true;
            }

            pcur_idx += 3;
        }

        return false;
    }

    auto graphic_collision_object::intersect_triangle(const math::vector3 &c_orig,
        const math::vector3 &c_dir,
        const math::vector3 &c_v0,
        const math::vector3 &c_v1,
        const math::vector3 &c_v2,
        float *pu,
        float *pv,
        float *pt) -> bool {
        math::vector3 edge1 = c_v1 - c_v0;
        math::vector3 edge2 = c_v2 - c_v0;
        math::vector3 pvec;
        pvec = math::cross(c_dir, edge2);

        float det = math::dot(edge1, pvec);
        math::vector3 tvec;

        if (det > 0) {
            tvec = c_orig - c_v0;
        } else {
            tvec = c_v0 - c_orig;
            det = -det;
        }

        if (det < 0.0001f)
            return false;

        float u, v, t;
        u = math::dot(tvec, pvec);
        if (u < 0.0f || u > det)
            return false;

        math::vector3 qvec;
        qvec = math::cross(tvec, edge1);

        v = math::dot(c_dir, qvec);
        if (v < 0.0f || u + v > det)
            return false;

        t = math::dot(edge2, qvec);
        float inv_det = 1.0f / det;
        t *= inv_det;
        u *= inv_det;
        v *= inv_det;

        math::vector3 spot = edge1 * u + edge2 * v;
        spot += c_v0;

        *pu = spot.x;
        *pv = spot.y;
        *pt = t;

        return true;
    }

    auto graphic_collision_object::intersect_sphere(const math::vector3 &position, float radius, const math::vector3 &c_rv3_ray_original, const math::vector3 &ray_direction) -> bool {
        math::vector3 v3_ray_original = c_rv3_ray_original - position;

        float a = math::dot(ray_direction, ray_direction);
        float b = 2 * math::dot(v3_ray_original, ray_direction);
        float c = math::dot(v3_ray_original, v3_ray_original) - radius * radius;

        float d = b * b - 4 * a * c;

        if (d >= 0)
            return true;

        return false;
    }

    auto graphic_collision_object::intersect_cylinder(const math::vector3 &position, float radius, float height, const math::vector3 &c_rv3_ray_original, const math::vector3 &ray_direction) -> bool {
        math::vector3 v3_ray_original = c_rv3_ray_original - position;

        float a = ray_direction.x * ray_direction.x + ray_direction.y * ray_direction.y;
        float b = 2 * (v3_ray_original.x * ray_direction.x + v3_ray_original.y * ray_direction.y);
        float c = v3_ray_original.x * v3_ray_original.x + v3_ray_original.y * v3_ray_original.y - radius * radius;

        float d = b * b - 4 * a * c;
        if (d > 0)
            if (0.0f != a) {
                float plus = (-b + sqrtf(d)) / (2 * a);
                float minus = (-b - sqrtf(d)) / (2 * a);
                float fz_plus = v3_ray_original.z + plus * ray_direction.z;
                float fz_minus = v3_ray_original.z + minus * ray_direction.z;

                if (fz_plus > 0.0f && fz_plus <= height)
                    return true;
                if (fz_minus > 0.0f && fz_minus <= height)
                    return true;
                if (fz_minus * fz_plus < 0.0f)
                    return true;
            }

        return false;
    }

    auto graphic_collision_object::intersect_sphere(const math::vector3 &position, float radius) -> bool {
        return graphic_collision_object::intersect_sphere(position, radius, vt_pick_ray_orig_, vt_pick_ray_dir_);
    }

    auto graphic_collision_object::intersect_cylinder(const math::vector3 &position, float radius, float height) -> bool {
        return graphic_collision_object::intersect_cylinder(position, radius, height, vt_pick_ray_orig_, vt_pick_ray_dir_);
    }

    graphic_collision_object::graphic_collision_object() = default;

    graphic_collision_object::~graphic_collision_object() = default;
}
