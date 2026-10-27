#include "grp_pixel_shader.h"
#include <metin2/log/log.h>
#include "grp_base.h"

namespace metin2::inline graphics {

    graphic_pixel_shader::graphic_pixel_shader(const char *file_name) : resource(file_name) {
    }

    graphic_pixel_shader::~graphic_pixel_shader() = default;

    auto graphic_pixel_shader::type() -> type_type {
        static type_type s_type = string_to_type("CGraphicPixelShader");
        return s_type;
    }

    void graphic_pixel_shader::destroy_device_objects() {
        shader_.reset();
    }

    auto graphic_pixel_shader::shader() -> gpu::pixel_shader * {
        if (!shader_ && !failed_ && !source_.empty()) {
            std::string error;
            shader_ = graphic_base::device().create_pixel_shader(source_, error);
            if (!shader_) {
                // Lines in the message count from the first line of the file.
                log::error("{}: the pixel shader does not compile, drawn without it:\n{}", file_name(), error);
                failed_ = true;
            }
        }
        return shader_.get();
    }

    auto graphic_pixel_shader::on_load(int size, const void *buf) -> bool {
        source_.assign(static_cast<const char *>(buf), static_cast<size_t>(size));
        shader_.reset();
        failed_ = false;
        return !source_.empty();
    }

    void graphic_pixel_shader::on_clear() {
        source_.clear();
        shader_.reset();
        failed_ = false;
    }

    auto graphic_pixel_shader::on_is_empty() const -> bool {
        return source_.empty();
    }

    auto graphic_pixel_shader::on_is_type(type_type type) -> bool {
        if (graphic_pixel_shader::type() == type)
            return true;
        return resource::on_is_type(type);
    }
}
