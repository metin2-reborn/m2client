#include "grp_sub_image.h"
#include <metin2/files/asset_toml.h>
#include "resource_manager.h"

#include <algorithm>

namespace metin2::inline graphics {

    auto graphic_sub_image::type() -> graphic_sub_image::type_type {
        static type_type s_type = string_to_type("CGraphicSubImage");
        return s_type;
    }

    graphic_sub_image::graphic_sub_image(const char *file_name) : graphic_image(file_name) {
    }

    graphic_sub_image::~graphic_sub_image() {
        ro_image_ = nullptr;
    }

    auto graphic_sub_image::create_device_objects() -> bool {
        image_texture_.create_from_texture_pointer(ro_image_->texture_pointer());
        return true;
    }

    void graphic_sub_image::set_image_pointer(graphic_image *image) {
        ro_image_ = image;
        create_device_objects();
    }

    auto graphic_sub_image::set_image_file_name(const char *file_name) -> bool {
        resource *resource = get_resource_manager().resource_pointer(file_name);

        if (!resource || !resource->is_type(graphic_image::type()))
            return false;

        set_image_pointer(static_cast<graphic_image *>(resource));
        return true;
    }

    void graphic_sub_image::set_rect_position(int left, int top, int right, int bottom) {
        rect_.left = left;
        rect_.top = top;
        rect_.right = right;
        rect_.bottom = bottom;
    }

    auto graphic_sub_image::on_load(int size, const void *buf) -> bool {
        if (!buf)
            return false;

        int rect[4] = {};
        std::string image;
        if (!load(std::string_view(static_cast<const char *>(buf), size), file_name_string(), image, rect)) {
            return false;
        }

        if (!set_image_file_name(image.c_str())) {
            return false;
        }
        set_rect_position(rect[0], rect[1], rect[2], rect[3]);
        return true;
    }

    auto graphic_sub_image::load(std::string_view text, std::string_view name, std::string &image, int (&rect)[4]) -> bool {
        const auto table = parse_asset_toml(text, name, "SubImage");
        if (!table) {
            return false;
        }

        asset_table root(*table, name);
        root.table("asset");
        const toml::table *sub_image = root.table("sub_image");
        if (!sub_image) {
            root.error("sub_image", "is missing");
            return false;
        }

        asset_table values(*sub_image, name);
        bool ok = values.required("image", image);
        const toml::array *numbers = values.array("rect");
        if (!numbers) {
            values.error("rect", "is missing");
            ok = false;
        } else if (numbers->size() != 4 || !std::ranges::all_of(*numbers, [](const toml::node &n) -> bool { return n.is_integer(); })) {
            values.error("rect", "must be four whole numbers: left, top, right, bottom");
            ok = false;
        } else {
            for (size_t i = 0; i < 4; ++i) {
                rect[i] = static_cast<int>(*(*numbers)[i].value<int64_t>());
            }
        }
        ok &= values.finish();
        return root.finish() && ok;
    }

    void graphic_sub_image::on_clear() {
        ro_image_ = nullptr;
        rect_ = {};
    }

    auto graphic_sub_image::on_is_empty() const -> bool {
        if (!ro_image_.is_null())
            if (!ro_image_->is_empty())
                return false;

        return true;
    }

    auto graphic_sub_image::on_is_type(type_type type) -> bool {
        if (graphic_sub_image::type() == type)
            return true;

        return graphic_image::on_is_type(type);
    }
}
