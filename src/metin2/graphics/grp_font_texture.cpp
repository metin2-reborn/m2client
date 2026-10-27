#include <metin2/core/stl.h>
#include <metin2/core/utf8.h>
#include "grp_base.h"
#include "grp_text.h"

#include <cmath>
#include <metin2/core/util.h>

namespace metin2::inline graphics {

    namespace {
        uint32_t next_generation = 0;
    }

    graphic_font_texture::graphic_font_texture() {
        initialize();
    }

    graphic_font_texture::~graphic_font_texture() {
        destroy();
    }

    void graphic_font_texture::initialize() {
        graphic_texture::initialize();
        is_dirty_ = false;
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

    auto graphic_font_texture::create(const std::string &file, int size) -> bool {
        destroy();

        generation_ = ++next_generation;

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

        if (!face_.open(file, size, graphic_base::interface_pixel_scale()))
            return false;

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

        // The cell is the ink box with one empty texel around it, wherever the ink lies relative to
        // the pen: an overhang to the left (j, italics) or above the ascent (stacked accents) must
        // not be cut off or land in a neighbour's cell. Point sampling never reads the padding, so
        // cells can sit side by side.
        const bool has_ink = glyph.width > 0 && glyph.rows > 0;
        const int cell_width = has_ink ? glyph.width + 2 : 0;
        const int cell_height = has_ink ? glyph.rows + 2 : 0;

        const int width_value = dib_.width();
        const int height = dib_.height();

        if (x_ + cell_width > width_value) {
            y_ += step_;
            step_ = 0;
            x_ = 0;
        }
        if (y_ + cell_height > height) {
            // A full page goes to its texture as it is; the next page starts from a clean buffer.
            if (!update_texture())
                return nullptr;
            if (!append_texture())
                return nullptr;
            std::fill_n(dib_.pixels(), static_cast<size_t>(width_value) * height, 0u);
            y_ = 0;
        }

        if (has_ink) {
            // White with the coverage as alpha (1-bit glyphs give 0 or 0xffffffff).
            uint32_t *row = dib_.pixels() + static_cast<size_t>(y_ + 1) * width_value + x_ + 1;
            for (int gy = 0; gy < glyph.rows; ++gy, row += width_value) {
                const uint8_t *coverage = glyph.coverage.data() + static_cast<size_t>(gy) * glyph.width;
                for (int gx = 0; gx < glyph.width; ++gx)
                    row[gx] = coverage[gx] ? (static_cast<uint32_t>(coverage[gx]) << 24) | 0x00ffffff : 0;
            }
            is_dirty_ = true;
        }

        const float rhwidth = 1.0f / float(width_value);
        const float rhheight = 1.0f / float(height);

        character_infomation &new_char_info = char_info_map_[code];

        new_char_info.index_value = static_cast<short>(font_texture_vector_.size() - 1);
        new_char_info.width = static_cast<short>(cell_width);
        new_char_info.height = static_cast<short>(cell_height);
        new_char_info.bearing_x = static_cast<short>(glyph.left - 1);
        new_char_info.bearing_y = static_cast<short>(face_.baseline() - glyph.top - 1);
        new_char_info.left = float(x_) * rhwidth;
        new_char_info.top = float(y_) * rhheight;
        new_char_info.right = float(x_ + cell_width) * rhwidth;
        new_char_info.bottom = float(y_ + cell_height) * rhheight;
        new_char_info.advance = glyph.advance;

        x_ += cell_width;
        step_ = std::max(step_, cell_height);

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
