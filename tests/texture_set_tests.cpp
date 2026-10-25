#include "check.h"

#include <metin2/world/terrain/texture_set.h>

#include <string>

using namespace metin2;

namespace {
    auto parse(const std::string &text) -> std::vector<texture_set_entry> {
        const auto entries = parse_texture_set(text, "test.toml");
        CHECK(entries.has_value());
        return entries.value_or(std::vector<texture_set_entry>{});
    }
}

TEST(texture_set_reads_entries) {
    const auto entries = parse("[[texture]]\nindex = 1\npath = \"Textures:terrainmaps/a.png\"\nuv_scale = [5.0, 6]\nuv_offset = [0.5, 0.25]\n"
                               "[[texture]]\nindex = 3\npath = \"Textures:terrainmaps/b.png\"\n");
    CHECK_EQ(entries.size(), std::size_t{2});
    CHECK_EQ(entries[0].index, uint32_t{1});
    CHECK_EQ(entries[0].path, std::string("Textures:terrainmaps/a.png"));
    CHECK_EQ(entries[0].u_scale, 5.0f);
    CHECK_EQ(entries[0].v_scale, 6.0f);
    CHECK_EQ(entries[0].u_offset, 0.5f);
    CHECK_EQ(entries[0].v_offset, 0.25f);
    // Index 2 is a gap; missing pairs keep the defaults.
    CHECK_EQ(entries[1].index, uint32_t{3});
    CHECK_EQ(entries[1].u_scale, 4.0f);
    CHECK_EQ(entries[1].v_offset, 0.0f);
}

TEST(texture_set_leaves_out_bad_entries) {
    const auto entries = parse("[[texture]]\nindex = 1\n"                                   // no path
                               "[[texture]]\nindex = 256\npath = \"a.dds\"\n"               // out of range
                               "[[texture]]\nindex = 0\npath = \"a.dds\"\n"                 // out of range
                               "[[texture]]\nindex = \"2\"\npath = \"a.dds\"\n"             // not a number
                               "[[texture]]\nindex = 2\npath = \"a.dds\"\nuv_scale = [1]\n" // one number
                               "[[texture]]\nindex = 2\npath = \"a.dds\"\nuv_offset = [\"x\", 1]\n"
                               "[[texture]]\nindex = 5\npath = \"ok.dds\"\n"
                               "[[texture]]\nindex = 5\npath = \"twice.dds\"\n");
    CHECK_EQ(entries.size(), std::size_t{1});
    CHECK_EQ(entries[0].path, std::string("ok.dds"));
}

TEST(texture_set_keeps_entries_with_unknown_keys) {
    const auto entries = parse("version = 2\n[[texture]]\nindex = 1\npath = \"a.dds\"\nsplat = 0\n");
    CHECK_EQ(entries.size(), std::size_t{1});
}

TEST(texture_set_rejects_text_that_is_not_toml) {
    CHECK(!parse_texture_set("TextureSet\nTextureCount 1\n", "old.txt").has_value());
    CHECK(parse_texture_set("", "empty.toml").value().empty());
}
