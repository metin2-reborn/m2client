#pragma once

#include <metin2/core/rectangle.h>
#include "grp_image_instance.h"
#include "grp_pixel_shader.h"

namespace metin2::inline graphics {

    class graphic_expanded_image_instance : public graphic_image_instance {
      public:
        static auto type() -> uint32_t;
        static void delete_expanded_image_instance(graphic_expanded_image_instance *instance) {
            instance->destroy();
            pool_value.free(instance);
        }

        enum rendering_mode {
            RENDERING_MODE_NORMAL,
            RENDERING_MODE_SCREEN,
            RENDERING_MODE_COLOR_DODGE,
            RENDERING_MODE_MODULATE,
        };

        // How the image fills a box of another size (set_fit): stretched to it, scaled uniformly
        // to cover it (the overflow is cropped, centred), to fit inside it (centred, with margins)
        // or repeated at its own size from the top left. A negative scale mirrors the image.
        enum fit_mode {
            FIT_NONE, // the scale and rendering rect apply
            FIT_STRETCH,
            FIT_COVER,
            FIT_CONTAIN,
            FIT_TILE,
        };

        // The quad of an image of image_width x image_height fitted into width x height at the
        // origin: its position and size, and the shown part of the image as fractions (0..1).
        struct fit_rect {
            float x = 0.0f;
            float y = 0.0f;
            float width = 0.0f;
            float height = 0.0f;
            float u0 = 0.0f;
            float v0 = 0.0f;
            float u1 = 1.0f;
            float v1 = 1.0f;
        };
        static auto fit(fit_mode mode, float image_width, float image_height, float width, float height) -> fit_rect;

      public:
        graphic_expanded_image_instance();
        ~graphic_expanded_image_instance() override;

        void destroy();

        void set_depth(float depth);
        void set_origin();
        void set_origin(float fx, float fy);
        void set_rotation(float rotation);
        void set_scale(float fx, float fy);
        auto scale() const -> const math::vector2 & { return scale_; }
        void set_rendering_rect(float left, float top, float right, float bottom);
        void set_rendering_mode(int mode);
        void set_fit(int mode, float width, float height);
        // A pixel shader for the fitted image (stretch, cover and contain), with the mask image in
        // texture stage 1; null for either clears it.
        void set_shader(graphic_pixel_shader *shader, graphic_image *mask);

      protected:
        void initialize();
        void render_tiled(graphic_image &image, graphic_texture &texture);

        void on_render() override;
        void on_set_image_pointer() override;

        auto on_is_type(uint32_t type) -> bool override;

      protected:
        float depth_;
        math::vector2 origin_;
        math::vector2 scale_;
        float rotation_;
        rectangle rendering_rect_;
        int rendering_mode_value_;
        int fit_mode_;
        float fit_width_;
        float fit_height_;
        graphic_pixel_shader::ref shader_;
        graphic_image::ref mask_;

      public:
        static void create_system(uint32_t capacity);
        static void destroy_system();

        static auto new_value() -> graphic_expanded_image_instance *;
        static void delete_value(graphic_expanded_image_instance *img_inst);

        static dynamic_pool<graphic_expanded_image_instance> pool_value;
    };
}
