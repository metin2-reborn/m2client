#include "check.h"

#include <metin2/graphics/font_face.h>

#include <cstdint>
#include <cstdio>
#include <string_view>
#include <vector>

using namespace metin2;

namespace {
    // The tests need some font installed; fontconfig substitutes any family.
    auto open_font(font_face &face) -> bool {
        if (face.open("Tahoma", 12, false, false))
            return true;
        std::puts("no font available, skipping");
        return false;
    }
}

TEST(font_has_sensible_metrics) {
    font_face face;
    if (!open_font(face))
        return;
    CHECK(face.ascent() > 0);
    CHECK(face.cell_height() > face.ascent());
    CHECK(face.cell_height() < 30);

    const font_face::glyph &g = face.glyph_of(U'W');
    CHECK(g.width > 0);
    CHECK(g.rows > 0);
    CHECK(g.advance > 0.0f);
    CHECK_EQ(g.coverage.size(), static_cast<size_t>(g.width) * g.rows);
    for (uint8_t coverage : face.glyph_of(U' ').coverage)
        CHECK_EQ(coverage, uint8_t{0});
}

TEST(font_text_width_adds_the_advances) {
    font_face face;
    if (!open_font(face))
        return;
    const float advances = face.glyph_of(U'a').advance + face.glyph_of(U'b').advance + face.glyph_of(U'c').advance;
    CHECK_EQ(face.text_width(U"abc"), static_cast<int>(advances + 0.999f));
    CHECK_EQ(face.text_width(U""), 0);
}

TEST(font_draws_inside_the_buffer) {
    font_face face;
    if (!open_font(face))
        return;
    const int width = 64;
    const int height = 20;
    std::vector<uint32_t> pixels(width * height + 1, 0x12345678);
    pixels.back() = 0xdeadbeef; // guard past the end

    face.draw(pixels.data(), width, height, 2, 1, U"Hi", 0x00ffffff, 0);
    int ink = 0;
    int background = 0;
    for (int i = 0; i < width * height; ++i) {
        ink += pixels[i] == 0x00ffffff;
        background += pixels[i] == 0;
    }
    CHECK(ink > 0);
    CHECK(background > 0);
    CHECK_EQ(pixels[0], uint32_t{0x12345678}); // left of the pen and above the cell
    CHECK_EQ(pixels.back(), uint32_t{0xdeadbeef});

    // Text running off every edge is clipped.
    face.draw(pixels.data(), width, height, -5, -5, U"WWWWWWWWWWWW", 0x00ff0000, 0x00000001);
    face.draw(pixels.data(), width, height, 50, 15, U"WWWW", 0x00ff0000);
    CHECK_EQ(pixels.back(), uint32_t{0xdeadbeef});
}

TEST(font_falls_back_per_character) {
    font_face face;
    if (!open_font(face))
        return;
    // Whatever face covers Hangul, the glyph must come from somewhere and advance the pen.
    CHECK(face.glyph_of(U'안').advance > 0.0f);
}

TEST(font_pixel_scale_rasterizes_larger) {
    font_face face;
    if (!open_font(face))
        return;
    font_face doubled;
    CHECK(doubled.open("Tahoma", 12, false, false, 2.0f));
    CHECK_EQ(doubled.pixel_scale(), 2.0f);
    CHECK_EQ(face.pixel_scale(), 1.0f);

    // Twice the pixels in every direction (give or take the rounding of the metrics), with glyph
    // bitmaps rendered at that size rather than enlarged.
    CHECK(doubled.cell_height() >= face.cell_height() * 2 - 2);
    CHECK(doubled.cell_height() <= face.cell_height() * 2 + 2);
    const font_face::glyph &w = face.glyph_of(U'W');
    const font_face::glyph &w2 = doubled.glyph_of(U'W');
    CHECK(w2.width >= w.width * 2 - 2);
    CHECK(w2.width <= w.width * 2 + 2);
    CHECK(w2.advance > w.advance * 1.8f);
    CHECK(w2.advance < w.advance * 2.2f);
}

TEST(font_glyph_overhangs_are_kept) {
    font_face face;
    if (!open_font(face))
        return;
    // The atlas cell is the ink box wherever it lies: a glyph may start left of the pen or reach
    // above the ascent, and the bitmap holds all of it.
    for (char32_t c : std::u32string_view(U"jÁÂÃ안W")) {
        const font_face::glyph &g = face.glyph_of(c);
        CHECK(g.width > 0);
        CHECK(g.rows > 0);
        CHECK_EQ(g.coverage.size(), static_cast<size_t>(g.width) * g.rows);
        bool ink = false;
        for (uint8_t coverage : g.coverage)
            ink = ink || coverage != 0;
        CHECK(ink);
    }
    // An accented capital is taller than the ascent only if its bitmap says so; either way its top
    // row carries ink, so a cell trimmed to the ascent would lose it.
    const font_face::glyph &a = face.glyph_of(U'Á');
    bool top_row_ink = false;
    for (int x = 0; x < a.width; ++x)
        top_row_ink = top_row_ink || a.coverage[static_cast<size_t>(x)] != 0;
    CHECK(top_row_ink);
}
