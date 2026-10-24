#include "particle_instance.h"
#include "particle_property.h"

#include <metin2/core/random.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/state_manager.h>

namespace metin2::inline effects {

    dynamic_pool<particle_instance> particle_instance::pool_value;

    using namespace update_decorators;

    void particle_instance::destroy_system() {
        pool_value.destroy();
    }

    auto particle_instance::new_value() -> particle_instance * {
        return pool_value.alloc();
    }

    void particle_instance::delete_this() {
        destroy();

        pool_value.free(this);
    }

    auto particle_instance::radius_approximation() -> float {
        return half_size_.y * scale_.y + half_size_.x * scale_.x;
    }
    auto particle_instance::update(float elapsed_time, float angle) -> bool {
        last_life_time_ -= elapsed_time;
        if (last_life_time_ < 0.0f)
            return false;

        float life_percentage = (life_time_ - last_life_time_) / life_time_;

        decorator_->excute(decorator_data(life_percentage, elapsed_time, this));

        last_position_ = position_value_;
        position_value_ += velocity_ * elapsed_time;

        if (angle) {
            // The emitter's angular velocity is in degrees per frame at 60 fps.
            angle *= elapsed_time * 60.0f;
            if (particle_property_value_->attach_flag) {
                float cos_value, sin_value;
                angle = math::to_radian(angle);
                cos_value = cos(angle);
                sin_value = sin(angle);

                float rx = position_value_.x - start_position_.x;
                float ry = position_value_.y - start_position_.y;

                position_value_.x = cos_value * rx + sin_value * ry + start_position_.x;
                position_value_.y = -sin_value * rx + cos_value * ry + start_position_.y;
            } else {
                math::quaternion q, qc;
                q = math::quaternion_rotation_axis(particle_property_value_->z_axis, math::to_radian(angle));
                qc = math::conjugate(q);

                math::quaternion qr(
                    position_value_.x - start_position_.x,
                    position_value_.y - start_position_.y,
                    position_value_.z - start_position_.z,
                    0.0f);
                qr = q * qr;
                qr = qr * qc;

                position_value_.x = qr.x;
                position_value_.y = qr.y;
                position_value_.z = qr.z;

                position_value_ += start_position_;
            }
        }

        return true;
    }

    void particle_instance::transform(const math::matrix *local) {
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, dc_color_);

        math::vector3 v3_up;
        math::vector3 v3_cross;

        if (!particle_property_value_->stretch_flag) {
            camera *current_camera = get_camera_manager().current_camera();
            const math::vector3 &c_rv3_up = current_camera->up();
            const math::vector3 &c_rv3_cross = current_camera->cross();

            switch (particle_property_value_->billboard_type) {
            case BILLBOARD_TYPE_LIE:
                {
                    float cos = cosf(math::to_radian(rotation_)), sin = sinf(math::to_radian(rotation_));
                    v3_up.x = cos;
                    v3_up.y = -sin;
                    v3_up.z = 0;
                    v3_cross.x = sin;
                    v3_cross.y = cos;
                    v3_cross.z = 0;
                }
                break;
            case BILLBOARD_TYPE_2FACE:
            case BILLBOARD_TYPE_3FACE:
                // Falls through: set up like BILLBOARD_TYPE_Y, the local rotation is applied at render time.
            case BILLBOARD_TYPE_Y:
                {
                    v3_up = math::vector3(0.0f, 0.0f, 1.0f);
                    const math::vector3 &view = current_camera->view();
                    if (v3_up.x * view.y - v3_up.y * view.x < 0)
                        v3_up *= -1;
                    auto diff_vector = math::vector3(view.x, view.y, 0);
                    v3_cross = math::cross(v3_up, diff_vector);
                    v3_cross = math::normalize(v3_cross);

                    if (rotation_) {
                        float cos = -sinf(math::to_radian(rotation_)); // rotation + 90 degrees
                        float sin = cosf(math::to_radian(rotation_));

                        math::vector3 temp = v3_up * cos - v3_cross * sin;
                        v3_cross = v3_cross * cos + v3_up * sin;
                        v3_up = temp;
                    }
                }
                break;
            case BILLBOARD_TYPE_ALL:
            default:
                {
                    // Rotates the camera up and cross vectors themselves around the view vector.
                    if (rotation_ == 0.0f) {
                        v3_up = -c_rv3_cross;
                        v3_cross = c_rv3_up;
                    } else {
                        const math::vector3 &view = current_camera->view();
                        math::quaternion q, qc;
                        q = math::quaternion_rotation_axis(view, math::to_radian(rotation_));
                        qc = math::conjugate(q);

                        {
                            math::quaternion qr(-c_rv3_cross.x, -c_rv3_cross.y, -c_rv3_cross.z, 0);
                            qr = qc * qr;
                            qr = qr * q;
                            v3_up.x = qr.x;
                            v3_up.y = qr.y;
                            v3_up.z = qr.z;
                        }
                        {
                            math::quaternion qr(c_rv3_up.x, c_rv3_up.y, c_rv3_up.z, 0);
                            qr = qc * qr;
                            qr = qr * q;
                            v3_cross.x = qr.x;
                            v3_cross.y = qr.y;
                            v3_cross.z = qr.z;
                        }
                    }
                }
                break;
            }

        } else {
            v3_up = position_value_ - last_position_;

            if (local) {
                v3_up = math::transform_normal(v3_up, *local);
            }

            // The length grows with log(velocity).
            float length = math::length(v3_up);
            if (length == 0.0f) {
                v3_up = math::vector3(0.0f, 0.0f, 1.0f);
            } else
                v3_up *= (1 + std::log(1 + length)) / length;

            camera *current_camera = get_camera_manager().current_camera();
            const math::vector3 &view = current_camera->view();
            v3_cross = math::cross(v3_up, view);
            v3_cross = math::normalize(v3_cross);
        }

