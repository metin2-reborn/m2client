#pragma once

#include <metin2/core/pool.h>
#include "grp_image.h"
#include "grp_index_buffer.h"

namespace metin2::inline graphics {

    class graphic_image_instance {
      public:
        static auto type() -> uint32_t;
        auto is_type(uint32_t type) -> bool;

      public:
        graphic_image_instance();
        virtual ~graphic_image_instance();

        void destroy();

        void render();

        void set_diffuse_color(float fr, float fg, float fb, float fa);
        void set_position(float fx, float fy);

        void set_image_pointer(graphic_image *image);
        auto is_empty() const -> bool;

        auto width() -> int;
        auto height() -> int;

        auto texture_pointer() -> graphic_texture *;
        auto texture_reference() const -> const graphic_texture &;
        auto graphic_image_pointer() -> graphic_image *;

        auto operator==(const graphic_image_instance &rhs) const -> bool;

      protected:
        void initialize();

        virtual void on_render();
        virtual void on_set_image_pointer();

        virtual auto on_is_type(uint32_t type) -> bool;

      protected:
        math::color diffuse_color_;
        math::vector2 position_value_;

        graphic_image::ref ro_image_;

      public:
        static void create_system(uint32_t capacity);
        static void destroy_system();

        static auto new_value() -> graphic_image_instance *;
        static void delete_value(graphic_image_instance *img_inst);

        static dynamic_pool<graphic_image_instance> pool_value;
    };
}
