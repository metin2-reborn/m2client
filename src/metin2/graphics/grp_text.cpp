#include "grp_text.h"
#include <metin2/core/parse.h>
#include <metin2/core/utils.h>
#include "grp_base.h"

namespace metin2::inline graphics {

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
        if (!font_texture_.is_empty() && font_texture_.pixel_scale() != graphic_base::interface_pixel_scale())
            font_texture_.create(font_name_.c_str(), font_size_, italic_);
        return &font_texture_;
    }

    auto graphic_text::type() -> graphic_text::type_type {
        static type_type s_type = string_to_type("CGraphicText");
        return s_type;
    }

    auto graphic_text::on_load(int /*iSize*/, const void * /*c_pvBuf*/) -> bool {
        std::string name;
        int size;
        bool italic = false;

        // Font file name format:
        // gulim.fnt      loads the "gulim" font at the default size 12
        // gulim:18.fnt   loads the "gulim" font at size 18
        // gulim:14i.fnt  loads the "gulim" font at size 14, italic
        const char *p = strrchr(file_name(), ':');

        if (p) {
            name.assign(file_name(), p - file_name());
            ++p;

            size = metin2::to_number<int>(p);
            while (*p && isdigit(*p))
                ++p;
            if (*p == 'i')
                italic = true;
        } else {
            p = strrchr(file_name(), '.');

            if (!p) {
                assert(!"CGraphicText::OnLoadFromFile there is no extension (ie: .fnt)");
                name.clear();
            } else
                name.assign(file_name(), p - file_name());

            size = 12;
        }

        font_name_ = name;
        font_size_ = size;
        italic_ = italic;
        return font_texture_.create(name.c_str(), size, italic);
    }

    void graphic_text::on_clear() {
        font_texture_.destroy();
    }

    auto graphic_text::on_is_empty() const -> bool {
        return font_texture_.is_empty();
    }

    auto graphic_text::on_is_type(type_type type) -> bool {
        if (graphic_text::type() == type)
            return true;

        return resource::on_is_type(type);
    }
}
