#include "check.h"

#include <metin2/client/guilds/mark_image.h>
#include <metin2/graphics/grp_expanded_image_instance.h>
#include <metin2/graphics/image_file.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

using namespace metin2;

namespace {
    auto temp_path(const char *name) -> std::string {
        static const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        return (std::filesystem::temp_directory_path() / ("metin2_image_tests_" + std::to_string(unique) + "_" + name)).string();
    }
}

TEST(tga_round_trips_bgra_pixels) {
    const std::string path = temp_path("round.tga");
    const std::vector<uint32_t> pixels = {0xff112233, 0x80445566, 0x00778899, 0xffaabbcc, 0x01020304, 0xfffefdfc};
    CHECK(save_tga_file(path.c_str(), 3, 2, pixels.data()));

    bgra_image image;
    CHECK(load_image_file(path.c_str(), image));
    CHECK_EQ(image.width, 3);
    CHECK_EQ(image.height, 2);
    CHECK(image.pixels == pixels);
    std::filesystem::remove(path);
}

TEST(missing_image_does_not_load) {
    bgra_image image;
    CHECK(!load_image_file(temp_path("missing.tga").c_str(), image));
}

TEST(jpeg_is_written) {
    const std::string path = temp_path("shot.jpg");
    std::vector<uint8_t> rgb(16 * 8 * 3, 200);
    CHECK(save_jpeg_file(path.c_str(), 16, 8, rgb.data(), 100));
    bgra_image image;
    CHECK(load_image_file(path.c_str(), image));
    CHECK_EQ(image.width, 16);
    CHECK_EQ(image.height, 8);
    std::filesystem::remove(path);
}

TEST(guild_mark_image_keeps_marks_through_a_file) {
    const std::string path = temp_path("mark.tga");
    {
        // A missing file is created empty.
        guild_mark_image image;
        CHECK(image.load(path.c_str()));
        std::vector<uint32_t> mark(guild_mark::SIZE);
        for (size_t i = 0; i < mark.size(); ++i)
            mark[i] = 0xff000000 | static_cast<uint32_t>(i * 977);
        image.put_data(32, 24, guild_mark::WIDTH, guild_mark::HEIGHT, mark.data());
        CHECK(image.save(path.c_str()));
    }
    guild_mark_image image;
    CHECK(image.load(path.c_str()));
    std::vector<uint32_t> mark(guild_mark::SIZE);
    image.get_data(32, 24, guild_mark::WIDTH, guild_mark::HEIGHT, mark.data());
    for (size_t i = 0; i < mark.size(); ++i)
        CHECK_EQ(mark[i], 0xff000000 | static_cast<uint32_t>(i * 977));
    std::vector<uint32_t> empty(guild_mark::SIZE, 1);
    image.get_data(0, 0, guild_mark::WIDTH, guild_mark::HEIGHT, empty.data());
    CHECK(empty == std::vector<uint32_t>(guild_mark::SIZE, 0));
    std::filesystem::remove(path);
}

TEST(image_fit_stretches_covers_and_contains) {
    using fit = graphic_expanded_image_instance;
    // 1024x768 into 1280x720.
    const fit::fit_rect stretched = fit::fit(fit::FIT_STRETCH, 1024, 768, 1280, 720);
    CHECK_EQ(stretched.x, 0.0f);
    CHECK_EQ(stretched.width, 1280.0f);
    CHECK_EQ(stretched.height, 720.0f);
    CHECK_EQ(stretched.u0, 0.0f);
    CHECK_EQ(stretched.v1, 1.0f);

    // Cover scales by 1.25 (1280x960): the full width, 240 image rows cropped as 120 drawn rows each side.
    const fit::fit_rect covered = fit::fit(fit::FIT_COVER, 1024, 768, 1280, 720);
    CHECK_EQ(covered.x, 0.0f);
    CHECK_EQ(covered.y, 0.0f);
    CHECK_EQ(covered.width, 1280.0f);
    CHECK_EQ(covered.height, 720.0f);
    CHECK_EQ(covered.u0, 0.0f);
    CHECK_EQ(covered.u1, 1.0f);
    CHECK_EQ(covered.v0, 0.125f);
    CHECK_EQ(covered.v1, 0.875f);

    // Contain scales by 0.9375 (960x720): centred with 160 px margins at the sides.
    const fit::fit_rect contained = fit::fit(fit::FIT_CONTAIN, 1024, 768, 1280, 720);
    CHECK_EQ(contained.x, 160.0f);
    CHECK_EQ(contained.y, 0.0f);
    CHECK_EQ(contained.width, 960.0f);
    CHECK_EQ(contained.height, 720.0f);
    CHECK_EQ(contained.u0, 0.0f);
    CHECK_EQ(contained.v1, 1.0f);

    CHECK_EQ(fit::fit(fit::FIT_COVER, 0, 768, 1280, 720).width, 0.0f);
}
