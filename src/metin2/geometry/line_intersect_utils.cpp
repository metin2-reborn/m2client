// Fast, robust intersection of 3D line segments, from Game Programming Gems II (Graham Rhodes).
// Equation and point names refer to that chapter.
#include "line_intersect_utils.h"
#include <assert.h>
#include <math.h>

namespace metin2::inline geometry {

#define FMAX(a, b)      ((a) > (b) ? (a) : (b))
#define FMIN(a, b)      ((a) > (b) ? (b) : (a))
#define FABS(a)         ((a) < 0.0f ? -(a) : (a))
#define OUT_OF_RANGE(a) ((a) < 0.0f || (a) > 1.f)

#define MY_EPSILON 0.1f

    inline void find_nearest_point_on_line_segment(const math::vector3 &a1,
        const math::vector3 &l,
        const math::vector3 &b,
        math::vector3 &nearest,
        float &parameter) {
        // Line/Segment is degenerate --- special case #1
        float d = math::length_sq(l);
        if (d < MY_EPSILON * MY_EPSILON) {
            parameter = 0.0f;
            nearest = a1;
            return;
        }

        math::vector3 ab = b - a1;

        // parameter is computed from Equation (20).
        parameter = (math::dot(ab, l)) / d;

        parameter = FMAX(0.0f, FMIN(1.0f, parameter));

        nearest = a1 + parameter * l;
        return;
    }

    // Nearest points of two parallel lines or segments, the hardest case: a whole range of points
    // can be nearest. la and lb are the vectors from the first to the second point (not end points);
    // the results are points C and D of the text.
    inline void find_nearest_point_of_parallel_line_segments(const math::vector3 &a1,
        const math::vector3 &a2,
        const math::vector3 &la,
        const math::vector3 &b1,
        const math::vector3 &b2,
        const math::vector3 &lb,
        math::vector3 &out_a,
        math::vector3 &out_b) {
        float s[2], temp;
        find_nearest_point_on_line_segment(a1, la, b1, out_a, s[0]);
        {
            math::vector3 tp;
            find_nearest_point_on_line_segment(a1, la, b2,
                tp, s[1]);
            if (s[0] < 0.f && s[1] < 0.f) {
                out_a = a1;
                if (s[0] < s[1]) {
                    out_b = b2;
                } else {
                    out_b = b1;
                }
            } else if (s[0] > 1.f && s[1] > 1.f) {
                out_a = a2;
                if (s[0] < s[1]) {
                    out_b = b1;
                } else {
                    out_b = b2;
                }
            } else {
                temp = 0.5f * (FMAX(0.0f, FMIN(1.0f, s[0])) + FMAX(0.0f, FMIN(1.0f, s[1])));
                out_a = a1 + temp * la;
                find_nearest_point_on_line_segment(b1, lb,
                    out_a, out_b, temp);
            }
        }
    }

    // Clamps the nearest points s and t of two infinite lines to the segments (la, lb are the vectors
    // from the first to the second point).
    inline void adjust_nearest_points(const math::vector3 &a1,
        const math::vector3 &la,
        const math::vector3 &b1,
        const math::vector3 &lb,
        float s, float t,
        math::vector3 &out_a,
        math::vector3 &out_b) {
        // handle the case where both parameter s and t are out of range
        if (OUT_OF_RANGE(s) && OUT_OF_RANGE(t)) {
            s = FMAX(0.0f, FMIN(1.0f, s));
            out_a = a1 + s * la;
            find_nearest_point_on_line_segment(b1, lb,
                out_a,
                out_b, t);
            if (OUT_OF_RANGE(t)) {
                t = FMAX(0.0f, FMIN(1.0f, t));
                out_b = b1 + t * lb;
                find_nearest_point_on_line_segment(a1, la, out_b,
                    out_a, s);
                find_nearest_point_on_line_segment(b1, lb, out_a,
                    out_b, t);
            }
        }
        // otherwise, handle the case where the parameter for only one segment is
        // out of range
        else if (OUT_OF_RANGE(s)) {
            s = FMAX(0.0f, FMIN(1.0f, s));
            out_a = a1 + s * la;
            find_nearest_point_on_line_segment(b1, lb,
                out_a,
                out_b, t);
        } else if (OUT_OF_RANGE(t)) {
            t = FMAX(0.0f, FMIN(1.0f, t));
            out_b = b1 + t * lb;
            find_nearest_point_on_line_segment(a1, la, out_b,
                out_a, s);
        } else {
            assert(0);
        }
    }

