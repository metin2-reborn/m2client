#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace metin2::inline core {

    // UTF-8 helpers. All text in the client (data files, scripts, network, input) is UTF-8; wide text
    // is UTF-32 in wchar_t (32-bit on Linux). Invalid bytes decode to U+FFFD, one byte at a time.
    namespace utf8 {
        constexpr char32_t kReplacement = 0xFFFD;

        // Length of the sequence started by lead byte c (1 for ASCII and for invalid lead bytes).
        constexpr auto sequence_length(unsigned char c) noexcept -> std::size_t {
            return c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2
                              : (c >> 4) == 0xE   ? 3
                              : (c >> 3) == 0x1E  ? 4
                                                  : 1;
        }

        // Decodes the code point at s[pos] and advances pos past it (at least one byte).
        auto next(std::string_view s, std::size_t &pos) noexcept -> char32_t;

        // Byte offset of the code point before pos (pos itself at 0).
        auto prev(std::string_view s, std::size_t pos) noexcept -> std::size_t;

        // Byte offset just past the code point at pos (s.size() at most).
        auto next_offset(std::string_view s, std::size_t pos) noexcept -> std::size_t;

        void append(std::string &out, char32_t cp);

        auto to_wide(std::string_view s) -> std::wstring;
        auto to_u32(std::string_view s) -> std::u32string;
        auto from_wide(std::wstring_view w) -> std::string;
        auto from_u32(std::u32string_view s) -> std::string;

        // Number of code points.
        auto length(std::string_view s) noexcept -> std::size_t;

        // Display columns of a character: East Asian wide characters (Hangul, CJK, full-width forms,
        // emoji) take two, everything else one, as double-byte characters did in the old code pages.
        auto columns(char32_t cp) noexcept -> int;
        auto columns(std::string_view s) noexcept -> int;

        auto is_valid(std::string_view s) noexcept -> bool;

        // Largest prefix of s that is at most max_bytes long and does not end inside a sequence.
        auto truncated_size(std::string_view s, std::size_t max_bytes) noexcept -> std::size_t;

        // Copies src into dst[dst_size] (NUL-terminated) without splitting a sequence; returns the
        // number of bytes copied (dst_size must be at least 1).
        auto copy_truncated(char *dst, std::size_t dst_size, std::string_view src) noexcept -> std::size_t;

        // Fills a fixed-size field (a packet or record member) with src: zero-padded, NUL-terminated,
        // truncated without splitting a sequence.
        template <std::size_t N>
        void copy_field(char (&dst)[N], std::string_view src) noexcept {
            static_assert(N > 0);
            std::memset(dst, 0, N);
            copy_truncated(dst, N, src);
        }
    }
}
