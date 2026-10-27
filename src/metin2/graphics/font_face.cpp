#include "font_face.h"

#include <metin2/core/mapped_file.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

namespace metin2::inline graphics {

    struct font_face::face {
        FT_Face ft = nullptr;
    };

    namespace {
        struct library {
            FT_Library ft = nullptr;
            bool antialias = false;
            float scale = 1.0f;
            // FreeType reads a memory face's bytes for as long as the face exists, and faces are
            // shared by every font of the same file and size and live until exit; so do the bytes.
            std::map<std::string, std::vector<uint8_t>> files;
            std::map<std::string, std::unique_ptr<font_face::face>> faces;

            library() {
                FT_Init_FreeType(&ft);
            }
        };

        auto get_library() -> library & {
            static library instance;
            return instance;
        }

        auto load_face(std::span<const uint8_t> bytes, const std::string &key, int pixel_size) -> font_face::face * {
            library &lib = get_library();
            const std::string face_key = key + ":" + std::to_string(pixel_size);
            if (auto it = lib.faces.find(face_key); it != lib.faces.end())
                return it->second.get();

            auto [stored, inserted] = lib.files.try_emplace(key);
            if (inserted)
                stored->second.assign(bytes.begin(), bytes.end());
            const std::vector<uint8_t> &data = stored->second;

            FT_Face ft = nullptr;
            if (FT_New_Memory_Face(lib.ft, data.data(), static_cast<FT_Long>(data.size()), 0, &ft) != 0) {
                metin2::log::error("{} is not a font file FreeType can read", key);
                return nullptr;
            }
            if (FT_Set_Pixel_Sizes(ft, 0, static_cast<FT_UInt>(pixel_size)) != 0) {
                metin2::log::error("{} cannot be set to {} pixels", key, pixel_size);
                FT_Done_Face(ft);
                return nullptr;
            }
            auto f = std::make_unique<font_face::face>();
            f->ft = ft;
            font_face::face *raw = f.get();
            lib.faces[face_key] = std::move(f);
            return raw;
        }
    }

    void font_face::configure(bool antialias, float scale) {
        library &lib = get_library();
        lib.antialias = antialias;
        lib.scale = scale > 0.1f ? scale : 1.0f;
    }

    auto font_face::open(const std::string &file, int size, float pixel_scale) -> bool {
        // The face cache is keyed by the file, so the bytes are read only for the first size.
        const std::string key = eter_file_manager::canonical_name(file);
        if (library &lib = get_library(); lib.files.contains(key))
            return open_face({}, key, size, pixel_scale);

        mapped_file mapped;
        const void *data = nullptr;
        if (!get_file_manager().get(mapped, file.c_str(), &data)) {
            metin2::log::error("Font file {} not found", file);
            face_ = nullptr;
            glyphs_.clear();
            return false;
        }
        return open_face(std::span(static_cast<const uint8_t *>(data), mapped.size()), key, size, pixel_scale);
    }

    auto font_face::open(std::span<const uint8_t> bytes, std::string_view key, int size, float pixel_scale) -> bool {
        return open_face(bytes, key, size, pixel_scale);
    }

    auto font_face::open_face(std::span<const uint8_t> bytes, std::string_view key, int size, float pixel_scale) -> bool {
        library &lib = get_library();

        pixel_scale_ = pixel_scale > 0.0f ? pixel_scale : 1.0f;
        face_ = nullptr;
        glyphs_.clear();

        const float em = size > 0 ? static_cast<float>(size) : 12.0f;
        pixel_size_ = std::max(1, static_cast<int>(lroundf(em * lib.scale * pixel_scale_)));

        face_ = load_face(bytes, std::string(key), pixel_size_);
        if (!face_)
            return false;

        // size->metrics.ascender/descender come rounded up to whole pixels; the design values
        // scaled by hand keep the fraction, so the baseline can be rounded once where it is used.
        const FT_Size_Metrics &metrics = face_->ft->size->metrics;
        ascent_ = static_cast<float>(FT_MulFix(face_->ft->ascender, metrics.y_scale)) / 64.0f;
        descent_ = static_cast<float>(-FT_MulFix(face_->ft->descender, metrics.y_scale)) / 64.0f;
        return true;
    }