    void intersect_line_segments(const math::vector3 &a1,
        const math::vector3 &a2,
        const math::vector3 &b1,
        const math::vector3 &b2,
        math::vector3 &out_a,
        math::vector3 &out_b) {
        float temp = 0.f;
        const float epsilon = MY_EPSILON;
        const float epsilon_squared = MY_EPSILON * MY_EPSILON;

        // Compute parameters from Equations (1) and (2) in the text
        math::vector3 la = a2 - a1;
        math::vector3 lb = b2 - b1;
        // From Equation (15)
        float l11 = math::length_sq(la);
        float l22 = math::length_sq(lb);

        // Line/Segment A is degenerate ---- Special Case #1
        if (l11 < epsilon_squared) {
            out_a = a1;
            find_nearest_point_on_line_segment(b1, lb, a1,
                out_b, temp);
        }
        // Line/Segment B is degenerate ---- Special Case #1
        else if (l22 < epsilon_squared) {
            out_b = b1;
            find_nearest_point_on_line_segment(a1, la, b1,
                out_a, temp);
        }
        // Neither line/segment is degenerate
        else {
            // Compute more parameters from Equation (3) in the text.
            math::vector3 ab = b1 - a1;

            // and from Equation (15).
            float l12 = -math::dot(la, lb);

            float det_l = l11 * l22 - l12 * l12;
            // Lines/Segments A and B are parallel ---- special case #2.
            if (FABS(det_l) < epsilon) {
                find_nearest_point_of_parallel_line_segments(a1, a2,
                    la,
                    b1, b2,
                    lb,
                    out_a, out_b);
            }
            // The general case
            else {
                // from Equation (15)
                float ra = math::dot(la, ab);
                float rb = math::dot(lb, ab);

                float t = (l11 * rb - ra * l12) / det_l; // Equation (12)

                float s = (ra - l12 * t) / l11; // Equation (13)

                // if we are dealing with infinite lines or if parameters s and t both
                // lie in the range [0,1] then just compute the points using Equations
                // (1) and (2) from the text.
                out_a = (a1 + s * la);
                out_b = (b1 + t * lb);
                // otherwise, at least one of s and t is outside of [0,1] and we have to
                // handle this case.
                if ((OUT_OF_RANGE(s) || OUT_OF_RANGE(t))) {
                    adjust_nearest_points(a1, la, b1, lb,
                        s, t,
                        out_a,
                        out_b);
                }
            }
        }
    }

    void intersect_line_segments(const float a1x, const float a1y, const float a1z,
        const float a2x, const float a2y, const float a2z,
        const float b1x, const float b1y, const float b1z,
        const float b2x, const float b2y, const float b2z,
        bool infinite_lines, float epsilon, float &point_on_seg_ax,
        float &point_on_seg_ay, float &point_on_seg_az, float &point_on_seg_bx,
        float &point_on_seg_by, float &point_on_seg_bz) {
        float temp = 0.f;
        float epsilon_squared = epsilon * epsilon;

        // Compute parameters from Equations (1) and (2) in the text
        float lax = a2x - a1x;
        float lay = a2y - a1y;
        float laz = a2z - a1z;
        float lbx = b2x - b1x;
        float lby = b2y - b1y;
        float lbz = b2z - b1z;
        // From Equation (15)
        float l11 = (lax * lax) + (lay * lay) + (laz * laz);
        float l22 = (lbx * lbx) + (lby * lby) + (lbz * lbz);

        // Line/Segment A is degenerate ---- Special Case #1
        if (l11 < epsilon_squared) {
            point_on_seg_ax = a1x;
            point_on_seg_ay = a1y;
            point_on_seg_az = a1z;
            find_nearest_point_on_line_segment(b1x, b1y, b1z, lbx, lby, lbz, a1x, a1y, a1z,
                infinite_lines, epsilon, point_on_seg_bx, point_on_seg_by,
                point_on_seg_bz, temp);
        }
        // Line/Segment B is degenerate ---- Special Case #1
        else if (l22 < epsilon_squared) {
            point_on_seg_bx = b1x;
            point_on_seg_by = b1y;
            point_on_seg_bz = b1z;
            find_nearest_point_on_line_segment(a1x, a1y, a1z, lax, lay, laz, b1x, b1y, b1z,
                infinite_lines, epsilon, point_on_seg_ax, point_on_seg_ay,
                point_on_seg_az, temp);
        }
        // Neither line/segment is degenerate
        else {
            // Compute more parameters from Equation (3) in the text.
            float a_bx = b1x - a1x;
            float a_by = b1y - a1y;
            float a_bz = b1z - a1z;

            // and from Equation (15).
            float l12 = -(lax * lbx) - (lay * lby) - (laz * lbz);

            float det_l = l11 * l22 - l12 * l12;
            // Lines/Segments A and B are parallel ---- special case #2.
            if (FABS(det_l) < epsilon) {
                find_nearest_point_of_parallel_line_segments(a1x, a1y, a1z, a2x, a2y, a2z,
                    lax, lay, laz,
                    b1x, b1y, b1z, b2x, b2y, b2z,
                    lbx, lby, lbz,
                    infinite_lines, epsilon,
                    point_on_seg_ax, point_on_seg_ay, point_on_seg_az,
                    point_on_seg_bx, point_on_seg_by, point_on_seg_bz);
            }
            // The general case
            else {
                // from Equation (15)
                float ra = lax * a_bx + lay * a_by + laz * a_bz;
                float rb = -lbx * a_bx - lby * a_by - lbz * a_bz;

                float t = (l11 * rb - ra * l12) / det_l; // Equation (12)

                float s = (ra - l12 * t) / l11; // Equation (13)

                // if we are dealing with infinite lines or if parameters s and t both
                // lie in the range [0,1] then just compute the points using Equations
                // (1) and (2) from the text.
                point_on_seg_ax = (a1x + s * lax);
                point_on_seg_ay = (a1y + s * lay);
                point_on_seg_az = (a1z + s * laz);
                point_on_seg_bx = (b1x + t * lbx);
                point_on_seg_by = (b1y + t * lby);
                point_on_seg_bz = (b1z + t * lbz);
                // otherwise, at least one of s and t is outside of [0,1] and we have to
                // handle this case.
                if (false == infinite_lines && (OUT_OF_RANGE(s) || OUT_OF_RANGE(t))) {
                    adjust_nearest_points(a1x, a1y, a1z, lax, lay, laz,
                        b1x, b1y, b1z, lbx, lby, lbz,
                        epsilon, s, t,
                        point_on_seg_ax, point_on_seg_ay, point_on_seg_az,
                        point_on_seg_bx, point_on_seg_by, point_on_seg_bz);
                }
            }
        }
    }

