#pragma once
#include <cmath>
#include <metin2/math/math.h>

namespace metin2::inline geometry {

    auto cross_product2d(float x1, float y1, float x2, float y2) -> float;

    auto is_in_triangle2d(float ax, float ay, float bx, float by, float cx, float cy, float tx, float ty) -> bool;

    inline auto clamp_degree(float degree) -> float {
        if (degree >= 360.0f)
            degree -= 360.0f;
        if (degree < 0.0f)
            degree += 360.0f;

        return degree;
    }

    inline auto get_vector3_distance(const math::vector3 &source, const math::vector3 &target) -> float {
        return (source.x - target.x) * (source.x - target.x) + (source.y - target.y) * (source.y - target.y);
    }

    inline auto safe_rotation_normalized_arc(const math::vector3 &v_from, const math::vector3 &v_to) -> math::quaternion {
        if (v_from == v_to)
            return math::quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        if (v_from == -v_to)
            return math::quaternion(0.0f, 0.0f, 1.0f, 0.0f);
        math::vector3 c;
        c = math::cross(v_from, v_to);
        float d = math::dot(v_from, v_to);
        float s = sqrtf((1 + d) * 2);

        return math::quaternion(c.x / s, c.y / s, c.z / s, s * 0.5f);
    }

    inline auto rotation_normalized_arc(const math::vector3 &v_from, const math::vector3 &v_to) -> math::quaternion

    {
        math::vector3 c;
        c = math::cross(v_from, v_to);
        float d = math::dot(v_from, v_to);
        float s = sqrtf((1 + d) * 2);

        return math::quaternion(c.x / s, c.y / s, c.z / s, s * 0.5f);
    }

    inline auto rotation_arc(const math::vector3 &v_from, const math::vector3 &v_to) -> math::quaternion {
        math::vector3 vn_from, vn_to;
        vn_from = math::normalize(v_from);
        vn_to = math::normalize(v_to);
        return rotation_normalized_arc(vn_from, vn_to);
    }

    inline auto square_distance_between_linesegment_and_point(const math::vector3 &p1, const math::vector3 &p2, const math::vector3 &x) -> float {
        const auto v1 = p2 - p1;
        float l = math::length_sq(v1);
        const auto v2 = x - p1;
        const auto v3 = p2 - p1;
        float d = math::dot(v2, v3);
        if (d <= 0.0f) {
            return math::length_sq(v2);
        } else if (d >= l) {
            const auto v4 = x - p2;
            return math::length_sq(v4);
        } else {
            return math::length_sq(math::cross(v2, v3)) / l;
        }
    }

    inline auto vec3_transform_quaternion_safe(math::vector3 *pvout, const math::vector3 *pv, const math::quaternion *pq) -> math::vector3 * {
        math::vector3 v;
        v = math::cross(*pv, *((math::vector3 *)pq));
        v *= -2 * pq->w;
        v += (pq->w * pq->w - math::length_sq(*((math::vector3 *)pq))) * (*pv);
        v += 2 * math::dot(*((math::vector3 *)pq), *pv) * (*(math::vector3 *)pq);
        *pvout = v;
        return pvout;
    }

    inline auto vec3_transform_quaternion(math::vector3 *pvout, const math::vector3 *pv, const math::quaternion *pq) -> math::vector3 * {
        *pvout = math::cross(*pv, *((math::vector3 *)pq));
        *pvout *= -2 * pq->w;
        *pvout += (pq->w * pq->w - math::length_sq(*((math::vector3 *)pq))) * (*pv);
        *pvout += 2 * math::dot(*((math::vector3 *)pq), *pv) * (*(math::vector3 *)pq);

        return pvout;
    }
}
