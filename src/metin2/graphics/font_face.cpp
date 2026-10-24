#include "font_face.h"

#include <metin2/log/log.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace metin2::inline graphics {

    struct font_face::face {
        FT_Face ft = nullptr;
    };

    namespace {
        struct library {
            FT_Library ft = nullptr;
            FcConfig *fc = nullptr;
            bool antialias = false;
            float scale = 1.0f;
            // Faces are shared by every font of the same file, index and size, and live until exit.
            std::map<std::string, std::unique_ptr<font_face::face>> faces;

            library() {
                FT_Init_FreeType(&ft);
                fc = FcInitLoadConfigAndFonts();
            }
        };

        auto get_library() -> library & {
            static library instance;
            return instance;
        }
    }

    void font_face::configure(bool antialias, float scale) {
        library &lib = get_library();
        lib.antialias = antialias;
        lib.scale = scale > 0.1f ? scale : 1.0f;
    }

    namespace {

        auto load_face(const std::string &path, int index, int pixel_size) -> font_face::face * {
            library &lib = get_library();
            const std::string key = path + ":" + std::to_string(index) + ":" + std::to_string(pixel_size);
            if (auto it = lib.faces.find(key); it != lib.faces.end())
                return it->second.get();

            FT_Face ft = nullptr;
            if (FT_New_Face(lib.ft, path.c_str(), index, &ft) != 0)
                return nullptr;
            if (FT_Set_Pixel_Sizes(ft, 0, static_cast<FT_UInt>(pixel_size)) != 0) {
                FT_Done_Face(ft);
                return nullptr;
            }
            auto f = std::make_unique<font_face::face>();
            f->ft = ft;
            font_face::face *raw = f.get();
            lib.faces[key] = std::move(f);
            return raw;
        }

        // The best file for the family and style, or for the one character when it is given.
        auto match_font(const std::string &family, bool bold, bool italic, std::optional<char32_t> codepoint, std::string &path, int &index) -> bool {
            library &lib = get_library();
            FcPattern *pattern = FcPatternCreate();
            FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8 *>(family.c_str()));
            FcPatternAddInteger(pattern, FC_WEIGHT, bold ? FC_WEIGHT_BOLD : FC_WEIGHT_REGULAR);
            FcPatternAddInteger(pattern, FC_SLANT, italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
            FcCharSet *charset = nullptr;
            if (codepoint) {
                charset = FcCharSetCreate();
                FcCharSetAddChar(charset, *codepoint);
                FcPatternAddCharSet(pattern, FC_CHARSET, charset);
            }
            FcConfigSubstitute(lib.fc, pattern, FcMatchPattern);
            FcDefaultSubstitute(pattern);
            FcResult result;
            FcPattern *match = FcFontMatch(lib.fc, pattern, &result);
            FcPatternDestroy(pattern);
            if (charset)
                FcCharSetDestroy(charset);
            if (!match)
                return false;

            FcChar8 *file = nullptr;
            int file_index = 0;
            const bool found = FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch;
            FcPatternGetInteger(match, FC_INDEX, 0, &file_index);
            if (found) {
                path = reinterpret_cast<const char *>(file);
                index = file_index;
            }
            FcPatternDestroy(match);
            return found;
        }
    }

    auto font_face::open(const char *family, int height, bool bold, bool italic) -> bool {
        library &lib = get_library();

        family_ = family && *family ? family : "Tahoma";
        bold_ = bold;
        italic_ = italic;
        faces_.clear();
        glyphs_.clear();

        // Tahoma's cell is 1.207 em, so "Tahoma 12" is a 10 px em; the same ratio keeps the UI's
        // proportions with whatever font fontconfig picked.
        const float em = height > 0 ? static_cast<float>(height) * 2048.0f / 2472.0f : height < 0 ? static_cast<float>(-height)
                                                                                                  : 10.0f;
        pixel_size_ = std::max(1, static_cast<int>(lroundf(em * lib.scale)));

        std::string path;
        int index = 0;
        face *f = match_font(family_, bold_, italic_, std::nullopt, path, index) ? load_face(path, index, pixel_size_) : nullptr;
        if (!f) {
            metin2::log::error("No usable font for {} (fontconfig)", family_);
            return false;
        }

        faces_.push_back(f);
        ascent_ = static_cast<int>(ceilf(f->ft->size->metrics.ascender / 64.0f));
        descent_ = static_cast<int>(ceilf(-f->ft->size->metrics.descender / 64.0f));
        return true;
    }

    auto font_face::glyph_of(char32_t codepoint) -> const glyph & {
        if (auto it = glyphs_.find(codepoint); it != glyphs_.end())
            return it->second;

        glyph &g = glyphs_[codepoint];
        if (faces_.empty())
            return g;

        face *use = nullptr;
        FT_UInt glyph_index = 0;
        for (face *f : faces_) {
            glyph_index = FT_Get_Char_Index(f->ft, codepoint);
            if (glyph_index) {
                use = f;
                break;
            }
        }
        if (!use) {
            std::string path;
            int index = 0;
            if (match_font(family_, bold_, italic_, codepoint, path, index)) {
                if (face *f = load_face(path, index, pixel_size_)) {
                    glyph_index = FT_Get_Char_Index(f->ft, codepoint);
                    if (glyph_index) {
                        use = f;
                        faces_.push_back(f);
                    }
                }
            }
        }
        if (!use) {
            use = faces_.front();
            glyph_index = 0;
        }

        // Light hinting snaps only vertically, so antialiased glyphs keep their designed widths.
        const FT_Int32 flags = FT_LOAD_RENDER | (get_library().antialias ? FT_LOAD_TARGET_LIGHT : FT_LOAD_TARGET_MONO);
        if (FT_Load_Glyph(use->ft, glyph_index, flags) != 0)
            return g;

        const FT_GlyphSlot slot = use->ft->glyph;
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
            const int origin_y = y + ascent_ - g.top;
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
