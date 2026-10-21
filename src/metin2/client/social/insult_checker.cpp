#include "insult_checker.h"
#include <metin2/core/utf8.h>

#include <cstring>
#include <string_view>
#include <strings.h>

namespace metin2::inline client::inline social {

    auto get_insult_checker() -> insult_checker & {
        static insult_checker insult_checker;
        return insult_checker;
    }

    insult_checker::insult_checker() = default;

    insult_checker::~insult_checker() = default;

    void insult_checker::clear() {
        list_insult_.clear();
    }

    void insult_checker::append_insult(const std::string &insult) {
        if (insult.length() > 0)
            list_insult_.push_back(insult);
    }

    auto insult_checker::insult_length(const char *word, uint32_t *insult_len) -> bool {
        std::list<std::string>::iterator i;
        for (i = list_insult_.begin(); i != list_insult_.end(); ++i) {
            std::string &insult = *i;

            int ret = strncasecmp(word, insult.c_str(), insult.length()); // ASCII-only folding
            if (0 == ret) {
                *insult_len = insult.length();
                return true;
            }
        }

        return false;
    }

    auto insult_checker::is_insult(const char *word) -> bool {
        uint32_t insult_len;
        return insult_length(word, &insult_len);
    }

    void insult_checker::filter_insult(char *line, uint32_t line_len) {
        const char insult_filter_char = '*';
        for (uint32_t pos = 0; pos < line_len;) {
            uint32_t insult_len;
            if (insult_length(line + pos, &insult_len)) {
                memset(line + pos, insult_filter_char, insult_len);
                pos += insult_len;
            } else {
                pos = static_cast<uint32_t>(utf8::next_offset(std::string_view(line, line_len), pos));
            }
        }
    }

    auto insult_checker::is_insult_in(const char *line, uint32_t line_len) -> bool {
        // insults start on a character boundary
        for (uint32_t pos = 0; pos < line_len; pos = static_cast<uint32_t>(utf8::next_offset(std::string_view(line, line_len), pos))) {
            if (is_insult(line + pos))
                return true;
        }

        return false;
    }
}
