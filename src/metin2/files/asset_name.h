#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <metin2/core/stl.h>

namespace metin2::inline files {

    // Case-insensitive (ASCII) name hashing and comparison for archive and entry lookups.
    namespace assets {
        struct case_insensitive_hash {
            using is_transparent = void;

            auto operator()(std::string_view s) const noexcept -> std::size_t {
                std::uint64_t h = 14695981039346656037ull;
                for (unsigned char c : s) {
                    h ^= static_cast<unsigned char>(ascii_tolower(static_cast<char>(c)));
                    h *= 1099511628211ull;
                }
                return static_cast<std::size_t>(h);
            }
        };

        struct case_insensitive_equal {
            using is_transparent = void;

            auto operator()(std::string_view a, std::string_view b) const noexcept -> bool {
                return a.size() == b.size() &&
                       std::equal(a.begin(), a.end(), b.begin(),
                           [](char x, char y) -> bool {
                               return ascii_tolower(x) == ascii_tolower(y);
                           });
            }
        };

        auto starts_with_no_case(std::string_view s, std::string_view prefix) noexcept -> bool;
    }
}
