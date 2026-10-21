#include "fly_target.h"
#include <cassert>

namespace metin2::inline world::inline flying {

    fly_target::fly_target() {
        initialize_impl();
    }

    fly_target::fly_target(fly_targetable_object *fly_target) {
        initialize_impl();

        type_value_ = TYPE_OBJECT;
        fly_target_value_ = fly_target;
        fly_target_value_->add_fly_targeter(this);
    }
    fly_target::fly_target(const math::vector3 &fly_target_position) {
        initialize_impl();

        type_value_ = TYPE_POSITION;
        fly_target_position_ = fly_target_position;
    }

    fly_target::fly_target(const fly_target &rhs) {
        initialize_impl();

        *this = rhs;
    }

    fly_target::~fly_target() {
        if (fly_target_value_)
            fly_target_value_->remove_fly_targeter(this);
    }

    void fly_target::initialize_impl() {
        fly_target_position_ = math::vector3(0.0f, 0.0f, 0.0f);
        fly_target_value_ = nullptr;
        type_value_ = TYPE_NONE;
    }

    void fly_target::clear() {
        type_value_ = TYPE_NONE;
        fly_target_value_ = nullptr;
    }

    auto fly_target::is_object() -> bool {
        return (TYPE_OBJECT == get_type());
    }

    auto fly_target::is_position() -> bool {
        return (TYPE_POSITION == get_type());
    }

    auto fly_target::is_valid_target() -> bool {
        return (TYPE_NONE != get_type());
    }

    void fly_target::notify_target_clear() {
        type_value_ = TYPE_POSITION;
        fly_target_value_ = nullptr;
    }

    auto fly_target::get_type() -> fly_target::type {
        return type_value_;
    }

    auto fly_target::get_fly_target() -> fly_targetable_object * {
        assert(TYPE_OBJECT == get_type() && "CFly::GetFlyTarget");
        return fly_target_value_;
    }

    auto fly_target::fly_target_position() const -> const math::vector3 & {
        if (type_value_ == TYPE_OBJECT) {
            fly_target_position_ = fly_target_value_->on_get_fly_target_position();
        }
        return fly_target_position_;
    }

    auto fly_target::operator=(const fly_target &rhs) -> fly_target & {
        if (fly_target_value_) {
            fly_target_value_->remove_fly_targeter(this);
        }
        type_value_ = rhs.type_value_;
        fly_target_position_ = rhs.fly_target_position_;
        fly_target_value_ = rhs.fly_target_value_;
        if (fly_target_value_) {
            fly_target_value_->add_fly_targeter(this);
        }
        return *this;
    }
}
