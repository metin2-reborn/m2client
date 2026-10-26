#pragma once

#include "file_base.h"

#include <cstdint>
#include <vector>

namespace metin2::inline core {

    // Read-only memory-mapped file view. The object either maps (part of) a file, is linked to a
    // caller-provided buffer, or owns a buffer handed to it (adopt).
    class mapped_file : public file_base {
      public:
        enum seek_type {
            SEEK_TYPE_BEGIN,
            SEEK_TYPE_CURRENT,
            SEEK_TYPE_END,
        };

      public:
        mapped_file();
        ~mapped_file() override;

        mapped_file(const mapped_file &) = delete;
        auto operator=(const mapped_file &) -> mapped_file & = delete;

        void link(uint32_t buf_size, const void *buf_data);
        auto adopt(std::vector<uint8_t> &&buffer_value) -> const void *; // takes the buffer over and links to it

        auto create(const char *filename) -> bool;
        auto create(const char *filename, const void **dest, int offset, int size) -> bool;
        auto get() -> const void *;
        void destroy();
        auto seek(uint32_t offset, int seek_type_value = SEEK_TYPE_BEGIN) -> int;
        auto map(const void **dest, int offset = 0, int size = 0) -> int;
        auto size() -> uint32_t;
        auto get_position() -> uint32_t;
        auto read(void *dest, int bytes) -> bool;

      private:
        auto current_seek_point() -> uint8_t *;
        auto is_open() const -> bool;
        void close_file();
        void unmap();

      private:
        uint8_t *buf_link_data_ = nullptr;
        uint32_t buf_link_size_ = 0;

        std::vector<uint8_t> owned_data_;

        uint32_t seek_position_ = 0;
        uint32_t data_offset_ = 0;
        uint32_t map_size_ = 0;
#ifdef _WIN32
        void *file_ = nullptr;    // HANDLE of the open file
        void *mapping_ = nullptr; // HANDLE of the file mapping object
#else
        int fd_ = -1;
#endif
        void *map_data_ = nullptr;   // mapping base (page aligned)
        size_t length_ = 0;          // mapping length
        void *data_value_ = nullptr; // map_data_ + offset delta
    };
}
