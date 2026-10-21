#pragma once
#include <cstdint>
#include <metin2/math/math.h>
#include <string>
#include <vector>

namespace metin2::inline world::inline maps {

    struct particle_vertex {
        math::vector3 pos;
        float u, v;
    };

    struct blur_vertex {
        math::vector3 pos;
        float rhw;
        uint32_t color_value;
        float tu, tv;

        static const uint32_t FVF;

        blur_vertex(math::vector3 p, float w, uint32_t c, float u, float v) : pos(p), rhw(w), color_value(c), tu(u), tv(v) {}
        ~blur_vertex() = default;
    };

    class snow_particle {
      public:
        snow_particle();
        ~snow_particle();

        static auto new_value() -> snow_particle *;
        static void delete_value(snow_particle *snow_particle);
        static void destroy_pool();

        void init(const math::vector3 &pos);

        void set_camera_vertex(const math::vector3 &up, const math::vector3 &cross);
        auto is_activate() -> bool;

        void update(float elapsed_time, const math::vector3 &pos);
        void verticies(particle_vertex &vertex1, particle_vertex &vertex2,
            particle_vertex &vertex3, particle_vertex &vertex4);

      protected:
        bool activate_;
        bool changed_size_;
        float half_width_;
        float half_height_;

        math::vector3 velocity_;
        math::vector3 position_value_;

        math::vector3 up_;
        math::vector3 cross_;

        float period_;
        float fcur_radian_;
        float amplitude_;

      public:
        static std::vector<snow_particle *> vct_snow_particle_pool;
    };
}
