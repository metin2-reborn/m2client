#pragma once

#include <metin2/graphics/grp_base.h>

namespace metin2::inline geometry {

    class graphic_collision_object : public graphic_base {
      public:
        graphic_collision_object();
        ~graphic_collision_object() override;

      protected:
        auto intersect_triangle(const math::vector3 &c_orig, const math::vector3 &c_dir, const math::vector3 &c_v0, const math::vector3 &c_v1, const math::vector3 &c_v2, float *pu, float *pv, float *pt) -> bool;
        auto intersect_cube(const math::matrix *world, float sx, float sy, float sz, float ex, float ey, float ez, math::vector3 &ray_original, math::vector3 &ray_direction, float *pu, float *pv, float *pt) -> bool;
        auto intersect_indexed_mesh(const math::matrix *world, const void *vertices, int step, int vtx_count, const void *indices, int idx_count, math::vector3 &ray_original, math::vector3 &ray_direction, float *pu, float *pv, float *pt) -> bool;

        auto intersect_sphere(const math::vector3 &position, float radius, const math::vector3 &ray_original, const math::vector3 &ray_direction) -> bool;
        auto intersect_cylinder(const math::vector3 &position, float radius, float height, const math::vector3 &ray_original, const math::vector3 &ray_direction) -> bool;

        // These use the current pick ray.
        auto intersect_sphere(const math::vector3 &position, float radius) -> bool;
        auto intersect_cylinder(const math::vector3 &position, float radius, float height) -> bool;
    };
}
