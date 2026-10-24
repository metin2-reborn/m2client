#include <metin2/core/stl.h>
#include <metin2/core/utf8.h>
#include "grp_text.h"

#include <cmath>
#include <metin2/core/util.h>

namespace metin2::inline graphics {

    graphic_font_texture::graphic_font_texture() {
        initialize();
    }

    graphic_font_texture::~graphic_font_texture() {
        destroy();
    }

    void graphic_font_texture::initialize() {
        graphic_texture::initialize();
        is_dirty_ = false;
        italic_ = false;
    }

    auto graphic_font_texture::is_empty() const -> bool {
        return !face_.is_open();
    }

    void graphic_font_texture::destroy() {
        dib_.destroy();

        texture_.reset();
        graphic_texture::destroy();
        stl_wipe(font_texture_vector_);
        char_info_map_.clear();
        face_ = font_face();

        initialize();
    }

    auto graphic_font_texture::create_device_objects() -> bool {
        return true;
    }

    void graphic_font_texture::destroy_device_objects() {
    }

    auto graphic_font_texture::create(const char *font_name, int font_size, bool italic) -> bool {
        destroy();

        font_size_ = font_size;
        italic_ = italic;

        x_ = 0;
        y_ = 0;
        step_ = 0;

        uint32_t width_value = 256, height = 256;
        if (get_max_texture_width() > 512)
            width_value = 512;
        if (get_max_texture_height() > 512)
            height = 512;

        if (!dib_.create(width_value, height))
            return false;

        face_.open(font_name, font_size, false, italic);

        if (!append_texture())
            return false;

        return true;
    }

    auto graphic_font_texture::append_texture() -> bool {
        auto *new_texture = new graphic_image_texture;

        if (!new_texture->create(dib_.width(), dib_.height(), gpu::format::A8R8G8B8)) {
            delete new_texture;
            return false;
        }

        font_texture_vector_.push_back(new_texture);
        return true;
    }

    auto graphic_font_texture::update_texture() -> bool {
        if (!is_dirty_)
            return true;

        is_dirty_ = false;

        graphic_image_texture *font_texture = font_texture_vector_.back();

        if (!font_texture)
            return false;

        uint32_t *dst;
        int pitch;

        if (!font_texture->lock(&pitch, (void **)&dst))
            return false;

        pitch /= 4;

        int width_value = dib_.width();
        int height = dib_.height();

        auto *src = (uint32_t *)dib_.pointer();

        for (int y = 0; y < height; ++y, dst += pitch, src += width_value)
            for (int x = 0; x < width_value; ++x)
                dst[x] = src[x];

        font_texture->unlock();
        return true;
    }

    auto graphic_font_texture::get_character_infomation(wchar_t key_value) -> graphic_font_texture::character_infomation * {
        auto f = char_info_map_.find(key_value);

        if (char_info_map_.end() == f)
            return update_character_infomation(key_value);

        return &f->second;
    }

    auto graphic_font_texture::update_character_infomation(character_key code) -> graphic_font_texture::character_infomation * {
        if (!face_.is_open())
            return nullptr;

        const font_face::glyph &glyph = face_.glyph_of(code);

        // The glyph's box: its ink plus any overhang on either side, and one pixel of spacing.
        float ink_left = 0.0f;
        float ink_width = 0.0f;
        float ink_right = glyph.advance;
        if (glyph.width != 0) {
            ink_left = static_cast<float>(glyph.left);
            ink_width = static_cast<float>(glyph.width);
            ink_right = glyph.advance - glyph.left - glyph.width;
        }

        struct {
            int cx;
            int cy;
        } size;
        size.cx = static_cast<int>(ink_width);
        if (ink_left > 0.0f)
            size.cx += static_cast<int>(ceilf(ink_left));
        if (ink_right > 0.0f)
            size.cx += static_cast<int>(ceilf(ink_right));
        size.cx++;
        size.cy = face_.cell_height();

        int32_t advance = static_cast<int32_t>(ceilf(ink_left + ink_width + ink_right));

        int width_value = dib_.width();
        int height = dib_.height();

        if (x_ + size.cx >= (width_value - 1)) {
            y_ += (step_ + 1);
            step_ = 0;
            x_ = 0;

            if (y_ + size.cy >= (height - 1)) {
                if (!update_texture()) {
                    return nullptr;
                }

                if (!append_texture())
                    return nullptr;

                y_ = 0;
            }
        }

        const char32_t character = code;
        face_.draw(dib_.pixels(), dib_.width(), dib_.height(), x_, y_, std::u32string_view(&character, 1), 0x00ffffff, 0);

        int chr_x;
        int chr_y;
        int chr_width = size.cx;
        int chr_height = size.cy;
        int dib_width = dib_.width();

        auto *dib_data = (uint32_t *)dib_.pointer();
        uint32_t *dib_base = dib_data + dib_width * y_ + x_;
        uint32_t *dib_row;

        // The glyph was drawn white on black, so each channel holds its coverage. The atlas is
        // A8R8G8B8: white with the coverage as alpha (1-bit glyphs give 0 or 0xffffffff).
        dib_row = dib_base;
        for (chr_y = 0; chr_y < chr_height; ++chr_y, dib_row += dib_width) {
            for (chr_x = 0; chr_x < chr_width; ++chr_x) {
                const uint32_t coverage = dib_row[chr_x] & 0xff;
                dib_row[chr_x] = coverage ? (coverage << 24) | 0x00ffffff : 0;
            }
        }

        float rhwidth = 1.0f / float(width_value);
        float rhheight = 1.0f / float(height);

        character_infomation &new_char_info = char_info_map_[code];

        new_char_info.index_value = static_cast<short>(font_texture_vector_.size() - 1);
        new_char_info.width = size.cx;
        new_char_info.height = size.cy;
        new_char_info.left = float(x_) * rhwidth;
        new_char_info.top = float(y_) * rhheight;
        new_char_info.right = float(x_ + size.cx) * rhwidth;
        new_char_info.bottom = float(y_ + size.cy) * rhheight;
        new_char_info.advance = (float)advance;

        x_ += size.cx;

        if (step_ < size.cy)
            step_ = size.cy;

        is_dirty_ = true;

        return &new_char_info;
    }

    auto graphic_font_texture::check_texture_index(uint32_t texture) -> bool {
        if (texture >= font_texture_vector_.size())
            return false;

        return true;
    }

    void graphic_font_texture::select_texture(uint32_t texture) {
        assert(check_texture_index(texture));
        texture_ = font_texture_vector_[texture]->shared_texture();
    }
}
