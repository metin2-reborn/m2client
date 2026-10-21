#pragma once

#include <cstdint>
#include <metin2/core/stl.h>
#include <string>

namespace metin2::inline world::inline maps {

    class property {
      public:
        property(const char *file_name);
        ~property();

        void clear();
        auto read_from_memory(const void *data, int len, const char *file_name) -> bool;

        auto file_name() -> const char *;

        auto vector(const char *key, token_vector &token_vector) -> bool;
        auto string(const char *key, const char **string) -> bool;

        void put_vector(const char *key, const token_vector &token_vector);

        auto size() -> uint32_t;
        auto get_crc() -> uint32_t;

      protected:
        std::string file_name_;
        std::string crc_text_;
        const char *mc_file_name_;
        uint32_t crc_;

        token_vector_map token_map_;
    };
}
