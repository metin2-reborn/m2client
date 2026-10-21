/* Copyright (C) John W. Ratcliff, 2001.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) John W. Ratcliff, 2001"
 */
#pragma once

#include "vector.h"

namespace metin2::inline geometry {

    class sphere {
      public:
        sphere();
        sphere(const vector3d &center, float radius);

        virtual ~sphere() = default;

        void set(const vector3d &center, float radius);

        auto radius(void) const -> float { return m_radius_; };
        auto radius2(void) const -> float { return m_radius2_; };
        auto center(void) const -> const vector3d & { return m_center_; };

        auto ray_intersection(const vector3d &ray_origin,
            const vector3d &v,
            float distance,
            vector3d *intersect) -> bool;

        auto ray_intersection(const vector3d &ray_origin,
            const vector3d &ray_direction,
            vector3d *intersect) -> bool;

        auto ray_intersection_in_front(const vector3d &ray_origin,
            const vector3d &ray_direction,
            vector3d *intersect) -> bool;

        void set_radius(float radius) {
            m_radius_ = radius;
            m_radius2_ = radius * radius;
        }

      protected:
        vector3d m_center_;

      private:
        float m_radius_;
        float m_radius2_;
    };

    inline sphere::sphere()
        : m_center_(vector3d(0.0f, 0.0f, 0.0f)), m_radius_(0.0f), m_radius2_(0.0f) {};

    inline sphere::sphere(const vector3d &center, float radius)
        : m_center_(center), m_radius_(radius), m_radius2_(radius * radius) {};
}
