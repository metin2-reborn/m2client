#include <metin2/math/math.h>

namespace metin2::inline geometry {
/* Copyright (C) Graham Rhodes, 2001.
 * All rights reserved worldwide.
 *
 * This software is provided "as is" without express or implied
 * warranties. You may freely copy and compile this source into
 * applications you distribute provided that the copyright text
 * below is included in the resulting source code, for example:
 * "Portions Copyright (C) Graham Rhodes, 2001"
 */
// Line segment intersection from Game Programming Gems II, "Fast, Robust Intersection of 3D Line Segments".
#pragma once

    void intersect_line_segments(const math::vector3 &a1,
        const math::vector3 &a2,
        const math::vector3 &b1,
        const math::vector3 &b2,
        math::vector3 &out_a,
        math::vector3 &out_b);

    void intersect_line_segments(const float a1x, const float a1y, const float a1z,
        const float a2x, const float a2y, const float a2z,
        const float b1x, const float b1y, const float b1z,
        const float b2x, const float b2y, const float b2z,
        bool infinite_lines, float epsilon, float &point_on_seg_ax,
        float &point_on_seg_ay, float &point_on_seg_az, float &point_on_seg_bx,
        float &point_on_seg_by, float &point_on_seg_bz);

    void find_nearest_point_on_line_segment(const float a1x, const float a1y, const float a1z,
        const float lx, const float ly, const float lz,
        const float bx, const float by, const float bz,
        bool infinite_line, float epsilon_squared, float &nearest_point_x,
        float &nearest_point_y, float &nearest_point_z,
        float &parameter);

    void find_nearest_point_of_parallel_line_segments(float a1x, float a1y, float a1z,
        float a2x, float a2y, float a2z,
        float lax, float lay, float laz,
        float b1x, float b1y, float b1z,
        float b2x, float b2y, float b2z,
        float lbx, float lby, float lbz,
        bool infinite_lines, float epsilon_squared,
        float &point_on_seg_ax, float &point_on_seg_ay, float &point_on_seg_az,
        float &point_on_seg_bx, float &point_on_seg_by, float &point_on_seg_bz);

    void adjust_nearest_points(float a1x, float a1y, float a1z,
        float lax, float lay, float laz,
        float b1x, float b1y, float b1z,
        float lbx, float lby, float lbz,
        float epsilon_squared, float s, float t,
        float &point_on_seg_ax, float &point_on_seg_ay, float &point_on_seg_az,
        float &point_on_seg_bx, float &point_on_seg_by, float &point_on_seg_bz);
}
