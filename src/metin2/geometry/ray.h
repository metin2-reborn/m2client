#pragma once

#include <cassert>
#include <metin2/math/math.h>

namespace metin2::inline geometry {

    class ray {
      public:
        ray(const math::vector3 &start, const math::vector3 &dir, float ray_range) : start_(start), direction_value_(dir) {
            assert(ray_range >= 0);
            ray_range_ = ray_range;
            direction_value_ = math::normalize(direction_value_);
            end_ = start_ + ray_range * direction_value_;
        }

        ray() = default;

        void set_start_point(const math::vector3 &start) {
            start_ = start;
        }

        void set_direction(const math::vector3 &dir, float ray_range) {
            assert(ray_range >= 0);
            direction_value_ = dir;
            direction_value_ = math::normalize(direction_value_);
            ray_range_ = ray_range;
            end_ = start_ + ray_range_ * direction_value_;
        }

        void start_point(math::vector3 *start) const {
            *start = start_;
        }

        void get_direction(math::vector3 *dir, float *ray_range) const {
            *dir = direction_value_;
            *ray_range = ray_range_;
        }

        void end_point(math::vector3 *end) const {
            *end = end_;
        }

        auto operator=(const ray &rhs) -> const ray & {
            assert(rhs.ray_range_ >= 0);
            start_ = rhs.start_;
            direction_value_ = rhs.direction_value_;
            ray_range_ = rhs.ray_range_;
            direction_value_ = math::normalize(direction_value_);
            end_ = start_ + ray_range_ * direction_value_;
            return *this;
        }

      private:
        math::vector3 start_;
        math::vector3 end_;
        math::vector3 direction_value_;
        float ray_range_;
    };
}