        v3_cross = -(half_size_.x * scale_.x) * v3_cross;
        v3_up = (half_size_.y * scale_.y) * v3_up;

        if (local && particle_property_value_->attach_flag) {
            math::vector3 position;
            position = math::transform_coord(position_value_, *local);
            particle_mesh_[0].position_value = position - v3_up + v3_cross;
            particle_mesh_[1].position_value = position - v3_up - v3_cross;
            particle_mesh_[2].position_value = position + v3_up + v3_cross;
            particle_mesh_[3].position_value = position + v3_up - v3_cross;
        } else {
            particle_mesh_[0].position_value = position_value_ - v3_up + v3_cross;
            particle_mesh_[1].position_value = position_value_ - v3_up - v3_cross;
            particle_mesh_[2].position_value = position_value_ + v3_up + v3_cross;
            particle_mesh_[3].position_value = position_value_ + v3_up - v3_cross;
        }
    }

    void particle_instance::transform(const math::matrix *local, const float z_rotation) {
        STATEMANAGER.set_render_state(gpu::state::TEXTURE_FACTOR, (uint32_t)dc_color_);

        math::vector3 v3_up;
        math::vector3 v3_cross;

        if (!particle_property_value_->stretch_flag) {
            camera *current_camera = get_camera_manager().current_camera();
            const math::vector3 &c_rv3_up = current_camera->up();
            const math::vector3 &c_rv3_cross = current_camera->cross();

            switch (particle_property_value_->billboard_type) {
            case BILLBOARD_TYPE_LIE:
                {
                    float cos = cosf(math::to_radian(rotation_)), sin = sinf(math::to_radian(rotation_));
                    v3_up.x = cos;
                    v3_up.y = -sin;
                    v3_up.z = 0;

                    v3_cross.x = sin;
                    v3_cross.y = cos;
                    v3_cross.z = 0;
                }
                break;
            case BILLBOARD_TYPE_2FACE:
            case BILLBOARD_TYPE_3FACE:
                // Falls through: set up like BILLBOARD_TYPE_Y, the local rotation is applied at render time.
            case BILLBOARD_TYPE_Y:
                {
                    v3_up = math::vector3(0.0f, 0.0f, 1.0f);
                    const math::vector3 &view = current_camera->view();
                    if (v3_up.x * view.y - v3_up.y * view.x < 0)
                        v3_up *= -1;
                    auto diff_vector = math::vector3(view.x, view.y, 0);
                    v3_cross = math::cross(v3_up, diff_vector);
                    v3_cross = math::normalize(v3_cross);

                    if (rotation_) {
                        float cos = -sinf(math::to_radian(rotation_)); // rotation + 90 degrees
                        float sin = cosf(math::to_radian(rotation_));

                        math::vector3 temp = v3_up * cos - v3_cross * sin;
                        v3_cross = v3_cross * cos + v3_up * sin;
                        v3_up = temp;
                    }
                }
                break;
            case BILLBOARD_TYPE_ALL:
            default:
                {
                    // Rotates the camera up and cross vectors themselves around the view vector.
                    if (rotation_ == 0.0f) {
                        v3_up = -c_rv3_cross;
                        v3_cross = c_rv3_up;
                    } else {
                        const math::vector3 &view = current_camera->view();
                        math::matrix mat_rotation;

                        mat_rotation = math::matrix_rotation_axis(view, math::to_radian(rotation_));
                        auto diff_vector = (-c_rv3_cross);
                        v3_up = math::transform_coord(diff_vector, mat_rotation);
                        v3_cross = math::transform_coord(c_rv3_up, mat_rotation);
                    }
                }
                break;
            }
        } else {
            v3_up = position_value_ - last_position_;

            if (local) {
                v3_up = math::transform_normal(v3_up, *local);
            }

            // The length grows with log(velocity).
            float length = math::length(v3_up);
            if (length == 0.0f) {
                v3_up = math::vector3(0.0f, 0.0f, 1.0f);
            } else
                v3_up *= (1 + std::log(1 + length)) / length;

            camera *current_camera = get_camera_manager().current_camera();
            const math::vector3 &view = current_camera->view();
            v3_cross = math::cross(v3_up, view);
            v3_cross = math::normalize(v3_cross);
        }

        if (z_rotation) {
            float x, y;
            float cos = cosf(z_rotation);
            float sin = sinf(z_rotation);

            x = v3_up.x;
            y = v3_up.y;
            v3_up.x = x * cos - y * sin;
            v3_up.y = y * cos + x * sin;

            x = v3_cross.x;
            y = v3_cross.y;
            v3_cross.x = x * cos - y * sin;
            v3_cross.y = y * cos + x * sin;
        }

        v3_cross = -(half_size_.x * scale_.x) * v3_cross;
        v3_up = (half_size_.y * scale_.y) * v3_up;

        if (local && particle_property_value_->attach_flag) {
            math::vector3 position;
            position = math::transform_coord(position_value_, *local);
            particle_mesh_[0].position_value = position - v3_up + v3_cross;
            particle_mesh_[1].position_value = position - v3_up - v3_cross;
            particle_mesh_[2].position_value = position + v3_up + v3_cross;
            particle_mesh_[3].position_value = position + v3_up - v3_cross;
        } else {
            particle_mesh_[0].position_value = position_value_ - v3_up + v3_cross;
            particle_mesh_[1].position_value = position_value_ - v3_up - v3_cross;
            particle_mesh_[2].position_value = position_value_ + v3_up + v3_cross;
            particle_mesh_[3].position_value = position_value_ + v3_up - v3_cross;
        }
    }

    void particle_instance::destroy() {
        if (decorator_)
            decorator_->delete_this();

        initialize_impl();
    }

    void particle_instance::initialize_impl() {
        decorator_ = nullptr;

        position_value_ = math::vector3(0.0f, 0.0f, 0.0f);
        last_position_ = position_value_;
        velocity_ = math::vector3(0.0f, 0.0f, 0.0f);

        scale_ = math::vector2(1.0f, 1.0f);
        dc_color_.color_value = 0xffffffff;

        frame_index_ = 0;
        particle_mesh_[0].tex_coord = math::vector2(0.0f, 1.0f);
        particle_mesh_[1].tex_coord = math::vector2(0.0f, 0.0f);
        particle_mesh_[2].tex_coord = math::vector2(1.0f, 1.0f);
        particle_mesh_[3].tex_coord = math::vector2(1.0f, 0.0f);
    }

    particle_instance::particle_instance() {
        initialize_impl();
    }

    particle_instance::~particle_instance() {
        destroy();
    }

    auto particle_instance::particle_mesh_pointer() -> pt_vertex * {
        return particle_mesh_;
    }
}
