#include "grp_sub_image.h"
#include <metin2/core/file_loader.h>
#include <metin2/core/parse.h>
#include <metin2/core/stl.h>
#include "resource_manager.h"

namespace metin2::inline graphics {

    char graphic_sub_image::search_path[256] = "D:/Ymir Work/UI/";

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

        if (!resource->is_type(graphic_image::type()))
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

        token_vector token_vector;
        std::map<std::string, std::string> token_map;

        memory_text_file_loader text_file_loader;

        text_file_loader.bind(size, buf);

        for (uint32_t i = 0; i < text_file_loader.line_count(); ++i) {
            if (!text_file_loader.split_line(i, &token_vector))
                continue;

            if (token_vector.size() != 2)
                return false;

            stl_lowers(token_vector[0]);
            stl_lowers(token_vector[1]);

            token_map[token_vector[0]] = token_vector[1];
        }

        const std::string &title = token_map["title"];
        const std::string &version = token_map["version"];
        const std::string &image = token_map["image"];
        const std::string &left = token_map["left"];
        const std::string &top = token_map["top"];
        const std::string &right = token_map["right"];
        const std::string &bottom = token_map["bottom"];

        if (title != "subimage")
            return false;

        char file_name[256];
        if ("2.0" == version) {
            const std::string &sub_file_name = file_name_string();
            int pos = sub_file_name.find_last_of('\\', -1);
            if (pos >= 0) {
                pos++;
                memcpy(file_name, sub_file_name.c_str(), pos);
                file_name[pos] = '\0';
                memcpy(file_name + pos, image.c_str(), image.length());
                file_name[pos + image.length()] = '\0';
            } else {
                memcpy(file_name, image.c_str(), image.length());
            }
        } else {
            snprintf(file_name, sizeof(file_name), "%s%s", search_path, image.c_str());
        }

        set_image_file_name(file_name);

        set_rect_position(metin2::to_number<int>(left.c_str()),
            metin2::to_number<int>(top.c_str()),
            metin2::to_number<int>(right.c_str()),
            metin2::to_number<int>(bottom.c_str()));

        return true;
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
