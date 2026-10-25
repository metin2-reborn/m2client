#include "python_application.h"

#include <metin2/core/parse.h>
#include <metin2/core/text_file_loader.h>
#include <metin2/core/timer.h>
#include <metin2/graphics/camera.h>

#include <cmath>

namespace metin2::inline client::inline app {
    auto blend_value_by_linear(float elapsed_time, float duration, float begin_value, float end_value) -> float {
        if (elapsed_time >= duration) {
            return end_value;
        }

        return (end_value - begin_value) * (elapsed_time / duration) + begin_value;
    }

    void python_application::update_camera() {
        auto *main_camera = get_camera_manager().current_camera();
        if (!main_camera) {
            return;
        }

        if (CAMERA_MODE_BLEND == camera_mode_) {
            float fcur_time = get_timer().current_second();
            float elapsed_time = fcur_time - blend_camera_start_time_;

            float fx_center = blend_value_by_linear(elapsed_time, blend_camera_blend_time_, event_camera_setting_.center_position.x, end_blend_camera_setting_.center_position.x);
            float fy_center = blend_value_by_linear(elapsed_time, blend_camera_blend_time_, event_camera_setting_.center_position.y, end_blend_camera_setting_.center_position.y);
            float fz_center = blend_value_by_linear(elapsed_time, blend_camera_blend_time_, event_camera_setting_.center_position.z, end_blend_camera_setting_.center_position.z);

            float distance = blend_value_by_linear(elapsed_time, blend_camera_blend_time_, event_camera_setting_.zoom, end_blend_camera_setting_.zoom);
            float pitch = blend_value_by_linear(elapsed_time, blend_camera_blend_time_, event_camera_setting_.pitch, end_blend_camera_setting_.pitch);
            float rotation = blend_value_by_linear(elapsed_time, blend_camera_blend_time_, event_camera_setting_.rotation, end_blend_camera_setting_.rotation);

            float up_dir = blend_value_by_linear(elapsed_time, blend_camera_blend_time_, event_camera_setting_.cmr_pos.up_dir, end_blend_camera_setting_.cmr_pos.up_dir);
            float view_dir = blend_value_by_linear(elapsed_time, blend_camera_blend_time_, event_camera_setting_.cmr_pos.view_dir, end_blend_camera_setting_.cmr_pos.view_dir);
            float cross_dir = blend_value_by_linear(elapsed_time, blend_camera_blend_time_, event_camera_setting_.cmr_pos.cross_dir, end_blend_camera_setting_.cmr_pos.cross_dir);

            // TODO: the camera class should handle this.
            main_camera->unlock();

            get_python_graphic().set_position_camera(fx_center, fy_center, fz_center, distance, pitch, rotation);

            main_camera->move_vertical(up_dir);
            main_camera->move_front(view_dir);
            main_camera->move_along_cross(cross_dir);
            main_camera->lock();
        } else if (CAMERA_MODE_STAND == camera_mode_) {
            float distance, pitch, rotation, height;

            get_camera(&distance, &pitch, &rotation, &height);
            get_python_graphic().set_position_camera(event_camera_setting_.center_position.x,
                event_camera_setting_.center_position.y,
                event_camera_setting_.center_position.z + main_camera->target_height(),
                distance, pitch, rotation);
        } else if (CAMERA_MODE_NORMAL == camera_mode_) {
            float distance, pitch, rotation, height;

            get_camera(&distance, &pitch, &rotation, &height);
            get_python_graphic().set_position_camera(center_position_.x, center_position_.y, center_position_.z + main_camera->target_height(), distance, pitch, rotation);
        }

        // The key speeds are per frame at 60 fps.
        const float frames = get_timer().elapsed_second() * 60.0f;
        if (0.0f != rotation_speed_) main_camera->roll(rotation_speed_ * frames);
        if (0.0f != pitch_speed_) main_camera->pitch(pitch_speed_ * frames);
        if (0.0f != zoom_speed_) main_camera->zoom(std::pow(zoom_speed_, frames));

        if (0.0f != cmr_spd_.view_dir) cmr_pos_.view_dir += cmr_spd_.view_dir * frames;
        if (0.0f != cmr_spd_.cross_dir) cmr_pos_.cross_dir += cmr_spd_.cross_dir * frames;
        if (0.0f != cmr_spd_.up_dir) cmr_pos_.up_dir += cmr_spd_.up_dir * frames;

        if (0.0f != cmr_pos_.view_dir) main_camera->move_front(cmr_pos_.view_dir);
        if (0.0f != cmr_pos_.cross_dir) main_camera->move_along_cross(cmr_pos_.cross_dir);
        if (0.0f != cmr_pos_.up_dir) main_camera->move_vertical(cmr_pos_.up_dir);

        if (main_camera->is_draging()) {
            skip_render_buffering(3000);
        }

        const auto &camera_direction = main_camera->view();
        const auto &camera_up = main_camera->up();

        get_sound_manager().set_position(center_position_.x, center_position_.y, center_position_.z); // the listener is at the character
        get_sound_manager().set_direction(camera_direction.x, camera_direction.y, camera_direction.z, camera_up.x, camera_up.y, camera_up.z);
        get_sound_manager().update();
    }

