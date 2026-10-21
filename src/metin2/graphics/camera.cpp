#include "camera.h"
#include <metin2/core/utils.h>

namespace metin2::inline graphics {

    const float c_fDefaultResistance = 0.3f;

    void camera::set_camera_max_distance(float max_value) {
        CAMERA_MAX_DISTANCE = max_value;
    }

    auto camera::target_height() -> float {
        return target_distance_;
    }

    void camera::set_target_height(float target) {
        target_distance_ = target;
    }

    camera::camera() {
        eye_ground_height_ratio_ = 0.3f;
        target_height_limit_ratio_ = 2.0f;
        resistance_ = c_fDefaultResistance;
        is_lock_ = false;
        distance_ = 1.0f;
        camera_state_ = CAMERA_STATE_NORMAL;
        camera_state_prev_ = CAMERA_STATE_NORMAL;
        num_screen_building_ = 0;

        pitch_sum_ = 0.0f;
        roll_sum_ = 0.0f;

        terrain_collision_radius_ = 50.0f;
        object_collision_radius_ = 50.0f;

        drag_ = false;

        mouse_pos_x_ = -1;
        mouse_pos_y_ = -1;

        target_distance_ = CAMERA_TARGET_STANDARD;

        angular_acceleration_ = math::vector3(0.0f, 0.0f, 0.0f);
        angular_velocity_ = math::vector3(0.0f, 0.0f, 0.0f);

        process_terrain_collision_ = true;

        set_view_params(math::vector3(0.0f, 0.0f, 1.0f), math::vector3(0.0f, 0.0f, 0.0f), math::vector3(0.0f, 1.0f, 0.0f));
    }

    camera::~camera() = default;

    void camera::lock() {
        is_lock_ = true;
    }

    void camera::unlock() {
        is_lock_ = false;
    }

    auto camera::is_lock() -> bool {
        return is_lock_;
    }

    void camera::set_resistance(float resistance) {
        resistance_ = c_fDefaultResistance * resistance;
    }

    void camera::wheel(int len) {
        if (is_lock())
            return;

        angular_velocity_.y = (float)(len)*resistance_;
    }

    void camera::begin_drag(int mouse_x, int mouse_y) {
        if (is_lock())
            return;

        drag_ = true;
        mouse_pos_x_ = mouse_x;
        mouse_pos_y_ = mouse_y;
        pitch_sum_ = 0.0f;
        roll_sum_ = 0.0f;
    }

    auto camera::is_draging() -> bool {
        if (is_lock())
            return false;

        return drag_;
    }

    auto camera::end_drag() -> bool {
        if (is_lock())
            return false;

        drag_ = false;

        float sum = sqrt(pitch_sum_ * pitch_sum_ + roll_sum_ * roll_sum_);

        pitch_sum_ = 0.0f;
        roll_sum_ = 0.0f;

        if (sum < 1.0f)
            return false;

        return true;
    }

    auto camera::drag(int n_mouse_x, int n_mouse_y, int &return_x, int &return_y) -> bool {
        if (is_lock())
            return false;

        if (!drag_) {
            mouse_pos_x_ = n_mouse_x;
            mouse_pos_y_ = n_mouse_y;
            return_x = mouse_pos_x_;
            return_y = mouse_pos_y_;
            return false;
        }

        int32_t l_mouse_x = n_mouse_x;
        int32_t l_mouse_y = n_mouse_y;

        float new_pitch_velocity = (float)(l_mouse_y - mouse_pos_y_) * resistance_;
        float new_rotation_velocity = (float)(l_mouse_x - mouse_pos_x_) * resistance_;

        pitch_sum_ += new_pitch_velocity;
        roll_sum_ += new_rotation_velocity;

        if (CAMERA_STATE_CANTGOLEFT == camera_state())
            new_rotation_velocity = f_max(0.0f, new_rotation_velocity);
        if (CAMERA_STATE_CANTGORIGHT == camera_state())
            new_rotation_velocity = f_min(0.0f, new_rotation_velocity);
        if (CAMERA_STATE_CANTGODOWN == camera_state())
            new_pitch_velocity = f_max(0.0f, new_pitch_velocity);

        angular_velocity_.x = new_rotation_velocity;
        angular_velocity_.z = new_pitch_velocity;

        return_x = mouse_pos_x_;
        return_y = mouse_pos_y_;
        return true;
    }

