#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace metin2::inline graphics {

    // One font file at one size, rendered with FreeType. The files come from the game packs (a
    // font_family lists them); there is no system font lookup, so a character the file lacks draws
    // its .notdef box.
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

        // The size is the em in interface pixels (what CSS calls px); the glyphs are rasterized at
        // size * font_scale * pixel_scale, so text drawn magnified by the interface scale still gets
        // freshly rendered pixels. The file is read through the file manager.
        auto open(const std::string &file, int size, float pixel_scale = 1.0f) -> bool;
        // The same from the bytes of a font file; key names them for the face cache and the logs.
        auto open(std::span<const uint8_t> bytes, std::string_view key, int size, float pixel_scale = 1.0f) -> bool;
        auto is_open() const -> bool { return face_ != nullptr; }
        auto pixel_scale() const -> float { return pixel_scale_; }

        auto glyph_of(char32_t codepoint) -> const glyph &;

        // Metrics in pixels of the rasterized size. The baseline is the rounded ascent from the top
        // of the line box, the cell height the line box rounded up; rounding each once keeps the
        // glyphs centred in the box (ceiling the ascent and descent apart pushes them down).
        auto ascent() const -> float { return ascent_; }
        auto descent() const -> float { return descent_; }
        auto baseline() const -> int;
        auto cell_height() const -> int;
        auto text_width(std::u32string_view text) -> int;

        // Draws into 32-bit pixels (0x00BBGGRR, rows of width pixels). With a background, each
        // character's advance by the cell height is filled with it first.
        void draw(uint32_t *pixels, int width, int height, int x, int y, std::u32string_view text, uint32_t color, std::optional<uint32_t> background = std::nullopt);

        // A loaded font file at one size (opaque; defined in font_face.cpp).
        struct face;

      private:
        auto open_face(std::span<const uint8_t> bytes, std::string_view key, int size, float pixel_scale) -> bool;

        int pixel_size_ = 0;
        float pixel_scale_ = 1.0f;
        float ascent_ = 0.0f;
        float descent_ = 0.0f;
        face *face_ = nullptr;
        std::map<char32_t, glyph> glyphs_;
    };
}
