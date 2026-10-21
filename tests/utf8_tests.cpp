#include "check.h"

#include <metin2/core/utf8.h>

#include <string>

using namespace metin2;

namespace {
    const std::string mixed = "a\xC3\xA4\xE2\x82\xAC\xF0\x9F\x98\x80z"; // a ä € 😀 z
}

TEST(utf8_decodes_and_encodes) {
    const std::wstring wide = utf8::to_wide(mixed);
    CHECK(wide == (std::wstring{L'a', 0xE4, 0x20AC, 0x1F600, L'z'}));
    CHECK(utf8::from_wide(wide) == mixed);
    CHECK_EQ(utf8::length(mixed), size_t{5});
    CHECK(utf8::is_valid(mixed));
}

TEST(utf8_steps_whole_sequences) {
    CHECK_EQ(utf8::next_offset(mixed, 1), size_t{3});
    CHECK_EQ(utf8::next_offset(mixed, 3), size_t{6});
    CHECK_EQ(utf8::next_offset(mixed, 6), size_t{10});
    CHECK_EQ(utf8::prev(mixed, 10), size_t{6});
    CHECK_EQ(utf8::prev(mixed, 6), size_t{3});
    CHECK_EQ(utf8::prev(mixed, 3), size_t{1});
    CHECK_EQ(utf8::prev(mixed, 1), size_t{0});
}

TEST(utf8_truncation_keeps_whole_sequences) {
    for (size_t max = 0; max <= mixed.size(); ++max) {
        const size_t size = utf8::truncated_size(mixed, max);
        CHECK(size <= max);
        CHECK(utf8::is_valid(std::string_view(mixed).substr(0, size)));
    }
    CHECK_EQ(utf8::truncated_size(mixed, 2), size_t{1});
    CHECK_EQ(utf8::truncated_size(mixed, 5), size_t{3});
    CHECK_EQ(utf8::truncated_size(mixed, 9), size_t{6});

    char buffer[5];
    CHECK_EQ(utf8::copy_truncated(buffer, sizeof(buffer), mixed), size_t{3});
    CHECK_EQ(std::string(buffer), std::string("a\xC3\xA4"));
}

TEST(utf8_replaces_invalid_input) {
    CHECK(utf8::to_wide("\x80") == std::wstring{0xFFFD});
    CHECK(utf8::to_wide("\xE2\x82") == (std::wstring{0xFFFD, 0xFFFD}));
    CHECK(utf8::to_wide("\xC0\xAF") == (std::wstring{0xFFFD, 0xFFFD}));
    CHECK(utf8::to_wide("\xED\xA0\x80") == (std::wstring{0xFFFD, 0xFFFD, 0xFFFD}));
    CHECK(!utf8::is_valid("\xC7\xD1\xB1\xDB"));
    CHECK(utf8::is_valid("\xEF\xBF\xBD"));

    std::string encoded;
    utf8::append(encoded, 0x110000);
    CHECK_EQ(encoded, std::string("\xEF\xBF\xBD"));
}

TEST(utf8_decodes_to_utf32) {
    CHECK(utf8::to_u32("a\xC3\xA4\xE2\x82\xAC\xF0\x9F\x98\x80") == (std::u32string{U'a', U'ä', U'€', U'😀'}));
    CHECK(utf8::to_u32("\x80z") == (std::u32string{0xFFFD, U'z'}));
    CHECK(utf8::to_u32("").empty());
}