    void camera::set_camera_state(e_camera_state new_camera_state) {
        if (new_camera_state == camera_state_)
            return;

        camera_state_prev_ = camera_state_;
        camera_state_ = new_camera_state;
    }

    void camera::set_view_params(const math::vector3 &eye, const math::vector3 &target, const math::vector3 &up) {
        if (is_lock())
            return;

        eye_ = eye;
        target_ = target;
        up_ = up;

        set_view_matrix();
    }

    void camera::set_eye(const math::vector3 &eye) {
        if (is_lock())
            return;

        eye_ = eye;

        set_view_matrix();
    }

    void camera::set_target(const math::vector3 &target) {
        if (is_lock())
            return;

        target_ = target;

        set_view_matrix();
    }

    void camera::set_up(const math::vector3 &up) {
        if (is_lock())
            return;

        up_ = up;

        set_view_matrix();
    }

    void camera::set_view_matrix() {
        view_ = target_ - eye_;
        math::vector3 center_ray = -view_;
        calculate_roll();
        distance_ = math::length(view_);
        assert(distance_ >= 0);
        view_ = math::normalize(view_);

        cross_ = math::cross(up_, view_);
        cross_ = math::normalize(cross_);

        up_ = math::cross(view_, cross_);
        up_ = math::normalize(up_);
        const auto vv = math::vector3(0.0f, 0.0f, 1.0f);
        pitch_ = math::dot(up_, vv);
        if (pitch_ >= 1)
            pitch_ = 1;
        else if (pitch_ <= -1)
            pitch_ = -1;
        pitch_ = acosf(pitch_);
        pitch_ *= (180.0f / math::PI);
        if (0 < view_.z)
            pitch_ = -pitch_;

        view_matrix_ = math::matrix_look_at_rh(eye_, target_, up_);

        float view_determinant = math::determinant(view_matrix_);
        math::inverse(view_matrix_, inverse_view_, &view_determinant);

        billboard_ = inverse_view_;
        billboard_._41 = 0.0f;
        billboard_._42 = 0.0f;
        billboard_._43 = 0.0f;

        view_ray_.set_start_point(target_);
        view_ray_.set_direction(center_ray, distance_);

        camera_bottom_to_terrain_ray_.set_start_point(eye_);
        camera_front_to_terrain_ray_.set_start_point(eye_);
        camera_back_to_terrain_ray_.set_start_point(eye_);
        camera_left_to_terrain_ray_.set_start_point(eye_);
        camera_right_to_terrain_ray_.set_start_point(eye_);
        target_to_camera_bottom_ray_.set_start_point(target_);

        camera_bottom_to_terrain_ray_.set_direction(-up_, 2.0f * terrain_collision_radius_);
        camera_front_to_terrain_ray_.set_direction(view_, 4.0f * terrain_collision_radius_);
        camera_back_to_terrain_ray_.set_direction(-view_, terrain_collision_radius_);
        camera_left_to_terrain_ray_.set_direction(-cross_, 3.0f * terrain_collision_radius_);
        camera_right_to_terrain_ray_.set_direction(cross_, 3.0f * terrain_collision_radius_);
        const auto vv2 = (center_ray - terrain_collision_radius_ * up_);
        target_to_camera_bottom_ray_.set_direction(center_ray - terrain_collision_radius_ * up_, math::length(vv2));

        left_object_collision_ray_.set_start_point(target_);
        top_object_collision_ray_.set_start_point(target_);
        right_object_collision_ray_.set_start_point(target_);
        bottom_object_collision_ray_.set_start_point(target_);
        const auto vv3 = (center_ray + object_collision_radius_ * cross_);
        const auto vv4 = (center_ray - object_collision_radius_ * cross_);
        const auto vv5 = (center_ray + object_collision_radius_ * up_);
        const auto vv6 = (center_ray + object_collision_radius_ * up_);
        left_object_collision_ray_.set_direction(center_ray + object_collision_radius_ * cross_, math::length(vv3));
        right_object_collision_ray_.set_direction(center_ray - object_collision_radius_ * cross_, math::length(vv4));
        top_object_collision_ray_.set_direction(center_ray + object_collision_radius_ * up_, math::length(vv5));
        bottom_object_collision_ray_.set_direction(center_ray - object_collision_radius_ * up_, math::length(vv6));
    }

    void camera::move(const math::vector3 &displacement) {
        if (is_lock())
            return;

        eye_ += displacement;
        target_ += displacement;

        set_view_matrix();
    }

