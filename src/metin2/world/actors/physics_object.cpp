#include "physics_object.h"
#include <metin2/log/log.h>

namespace metin2::inline world::inline actors {

    const float c_fFrameTime = 0.02f;
    const float EPSILON = 0.001f;

    physics_world *physics_world::world_ = nullptr;
    object_manager *object_manager::obj_manager_ = nullptr;

    void physics_object::update(float elapsed_time) {
        if (x_pushing_position_.is_playing())
            x_pushing_position_.interpolate(elapsed_time);
        if (y_pushing_position_.is_playing())
            y_pushing_position_.interpolate(elapsed_time);
    }

    void physics_object::accumulate(math::vector3 *position) {
        float force = 0.0f;

        if (fabs(velocity_.x) < EPSILON ||
            fabs(velocity_.y) < EPSILON ||
            fabs(velocity_.z) < EPSILON) {
            force -= (mass_ * friction_);
        }

        acceleration_ = direction_value_ * (force / mass_);
        velocity_ += acceleration_;
        if (velocity_.x * direction_value_.x < EPSILON) {
            velocity_.x = 0.0f;
            direction_value_.x = 0.0f;
        }
        if (velocity_.y * direction_value_.y < EPSILON) {
            velocity_.y = 0.0f;
            direction_value_.y = 0.0f;
        }
        if (velocity_.z * direction_value_.z < EPSILON) {
            velocity_.z = 0.0f;
            direction_value_.z = 0.0f;
        }

        position->x += velocity_.x;
        position->y += velocity_.y;
        position->z += velocity_.z;
    }

    void physics_object::increase_external_force(const math::vector3 &base_position, float force) {
        acceleration_ = direction_value_ * (force / mass_);
        velocity_ = acceleration_;
        // Compute the final position up front. A push that needs more than 100 steps is cut short.
        const int loop_value = 100;
        math::vector3 movement(0.0f, 0.0f, 0.0f);

        for (int i = 0; i < loop_value; ++i) {
            accumulate(&movement);

            physics_world *world = physics_world::get_physics_world();
            if (world) {
                if (world->is_physical_collision(base_position + movement)) {
                    initialize();
                    return;
                }
            }

            if (fabs(velocity_.x) < EPSILON &&
                fabs(velocity_.y) < EPSILON &&
                fabs(velocity_.z) < EPSILON)
                break;
        }

        set_last_position(movement, float(loop_value) * c_fFrameTime);

        if (actor_instance_value_) {
            object_manager *object_manager = object_manager::get_object_manager();
            object_manager->adjust_collision_with_other_objects(actor_instance_value_);
        }
    }

    void physics_object::set_last_position(const pixel_position &position, float blending_time) {
        last_position_.x = float(position.x);
        last_position_.y = float(position.y);
        last_position_.z = float(position.z);
        x_pushing_position_.setup(0.0f, position.x, blending_time);
        y_pushing_position_.setup(0.0f, position.y, blending_time);
    }

    void physics_object::last_position(pixel_position *position) {
        position->x = (last_position_.x);
        position->y = (last_position_.y);
        position->z = (last_position_.z);
    }

    void physics_object::set_direction(const math::vector3 &direction) {
        direction_value_.x = direction.x;
        direction_value_.y = direction.y;
        direction_value_.z = direction.z;
    }

    auto physics_object::x_movement() -> float {
        return x_pushing_position_.changing_value();
    }

    auto physics_object::y_movement() -> float {
        return y_pushing_position_.changing_value();
    }

    auto physics_object::is_blending() -> bool {
        // Being pushed by increase_external_force().
        if (0.0f != math::length(velocity_))
            return true;

        // Being pushed by set_last_position().
        if (x_pushing_position_.is_playing() ||
            y_pushing_position_.is_playing())
            return true;

        return false;
    }

    void physics_object::initialize() {
        mass_ = 1.0f;
        friction_ = 0.3f;
        direction_value_ = math::vector3(0.0f, 0.0f, 0.0f);
        acceleration_ = math::vector3(0.0f, 0.0f, 0.0f);
        velocity_ = math::vector3(0.0f, 0.0f, 0.0f);
        last_position_ = math::vector3(0.0f, 0.0f, 0.0f);

        x_pushing_position_.initialize();
        y_pushing_position_.initialize();
    }

    physics_object::physics_object() {
        actor_instance_value_ = nullptr;
    }

    physics_object::~physics_object() = default;
}
