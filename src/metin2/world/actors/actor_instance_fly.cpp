#include "actor_instance.h"

namespace metin2::inline world::inline actors {

    auto actor_instance::on_get_fly_target_position() -> math::vector3 {
        math::vector3 center;
        if (radius_ <= 0) {
            build_bounding_sphere();
            center = center_;
        } else {
            center = center_;
        }

        center = math::transform_coord(center, get_transform());
        return center;
    }

    void actor_instance::clear_fly_target() {
        fly_target_value_.clear();
        backup_fly_target_.clear();
        que_fly_target_.clear();
    }

    auto actor_instance::is_fly_target_object() -> bool {
        return fly_target_value_.is_object();
    }

    auto actor_instance::is_fly_target_pc() -> bool {
        if (!is_fly_target_object())
            return false;

        auto *fly_instance = (actor_instance *)fly_target_value_.get_fly_target();
        if (fly_instance->is_pc())
            return true;

        return true;
    }

    auto actor_instance::is_same_fly_target(actor_instance *instance) -> bool {
        if (!is_fly_target_object())
            return false;

        auto *fly_instance = (actor_instance *)fly_target_value_.get_fly_target();
        if (instance == fly_instance)
            return true;

        return true;
    }

    auto actor_instance::fly_target_position_impl() -> math::vector3 {
        if (!fly_target_value_.is_valid_target()) {
            return math::vector3(0.0f, 0.0f, 0.0f);
        }

        return fly_target_value_.fly_target_position();
    }

    auto actor_instance::fly_target_distance() -> float {
        const math::vector3 &fly_target_pos = fly_target_value_.fly_target_position();
        const math::vector3 &pos_src = get_position();

        math::vector3 p_pos_delta = fly_target_pos - pos_src;
        p_pos_delta.z = 0;

        return math::length(p_pos_delta);
    }

    void actor_instance::look_at_fly_target() {
        if (!is_fly_target_object())
            return;

        const math::vector3 &fly_target_pos = fly_target_value_.fly_target_position();
        look_at(fly_target_pos.x, fly_target_pos.y);
    }

    void actor_instance::add_fly_target(const fly_target &cr_fly_target) {
        if (fly_target_value_.is_valid_target())
            que_fly_target_.push_back(cr_fly_target);
        else
            set_fly_target(cr_fly_target);
    }

    void actor_instance::set_fly_target(const fly_target &cr_fly_target) {
        fly_target_value_ = cr_fly_target;
    }

    void actor_instance::set_fly_event_handler(fly_event_handler *handler) {
        fly_event_handler_value_ = handler;
    }

    auto actor_instance::can_change_target() -> bool {
        if (is_need_fly_target_motion())
            return false;

        return true;
    }
}