    void camera::zoom(float ratio) {
        if (is_lock())
            return;

        if (ratio == 1.0f)
            return;

        math::vector3 temp = eye_ - target_;
        temp *= ratio;
        eye_ = temp + target_;

        set_view_matrix();
    }

    void camera::move_along_cross(float distance) {
        if (is_lock())
            return;

        math::vector3 temp;
        temp = math::normalize(cross_);

        eye_ += temp * distance;
        target_ += temp * distance;

        set_view_matrix();
    }

    void camera::move_front(float distance) {
        if (is_lock())
            return;

        math::vector3 temp = math::vector3(view_.x, view_.y, 0.0f);
        temp = math::normalize(temp);

        eye_ += temp * distance;
        target_ += temp * distance;

        set_view_matrix();
    }

    void camera::move_vertical(float distance) {
        if (is_lock())
            return;

        eye_.z += distance;
        target_.z += distance;

        set_view_matrix();
    }

    void camera::rotate_eye_around_target(float pitch_degree, float roll_degree) {
        if (is_lock())
            return;

        math::matrix rot, rot_pitch, rot_roll;

        if (pitch_ + pitch_degree > 80.0f) {
            pitch_degree = 80.0f - pitch_;
        } else if (pitch_ + pitch_degree < -80.0f) {
            pitch_degree = -80.0f - pitch_;
        }

        rot_pitch = math::matrix_rotation_axis(cross_, math::to_radian(pitch_degree));

        rot_roll = math::matrix_rotation_z(-math::to_radian(roll_degree));
        rot = rot_pitch * rot_roll;

        math::vector3 temp = eye_ - target_;
        eye_ = math::transform_coord(temp, rot);
        eye_ += target_;

        set_up(math::vector3(0.0f, 0.0f, 1.0f));

        roll_ += roll_degree;

        if (roll_ > 360.0f)
            roll_ -= 360.0f;
        else if (roll_ < -360.0f)
            roll_ += 360.0f;
    }

    void camera::pitch(const float pitch_delta) {

        rotate_eye_around_target(pitch_delta, 0.0f);
    }

    void camera::roll(const float roll_delta) {

        rotate_eye_around_target(0.0f, roll_delta);
    }

    void camera::set_distance(const float fdistance) {

        zoom(fdistance / distance_);
    }

    void camera::calculate_roll() {
        math::vector2 view_xy;
        view_xy.x = view_.x;
        view_xy.y = view_.y;
        view_xy = math::normalize(view_xy);
        const auto vv = math::vector2(0.0f, 1.0f);
        float dot = math::dot(view_xy, vv);
        if (dot >= 1)
            dot = 1;
        else if (dot <= -1)
            dot = -1;
        dot = acosf(dot);
        dot *= (180.0f / math::PI);
        float cross = math::ccw(view_xy, vv);
        if (0 > cross) {
            dot = -dot;
        }

        roll_ = dot;
    }

    camera_manager::camera_manager() : current_camera_(nullptr),
                                       previous_camera_(nullptr) {
        add_camera(DEFAULT_PERSPECTIVE_CAMERA);
        add_camera(DEFAULT_ORTHO_CAMERA);

        set_current_camera(DEFAULT_PERSPECTIVE_CAMERA);
    }

    camera_manager::~camera_manager() = default;

    auto camera_manager::current_camera() -> camera * {
        if (!current_camera_)
            assert(false);
        return current_camera_;
    }

    void camera_manager::set_current_camera(unsigned char camera_num_value) {
        camera *camera = camera_map_value_[camera_num_value].get();
        if (current_camera_ != camera)
            previous_camera_ = current_camera_;

        current_camera_ = camera;
    }

    auto camera_manager::add_camera(unsigned char camera_num_value) -> bool {
        if (camera_map_value_.end() != camera_map_value_.find(camera_num_value))
            return false;
        camera_map_value_.emplace(camera_num_value, std::make_unique<camera>());
        return true;
    }

    auto camera_manager::current_camera_num() -> unsigned char {
        if (!current_camera_)
            return NO_CURRENT_CAMERA;
        for (auto itor = camera_map_value_.begin(); itor != camera_map_value_.end(); ++itor)
            if (current_camera_ == (*itor).second.get())
                return (*itor).first;
        return NO_CURRENT_CAMERA;
    }

    auto camera_manager::is_terrain_collision_enable() -> bool {
        return current_camera_->is_terrain_collision_enable();
    }

    void camera_manager::set_terrain_collision(bool enable) {
        current_camera_->set_terrain_collision(enable);
    }
}
