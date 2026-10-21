#include <metin2/geometry/line_intersect_utils.h>

#include <metin2/world/maps/map_util.h>
#include "game_util.h"

namespace metin2::inline world::inline actors {

    auto detect_collision_dynamic_z_cylinder_vs_dynamic_z_cylinder(const dynamic_sphere_instance &sphere1, const dynamic_sphere_instance &sphere2) -> bool {
        dynamic_sphere_instance cylinder1 = sphere1;
        dynamic_sphere_instance cylinder2 = sphere2;

        cylinder1.position_value.z = 0;
        cylinder1.last_position.z = 0;

        cylinder2.position_value.z = 0;
        cylinder2.last_position.z = 0;

        float r = cylinder1.radius + cylinder2.radius;
        float rsq = r * r;

        // AABB check
        math::vector3 mi1 = cylinder1.last_position, mi2 = cylinder1.position_value;
        math::vector3 mi3 = cylinder2.last_position, mi4 = cylinder2.position_value;
        if (mi1.x > mi2.x) std::swap(mi1.x, mi2.x);
        if (mi1.y > mi2.y) std::swap(mi1.y, mi2.y);
        if (mi1.z > mi2.z) std::swap(mi1.z, mi2.z);
        if (mi3.x > mi4.x) std::swap(mi3.x, mi4.x);
        if (mi3.y > mi4.y) std::swap(mi3.y, mi4.y);
        if (mi3.z > mi4.z) std::swap(mi3.z, mi4.z);
        mi1.x -= cylinder1.radius;
        mi1.y -= cylinder1.radius;
        mi1.z -= cylinder1.radius;
        mi2.x += cylinder1.radius;
        mi2.y += cylinder1.radius;
        mi2.z += cylinder1.radius;
        mi3.x -= cylinder2.radius;
        mi3.y -= cylinder2.radius;
        mi3.z -= cylinder2.radius;
        mi4.x += cylinder2.radius;
        mi4.y += cylinder2.radius;
        mi4.z += cylinder2.radius;
        if (mi4.x < mi1.x || mi2.x < mi3.x) return false;
        if (mi4.y < mi1.y || mi2.y < mi3.y) return false;
        if (mi4.z < mi1.z || mi2.z < mi3.z) return false;

        math::vector3 v_a, v_b;
        intersect_line_segments(cylinder1.last_position, cylinder1.position_value,
            cylinder2.last_position, cylinder2.position_value,
            v_a, v_b);
        const auto vv = (v_a - v_b);
        return (math::length_sq(vv) <= rsq);
    }

    auto detect_collision_dynamic_sphere_vs_dynamic_sphere(const dynamic_sphere_instance &sphere1, const dynamic_sphere_instance &sphere2) -> bool {
        float r = sphere1.radius + sphere2.radius;
        float rsq = r * r;

        // AABB check

        math::vector3 mi1 = sphere1.last_position, mi2 = sphere1.position_value;
        math::vector3 mi3 = sphere2.last_position, mi4 = sphere2.position_value;
        if (mi1.x > mi2.x) std::swap(mi1.x, mi2.x);
        if (mi1.y > mi2.y) std::swap(mi1.y, mi2.y);
        if (mi1.z > mi2.z) std::swap(mi1.z, mi2.z);
        if (mi3.x > mi4.x) std::swap(mi3.x, mi4.x);
        if (mi3.y > mi4.y) std::swap(mi3.y, mi4.y);
        if (mi3.z > mi4.z) std::swap(mi3.z, mi4.z);
        mi1.x -= sphere1.radius;
        mi1.y -= sphere1.radius;
        mi1.z -= sphere1.radius;
        mi2.x += sphere1.radius;
        mi2.y += sphere1.radius;
        mi2.z += sphere1.radius;
        mi3.x -= sphere2.radius;
        mi3.y -= sphere2.radius;
        mi3.z -= sphere2.radius;
        mi4.x += sphere2.radius;
        mi4.y += sphere2.radius;
        mi4.z += sphere2.radius;
        if (mi4.x < mi1.x || mi2.x < mi3.x) return false;
        if (mi4.y < mi1.y || mi2.y < mi3.y) return false;
        if (mi4.z < mi1.z || mi2.z < mi3.z) return false;

        math::vector3 v_a, v_b;
        intersect_line_segments(sphere1.last_position, sphere1.position_value,
            sphere2.last_position, sphere2.position_value,
            v_a, v_b);
        const auto vvv = (v_a - v_b);
        return (math::length_sq(vvv) <= rsq);
    }

    auto is_cw_acute_angle(float begin, float end) -> bool {
        // 360 - src + dest		// counter-clockwise
        // src - dest			// clockwise
        return ((360.0f - begin + end) > (begin - end));
    }

    auto is_ccw_acute_angle(float begin, float end) -> bool {
        // abs(360 - dest + src) 	// clockwise
        // dest - src				// counter-clockwise
        int value = abs((int)(360.0f - end + begin));
        return value >= (end - begin) ? true : false;
    }

    auto is_ccw_rotation(float begin, float end) -> bool {
        //     180
        // 270      90
        //      0
        // counter-clockwise
        return (begin - end < 0);
    }

    auto get_interpolated_rotation(float begin, float end, float cur_rate) -> float {
        if (is_ccw_rotation(begin, end)) {
            if (is_ccw_acute_angle(begin, end))
                return get_linear_interpolation(begin, end, cur_rate);

            return (360.0f + get_linear_interpolation(begin, end - 360.0f, cur_rate));
        }

        if (is_cw_acute_angle(begin, end))
            return get_linear_interpolation(begin, end, cur_rate);

        return get_linear_interpolation(begin, end + 360.0f, cur_rate);
    }

    auto get_degree_from_position(float x, float y) -> float {
        math::vector3 vt_dir(floor(x), floor(y), 0.0f);
        vt_dir = math::normalize(vt_dir);

        math::vector3 vt_stan(0, -1, 0);
        float ret = math::to_degree(acosf(math::dot(vt_dir, vt_stan)));

        if (vt_dir.x < 0.0f)
            ret = 360.0f - ret;

        return ret;
    }

    auto get_degree_from_position2(float sx, float sy, float ex, float ey) -> float {
        return get_degree_from_position(ex - sx, ey - sy);
    }

    auto get_degree_difference(float source, float target) -> float {
        if (source < 180.0f) {
            if (target < source)
                return source - target;

            else if (target - source > 180.0f)
                return (360.0f - (target - source));

            return target - source;
        } else {
            if (target > source)
                return target - source;

            else if (source - target > 180.0f) {
                return (360.0f - (source - target));
            }

            return source - target;
        }
    }
    auto get_rotating_direction(float source, float target) -> int {
        if (source < 180.0f) {
            if (target < source)
                return DEGREE_DIRECTION_RIGHT;

            else if ((360.0f - target) + source < 180.0f)
                return DEGREE_DIRECTION_RIGHT;

            return DEGREE_DIRECTION_LEFT;
        } else {
            if (target > source) {
                return DEGREE_DIRECTION_LEFT;
            } else if ((360.0f - source) + target < 180.0f) {
                return DEGREE_DIRECTION_LEFT;
            }

            return DEGREE_DIRECTION_RIGHT;
        }
    }

    auto camera_rotation_to_character_rotation(float camera_rotation) -> float {
        return fmod((540.0f - camera_rotation), 360.0f);
    }
}