    auto font_face::baseline() const -> int {
        return static_cast<int>(lroundf(ascent_));
    }

    auto font_face::cell_height() const -> int {
        return static_cast<int>(ceilf(ascent_ + descent_));
    }

    auto font_face::glyph_of(char32_t codepoint) -> const glyph & {
        if (auto it = glyphs_.find(codepoint); it != glyphs_.end())
            return it->second;

        glyph &g = glyphs_[codepoint];
        if (!face_)
            return g;

        // Index 0 is the font's .notdef glyph: a character the file lacks draws that box.
        const FT_UInt glyph_index = FT_Get_Char_Index(face_->ft, codepoint);

        // The auto-hinter in normal mode snaps stems to whole pixels in both directions, so at the
        // interface sizes (12 to 16 px) verticals are one solid column instead of two grey ones.
        // Light hinting (vertical only) and the font's own bytecode both leave them grey.
        const FT_Int32 flags = FT_LOAD_RENDER | (get_library().antialias ? FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO);
        if (FT_Load_Glyph(face_->ft, glyph_index, flags) != 0)
            return g;

        const FT_GlyphSlot slot = face_->ft->glyph;
        const FT_Bitmap &bitmap = slot->bitmap;
        g.left = slot->bitmap_left;
        g.top = slot->bitmap_top;
        g.width = static_cast<int>(bitmap.width);
        g.rows = static_cast<int>(bitmap.rows);
        g.advance = slot->advance.x / 64.0f;
        g.coverage.assign(static_cast<size_t>(g.width) * g.rows, 0);
        for (int y = 0; y < g.rows; ++y) {
            const uint8_t *row = bitmap.buffer + static_cast<ptrdiff_t>(y) * bitmap.pitch;
            for (int x = 0; x < g.width; ++x) {
                const uint8_t value = bitmap.pixel_mode == FT_PIXEL_MODE_MONO ? (((row[x >> 3] >> (7 - (x & 7))) & 1) ? 255 : 0) : row[x];
                g.coverage[static_cast<size_t>(y) * g.width + x] = value;
            }
        }
        return g;
    }

    auto font_face::text_width(std::u32string_view text) -> int {
        float width = 0.0f;
        for (char32_t c : text)
            width += glyph_of(c).advance;
        return static_cast<int>(ceilf(width));
    }

    void font_face::draw(uint32_t *pixels, int width, int height, int x, int y, std::u32string_view text, uint32_t color, std::optional<uint32_t> background) {
        float pen = static_cast<float>(x);
        for (char32_t c : text) {
            const glyph &g = glyph_of(c);
            const int pen_x = static_cast<int>(lroundf(pen));

            if (background) {
                const int advance = static_cast<int>(ceilf(g.advance));
                for (int row = std::max(y, 0); row < std::min(y + cell_height(), height); ++row)
                    for (int column = std::max(pen_x, 0); column < std::min(pen_x + advance, width); ++column)
                        pixels[static_cast<size_t>(row) * width + column] = *background;
            }

            const int origin_x = pen_x + g.left;
            const int origin_y = y + baseline() - g.top;
            for (int gy = 0; gy < g.rows; ++gy) {
                const int dy = origin_y + gy;
                if (dy < 0 || dy >= height)
                    continue;
                for (int gx = 0; gx < g.width; ++gx) {
                    const int dx = origin_x + gx;
                    if (dx < 0 || dx >= width)
                        continue;
                    const uint8_t coverage = g.coverage[static_cast<size_t>(gy) * g.width + gx];
                    if (!coverage)
                        continue;
                    uint32_t &pixel = pixels[static_cast<size_t>(dy) * width + dx];
                    if (coverage == 255) {
                        pixel = color;
                        continue;
                    }
                    uint32_t blended = 0;
                    for (int channel = 0; channel < 3; ++channel) {
                        const uint32_t under = (pixel >> (channel * 8)) & 255;
                        const uint32_t over = (color >> (channel * 8)) & 255;
                        blended |= ((under * (255 - coverage) + over * coverage) / 255) << (channel * 8);
                    }
                    pixel = blended;
                }
            }
            pen += g.advance;
        }
    }
}
