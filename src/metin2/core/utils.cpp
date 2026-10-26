#include "utils.h"

#include "stl.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace metin2::inline core {

    void get_file_extension(const char *file, int len, std::string *pst_ext) {
        int ext = len;
        int pos = len;
        while (pos > 0) {
            --pos;
            char c = file[pos];
            if (ext == len && c == '.') {
                ext = pos;
                break;
            }

            if (c == '/' || c == '\\')
                break;
        }

        ++ext;
        if (len > ext)
            pst_ext->append(file + ext, len - ext);
    }

    void get_only_path_name(const char *sz_name, std::string &only_path_name) {
        int i;
        for (i = static_cast<int>(strlen(sz_name)) - 1; i >= 0; --i) {
            if ('\\' == sz_name[i] || '/' == sz_name[i]) {
                ++i;
                break;
            }
        }

        if (i == -1)
            i = 0;

        only_path_name.assign(sz_name, i);
    }

    auto get_only_path_name(const char *name) -> const char * {
        static std::string path_name;
        get_only_path_name(name, path_name);

        return path_name.c_str();
    }

    void string_path(std::string &string) {
        for (size_t i = 0; i < string.length(); ++i) {
            if (string[i] == '\\')
                string[i] = '/';
            else
                string[i] = ascii_tolower(string[i]);
        }
    }

    void string_path(char *string) {
        for (size_t i = 0; string[i]; ++i) {
            if (string[i] == '\\')
                string[i] = '/';
            else
                string[i] = ascii_tolower(string[i]);
        }
    }

    void string_path(const char *src, char *dest) {
        size_t i = 0;
        for (; src[i]; ++i) {
            if (src[i] == '\\')
                dest[i] = '/';
            else
                dest[i] = ascii_tolower(src[i]);
        }
        dest[i] = '\0';
    }

    void string_path(const char *src, std::string &string) {
        string = src;
        string_path(string);
    }

    auto f_minmax(float min_value, float value, float max_value) -> float {
        float tv = (min_value > value ? min_value : value);
        return (max_value < tv) ? max_value : tv;
    }

    auto is_file(const char *filename) -> bool {
        std::error_code error;
        return std::filesystem::exists(filename, error);
    }

    auto is_global_file_name(const char *file_name) -> bool {
        return strchr(file_name, ':') != nullptr;
    }

    void my_create_directory(const char *path) {
        if (!path || !*path)
            return;

        // Every directory on the path ("a/b/c/" -> a, a/b, a/b/c).
        std::error_code error;
        std::filesystem::create_directories(path, error);
    }

    auto split_line(const char *c_sz_line, const char *delimeter, std::vector<std::string> *vec_token) -> bool {
        vec_token->reserve(10);
        vec_token->clear();

        std::string str_line = c_sz_line;

        size_t base_pos = 0;

        do {
            size_t begin_pos = str_line.find_first_not_of(delimeter, base_pos);
            if (begin_pos == std::string::npos)
                return false;

            size_t end_pos;

            if (str_line[begin_pos] == '"') {
                ++begin_pos;
                end_pos = str_line.find_first_of('"', begin_pos);

                if (end_pos == std::string::npos)
                    return false;

                base_pos = end_pos + 1;
            } else {
                end_pos = str_line.find_first_of(delimeter, begin_pos);
                base_pos = end_pos;
            }

            vec_token->push_back(str_line.substr(begin_pos, end_pos == std::string::npos ? std::string::npos : end_pos - begin_pos));
        } while (base_pos < str_line.length());

        return true;
    }

    void get_excuted_file_name(std::string &r_str) {
        std::error_code ec;
        std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", ec);
        r_str = ec ? std::string() : exe.string();
    }

    auto format_string(const char *format, ...) -> const char * {
        static char buf[256];

        va_list args;
        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);

        return buf;
    }
}
