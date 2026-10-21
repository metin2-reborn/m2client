#pragma once
#include <cstdint>
#include <list>
#include <string>

namespace metin2::inline client::inline social {

    class insult_checker {
      public:
        insult_checker();
        virtual ~insult_checker();

        void clear();

        void append_insult(const std::string &insult);
        auto is_insult_in(const char *line, uint32_t line_len) -> bool;
        void filter_insult(char *line, uint32_t line_len);

      private:
        auto insult_length(const char *word, uint32_t *insult_len) -> bool;
        auto is_insult(const char *word) -> bool;

      private:
        std::list<std::string> list_insult_;
    };

    auto get_insult_checker() -> insult_checker &;
}
