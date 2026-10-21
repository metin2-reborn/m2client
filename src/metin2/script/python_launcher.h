#pragma once

#include <cstdint>

#include "python_header.h"

namespace metin2::inline script {

    class python_launcher {
      public:
        python_launcher();
        ~python_launcher();

        void clear();

        auto create(const char *program_name = "eter.python") -> bool;
        auto run_line(const char *line) -> bool;
        auto run_file(const char *file_name) -> bool;

      protected:
        auto run_memory_text_file(const char *file_name, uint32_t file_size, const void *file_data) -> bool;

      protected:
        PyObject *module_;
        PyObject *dic_;
    };

    inline auto get_python_launcher() -> python_launcher & {
        static python_launcher instance;
        return instance;
    }
}
