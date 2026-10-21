#include "check.h"

#include <metin2/core/parse.h>

using namespace metin2;

TEST(parse_reads_numbers_like_atoi) {
    CHECK_EQ(metin2::to_number<int>("42"), 42);
    CHECK_EQ(metin2::to_number<int>("  -7"), -7);
    CHECK_EQ(metin2::to_number<int>("+3 apples"), 3);
    CHECK_EQ(metin2::to_number<int>("apples"), 0);
    CHECK_EQ(metin2::to_number<int>(""), 0);
    CHECK_EQ(metin2::to_number<double>("1.5"), 1.5);
    CHECK_EQ(metin2::to_number<double>(" .25"), 0.25);
    CHECK_EQ(metin2::to_number<float>("-2.5e1"), -25.0f);
}

TEST(parse_reports_errors) {
    CHECK(!metin2::parse<int>("x").has_value());
    CHECK(metin2::parse<int>("0").has_value());
    CHECK(metin2::parse<bool>("1").value());
}
