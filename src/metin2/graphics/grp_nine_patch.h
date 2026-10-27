#pragma once

#include "grp_sub_image.h"

#include <array>
#include <string>
#include <string_view>

namespace metin2::inline graphics {

    // A nine patch: a region of an image split by four margins into corners that keep their size,
    // edges that repeat along one axis and a middle that repeats along both (tile = false stretches
    // them instead), so one picture draws a panel or button of any size. TOML, [asset] type
    // "NinePatch":
    //
    //   [nine_patch]
    //   image = "ui:default/sheet.png"
    //   region_rect = [x, y, width, height]
    //   patch_margins = [left, top, right, bottom]
    //   tile = true
    class graphic_nine_patch : public graphic_sub_image {
      public:
        struct margins {
            int left = 0;
            int top = 0;
            int right = 0;
            int bottom = 0;
        };

        static auto type() -> type_type;
        // Parses the asset text; region is x, y, width, height.
        static auto load(std::string_view text, std::string_view name, std::string &image, int (&region)[4], margins &margins, bool &tile) -> bool;
        // The four cut positions along one axis of a patch drawn `drawn` long: 0, the end of the
        // first margin, the start of the last margin, `drawn`. Margins that do not fit shrink in
        // proportion to each other instead of overlapping.
        static auto cuts(int first, int last, int drawn) -> std::array<float, 4>;

        explicit graphic_nine_patch(const char *file_name);

        auto patch_margins() const -> const margins & { return margins_; }
        auto tiled() const -> bool { return tile_; }

      protected:
        auto on_load(int size, const void *buf) -> bool override;
        void on_clear() override;
        auto on_is_type(type_type type) -> bool override;

      private:
        margins margins_;
        bool tile_ = true;
    };
}
