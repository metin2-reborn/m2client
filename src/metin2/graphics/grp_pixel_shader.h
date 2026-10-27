#pragma once

#include <metin2/core/ref.h>
#include <metin2/gpu/pixel_shader.h>
#include "resource.h"

#include <memory>
#include <string>

namespace metin2::inline graphics {

    // A .glsl file: the body of a pixel shader (gpu/pixel_shader.h). It is compiled on first use; one
    // that does not compile is logged once and draws as if it were not set.
    class graphic_pixel_shader : public resource {
      public:
        using ref = ref_ptr<graphic_pixel_shader>;

        static auto type() -> type_type;

        explicit graphic_pixel_shader(const char *file_name);
        ~graphic_pixel_shader() override;

        void destroy_device_objects() override;

        // Null while the source does not compile.
        auto shader() -> gpu::pixel_shader *;

      protected:
        auto on_load(int size, const void *buf) -> bool override;
        void on_clear() override;
        auto on_is_empty() const -> bool override;
        auto on_is_type(type_type type) -> bool override;

      private:
        std::string source_;
        std::shared_ptr<gpu::pixel_shader> shader_;
        bool failed_ = false;
    };
}
