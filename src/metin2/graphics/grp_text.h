#pragma once

#include <metin2/core/ref.h>
#include "font_family.h"
#include "grp_font_texture.h"
#include "resource.h"

#include <optional>
#include <string>
#include <string_view>

namespace metin2::inline graphics {

    // What a piece of text is drawn with: a Font asset (font_family), the em size in interface
    // pixels, a CSS weight (400 regular, 700 bold) and the style.
    struct font_spec {
        std::string family;
        int size = 12;
        int weight = 400;
        bool italic = false;

        auto operator==(const font_spec &) const -> bool = default;
    };

    // The glyph atlas of one font_spec, a resource under a virtual name (resource_name) so that
    // every text of the same spec shares it.
    class graphic_text : public resource {
      public:
        using ref = ref_ptr<graphic_text>;

      public:
        static auto type() -> type_type;
        // "<family>:<size>:<weight>:<normal|italic>.fnt"; the family keeps its archive colon, the
        // three fields are split off from the right. The resource manager lowercases the name, so the
        // family comes back in that spelling (the file manager does not mind).
        static auto resource_name(const font_spec &spec) -> std::string;
        static auto parse_resource_name(std::string_view name) -> std::optional<font_spec>;

      public:
        graphic_text(const char *file_name);
        ~graphic_text() override;

        auto create_device_objects() -> bool override;
        void destroy_device_objects() override;

        // The atlas, rasterized again first when the interface pixel scale changed since.
        auto font_texture_pointer() -> graphic_font_texture *;

      protected:
        auto on_load(int size, const void *buf) -> bool override;
        void on_clear() override;
        auto on_is_empty() const -> bool override;
        auto on_is_type(type_type type) -> bool override;

      protected:
        graphic_font_texture font_texture_;
        font_family::ref family_; // keeps the Font asset alive with the atlas
        std::string file_;        // the .ttf of the spec's style and weight
        int size_ = 12;
    };
}
