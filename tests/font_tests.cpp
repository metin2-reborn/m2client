#include "check.h"

#include <metin2/graphics/font_face.h>
#include <metin2/graphics/font_family.h>
#include <metin2/graphics/grp_text.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string_view>
#include <vector>

using namespace metin2;

namespace {
    // The fonts live in the game data checkout next to this repo; without it the face tests skip.
    constexpr const char *REGULAR = M2_GAME_DIR "/data/ui/fonts/PTSans-Regular.ttf";

    auto read_file(const char *path) -> std::vector<uint8_t> {
        std::ifstream in(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    }

    auto open_font(font_face &face, float pixel_scale = 1.0f) -> bool {
        static const std::vector<uint8_t> bytes = read_file(REGULAR);
        if (bytes.empty()) {
            std::puts("no font available, skipping");
            return false;
        }
        return face.open(bytes, "ptsans-regular", 12, pixel_scale);
    }

    constexpr std::string_view FAMILY_TOML = R"([asset]
type = "Font"

[normal]
[[normal.weights]]
weight = 400
path = "ui:fonts/PTSans-Regular.ttf"
[[normal.weights]]
weight = 700
path = "ui:fonts/PTSans-Bold.ttf"

[italic]
[[italic.weights]]
weight = 400
path = "ui:fonts/PTSans-Italic.ttf"
)";
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

TEST(font_size_is_the_em) {
    font_face face;
    if (!open_font(face))
        return;
    // An em of 12 px: the cell (ascent + descent) of a text face is a little over the em.
    CHECK(face.cell_height() >= 12);
    CHECK(face.cell_height() <= 18);
    // The metrics are fractional and rounded once each: the baseline to the nearest pixel, the
    // cell up, so the glyphs are not pushed down by two separate ceilings.
    CHECK(face.ascent() > 0.0f);
    CHECK(face.descent() > 0.0f);
    CHECK_EQ(face.baseline(), static_cast<int>(lroundf(face.ascent())));
    CHECK_EQ(face.cell_height(), static_cast<int>(ceilf(face.ascent() + face.descent())));
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

TEST(font_missing_glyph_draws_notdef) {
    font_face face;
    if (!open_font(face))
        return;
    // PT Sans has no Hangul: the character gets the font's .notdef box, not nothing.
    const font_face::glyph &g = face.glyph_of(U'안');
    CHECK(g.advance > 0.0f);
    CHECK(g.width > 0);
    bool ink = false;
    for (uint8_t coverage : g.coverage)
        ink = ink || coverage != 0;
    CHECK(ink);
}

TEST(font_pixel_scale_rasterizes_larger) {
    font_face face;
    if (!open_font(face))
        return;
    font_face doubled;
    CHECK(open_font(doubled, 2.0f));
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
    for (char32_t c : std::u32string_view(U"jÁÂÃW")) {
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

TEST(font_family_parses_styles_and_weights) {
    font_family::table files;
    CHECK(font_family::parse(FAMILY_TOML, "ptsans.toml", files));
    CHECK_EQ(files.normal.size(), size_t{2});
    CHECK_EQ(files.italic.size(), size_t{1});
    CHECK_EQ(files.normal[0].weight, 400);
    CHECK_EQ(files.normal[1].path, std::string("ui:fonts/PTSans-Bold.ttf"));
}

TEST(font_family_rejects_bad_tables) {
    font_family::table files;
    CHECK(!font_family::parse("[asset]\ntype = \"Font\"\n[italic]\n[[italic.weights]]\nweight = 400\npath = \"a.ttf\"\n", "no_normal.toml", files));
    CHECK(!font_family::parse("[asset]\ntype = \"Font\"\n[normal]\n[[normal.weights]]\nweight = 0\npath = \"a.ttf\"\n", "bad_weight.toml", files));
    CHECK(!font_family::parse("[asset]\ntype = \"Font\"\n[normal]\n[[normal.weights]]\nweight = 400\npath = \"a.ttf\"\n[[normal.weights]]\nweight = 400\npath = \"b.ttf\"\n", "twice.toml", files));
    CHECK(!font_family::parse("[asset]\ntype = \"Font\"\n[normal]\n[[normal.weights]]\nweight = 400\n", "no_path.toml", files));
    CHECK(!font_family::parse("[asset]\ntype = \"NinePatch\"\n[normal]\n", "wrong_type.toml", files));
}

TEST(font_family_picks_the_closest_weight) {
    font_family::table files;
    CHECK(font_family::parse(FAMILY_TOML, "ptsans.toml", files));
    CHECK_EQ(font_family::file_for(files, false, 400), std::string("ui:fonts/PTSans-Regular.ttf"));
    CHECK_EQ(font_family::file_for(files, false, 700), std::string("ui:fonts/PTSans-Bold.ttf"));
    CHECK_EQ(font_family::file_for(files, false, 300), std::string("ui:fonts/PTSans-Regular.ttf"));
    CHECK_EQ(font_family::file_for(files, false, 900), std::string("ui:fonts/PTSans-Bold.ttf"));
    // Equally close weights go to the heavier one.
    CHECK_EQ(font_family::file_for(files, false, 550), std::string("ui:fonts/PTSans-Bold.ttf"));
    // Italic has only a regular; a style the font lacks falls back to normal.
    CHECK_EQ(font_family::file_for(files, true, 700), std::string("ui:fonts/PTSans-Italic.ttf"));
    font_family::table upright;
    upright.normal = files.normal;
    CHECK_EQ(font_family::file_for(upright, true, 700), std::string("ui:fonts/PTSans-Bold.ttf"));
    const font_family::table empty;
    CHECK_EQ(font_family::file_for(empty, false, 400), std::string());
}

TEST(font_resource_name_round_trips) {
    const font_spec spec{"ui:fonts/ptsans.toml", 14, 700, true};
    const std::string name = graphic_text::resource_name(spec);
    CHECK_EQ(name, std::string("ui:fonts/ptsans.toml:14:700:italic.fnt"));
    const auto parsed = graphic_text::parse_resource_name(name);
    CHECK(parsed.has_value());
    CHECK(parsed && *parsed == spec);

    const auto plain = graphic_text::parse_resource_name("fonts/x.toml:12:400:normal.fnt");
    CHECK(plain.has_value());
    CHECK(plain && plain->family == "fonts/x.toml" && plain->size == 12 && plain->weight == 400 && !plain->italic);

    CHECK(!graphic_text::parse_resource_name("tahoma:14.fnt").has_value());
    CHECK(!graphic_text::parse_resource_name("ui:fonts/x.toml:12:400:bold.fnt").has_value());
    CHECK(!graphic_text::parse_resource_name("ui:fonts/x.toml:0:400:normal.fnt").has_value());
    CHECK(!graphic_text::parse_resource_name("ui:fonts/x.toml:12:400:normal").has_value());
}
