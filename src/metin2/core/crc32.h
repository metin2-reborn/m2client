#pragma once

#include <cstddef>
#include <cstdint>

namespace metin2::inline core {

    auto get_crc32(const char *buffer_value, size_t count) -> uint32_t;
    // CRC32 over the upper-cased bytes (case-insensitive file name hashing).
    auto get_case_crc32(const char *buf, size_t len) -> uint32_t;
    // CRC32 of a whole file, 0 when it cannot be opened.
    auto get_file_crc32(const char *file_name) -> uint32_t;
    // Size of a file in bytes, 0 when it does not exist.
    auto get_file_size(const char *file_name) -> uint32_t;
}
