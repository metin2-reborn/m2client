#pragma once

#include <metin2/core/rectangle.h>
#include <metin2/core/ref.h>
#include "grp_image_texture.h"
#include "resource.h"

namespace metin2::inline graphics {

    class graphic_image : public resource {
      public:
        using ref = ref_ptr<graphic_image>;

      public:
        static auto type() -> type_type;

      public:
        explicit graphic_image(const char *file_name);
        ~graphic_image() override;

        auto create_device_objects() -> bool override;
        void destroy_device_objects() override;

        auto width() const -> int;
        auto height() const -> int;

        auto rect_reference() const -> const rectangle &;

        auto texture_reference() const -> const graphic_texture &;
        auto texture_pointer() -> graphic_texture *;

      protected:
        auto on_load(int size, const void *buf) -> bool override;

        void on_clear() override;
        auto on_is_empty() const -> bool override;
        auto on_is_type(type_type type) -> bool override;

      protected:
        graphic_image_texture image_texture_;
        rectangle rect_;
    };
}
