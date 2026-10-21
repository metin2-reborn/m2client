#include "actor_instance.h"
#include "game_util.h"

namespace metin2::inline world::inline actors {

    void actor_instance::set_xy_rotation(float rot_x, float rot_y) {
        rot_x_ = rot_x;
        rot_y_ = rot_y;
    }

    void actor_instance::set_rotation(float rot) {
        if (horse_)
            horse_->set_rotation(rot);

        fcur_rotation_ = rot;
        rot_begin_ = fcur_rotation_;
        rot_end_ = fcur_rotation_;

        rot_blend_time_ = 0.0f;
        rot_begin_time_ = 0.0f;
        rot_end_time_ = 0.0f;

        need_update_collision_ = true;
    }

    void actor_instance::blend_rotation(float rot, float blend_time) {
        if (horse_)
            horse_->blend_rotation(rot, blend_time);

        if (fcur_rotation_ == rot)
            return;

        rot_begin_ = fmod(fcur_rotation_, 360.0f);
        rot_end_ = rot;

        rot_blend_time_ = blend_time;
        rot_begin_time_ = local_time();
        rot_end_time_ = rot_begin_time_ + rot_blend_time_;
    }

    void actor_instance::set_advancing_rotation(float rot) {
        if (horse_)
            horse_->set_advancing_rotation(rot);

        advancing_rotation_ = rot;
    }

    void actor_instance::rotation_process() {
        if (horse_)
            horse_->rotation_process();

        if (local_time() < rot_end_time_) {
            fcur_rotation_ = get_interpolated_rotation(rot_begin_, rot_end_, (local_time() - rot_begin_time_) / rot_blend_time_);
            set_advancing_rotation(fcur_rotation_);
        } else {
            fcur_rotation_ = rot_end_;
        }

        // FIXME: should only be checked for buildings.
        if (0.0f != rot_x_ || 0.0f != rot_y_) {
            graphic_object_instance::set_rotation(rot_x_, rot_y_, fcur_rotation_);
        } else {
            graphic_object_instance::set_rotation(fcur_rotation_);
        }
    }

    void actor_instance::look_at(float dir_rot) {
        blend_rotation(dir_rot, 0.3f);
    }

    void actor_instance::look_at(float fx, float fy) {
        float rot = get_degree_from_position2(x_, y_, fx, fy);

        look_at(rot);
    }

    void actor_instance::look_at(actor_instance *instance) {
        pixel_position pixel_position;
        instance->get_pixel_position(&pixel_position);
        look_at(pixel_position.x, pixel_position.y);
    }

    auto actor_instance::rotation() -> float {
        return fcur_rotation_;
    }

    auto actor_instance::target_rotation() -> float {
        return rot_end_;
    }

    auto actor_instance::advancing_rotation() -> float {
        return advancing_rotation_;
    }
}
