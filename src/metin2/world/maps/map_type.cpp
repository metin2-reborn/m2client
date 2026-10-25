#include "map_type.h"

namespace metin2::inline world::inline maps {

    auto environment_data::get_fog_near_distance() const -> float {
        return fog_near_distance;
    }

    auto environment_data::get_fog_far_distance() const -> float {
        return fog_far_distance;
    }

    auto s_pixel_position_calculate_distance_sq3d(const pixel_position &p_pos_left, const pixel_position &p_pos_right) -> float {
        float dx = p_pos_left.x - p_pos_right.x;
        float dy = p_pos_left.y - p_pos_right.y;
        float dz = p_pos_left.z - p_pos_right.z;

        return dx * dx + dy * dy + dz * dz;
    }
}
