#include "grp_text.h"
#include <metin2/core/parse.h>
#include <metin2/log/log.h>
#include "grp_base.h"
#include "resource_manager.h"

#include <charconv>

namespace metin2::inline graphics {

    namespace {
        constexpr std::string_view EXTENSION = ".fnt";

        auto parse_int(std::string_view text, int &value) -> bool {
            const char *end = text.data() + text.size();
            auto [ptr, ec] = std::from_chars(text.data(), end, value);
            return ec == std::errc() && ptr == end;
        }
    }

    graphic_text::graphic_text(const char *file_name) : resource(file_name) {
    }

    graphic_text::~graphic_text() = default;

    auto graphic_text::create_device_objects() -> bool {
        return font_texture_.create_device_objects();
    }

    void graphic_text::destroy_device_objects() {
        font_texture_.destroy_device_objects();
    }

    auto graphic_text::font_texture_pointer() -> graphic_font_texture * {
        if (!font_texture_.is_empty() && font_texture_.pixel_scale() != graphic_base::interface_pixel_scale()) {
            font_texture_.create(file_, size_);
        }
        return &font_texture_;
    }

    auto graphic_text::type() -> graphic_text::type_type {
        static type_type s_type = string_to_type("CGraphicText");
        return s_type;
    }

    auto graphic_text::resource_name(const font_spec &spec) -> std::string {
        std::string name = spec.family;
        name += ':';
        name += std::to_string(spec.size);
        name += ':';
        name += std::to_string(spec.weight);
        name += spec.italic ? ":italic" : ":normal";
        name += EXTENSION;
        return name;
    }

    auto graphic_text::parse_resource_name(std::string_view name) -> std::optional<font_spec> {
        if (name.size() <= EXTENSION.size() || name.substr(name.size() - EXTENSION.size()) != EXTENSION) {
            return std::nullopt;
        }
        name.remove_suffix(EXTENSION.size());

        std::string_view fields[3];
        for (std::string_view &field : fields) {
            const size_t colon = name.rfind(':');
            if (colon == std::string_view::npos) {
                return std::nullopt;
            }
            field = name.substr(colon + 1);
            name.remove_suffix(name.size() - colon);
        }
        // fields: style, weight, size (right to left).
        font_spec spec;
        spec.family = std::string(name);
        if (spec.family.empty() || !parse_int(fields[2], spec.size) || !parse_int(fields[1], spec.weight) || spec.size <= 0) {
            return std::nullopt;
        }
        if (fields[0] == "italic") {
            spec.italic = true;
        } else if (fields[0] != "normal") {
            return std::nullopt;
        }
        return spec;
    }

    auto graphic_text::on_load(int /*size*/, const void * /*buf*/) -> bool {
        const std::optional<font_spec> spec = parse_resource_name(file_name());
        if (!spec) {
            metin2::log::error("Font resource {} is not of the form <font asset>:<size>:<weight>:<normal|italic>.fnt", file_name());
            return false;
        }

        resource *asset = get_resource_manager().resource_pointer(spec->family.c_str());
        if (!asset || !asset->is_type(font_family::type())) {
            metin2::log::error("{} is not a Font asset", spec->family);
            return false;
        }
        family_ = static_cast<font_family *>(asset);
        if (family_->is_empty()) {
            // The asset logged what is wrong with it.
            return false;
        }

        file_ = family_->file_for(spec->italic, spec->weight);
        size_ = spec->size;
        if (!font_texture_.create(file_, size_)) {
            metin2::log::error("{}: cannot open {} for weight {} {}", spec->family, file_, spec->weight, spec->italic ? "italic" : "normal");
            return false;
        }
        return true;
    }

    void graphic_text::on_clear() {
        font_texture_.destroy();
        family_ = nullptr;
    }

    auto graphic_text::on_is_empty() const -> bool {
        return font_texture_.is_empty();
    }

    auto graphic_text::on_is_type(type_type type) -> bool {
        if (graphic_text::type() == type) {
            return true;
        }

        return resource::on_is_type(type);
    }
}
