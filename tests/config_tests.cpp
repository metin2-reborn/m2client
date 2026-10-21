#include "check.h"

#include <metin2/config/config_reader.h>

#include <cstdint>
#include <string>

using namespace metin2;

namespace {
    struct settings {
        uint32_t width = 800;
        bool windowed = false;
        float volume = 1.0f;
        uint8_t voice = 5;
        std::string name = "default";
    };

    auto read(const std::string &text) -> settings {
        settings out;
        toml::parse_result parsed = toml::parse(std::string_view(text), std::string_view("test.toml"));
        CHECK(static_cast<bool>(parsed));
        if (!parsed)
            return out;
        metin2::config::reader settings_reader(parsed.table(), "test.toml");
        settings_reader.read("display", "width", out.width);
        settings_reader.read("display", "windowed", out.windowed);
        settings_reader.read("sound", "volume", out.volume);
        settings_reader.read("sound", "voice", out.voice);
        settings_reader.read("login", "name", out.name);
        settings_reader.report_unknown_keys();
        return out;
    }
}

TEST(config_reads_every_type) {
    const settings s = read("[display]\nwidth = 1280\nwindowed = true\n[sound]\nvolume = 0.25\nvoice = 3\n[login]\nname = \"Hero\"\n");
    CHECK_EQ(s.width, uint32_t{1280});
    CHECK(s.windowed);
    CHECK_EQ(s.volume, 0.25f);
    CHECK_EQ(s.voice, uint8_t{3});
    CHECK_EQ(s.name, std::string("Hero"));
}

TEST(config_keeps_defaults_for_missing_keys) {
    const settings s = read("[display]\nwidth = 1024\n");
    CHECK_EQ(s.width, uint32_t{1024});
    CHECK(!s.windowed);
    CHECK_EQ(s.name, std::string("default"));
}

TEST(config_keeps_defaults_for_bad_values) {
    const settings s = read("[display]\nwidth = -5\nwindowed = 1\n[sound]\nvoice = 300\nvolume = \"loud\"\n[login]\nname = 7\n");
    CHECK_EQ(s.width, uint32_t{800});
    CHECK(!s.windowed);
    CHECK_EQ(s.voice, uint8_t{5});
    CHECK_EQ(s.volume, 1.0f);
    CHECK_EQ(s.name, std::string("default"));
}

TEST(config_accepts_whole_numbers_as_floats) {
    const settings s = read("[sound]\nvolume = 1\n[extra]\nunknown = true\n");
    CHECK_EQ(s.volume, 1.0f);
}
