#include "grp_nine_patch.h"
#include <metin2/files/asset_toml.h>

#include <algorithm>

namespace metin2::inline graphics {

    namespace {
        auto read_four(asset_table &values, std::string_view key, int (&out)[4], std::string_view shape) -> bool {
            const toml::array *numbers = values.array(key);
            if (!numbers) {
                values.error(key, "is missing");
                return false;
            }
            if (numbers->size() != 4 || !std::ranges::all_of(*numbers, [](const toml::node &n) -> bool { return n.is_integer(); })) {
                values.error(key, shape);
                return false;
            }
            for (size_t i = 0; i < 4; ++i) {
                out[i] = static_cast<int>(*(*numbers)[i].value<int64_t>());
            }
            return true;
        }
    }

    auto graphic_nine_patch::type() -> type_type {
        static type_type s_type = string_to_type("CGraphicNinePatch");
        return s_type;
    }

    graphic_nine_patch::graphic_nine_patch(const char *file_name) : graphic_sub_image(file_name) {
    }

    auto graphic_nine_patch::cuts(int first, int last, int drawn) -> std::array<float, 4> {
        auto a = static_cast<float>(first);
        auto b = static_cast<float>(last);
        const auto size = static_cast<float>(std::max(0, drawn));
        if (a + b > size) {
            a = a + b > 0.0f ? size * a / (a + b) : 0.0f;
            b = size - a;
        }
        return {0.0f, a, size - b, size};
    }

    auto graphic_nine_patch::load(std::string_view text, std::string_view name, std::string &image, int (&region)[4], margins &margins, bool &tile) -> bool {
        const auto table = parse_asset_toml(text, name, "NinePatch");
        if (!table) {
            return false;
        }

        asset_table root(*table, name);
        root.table("asset");
        const toml::table *patch = root.table("nine_patch");
        if (!patch) {
            root.error("nine_patch", "is missing");
            return false;
        }

        asset_table values(*patch, name);
        bool ok = values.required("image", image);
        tile = true;
        values.optional("tile", tile);
        int edges[4] = {};
        if (read_four(values, "region_rect", region, "must be four whole numbers: x, y, width, height")) {
            if (region[2] <= 0 || region[3] <= 0) {
                values.error("region_rect", "needs a positive width and height");
                ok = false;
            }
        } else {
            ok = false;
        }
        if (read_four(values, "patch_margins", edges, "must be four whole numbers: left, top, right, bottom")) {
            if (std::ranges::any_of(edges, [](int edge) -> bool { return edge < 0; })) {
                values.error("patch_margins", "must not be negative");
                ok = false;
            } else if (ok && (edges[0] + edges[2] > region[2] || edges[1] + edges[3] > region[3])) {
                values.error("patch_margins", "do not fit into region_rect");
                ok = false;
            }
            margins = {edges[0], edges[1], edges[2], edges[3]};
        } else {
            ok = false;
        }
        ok &= values.finish();
        return root.finish() && ok;
    }

    auto graphic_nine_patch::on_load(int size, const void *buf) -> bool {
        if (!buf) {
            return false;
        }

        int region[4] = {};
        std::string image;
        margins margins;
        bool tile = true;
        if (!load(std::string_view(static_cast<const char *>(buf), size), file_name_string(), image, region, margins, tile)) {
            return false;
        }

        if (!set_image_file_name(image.c_str())) {
            return false;
        }
        set_rect_position(region[0], region[1], region[0] + region[2], region[1] + region[3]);
        margins_ = margins;
        tile_ = tile;
        return true;
    }

    void graphic_nine_patch::on_clear() {
        graphic_sub_image::on_clear();
        margins_ = {};
        tile_ = true;
    }

    auto graphic_nine_patch::on_is_type(type_type type) -> bool {
        if (graphic_nine_patch::type() == type) {
            return true;
        }

        return graphic_sub_image::on_is_type(type);
    }
}
