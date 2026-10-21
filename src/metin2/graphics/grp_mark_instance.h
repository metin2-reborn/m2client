#pragma once

#include <metin2/core/pool.h>
#include "grp_image.h"

namespace metin2::inline graphics {

    class graphic_mark_instance {
      public:
        static auto type() -> uint32_t;
        auto is_type(uint32_t type) -> bool;

        void set_image_file_name(const char *file_name);
        auto image_file_name() -> const std::string &;

      public:
        graphic_mark_instance();
        virtual ~graphic_mark_instance();

        void destroy();

        void render();

        void set_depth(float depth);
        void set_diffuse_color(float fr, float fg, float fb, float fa);
        void set_position(float fx, float fy);
        void set_index(uint32_t index);
        void set_scale(float scale);

        void load();
        auto is_empty() const -> bool;

        auto width() -> int;
        auto height() -> int;

        auto texture_pointer() -> graphic_texture *;
        auto texture_reference() const -> const graphic_texture &;
        auto graphic_image_pointer() -> graphic_image *;

        auto operator==(const graphic_mark_instance &rhs) const -> bool;

      protected:
        enum {
            MARK_WIDTH = 16,
            MARK_HEIGHT = 12,
        };

        void initialize();

        virtual void on_render();
        virtual void on_set_image_pointer();

        virtual auto on_is_type(uint32_t type) -> bool;

        void set_image_pointer(graphic_image *image);

      protected:
        math::color diffuse_color_;
        math::vector2 position_value_;

        uint32_t index_value_;

        float scale_;
        float depth_;

        graphic_image::ref ro_image_;
        std::string image_file_name_;

      public:
        static void create_system(uint32_t capacity);
        static void destroy_system();

        static auto new_value() -> graphic_mark_instance *;
        static void delete_value(graphic_mark_instance *img_inst);

        static dynamic_pool<graphic_mark_instance> pool_value;
    };
}
