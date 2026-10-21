#pragma once

#include <metin2/core/rectangle.h>
#include "grp_image_instance.h"

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

      protected:
        void initialize();

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

      public:
        static void create_system(uint32_t capacity);
        static void destroy_system();

        static auto new_value() -> graphic_expanded_image_instance *;
        static void delete_value(graphic_expanded_image_instance *img_inst);

        static dynamic_pool<graphic_expanded_image_instance> pool_value;
    };
}
