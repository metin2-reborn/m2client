#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

namespace metin2::inline core {

    // Thin binary file wrapper (read-only or read/write, the latter creating the file if needed).
    class file_base {
      public:
        enum file_mode {
            FILEMODE_READ = (1 << 0),
            FILEMODE_WRITE = (1 << 1),
        };

        file_base() = default;

        virtual ~file_base();

        void destroy();
        void close();

        auto create(const char *filename, file_mode mode) -> bool;
        auto size() -> uint32_t;
        void seek(uint32_t offset);
        auto get_position() -> uint32_t;

        virtual auto write(const void *src, int bytes) -> bool;
        auto read(void *dest, int bytes) -> bool;

        auto file_name() -> const char *;
        auto is_null() -> bool;

      protected:
        int mode_value_ = 0;
        std::string filename_;
        FILE *fp_ = nullptr;
        uint32_t size_ = 0;
    };
}
