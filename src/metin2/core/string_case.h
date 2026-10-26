#pragma once

#include <string_view>

namespace metin2::inline core {

    // ASCII case-insensitive comparisons for names, commands and extensions, which are ASCII;
    // strcasecmp is not portable.
    constexpr auto to_lower_ascii(char c) -> char {
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
    }

    constexpr auto iequals(std::string_view a, std::string_view b) -> bool {
        if (a.size() != b.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (to_lower_ascii(a[i]) != to_lower_ascii(b[i])) {
                return false;
            }
        }
        return true;
    }

    constexpr auto istarts_with(std::string_view text, std::string_view prefix) -> bool {
        return text.size() >= prefix.size() && iequals(text.substr(0, prefix.size()), prefix);
    }
}
