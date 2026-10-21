#include "grp_image.h"

namespace metin2::inline graphics {

    graphic_image::graphic_image(const char *file_name) : resource(file_name) {
        rect_.bottom = rect_.right = rect_.top = rect_.left = 0;
    }

    graphic_image::~graphic_image() = default;

    auto graphic_image::create_device_objects() -> bool {
        return image_texture_.create_device_objects();
    }

    void graphic_image::destroy_device_objects() {
        image_texture_.destroy_device_objects();
    }

    auto graphic_image::type() -> graphic_image::type_type {
        static type_type s_type = string_to_type("CGraphicImage");
        return s_type;
    }

    auto graphic_image::on_is_type(type_type type) -> bool {
        if (graphic_image::type() == type)
            return true;

        return resource::on_is_type(type);
    }

    auto graphic_image::width() const -> int {
        return rect_.right - rect_.left;
    }

    auto graphic_image::height() const -> int {
        return rect_.bottom - rect_.top;
    }

    auto graphic_image::texture_reference() const -> const graphic_texture & {
        return image_texture_;
    }

    auto graphic_image::texture_pointer() -> graphic_texture * {
        return &image_texture_;
    }

    auto graphic_image::rect_reference() const -> const rectangle & {
        return rect_;
    }

    auto graphic_image::on_load(int size, const void *buf) -> bool {
        if (!buf)
            return false;

        image_texture_.set_file_name(resource::file_name());

        // Some machines crash unless the format is left as gpu::format::UNKNOWN.
        if (!image_texture_.create_from_memory_file(size, buf, gpu::format::UNKNOWN))
            return false;

        rect_.left = 0;
        rect_.top = 0;
        rect_.right = image_texture_.width();
        rect_.bottom = image_texture_.height();
        return true;
    }

    void graphic_image::on_clear() {
        image_texture_.destroy();
        rect_ = {};
    }

    auto graphic_image::on_is_empty() const -> bool {
        return image_texture_.is_empty();
    }
}
