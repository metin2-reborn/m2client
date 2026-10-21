#include "check.h"

#include <metin2/core/file_loader.h>

#include <string>

using namespace metin2;

namespace {
    auto loader_for(const std::string &text) -> memory_text_file_loader {
        memory_text_file_loader loader;
        loader.bind(static_cast<int>(text.size()), text.data());
        return loader;
    }
}

TEST(text_loader_splits_lines_and_tokens) {
    memory_text_file_loader loader = loader_for("first line\n\tsecond\tline  here\r\nthird");
    CHECK_EQ(loader.line_count(), uint32_t{3});

    token_vector tokens;
    CHECK(loader.split_line(1, &tokens));
    CHECK_EQ(tokens.size(), size_t{3});
    CHECK_EQ(tokens[0], std::string("second"));
    CHECK_EQ(tokens[2], std::string("here"));
}

TEST(text_loader_splits_by_tab) {
    memory_text_file_loader loader = loader_for("a\tb c\t\td");
    token_vector tokens;
    CHECK(loader.split_line_by_tab(0, &tokens));
    CHECK_EQ(tokens.size(), size_t{4});
    CHECK_EQ(tokens[1], std::string("b c"));
    CHECK_EQ(tokens[2], std::string());
}

TEST(text_loader_reports_quotes) {
    memory_text_file_loader loader = loader_for("name \"quoted value\" end\nopen \"quote");
    token_vector tokens;
    CHECK_EQ(loader.split_line2(0, &tokens), 0);
    CHECK_EQ(tokens.size(), size_t{3});
    CHECK_EQ(tokens[1], std::string("quoted value"));
    CHECK_EQ(loader.split_line2(1, &tokens), -2);
    CHECK(!loader.check_line_index(2));
}