    void python_application::set_view_dir_camera_speed(float speed) {
        if (is_lock_current_camera()) {
            return;
        }

        cmr_spd_.view_dir = speed;
    }

    void python_application::set_cross_dir_camera_speed(float speed) {
        if (is_lock_current_camera()) {
            return;
        }

        cmr_spd_.cross_dir = speed;
    }

    void python_application::set_up_dir_camera_speed(float speed) {
        if (is_lock_current_camera()) {
            return;
        }

        cmr_spd_.up_dir = speed;
    }

    void python_application::set_camera(float distance, float pitch, float rotation, float destination_height) {
        if (is_lock_current_camera()) {
            return;
        }

        auto *current_camera = get_camera_manager().current_camera();
        if (!current_camera) {
            return;
        }

        auto target = current_camera->target();

        get_python_graphic().set_position_camera(target.x, target.y, target.z, distance, pitch, rotation);

        auto *main_camera = get_camera_manager().current_camera();
        if (main_camera) {
            main_camera->set_target_height(destination_height);
        }
    }

    void python_application::get_camera(float *distance, float *pitch, float *rotation, float *destination_height) {
        auto *current_camera = get_camera_manager().current_camera();

        *distance = current_camera->distance();
        *pitch = current_camera->get_pitch();
        *rotation = current_camera->get_roll();
        *destination_height = current_camera->target().z;
    }

    void python_application::rotate_camera(int direction) {
        if (is_lock_current_camera()) {
            return;
        }

        float degree = camera_rotate_speed_ * float(direction);

        rotation_speed_ = degree;
    }

    void python_application::pitch_camera(int direction) {
        if (is_lock_current_camera()) {
            return;
        }

        float degree = camera_pitch_speed_ * float(direction);

        pitch_speed_ = degree;
    }

    void python_application::zoom_camera(int direction) {
        if (is_lock_current_camera()) {
            return;
        }

        float ratio = 1.0f + camera_zoom_speed_ * float(direction);

        zoom_speed_ = ratio;
    }

    void python_application::movie_rotate_camera(int direction) {
        if (is_lock_current_camera()) {
            return;
        }

        float degree = camera_rotate_speed_ * float(direction);

        if (is_special_camera_mode_) {
            if (scroll_lock()) {
                set_cross_dir_camera_speed(-degree * 6.0f);
                return;
            }
        }

        rotation_speed_ = degree;
    }

    void python_application::movie_pitch_camera(int direction) {
        if (is_lock_current_camera()) {
            return;
        }

        float degree = camera_pitch_speed_ * float(direction);

        if (is_special_camera_mode_) {
            if (scroll_lock()) {
                set_view_dir_camera_speed(-degree * 6.0f);
                return;
            }
        }

        pitch_speed_ = degree;
    }

    void python_application::movie_zoom_camera(int direction) {
        if (is_lock_current_camera()) {
            return;
        }

        float ratio = 1.0f + camera_zoom_speed_ * float(direction);

        if (is_special_camera_mode_) {
            if (scroll_lock()) {
                set_up_dir_camera_speed((1.0f - ratio) * 200.0f);
                return;
            }
        }

        zoom_speed_ = ratio;
    }

    void python_application::movie_reset_camera() {
        if (is_lock_current_camera()) {
            return;
        }

        if (is_special_camera_mode_) {
            set_cross_dir_camera_speed(0.0f);
            set_view_dir_camera_speed(0.0f);
            set_up_dir_camera_speed(0.0f);

            cmr_pos_.view_dir = 0;
            cmr_pos_.cross_dir = 0;
            cmr_pos_.up_dir = 0;
        }
    }

    auto python_application::rotation() -> float {
        return get_camera_manager().current_camera()->get_roll();
    }

    auto python_application::get_pitch() -> float {
        return get_camera_manager().current_camera()->get_pitch();
    }

    auto python_application::is_lock_current_camera() -> bool {
        auto *camera = get_camera_manager().current_camera();
        if (!camera) {
            return false;
        }

        return camera->is_lock();
    }

    void python_application::set_event_camera(const camera_setting &camera_setting) {
        if (camera_manager::DEFAULT_PERSPECTIVE_CAMERA == get_camera_manager().current_camera_num()) {
            get_camera_setting(&default_camera_setting_);
        }

        get_camera_manager().set_current_camera(EVENT_CAMERA_NUMBER);

        auto *camera = get_camera_manager().current_camera();
        if (!camera) {
            return;
        }

        set_camera_setting(camera_setting);

        event_camera_setting_ = camera_setting;
        camera_mode_ = CAMERA_MODE_STAND;
    }

    void python_application::blend_event_camera(const camera_setting &camera_setting, float blend_time) {
        camera_mode_ = CAMERA_MODE_BLEND;
        blend_camera_start_time_ = get_timer().current_second();
        blend_camera_blend_time_ = blend_time;
        end_blend_camera_setting_ = camera_setting;

        auto *camera = get_camera_manager().current_camera();
        if (camera) {
            camera->lock();
        }
    }