    // Point on the line or segment through a1 with vector l (not an end point) nearest to b;
    // parameter is 0 at a1 and 1 at a1 + l.
    void find_nearest_point_on_line_segment(const float a1x, const float a1y, const float a1z,
        const float lx, const float ly, const float lz,
        const float bx, const float by, const float bz,
        bool infinite_line, float epsilon_squared, float &nearest_point_x,
        float &nearest_point_y, float &nearest_point_z,
        float &parameter) {
        // Line/Segment is degenerate --- special case #1
        float d = lx * lx + ly * ly + lz * lz;
        if (d < epsilon_squared) {
            parameter = 0.0f;
            nearest_point_x = a1x;
            nearest_point_y = a1y;
            nearest_point_z = a1z;
            return;
        }

        float a_bx = bx - a1x;
        float a_by = by - a1y;
        float a_bz = bz - a1z;

        // parameter is computed from Equation (20).
        parameter = (lx * a_bx + ly * a_by + lz * a_bz) / d;

        if (false == infinite_line) parameter = FMAX(0.0f, FMIN(1.0f, parameter));

        nearest_point_x = a1x + parameter * lx;
        nearest_point_y = a1y + parameter * ly;
        nearest_point_z = a1z + parameter * lz;
        return;
    }

    // Nearest points of two parallel lines or segments, the hardest case: a whole range of points
    // can be nearest. la and lb are the vectors from the first to the second point (not end points);
    // the results are points C and D of the text.
    void find_nearest_point_of_parallel_line_segments(float a1x, float a1y, float a1z,
        float a2x, float a2y, float a2z,
        float lax, float lay, float laz,
        float b1x, float b1y, float b1z,
        float b2x, float b2y, float b2z,
        float lbx, float lby, float lbz,
        bool infinite_lines, float epsilon_squared,
        float &point_on_seg_ax, float &point_on_seg_ay, float &point_on_seg_az,
        float &point_on_seg_bx, float &point_on_seg_by, float &point_on_seg_bz) {
        float s[2], temp;
        find_nearest_point_on_line_segment(a1x, a1y, a1z, lax, lay, laz, b1x, b1y, b1z,
            true, epsilon_squared, point_on_seg_ax, point_on_seg_ay, point_on_seg_az, s[0]);
        if (true == infinite_lines) {
            point_on_seg_bx = b1x;
            point_on_seg_by = b1y;
            point_on_seg_bz = b1z;
        } else {
            float tp[3];
            find_nearest_point_on_line_segment(a1x, a1y, a1z, lax, lay, laz, b2x, b2y, b2z,
                true, epsilon_squared, tp[0], tp[1], tp[2], s[1]);
            if (s[0] < 0.f && s[1] < 0.f) {
                point_on_seg_ax = a1x;
                point_on_seg_ay = a1y;
                point_on_seg_az = a1z;
                if (s[0] < s[1]) {
                    point_on_seg_bx = b2x;
                    point_on_seg_by = b2y;
                    point_on_seg_bz = b2z;
                } else {
                    point_on_seg_bx = b1x;
                    point_on_seg_by = b1y;
                    point_on_seg_bz = b1z;
                }
            } else if (s[0] > 1.f && s[1] > 1.f) {
                point_on_seg_ax = a2x;
                point_on_seg_ay = a2y;
                point_on_seg_az = a2z;
                if (s[0] < s[1]) {
                    point_on_seg_bx = b1x;
                    point_on_seg_by = b1y;
                    point_on_seg_bz = b1z;
                } else {
                    point_on_seg_bx = b2x;
                    point_on_seg_by = b2y;
                    point_on_seg_bz = b2z;
                }
            } else {
                temp = 0.5f * (FMAX(0.0f, FMIN(1.0f, s[0])) + FMAX(0.0f, FMIN(1.0f, s[1])));
                point_on_seg_ax = (a1x + temp * lax);
                point_on_seg_ay = (a1y + temp * lay);
                point_on_seg_az = (a1z + temp * laz);
                find_nearest_point_on_line_segment(b1x, b1y, b1z, lbx, lby, lbz,
                    point_on_seg_ax, point_on_seg_ay, point_on_seg_az, true,
                    epsilon_squared, point_on_seg_bx, point_on_seg_by, point_on_seg_bz, temp);
            }
        }
    }

