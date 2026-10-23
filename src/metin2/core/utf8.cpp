#include "utf8.h"

#include <cstring>

namespace metin2::inline core {

    namespace utf8 {
        namespace {
            auto is_continuation(unsigned char c) noexcept -> bool {
                return (c & 0xC0) == 0x80;
            }
        }

        auto next(std::string_view s, std::size_t &pos) noexcept -> char32_t {
            auto c = static_cast<unsigned char>(s[pos]);
            std::size_t len = sequence_length(c);
            if (len == 1) {
                ++pos;
                return c < 0x80 ? c : kReplacement;
            }
            if (pos + len > s.size()) {
                ++pos;
                return kReplacement;
            }

            char32_t cp = c & (0x7F >> len);
            for (std::size_t i = 1; i < len; ++i) {
                auto cc = static_cast<unsigned char>(s[pos + i]);
                if (!is_continuation(cc)) {
                    ++pos;
                    return kReplacement;
                }
                cp = (cp << 6) | (cc & 0x3F);
            }

            // reject overlong forms, surrogates and values past U+10FFFF
            static constexpr char32_t min_value[5] = {0, 0, 0x80, 0x800, 0x10000};
            if (cp < min_value[len] || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
                ++pos;
                return kReplacement;
            }

            pos += len;
            return cp;
        }

        auto prev(std::string_view s, std::size_t pos) noexcept -> std::size_t {
            if (pos == 0)
                return 0;
            std::size_t start = pos - 1;
            while (start > 0 && pos - start < 4 && is_continuation(static_cast<unsigned char>(s[start])))
                --start;
            // only accept the lead byte if its sequence really ends at pos
            std::size_t probe = start;
            next(s, probe);
            return probe == pos ? start : pos - 1;
        }

        auto next_offset(std::string_view s, std::size_t pos) noexcept -> std::size_t {
            if (pos >= s.size())
                return s.size();
            next(s, pos);
            return pos;
        }

        void append(std::string &out, char32_t cp) {
            if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
                cp = kReplacement;
            if (cp < 0x80) {
                out += static_cast<char>(cp);
            } else if (cp < 0x800) {
                out += static_cast<char>(0xC0 | (cp >> 6));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                out += static_cast<char>(0xE0 | (cp >> 12));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                out += static_cast<char>(0xF0 | (cp >> 18));
                out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }

        auto to_u32(std::string_view s) -> std::u32string {
            std::u32string out;
            out.reserve(s.size());
            for (std::size_t pos = 0; pos < s.size();)
                out.push_back(next(s, pos));
            return out;
        }

        auto to_wide(std::string_view s) -> std::wstring {
            std::wstring out;
            out.reserve(s.size());
            for (std::size_t pos = 0; pos < s.size();)
                out += static_cast<wchar_t>(next(s, pos));
            return out;
        }

        auto from_wide(std::wstring_view w) -> std::string {
            std::string out;
            out.reserve(w.size());
            for (wchar_t c : w)
                append(out, static_cast<char32_t>(c));
            return out;
        }

        auto from_u32(std::u32string_view s) -> std::string {
            std::string out;
            out.reserve(s.size());
            for (char32_t cp : s)
                append(out, cp);
            return out;
        }

        auto length(std::string_view s) noexcept -> std::size_t {
            std::size_t n = 0;
            for (std::size_t pos = 0; pos < s.size(); ++n)
                next(s, pos);
            return n;
        }

        auto columns(char32_t cp) noexcept -> int {
            return (cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0xA4CF) || (cp >= 0xAC00 && cp <= 0xD7A3) ||
                           (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFE30 && cp <= 0xFE4F) || (cp >= 0xFF00 && cp <= 0xFF60) ||
                           (cp >= 0xFFE0 && cp <= 0xFFE6) || (cp >= 0x1F300 && cp <= 0x1FAFF) || (cp >= 0x20000 && cp <= 0x3FFFD)
                       ? 2
                       : 1;
        }

        auto columns(std::string_view s) noexcept -> int {
            int total = 0;
            for (std::size_t pos = 0; pos < s.size();)
                total += columns(next(s, pos));
            return total;
        }

        auto is_valid(std::string_view s) noexcept -> bool {
            for (std::size_t pos = 0; pos < s.size();) {
                std::size_t start = pos;
                if (next(s, pos) == kReplacement && !(pos - start == 3 && s.compare(start, 3, "\xEF\xBF\xBD") == 0))
                    return false;
            }
            return true;
        }

        auto truncated_size(std::string_view s, std::size_t max_bytes) noexcept -> std::size_t {
            if (s.size() <= max_bytes)
                return s.size();
            std::size_t pos = 0;
            while (pos < s.size()) {
                std::size_t next = next_offset(s, pos);
                if (next > max_bytes)
                    break;
                pos = next;
            }
            return pos;
        }

        auto copy_truncated(char *dst, std::size_t dst_size, std::string_view src) noexcept -> std::size_t {
            std::size_t n = truncated_size(src, dst_size - 1);
            std::memcpy(dst, src.data(), n);
            dst[n] = '\0';
            return n;
        }
    }
}
