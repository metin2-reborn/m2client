#pragma once

#include <map>
#include <memory>

#include <metin2/geometry/ray.h>

namespace metin2::inline graphics {

    const float CAMERA_TARGET_STANDARD = 100.0f;
    const float CAMERA_TARGET_FACE = 150.0f;

    enum e_camera_state {
        CAMERA_STATE_NORMAL,
        CAMERA_STATE_CANTGODOWN,
        CAMERA_STATE_CANTGORIGHT,
        CAMERA_STATE_CANTGOLEFT,
        CAMERA_STATE_SCREEN_BY_BUILDING,
        CAMERA_STATE_SCREEN_BY_BUILDING_AND_TOOCLOSE,
    };

    class camera {
      public:
        camera();
        virtual ~camera();

        static void set_camera_max_distance(float max_value);

        void lock();
        void unlock();
        auto is_lock() -> bool;

        void wheel(int wheel_len);
        auto drag(int mouse_x, int mouse_y, int &return_x, int &return_y) -> bool;

        auto end_drag() -> bool;
        void begin_drag(int mouse_x, int mouse_y);

        auto is_draging() -> bool;

        void set_resistance(float resistance);

      private:
        auto operator=(const camera &) -> const camera &;
        camera(const camera &);

        e_camera_state camera_state_;
        e_camera_state camera_state_prev_;
        float pitch_backup_;
        float roll_backup_;
        float distance_backup_;
        float target_z_back_up_;

        math::vector3 eye_backup_;

        uint32_t num_screen_building_;

        bool is_lock_;

        math::vector3 eye_;
        math::vector3 target_;
        math::vector3 up_;

        // view_ = target_ - eye_
        math::vector3 view_;
        // cross_ = cross(up_, view_)
        math::vector3 cross_;

        math::matrix view_matrix_;
        math::matrix inverse_view_;
        math::matrix billboard_; // Special matrix for billboarding effects

        float pitch_;
        float roll_;
        float distance_;

        ray camera_bottom_to_terrain_ray_;
        ray camera_front_to_terrain_ray_;
        ray camera_back_to_terrain_ray_;
        ray camera_left_to_terrain_ray_;
        ray camera_right_to_terrain_ray_;

        ray target_to_camera_bottom_ray_;

        ray view_ray_;
        ray left_object_collision_ray_;
        ray top_object_collision_ray_;
        ray right_object_collision_ray_;
        ray bottom_object_collision_ray_;

        float terrain_collision_radius_;
        float object_collision_radius_;

        float target_distance_;

        float eye_ground_height_ratio_;
        float target_height_limit_ratio_;
        float pitch_sum_;
        float roll_sum_;

        int32_t mouse_pos_x_;
        int32_t mouse_pos_y_;

        bool drag_;

        math::vector3 angular_acceleration_;
        math::vector3 angular_velocity_;

        float resistance_;

      public:
        auto view_ray() -> ray & { return view_ray_; }
        auto left_object_collision_ray() -> ray & { return left_object_collision_ray_; }
        auto right_object_collision_ray() -> ray & { return right_object_collision_ray_; }
        auto top_object_collision_ray() -> ray & { return top_object_collision_ray_; }
        auto bottom_object_collision_ray() -> ray & { return bottom_object_collision_ray_; }

        void update();
        auto camera_state() -> e_camera_state { return camera_state_; }
        void set_camera_state(e_camera_state new_camera_state);
        auto num_screen_building() -> uint32_t & { return num_screen_building_; }

        auto pitch_back_up() -> const float & { return pitch_backup_; }
        auto roll_back_up() -> const float & { return roll_backup_; }
        auto distance_back_up() -> const float & { return distance_backup_; }

        auto eye() const -> const math::vector3 & { return eye_; }
        auto target() const -> const math::vector3 & { return target_; }
        auto up() const -> const math::vector3 & { return up_; }
        auto view() const -> const math::vector3 & { return view_; }
        auto cross() const -> const math::vector3 & { return cross_; }

        auto view_matrix() const -> const math::matrix & { return view_matrix_; }
        auto inverse_view_matrix() const -> const math::matrix & { return inverse_view_; }
        auto billboard_matrix() const -> const math::matrix & { return billboard_; }

        void set_view_params(const math::vector3 &eye, const math::vector3 &target, const math::vector3 &up);

        void set_eye(const math::vector3 &eye);
        void set_target(const math::vector3 &target);
        void set_up(const math::vector3 &up);

        auto get_pitch() const -> float { return pitch_; }
        auto get_roll() const -> float { return roll_; }
        auto distance() const -> float { return distance_; }

        void pitch(const float pitch_delta);
        void roll(const float roll_delta);
        void set_distance(const float fdistance);

        // Moves both the eye and the target.
        void move(const math::vector3 &displacement);
        // Moves only the eye; the target stays.
        void zoom(float ratio);

        void move_along_cross(float distance);

        // Moves in the XY plane along the view direction with its Z component dropped.
        void move_front(float distance);
        void move_vertical(float distance);

        // pitch_degree is the angle below the horizon, roll_degree the clockwise angle around the target.
        void rotate_eye_around_target(float pitch_degree, float roll_degree);

      protected:
        void set_view_matrix();
        void calculate_roll();

      public:
        auto target_height() -> float;
        void set_target_height(float target);
        auto is_terrain_collision_enable() -> bool { return process_terrain_collision_; }
        void set_terrain_collision(bool enable) { process_terrain_collision_ = enable; }

      private:
        void process_terrain_collision();
        void process_building_collision();

      private:
        bool process_terrain_collision_;

        static float CAMERA_MIN_DISTANCE;
        static float CAMERA_MAX_DISTANCE;
    };

    using camera_map = std::map<uint8_t, std::unique_ptr<camera>>;

    class camera_manager {
      public:
        enum camera_num {
            NO_CURRENT_CAMERA,
            DEFAULT_PERSPECTIVE_CAMERA,
            DEFAULT_ORTHO_CAMERA,
            CAMERA_MAX = 255
        };

        camera_manager();
        ~camera_manager();

        auto add_camera(unsigned char camera_num_value) -> bool;

        auto current_camera() -> camera *;
        void set_current_camera(unsigned char camera_num_value);

        auto current_camera_num() -> unsigned char;

        auto is_terrain_collision_enable() -> bool;
        void set_terrain_collision(bool enable);

      private:
        camera_map camera_map_value_;
        camera *current_camera_;
        camera *previous_camera_;
    };

    inline auto get_camera_manager() -> camera_manager & {
        static camera_manager instance;
        return instance;
    }
}
