#pragma once
#include <metin2/geometry/collision_data.h>

namespace metin2::inline world::inline actors {

    auto detect_collision_dynamic_sphere_vs_dynamic_sphere(const dynamic_sphere_instance &sphere1, const dynamic_sphere_instance &sphere2) -> bool;
    auto detect_collision_dynamic_z_cylinder_vs_dynamic_z_cylinder(const dynamic_sphere_instance &sphere1, const dynamic_sphere_instance &sphere2) -> bool;

    auto get_degree_from_position(float x, float y) -> float;
    auto get_degree_from_position2(float sx, float sy, float ex, float ey) -> float;

    auto get_interpolated_rotation(float begin, float end, float cur_rate) -> float;
    auto is_ccw_rotation(float begin, float end) -> bool;

    auto is_cw_acute_angle(float begin, float end) -> bool;
    auto is_ccw_acute_angle(float begin, float end) -> bool;

    enum degree_direction {
        DEGREE_DIRECTION_SAME = 0,
        DEGREE_DIRECTION_RIGHT = 1,
        DEGREE_DIRECTION_LEFT = 2,
    };
    auto get_degree_difference(float source, float target) -> float;
    auto get_rotating_direction(float source, float target) -> int;

    auto camera_rotation_to_character_rotation(float camera_rotation) -> float;
}
