/* Copyright (C) John W. Ratcliff, 2001.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) John W. Ratcliff, 2001"
 */

#include "sphere.h"

#include <cmath>

namespace metin2::inline geometry {

    void sphere::set(const vector3d &center, float radius) {
        m_center_ = center;
        m_radius_ = radius;
        m_radius2_ = radius * radius;
    }

    // Ray-sphere intersection from Graphics Gems p. 388. The Gem reports the wrong intersection when
    // the ray starts inside the sphere; that case is fixed here by inverting the direction.
    auto sphere::ray_intersection(const vector3d &ray_origin,
        const vector3d &dir,
        vector3d *intersect) -> bool {
        vector3d eo = m_center_ - ray_origin;
        vector3d v = dir;
        float dist2 = eo.x * eo.x + eo.y * eo.y + eo.z * eo.z;
        if (dist2 < m_radius2_) v *= -1;

        float v_2 = eo.dot(v);

        float disc = m_radius2_ - (eo.length2() - v_2 * v_2);

        if (disc > 0.0f) {

            if (intersect) {

                float d = sqrtf(disc);

                *intersect = ray_origin + v * (v_2 - d);
            }

            return true;
        }
        return false;
    }

    auto sphere::ray_intersection(const vector3d &ray_origin,
        const vector3d &v,
        float distance,
        vector3d *intersect) -> bool {
        vector3d sect;
        bool hit = ray_intersection_in_front(ray_origin, v, &sect);

        if (hit) {
            float d = ray_origin.distance_sq(sect);
            if (d > (distance * distance)) return false;
            if (intersect) *intersect = sect;
            return true;
        }
        return false;
    }

    auto sphere::ray_intersection_in_front(const vector3d &ray_origin,
        const vector3d &v,
        vector3d *intersect) -> bool {
        vector3d sect;
        bool hit = ray_intersection(ray_origin, v, &sect);

        if (hit) {

            vector3d dir = sect - ray_origin;

            float dot = dir.dot(v);

            if (dot >= 0) {
                if (intersect) *intersect = sect;
                return true;
            }
        }
        return false;
    }
}
