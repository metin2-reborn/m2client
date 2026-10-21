#include <metin2/client/background/python_background.h>
#include <metin2/core/utils.h>
#include <metin2/graphics/camera.h>

namespace metin2::inline graphics {

    extern void set_height_log(bool is_log);

    float camera::CAMERA_MIN_DISTANCE = 200.0f;
    float camera::CAMERA_MAX_DISTANCE = 2500.0f;

    void camera::process_terrain_collision() {
        python_background &python_background = get_python_background();
        math::vector3 collision_point;

        if (python_background.picking_point_with_ray_only_terrain(target_to_camera_bottom_ray_, &collision_point)) {
            set_camera_state(CAMERA_STATE_CANTGODOWN);
            math::vector3 check_vector = eye_ - 2.0f * terrain_collision_radius_ * up_;
            check_vector.z = python_background.height(floorf(check_vector.x), floorf(check_vector.y));
            math::vector3 new_eye = check_vector + 2.0f * terrain_collision_radius_ * up_;
            if (new_eye.z > eye_.z) {
                set_eye(new_eye);
            }
        } else
            set_camera_state(CAMERA_STATE_NORMAL);

        if (python_background.picking_point_with_ray_only_terrain(camera_bottom_to_terrain_ray_, &collision_point)) {
            set_camera_state(CAMERA_STATE_CANTGODOWN);
            auto diff_vector = (eye_ - collision_point);
            if (math::length(diff_vector) < 2.0f * terrain_collision_radius_) {
                math::vector3 new_eye = collision_point + 2.0f * terrain_collision_radius_ * up_;
                set_eye(new_eye);
            }
        } else
            set_camera_state(CAMERA_STATE_NORMAL);
    }

    struct camera_collision_checker {
        bool is_blocked;
        std::vector<math::vector3> *vct_position;
        dynamic_sphere_instance *pdsi;

        camera_collision_checker(dynamic_sphere_instance *pdsi_value, std::vector<math::vector3> *vct_position_value) : is_blocked(false), vct_position(vct_position_value), pdsi(pdsi_value) {
        }
        void operator()(graphic_object_instance *opponent) {
            if (opponent->collision_dynamic_sphere(*pdsi)) {
                vct_position->push_back(opponent->get_position());
                is_blocked = true;
            }
        }
    };

