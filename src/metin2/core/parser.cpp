#include "parser.h"
#include <metin2/log/log.h>
#include "utf8.h"

#include <cctype>
#include <cstring>
#include <string_view>

namespace metin2::inline core {

    using namespace event_script;

    // Event scripts are UTF-8. Every syntax character ('[', ']', ' ', '|', ';') is ASCII, so the walk
    // steps over whole UTF-8 sequences and compares only single-byte characters.
    namespace {
        // Byte length of the character at s[0] (at least 1, at most len).
        auto char_len(const char *s, int len) -> int {
            return static_cast<int>(utf8::next_offset(std::string_view(s, static_cast<std::size_t>(len)), 0));
        }

        auto is_blank(char c) -> bool {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
        }

        auto find_char(const char *base, int len, char test) -> const char * {
            for (int pos_value = 0; pos_value < len;) {
                int char_len_value = char_len(base + pos_value, len - pos_value);
                if (char_len_value == 1 && base[pos_value] == test)
                    return base + pos_value;
                pos_value += char_len_value;
            }
            return nullptr;
        }

        // Cuts trailing blanks (never part of a multi-byte character); returns the new length.
        auto right_trim(char *base, int len) -> int {
            while (len > 0 && is_blank(base[len - 1]))
                base[--len] = '\0';
            return len;
        }

        auto skip_blanks(const char *cur, const char *end) -> const char * {
            while (cur < end && is_blank(*cur))
                ++cur;
            return cur;
        }

        // Appends one character to buf (capacity bytes plus the NUL); false if it does not fit.
        auto append_char(char *buf, int &len, int capacity, const char *c, int char_len, const char *what) -> bool {
            if (len + char_len > capacity) {
                metin2::log::error("Argument {} is too long: it must be shorter than {} bytes", what, capacity);
                return false;
            }
            memcpy(buf + len, c, char_len);
            len += char_len;
            buf[len] = '\0';
            return true;
        }
    }

    auto group::get_arg(const char *c_arg_base, int arg_len, arg_list &arg_list_value) -> bool {
        constexpr int name_max = 32;
        constexpr int value_max = 64;
        char name_value[name_max + 1] = "";
        char value_value[value_max + 1] = "";

        int name_len = 0;
        int value_len = 0;

        bool is_value = false;

        for (int pos_value = 0; pos_value < arg_len;) {
            const char *cur = c_arg_base + pos_value;
            int char_len_value = char_len(cur, arg_len - pos_value);
            pos_value += char_len_value;

            if (char_len_value == 1) {
                const char c = *cur;
                if (c == '|') {
                    if (name_len == 0) {
                        metin2::log::error("Argument has no name");
                        return false;
                    }

                    is_value = false;

                    name_len = right_trim(name_value, name_len);
                    value_len = right_trim(value_value, value_len);
                    arg_list_value.push_back(arg(name_value, value_value));

                    name_len = 0;
                    value_len = 0;
                    name_value[0] = value_value[0] = '\0';
                    continue;
                }
                if (c == ';') {
                    is_value = true;
                    continue;
                }
                // outside a value and before a name starts, skip blanks; skip line breaks.
                if ((!is_value && name_len == 0 && is_blank(c)) || c == '\r' || c == '\n')
                    continue;
            }

            bool ok = is_value ? append_char(value_value, value_len, value_max, cur, char_len_value, "value")
                               : append_char(name_value, name_len, name_max, cur, char_len_value, "name");
            if (!ok)
                return false;
        }

        if (name_len != 0 && value_len != 0) {
            name_len = right_trim(name_value, name_len);
            value_len = right_trim(value_value, value_len);
            arg_list_value.push_back(arg(name_value, value_value));
        }

        return true;
    }

    auto group::create(const std::string &source) -> bool {
        cmd_list_.clear();

        if (source.empty())
            return false;

        const char *str_base = source.c_str();
        int str_len = static_cast<int>(source.length());
        int str_pos = 0;

        char box_data[1024 + 1];

        while (str_pos < str_len) {
            cmd cmd;

            const char *word = str_base + str_pos;
            int word_len = char_len(word, str_len - str_pos);

            if (word_len == 1 && *word == '[') {
                ++str_pos;

                const char *box_begin = str_base + str_pos;
                const char *box_end = find_char(box_begin, str_len - str_pos, ']');
                if (!box_end) {
                    metin2::log::error("Missing closing bracket in {}", box_begin);
                    return false;
                }
                str_pos += box_end - box_begin + 1;

                const char *data_begin = skip_blanks(box_begin, box_end);
                int data_len = static_cast<int>(box_end - data_begin);
                if (data_len >= 1024) {
                    metin2::log::error("Command of {} bytes is too long in {}", data_len, str_base);
                    return false;
                }
                memcpy(box_data, data_begin, data_len);
                box_data[data_len] = '\0';
                data_len = right_trim(box_data, data_len);

                const char *space = find_char(box_data, data_len, ' ');
                if (space) { // there are arguments
                    cmd.name_value.assign(box_data, space - box_data);

                    const char *arg = skip_blanks(space + 1, box_data + data_len);
                    int arg_len = data_len - static_cast<int>(arg - box_data);

                    if (!get_arg(arg, arg_len, cmd.arg_list_value)) {
                        metin2::log::error("Cannot parse {} bytes of arguments: {}", arg_len, arg);
                        return false;
                    }
                } else { // no arguments: the whole string is the command.
                    cmd.name_value.assign(box_data);
                    cmd.arg_list_value.clear();
                }

                cmd_list_.push_back(cmd);
                continue;
            }

            str_pos += word_len;
            if (word_len == 1 && (*word == '\r' || *word == '\n'))
                continue;

            // one LETTER command per character (the typewriter effect shows them one by one)
            cmd.name_value.assign("LETTER");
            cmd.arg_list_value.push_back(arg("value", std::string(word, word_len)));
            cmd_list_.push_back(cmd);
        }

        return true;
    }

    auto group::get_cmd(cmd &cmd) -> bool {
        if (cmd_list_.empty())
            return false;

        cmd = cmd_list_.front();
        cmd_list_.pop_front();
        return true;
    }

    auto group::read_cmd(cmd &cmd) -> bool {
        if (cmd_list_.empty())
            return false;

        cmd = cmd_list_.front();
        return true;
    }

    void group::set_error(const char *error) {
        error_.assign(error);
    }

    group::group() = default;

    group::~group() = default;
}