    // Clamps the nearest points s and t of two infinite lines to the segments (la, lb are the vectors
    // from the first to the second point).
    void adjust_nearest_points(float a1x, float a1y, float a1z,
        float lax, float lay, float laz,
        float b1x, float b1y, float b1z,
        float lbx, float lby, float lbz,
        float epsilon_squared, float s, float t,
        float &point_on_seg_ax, float &point_on_seg_ay, float &point_on_seg_az,
        float &point_on_seg_bx, float &point_on_seg_by, float &point_on_seg_bz) {
        // handle the case where both parameter s and t are out of range
        if (OUT_OF_RANGE(s) && OUT_OF_RANGE(t)) {
            s = FMAX(0.0f, FMIN(1.0f, s));
            point_on_seg_ax = (a1x + s * lax);
            point_on_seg_ay = (a1y + s * lay);
            point_on_seg_az = (a1z + s * laz);
            find_nearest_point_on_line_segment(b1x, b1y, b1z, lbx, lby, lbz, point_on_seg_ax,
                point_on_seg_ay, point_on_seg_az, true, epsilon_squared,
                point_on_seg_bx, point_on_seg_by, point_on_seg_bz, t);
            if (OUT_OF_RANGE(t)) {
                t = FMAX(0.0f, FMIN(1.0f, t));
                point_on_seg_bx = (b1x + t * lbx);
                point_on_seg_by = (b1y + t * lby);
                point_on_seg_bz = (b1z + t * lbz);
                find_nearest_point_on_line_segment(a1x, a1y, a1z, lax, lay, laz, point_on_seg_bx,
                    point_on_seg_by, point_on_seg_bz, false, epsilon_squared,
                    point_on_seg_ax, point_on_seg_ay, point_on_seg_az, s);
                find_nearest_point_on_line_segment(b1x, b1y, b1z, lbx, lby, lbz, point_on_seg_ax,
                    point_on_seg_ay, point_on_seg_az, false, epsilon_squared,
                    point_on_seg_bx, point_on_seg_by, point_on_seg_bz, t);
            }
        }
        // otherwise, handle the case where the parameter for only one segment is
        // out of range
        else if (OUT_OF_RANGE(s)) {
            s = FMAX(0.0f, FMIN(1.0f, s));
            point_on_seg_ax = (a1x + s * lax);
            point_on_seg_ay = (a1y + s * lay);
            point_on_seg_az = (a1z + s * laz);
            find_nearest_point_on_line_segment(b1x, b1y, b1z, lbx, lby, lbz, point_on_seg_ax,
                point_on_seg_ay, point_on_seg_az, false, epsilon_squared,
                point_on_seg_bx, point_on_seg_by, point_on_seg_bz, t);
        } else if (OUT_OF_RANGE(t)) {
            t = FMAX(0.0f, FMIN(1.0f, t));
            point_on_seg_bx = (b1x + t * lbx);
            point_on_seg_by = (b1y + t * lby);
            point_on_seg_bz = (b1z + t * lbz);
            find_nearest_point_on_line_segment(a1x, a1y, a1z, lax, lay, laz, point_on_seg_bx,
                point_on_seg_by, point_on_seg_bz, false, epsilon_squared,
                point_on_seg_ax, point_on_seg_ay, point_on_seg_az, s);
        } else {
            assert(0);
        }
    }
}
