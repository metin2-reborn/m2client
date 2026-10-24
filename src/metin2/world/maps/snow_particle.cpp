#include "snow_particle.h"
#include <metin2/core/random.h>
#include <metin2/core/stl.h>

namespace metin2::inline world::inline maps {

    const float c_fSnowDistance = 70000.0f;

    std::vector<snow_particle *> snow_particle::vct_snow_particle_pool;

    void snow_particle::set_camera_vertex(const math::vector3 &up, const math::vector3 &cross) {
        up_ = up * half_width_;
        cross_ = cross * half_height_;
    }

    auto snow_particle::is_activate() -> bool {
        return activate_;
    }

    void snow_particle::update(float elapsed_time, const math::vector3 &pos) {
        position_value_ += velocity_ * elapsed_time;

        // The sideways drift was a tenth of the flake's width per frame at 60 fps.
        const float drift = sin(fcur_radian_) / 10.0f * elapsed_time * 60.0f;
        position_value_.x += cross_.x * drift;
        position_value_.y += cross_.y * drift;
        fcur_radian_ += period_ * elapsed_time;

        if (position_value_.z < pos.z - 500.0f)
            activate_ = false;
        else if (fabsf(position_value_.x - pos.x) > c_fSnowDistance)
            activate_ = false;
        else if (fabsf(position_value_.y - pos.y) > c_fSnowDistance)
            activate_ = false;
    }

    void snow_particle::verticies(particle_vertex &vertex1, particle_vertex &vertex2,
        particle_vertex &vertex3, particle_vertex &vertex4) {
        vertex1.pos = position_value_ - cross_ - up_;
        vertex1.u = 0.0f;
        vertex1.v = 0.0f;

        vertex2.pos = position_value_ + cross_ - up_;
        vertex2.u = 1.0f;
        vertex2.v = 0.0f;

        vertex3.pos = position_value_ - cross_ + up_;
        vertex3.u = 0.0f;
        vertex3.v = 1.0f;

        vertex4.pos = position_value_ + cross_ + up_;
        vertex4.u = 1.0f;
        vertex4.v = 1.0f;
    }

    void snow_particle::init(const math::vector3 &pos) {
        float rot = metin2::random::fnumber(0.0f, 36000.0f) / 100.0f;
        float distance = metin2::random::fnumber(0.0f, c_fSnowDistance) / 10.0f;

        position_value_.x = pos.x + distance * sin((double)math::to_radian(rot));
        position_value_.y = pos.y + distance * cos((double)math::to_radian(rot));
        position_value_.z = pos.z + metin2::random::fnumber(1500.0f, 2000.0f);
        velocity_.x = 0.0f;
        velocity_.y = 0.0f;
        velocity_.z = metin2::random::fnumber(-50.0f, -200.0f);
        half_width_ = metin2::random::fnumber(2.0f, 7.0f);
        half_height_ = half_width_;
        activate_ = true;
        changed_size_ = false;

        period_ = metin2::random::fnumber(1.5f, 5.0f);
        fcur_radian_ = metin2::random::fnumber(-1.6f, 1.6f);
        amplitude_ = metin2::random::fnumber(1.0f, 3.0f);
    }

    auto snow_particle::new_value() -> snow_particle * {
        if (vct_snow_particle_pool.empty()) {
            return new snow_particle;
        }

        snow_particle *particle = vct_snow_particle_pool.back();
        vct_snow_particle_pool.pop_back();
        return particle;
    }

    void snow_particle::delete_value(snow_particle *snow_particle) {
        vct_snow_particle_pool.push_back(snow_particle);
    }

    void snow_particle::destroy_pool() {
        stl_wipe(vct_snow_particle_pool);
    }

    snow_particle::snow_particle() = default;
    snow_particle::~snow_particle() = default;
}
