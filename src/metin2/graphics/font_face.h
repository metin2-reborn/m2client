#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace metin2::inline graphics {

    // A font rendered with FreeType. fontconfig picks the file for the family and, per character,
    // a fallback face for scripts the font lacks (Hangul, CJK, Cyrillic...).
    //
    // Glyphs are hinted 1-bit bitmaps unless antialiasing is configured: the Windows client got
    // aliased Tahoma at its small sizes.
    class font_face {
      public:
        // [interface] font_antialias and font_scale; applies to the fonts opened afterwards.
        static void configure(bool antialias, float scale);

        struct glyph {
            int left = 0; // bitmap offset from the pen position
            int top = 0;  // bitmap top above the baseline
            int width = 0;
            int rows = 0;
            float advance = 0.0f;
            std::vector<uint8_t> coverage; // width * rows, 0..255
        };

        // The height is the cell height (ascent + descent) in pixels, as the Windows client asked GDI
        // for its fonts.
        auto open(const char *family, int height, bool bold, bool italic) -> bool;
        auto is_open() const -> bool { return !faces_.empty(); }

        auto glyph_of(char32_t codepoint) -> const glyph &;

        auto ascent() const -> int { return ascent_; }
        auto cell_height() const -> int { return ascent_ + descent_; }
        auto text_width(std::u32string_view text) -> int;

        // Draws into 32-bit pixels (0x00BBGGRR, rows of width pixels). With a background, each
        // character's advance by the cell height is filled with it first.
        void draw(uint32_t *pixels, int width, int height, int x, int y, std::u32string_view text, uint32_t color, std::optional<uint32_t> background = std::nullopt);

        // A loaded font file at one size (opaque; defined in font_face.cpp).
        struct face;

      private:
        std::string family_;
        bool bold_ = false;
        bool italic_ = false;
        int pixel_size_ = 0;
        int ascent_ = 0;
        int descent_ = 0;
        std::vector<face *> faces_; // the matched face, then fallbacks as characters need them
        std::map<char32_t, glyph> glyphs_;
    };
}
