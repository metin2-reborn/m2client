#include <metin2/client/background/python_background.h>
#include "instance_base.h"

namespace metin2::inline client::inline characters {

    void instance_base::script_set_pixel_position(float fx, float fy) {
        float fz = background_height(fx, fy);
        new_set_pixel_position(pixel_position(fx, fy, fz));
    }

    void instance_base::new_set_pixel_position(const pixel_position &pixel_position) {
        graphic_thing_instance_value_.set_cur_pixel_position(pixel_position);
    }

    void instance_base::new_get_pixel_position(pixel_position *pixel_position) {
        *pixel_position = graphic_thing_instance_value_.new_get_cur_pixel_position_ref();
    }

    void instance_base::set_rotation(float rotation) {
        graphic_thing_instance_value_.set_rotation(rotation);
    }

    void instance_base::blend_rotation(float rotation, float blend_time) {
        graphic_thing_instance_value_.blend_rotation(rotation, blend_time);
    }

    void instance_base::new_look_at_fly_target() {
        graphic_thing_instance_value_.look_at_fly_target();
    }

    void instance_base::new_look_at_dest_pixel_position(const pixel_position &p_pos_dst) {
        graphic_thing_instance_value_.look_at(p_pos_dst.x, -p_pos_dst.y);
    }

    void instance_base::new_look_at_dest_instance(instance_base &inst_dst) {
        graphic_thing_instance_value_.look_at(&inst_dst.graphic_thing_instance_value_);
    }

    auto instance_base::rotation() -> float {
        return graphic_thing_instance_value_.rotation();
    }

    auto instance_base::advancing_rotation() -> float {
        return graphic_thing_instance_value_.advancing_rotation();
    }

    void instance_base::set_direction(int dir) {
        float degree = degree_from_direction(dir);
        set_rotation(degree);
        set_advancing_rotation(degree);
    }

    auto instance_base::degree_from_direction(int dir) -> float {
        if (dir < 0)
            return 0.0f;

        if (dir >= DIR_MAX_NUM)
            return 0.0f;

        static float dir_rot[DIR_MAX_NUM] =
            {
                +45.0f * 4,
                +45.0f * 3,
                +45.0f * 2,
                +45.0f,
                +0.0f,
                360.0f - 45.0f,
                360.0f - 45.0f * 2,
                360.0f - 45.0f * 3,
        };

        return dir_rot[dir];
    }
}
