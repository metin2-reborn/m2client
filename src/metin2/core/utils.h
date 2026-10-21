#pragma once

#include <metin2/log/log.h>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace metin2::inline core {

#define AssertLog(str)                 \
    do {                               \
        metin2::log::error("{}", str); \
        assert(!str);                  \
    } while (0)

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                            \
    ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) | \
        ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24))
#endif // defined(MAKEFOURCC)

#ifndef IS_SET
#define IS_SET(flag, bit) ((flag) & (bit))
#endif

#ifndef SET_BIT
#define SET_BIT(var, bit) ((var) |= (bit))
#endif

#ifndef REMOVE_BIT
#define REMOVE_BIT(var, bit) ((var) &= ~(bit))
#endif

    // Appends the extension of file (text after the last '.' of the last path component).
    void get_file_extension(const char *file, int len, std::string *ext);
    // Directory part of a path including the trailing separator.
    void get_only_path_name(const char *sz_name, std::string &only_path_name);
    auto get_only_path_name(const char *name) -> const char *;
    // Lower-cases everything and turns \ into /.
    void string_path(std::string &string);
    void string_path(char *string);
    void string_path(const char *src, char *dest);
    void string_path(const char *src, std::string &string);
    auto is_file(const char *filename) -> bool;
    // True when the name contains a drive separator (':'), i.e. is not relative to the game directory.
    auto is_global_file_name(const char *file_name) -> bool;
    inline auto f_min(float a, float b) -> float { return a < b ? a : b; }
    inline auto f_max(float a, float b) -> float { return a > b ? a : b; }
    auto f_minmax(float min_value, float value, float max_value) -> float;
    // Creates every directory on the path (the path must end with a separator to be created itself).
    void my_create_directory(const char *path);
    auto split_line(const char *line, const char *delimeter, std::vector<std::string> *vec_token) -> bool;
    // printf into a static 256-byte buffer; the result is valid until the next call.
    auto format_string(const char *format, ...) -> const char *;

    template <typename T>
    auto el_degree_to_radian(T degree) -> T {
        const T pi = T(3.141592);
        return T(pi * degree / 180.0f);
    }

    template <typename T>
    void el_plain_coord_get_rotated_pixel_position(T center_x, T center_y, T distance, T rot_degree, T *pdst_x, T *pdst_y) {
        T rot_radian = el_degree_to_radian(rot_degree);
        *pdst_x = center_x + distance * T(sin((double)rot_radian));
        *pdst_y = center_y + distance * T(cos((double)rot_radian));
    }

    template <typename C>
    void string_join(const std::string &sep, const C &container, std::string *ret) {
        std::string buf;

        if (!container.empty()) {
            size_t capacity = sep.length() * (container.size() - 1);
            for (auto i = container.begin(); i != container.end(); ++i)
                capacity += (*i).length();
            buf.reserve(capacity);

            auto cur = container.begin();
            auto end = container.end();
            --end;

            while (cur != end) {
                buf.append(*cur++);
                buf.append(sep);
            }
            buf.append(*cur);
        }

        std::swap(*ret, buf);
    }

    // Hexadecimal text to int; -1 on an invalid digit.
    template <typename TChar>
    inline auto htoi_n(const TChar *s, int size) -> int {
        const TChar *t = s;
        int x = 0, y = 1;
        s += size;

        while (t <= --s) {
            if (TChar('0') <= *s && *s <= TChar('9'))
                x += y * (*s - TChar('0'));
            else if (TChar('a') <= *s && *s <= TChar('f'))
                x += y * (*s - TChar('a') + 10);
            else if (TChar('A') <= *s && *s <= TChar('F'))
                x += y * (10 + *s - TChar('A'));
            else
                return -1;
            y <<= 4;
        }

        return x;
    }

    inline auto htoi(const wchar_t *s, int size) -> int { return htoi_n(s, size); }
    inline auto htoi(const char *s, int size) -> int { return htoi_n(s, size); }
    inline auto htoi(const char *s) -> int { return htoi_n(s, static_cast<int>(strlen(s))); }

    // Absolute path of the running executable.
    void get_excuted_file_name(std::string &r_str);
}
