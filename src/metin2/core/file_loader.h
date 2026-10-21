#pragma once

#include "stl.h"

#include <cstdint>
#include <string>
#include <vector>

namespace metin2::inline core {

    // Splits an in-memory UTF-8 text file into lines.
    class memory_text_file_loader {
      public:
        memory_text_file_loader();
        virtual ~memory_text_file_loader();

        void bind(int buf_size, const void *buf);
        auto line_count() -> uint32_t;
        auto check_line_index(uint32_t line) -> bool;
        auto split_line(uint32_t line, token_vector *token_vector, const char *delimeter = " \t") -> bool;
        // 0 on success, -1 when the line has no token, -2 on an unterminated quote.
        auto split_line2(uint32_t line, token_vector *token_vector, const char *delimeter = " \t") -> int;
        auto split_line_by_tab(uint32_t line, token_vector *token_vector) -> bool;
        auto line_string(uint32_t line) -> const std::string &;

      protected:
        std::vector<std::string> line_vector_;
    };
}
