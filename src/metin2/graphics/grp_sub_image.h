#pragma once

#include "grp_image.h"

namespace metin2::inline graphics {

    class graphic_sub_image : public graphic_image {
      public:
        using ref = ref_ptr<graphic_image>;

      public:
        static auto type() -> type_type;
        static char search_path[256];

      public:
        graphic_sub_image(const char *file_name);
        ~graphic_sub_image() override;

        auto create_device_objects() -> bool override;

        auto set_image_file_name(const char *file_name) -> bool;

        void set_rect_position(int left, int top, int right, int bottom);

      protected:
        void set_image_pointer(graphic_image *image);

        auto on_load(int size, const void *buf) -> bool override;
        void on_clear() override;
        auto on_is_empty() const -> bool override;
        auto on_is_type(type_type type) -> bool override;

      protected:
        graphic_image::ref ro_image_;
    };
}
