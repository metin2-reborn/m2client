#pragma once

#include <metin2/core/ref.h>
#include "grp_font_texture.h"
#include "resource.h"

namespace metin2::inline graphics {

    class graphic_text : public resource {
      public:
        using ref = ref_ptr<graphic_text>;

      public:
        static auto type() -> type_type;

      public:
        graphic_text(const char *file_name);
        ~graphic_text() override;

        auto create_device_objects() -> bool override;
        void destroy_device_objects() override;

        auto font_texture_pointer() -> graphic_font_texture *;

      protected:
        auto on_load(int size, const void *buf) -> bool override;
        void on_clear() override;
        auto on_is_empty() const -> bool override;
        auto on_is_type(type_type type) -> bool override;

      protected:
        graphic_font_texture font_texture_;
    };
}