    void camera::process_building_collision() {
        float move_amount_small = 2.0f;
        float move_amount_large = 4.0f;

        math::vector3 check_vector;

        dynamic_sphere_instance s;
        s.radius = object_collision_radius_;
        s.last_position = eye_;

        vector3d vector3d;
        vector3d.set(eye_.x, eye_.y, eye_.z);

        culling_manager &culling_mgr = get_culling_manager();

        {
            check_vector = eye_ - object_collision_radius_ * view_;
            s.position_value = check_vector;

            std::vector<math::vector3> vct_position;
            camera_collision_checker camera_collision_checker(&s, &vct_position);
            culling_mgr.for_in_range(vector3d, object_collision_radius_, &camera_collision_checker);
            bool collide = camera_collision_checker.is_blocked;

            if (collide) {
                if (angular_velocity_.y > 0.0f) {
                    angular_velocity_.y = 0.0f;
                    angular_velocity_.z += move_amount_large;
                }

                if (vct_position.size() > 1) {
                    angular_velocity_.z += move_amount_small;
                } else {
                    auto diff_vector = (vct_position[0] - eye_);
                    check_vector = math::cross(diff_vector, view_);
                    float dot = math::dot(check_vector, up_);
                    if (dot < 0) {
                        angular_velocity_.x -= move_amount_small;
                    } else if (dot > 0) {
                        angular_velocity_.x += move_amount_small;
                    } else {
                        angular_velocity_.z += move_amount_small;
                    }
                }
            }
        }

        {
            check_vector = eye_ + 2.0f * object_collision_radius_ * up_;
            s.position_value = check_vector;

            std::vector<math::vector3> vct_position;
            camera_collision_checker camera_collision_checker(&s, &vct_position);
            culling_mgr.for_in_range(vector3d, object_collision_radius_, &camera_collision_checker);
            bool collide = camera_collision_checker.is_blocked;

            if (collide) {
                angular_velocity_.z = f_min(-move_amount_small, angular_velocity_.y);
            }
        }

        {
            check_vector = eye_ + 3.0f * object_collision_radius_ * cross_;
            s.position_value = check_vector;

            std::vector<math::vector3> vct_position;
            camera_collision_checker camera_collision_checker(&s, &vct_position);
            culling_mgr.for_in_range(vector3d, object_collision_radius_, &camera_collision_checker);
            bool collide = camera_collision_checker.is_blocked;

            if (collide) {
                if (angular_velocity_.x > 0.0f) {
                    angular_velocity_.x = 0.0f;
                    angular_velocity_.y += move_amount_large;
                }
            }
        }

        {
            check_vector = eye_ - 3.0f * object_collision_radius_ * cross_;
            s.position_value = check_vector;

            std::vector<math::vector3> vct_position;
            camera_collision_checker camera_collision_checker(&s, &vct_position);
            culling_mgr.for_in_range(vector3d, object_collision_radius_, &camera_collision_checker);
            bool collide = camera_collision_checker.is_blocked;

            if (collide) {
                if (angular_velocity_.x < 0.0f) {
                    angular_velocity_.x = 0.0f;
                    angular_velocity_.y += move_amount_large;
                }
            }
        }

        {
            check_vector = eye_ - 2.0f * terrain_collision_radius_ * up_;
            s.position_value = check_vector;

            std::vector<math::vector3> vct_position;
            camera_collision_checker camera_collision_checker(&s, &vct_position);
            culling_mgr.for_in_range(vector3d, object_collision_radius_, &camera_collision_checker);
            bool collide = camera_collision_checker.is_blocked;

            if (collide) {
                if (angular_velocity_.z < 0.0f) {
                    angular_velocity_.z = 0.0f;
                    angular_velocity_.y += move_amount_large;
                }
            }
        }

        {
            check_vector = eye_ + 4.0f * object_collision_radius_ * view_;
            s.position_value = check_vector;

            std::vector<math::vector3> vct_position;
            camera_collision_checker camera_collision_checker(&s, &vct_position);
            culling_mgr.for_in_range(vector3d, object_collision_radius_, &camera_collision_checker);
            bool collide = camera_collision_checker.is_blocked;

            if (collide) {
                if (angular_velocity_.y < 0.0f) {
                    angular_velocity_.y = 0.0f;
                    angular_velocity_.z += move_amount_large;
                }

                if (vct_position.size() > 1) {
                    angular_velocity_.z += move_amount_large;
                } else {
                    auto diff_vector = (vct_position[0] - eye_);
                    check_vector = math::cross(diff_vector, view_);
                    float dot = math::dot(check_vector, up_);
                    if (dot < 0) {
                        angular_velocity_.x -= move_amount_small;
                    } else if (dot > 0) {
                        angular_velocity_.x += move_amount_small;
                    } else {
                        angular_velocity_.z += move_amount_small;
                    }
                }
            }
        }
    }

    void camera::update() {

        rotate_eye_around_target(angular_velocity_.z, angular_velocity_.x);

        float new_distance = f_max(CAMERA_MIN_DISTANCE, f_min(CAMERA_MAX_DISTANCE, distance() - angular_velocity_.y));
        set_distance(new_distance);

        if (process_terrain_collision_)
            process_terrain_collision();

        angular_velocity_ *= 0.5f;
        if (fabs(angular_velocity_.x) < 1.0f)
            angular_velocity_.x = 0.0f;
        if (fabs(angular_velocity_.y) < 1.0f)
            angular_velocity_.y = 0.0f;
        if (fabs(angular_velocity_.z) < 1.0f)
            angular_velocity_.z = 0.0f;

        const float camera_movable_distance = CAMERA_MAX_DISTANCE - CAMERA_MIN_DISTANCE;
        const float camera_target_delta = CAMERA_TARGET_FACE - CAMERA_TARGET_STANDARD;
        float camera_cur_movable_distance = CAMERA_MAX_DISTANCE - distance();
        float new_target_height = CAMERA_TARGET_STANDARD + camera_target_delta * camera_cur_movable_distance / camera_movable_distance;

        set_target_height(new_target_height);
    }
}
