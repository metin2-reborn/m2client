#pragma once

#include <metin2/graphics/grp_image_instance.h>
#include <metin2/graphics/grp_screen.h>
#include <metin2/graphics/render_target.h>

namespace metin2::inline world::inline maps {

    class snow_particle;

    class snow_environment : public screen {
      public:
        snow_environment();
        ~snow_environment() override;

        auto create() -> bool;
        void destroy();

        void enable();
        void disable();

        void update(const math::vector3 &pos);
        void deform();
        void render();

      protected:
        void initialize_impl();
        auto create_blur_texture() -> bool;
        auto create_geometry() -> bool;
        void begin_blur();
        void apply_blur();

      protected:
        render_target snow_target_;

        std::shared_ptr<gpu::vertex_buffer> vb_;
        std::shared_ptr<gpu::index_buffer> ib_;

        math::vector3 center_;

        uint16_t blur_texture_size_;
        graphic_image_instance *image_instance_;
        std::vector<snow_particle *> vct_particle_snow_;

        uint32_t particle_max_num_;
        bool blur_enable_;

        bool snow_enable_;
    };
}
