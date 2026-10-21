#pragma once

#include <cctype>
#include <charconv>
#include <optional>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace metin2 {
    // Leading whitespace and a '+' are skipped; parsing stops at the first character that does not
    // belong to the number (as atoi/atof did), but a text without any digits is an error.
    template <typename T>
        requires std::is_arithmetic_v<T>
    auto parse(std::string_view text) -> std::optional<T> {
        while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
            text.remove_prefix(1);
        if (text.starts_with('+'))
            text.remove_prefix(1);

        const char *begin = text.data();
        const char *end = begin + text.size();

        if constexpr (std::is_floating_point_v<T>) {
            T value{};
            if (std::from_chars(begin, end, value).ec != std::errc{})
                return std::nullopt;
            return value;
        } else {
            long long value = 0;
            if (std::from_chars(begin, end, value).ec != std::errc{})
                return std::nullopt;
            if constexpr (std::is_same_v<T, bool>)
                return value != 0;
            else
                return static_cast<T>(value);
        }
    }

    // parse<T>, with 0 for a text that is not a number.
    template <typename T>
        requires std::is_arithmetic_v<T>
    auto to_number(std::string_view text) -> T {
        return parse<T>(text).value_or(T{});
    }
}