    void python_application::set_default_camera() {
        camera_mode_ = CAMERA_MODE_NORMAL;
        blend_camera_start_time_ = 0.0f;
        blend_camera_blend_time_ = 0.0f;

        auto *camera = get_camera_manager().current_camera();
        if (camera) {
            camera->unlock();
        }

        if (camera_manager::DEFAULT_PERSPECTIVE_CAMERA != get_camera_manager().current_camera_num()) {
            get_camera_manager().set_current_camera(camera_manager::DEFAULT_PERSPECTIVE_CAMERA);
            set_camera_setting(default_camera_setting_);
        }
    }

    void python_application::set_camera_setting(const camera_setting &camera_setting) {
        auto *camera = get_camera_manager().current_camera();
        if (!camera) {
            return;
        }

        get_python_graphic().set_position_camera(
            camera_setting.center_position.x,
            camera_setting.center_position.y,
            camera_setting.center_position.z,
            camera_setting.zoom,
            camera_setting.pitch,
            camera_setting.rotation);

        if (0.0f != camera_setting.cmr_pos.view_dir) camera->move_front(camera_setting.cmr_pos.view_dir);
        if (0.0f != camera_setting.cmr_pos.cross_dir) camera->move_along_cross(camera_setting.cmr_pos.cross_dir);
        if (0.0f != camera_setting.cmr_pos.up_dir) camera->move_vertical(camera_setting.cmr_pos.up_dir);

        cmr_pos_.up_dir = 0.0f;
        cmr_pos_.view_dir = 0.0f;
        cmr_pos_.cross_dir = 0.0f;
        cmr_spd_.up_dir = 0.0f;
        cmr_spd_.view_dir = 0.0f;
        cmr_spd_.cross_dir = 0.0f;
        zoom_speed_ = 0.0f;
        pitch_speed_ = 0.0f;
        rotation_speed_ = 0.0f;
    }

    void python_application::get_camera_setting(camera_setting *camera_setting) {
        camera_setting->center_position = center_position_;
        camera_setting->cmr_pos = cmr_pos_;

        if (get_camera_manager().current_camera()) {
            camera_setting->center_position.z += get_camera_manager().current_camera()->target_height();
        }

        float height;

        get_camera(&camera_setting->zoom, &camera_setting->pitch, &camera_setting->rotation, &height);
    }

    void python_application::save_camera_setting(const char *file_name) {
        camera_setting camera_setting;

        get_camera_setting(&camera_setting);

        auto *file = fopen(file_name, "w");

        printf_tabs(file, 0, "CenterPos %f %f %f\n", camera_setting.center_position.x, camera_setting.center_position.y, camera_setting.center_position.z);
        printf_tabs(file, 0, "CameraSetting %f %f %f\n", camera_setting.zoom, camera_setting.pitch, camera_setting.rotation);
        printf_tabs(file, 0, "CmrPos %f %f %f\n", camera_setting.cmr_pos.up_dir, camera_setting.cmr_pos.view_dir, camera_setting.cmr_pos.cross_dir);
        printf_tabs(file, 0, "Line \"x;%d|y;%d|z;%d|distance;%d|pitch;%d|rot;%d|up;%d|view;%d|cross;%d\"\n",
            int(camera_setting.center_position.x),
            int(camera_setting.center_position.y),
            int(camera_setting.center_position.z),
            int(camera_setting.zoom),
            int(camera_setting.pitch),
            int(camera_setting.rotation),
            int(camera_setting.cmr_pos.up_dir),
            int(camera_setting.cmr_pos.view_dir),
            int(camera_setting.cmr_pos.cross_dir));

        fclose(file);
    }

    auto python_application::load_camera_setting(const char *file_name) -> bool {
        text_file_loader text_file_loader;
        if (!text_file_loader.load(file_name)) {
            return false;
        }

        text_file_loader.set_top();

        math::vector3 center_position;

        token_vector *p_camera_setting;
        token_vector *cmr_pos;

        if (text_file_loader.token_vector3("centerpos", &center_position)) {
            if (text_file_loader.get_token_vector("camerasetting", &p_camera_setting)) {
                if (text_file_loader.get_token_vector("cmrpos", &cmr_pos)) {
                    if (3 == p_camera_setting->size()) {
                        if (3 == cmr_pos->size()) {
                            camera_setting camera_setting;
                            camera_setting.center_position = center_position;

                            camera_setting.zoom = metin2::to_number<double>(p_camera_setting->at(0).c_str());
                            camera_setting.pitch = metin2::to_number<double>(p_camera_setting->at(1).c_str());
                            camera_setting.rotation = metin2::to_number<double>(p_camera_setting->at(2).c_str());

                            camera_setting.cmr_pos.up_dir = metin2::to_number<double>(cmr_pos->at(0).c_str());
                            camera_setting.cmr_pos.view_dir = metin2::to_number<double>(cmr_pos->at(1).c_str());
                            camera_setting.cmr_pos.cross_dir = metin2::to_number<double>(cmr_pos->at(2).c_str());

                            set_event_camera(camera_setting);
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }
}
