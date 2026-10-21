#include "file_loader.h"

#include <cassert>

namespace metin2::inline core {

    namespace {
        // Tokenizes a line on the delimiter set; double-quoted tokens may contain delimiters.
        // Returns 0 on success, -1 when there is no token at all, -2 on an unterminated quote.
        auto split_tokens(const std::string &line, const char *delimeter, token_vector *token_vector) -> int {
            token_vector->reserve(10);
            token_vector->clear();

            size_t base_pos = 0;

            do {
                size_t begin_pos = line.find_first_not_of(delimeter, base_pos);
                if (begin_pos == std::string::npos)
                    return -1;

                size_t end_pos;

                if (line[begin_pos] == '"') {
                    ++begin_pos;
                    end_pos = line.find_first_of('"', begin_pos);

                    if (end_pos == std::string::npos)
                        return -2;

                    base_pos = end_pos + 1;
                } else {
                    end_pos = line.find_first_of(delimeter, begin_pos);
                    base_pos = end_pos;
                }

                token_vector->push_back(line.substr(begin_pos, end_pos == std::string::npos ? std::string::npos : end_pos - begin_pos));

                // Stop at trailing delimiters (such as a tab at the end of the line).
                if (line.find_first_not_of(delimeter, base_pos) == std::string::npos)
                    break;
            } while (base_pos < line.length());

            return 0;
        }
    } // namespace

    memory_text_file_loader::memory_text_file_loader() = default;

    memory_text_file_loader::~memory_text_file_loader() = default;

    auto memory_text_file_loader::split_line_by_tab(uint32_t dw_line, token_vector *token_vector) -> bool {
        token_vector->reserve(10);
        token_vector->clear();

        const std::string &c_rst_line = line_string(dw_line);
        const size_t line_length = c_rst_line.length();

        if (0 == line_length)
            return false;

        size_t base_pos = 0;

        do {
            size_t begin_pos = c_rst_line.find_first_of('\t', base_pos);

            token_vector->push_back(c_rst_line.substr(base_pos, begin_pos == std::string::npos ? std::string::npos : begin_pos - base_pos));

            if (begin_pos == std::string::npos)
                break;

            base_pos = begin_pos + 1;
        } while (base_pos < line_length);

        return true;
    }

    auto memory_text_file_loader::split_line2(uint32_t line, token_vector *token_vector, const char *delimeter) -> int {
        return split_tokens(line_string(line), delimeter, token_vector);
    }

    auto memory_text_file_loader::split_line(uint32_t line, token_vector *token_vector, const char *delimeter) -> bool {
        return split_tokens(line_string(line), delimeter, token_vector) == 0;
    }

    auto memory_text_file_loader::line_count() -> uint32_t {
        return static_cast<uint32_t>(line_vector_.size());
    }

    auto memory_text_file_loader::check_line_index(uint32_t line) -> bool {
        return line < line_vector_.size();
    }

    auto memory_text_file_loader::line_string(uint32_t line) -> const std::string & {
        assert(check_line_index(line));
        return line_vector_[line];
    }

    void memory_text_file_loader::bind(int buf_size, const void *c_pv_buf) {
        line_vector_.reserve(128);
        line_vector_.clear();

        const char *c_pc_buf = static_cast<const char *>(c_pv_buf);
        std::string line;
        int pos = 0;

        while (pos < buf_size) {
            const char c = c_pc_buf[pos++];

            if ('\n' == c || '\r' == c) {
                if (pos < buf_size)
                    if ('\n' == c_pc_buf[pos] || '\r' == c_pc_buf[pos])
                        ++pos;

                line_vector_.push_back(line);
                line = "";
            } else {
                line += c;
            }
        }

        line_vector_.push_back(line);
    }
}
