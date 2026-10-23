#pragma once

#include <cstdint>
#include <lzo/lzo1x.h>

namespace metin2::inline core {
    // One LZO1X compressed block, optionally TEA-encrypted ("MCOZ" container used by the packs).
    class lz_object {
      public:
#pragma pack(push, 4)
        struct header // on-disk layout, 16 bytes
        {
            uint32_t four_cc;
            uint32_t encrypt_size;
            uint32_t compressed_size;
            uint32_t real_size; // uncompressed size
        };
#pragma pack(pop)

        static_assert(sizeof(header) == 16, "CLZObject::SHeader is serialized into pack files");

        lz_object();
        ~lz_object();

        void clear();

        auto compress() -> bool;

        auto begin_decompress(const void *in) -> bool;
        auto decompress(uint32_t *key = nullptr) -> bool;

        auto encrypt(uint32_t *key) -> bool;

        auto get_header() -> const header & { return *header_value_; }
        auto buffer() -> uint8_t * { return buffer_; }
        auto size() -> uint32_t;
        void alloc_buffer(uint32_t size);
        [[nodiscard]] auto buffer_size() const -> uint32_t { return buffer_size_; }

      private:
        void initialize();
        auto decrypt_impl(uint32_t *key, uint8_t *data) -> bool;

        uint8_t *buffer_;
        uint32_t buffer_size_;

        header *header_value_;
        const uint8_t *in_;
        bool compressed_;

      public:
        static uint32_t four_cc;
    };

    class clzo {
      public:
        clzo();
        ~clzo();

        auto decompress(lz_object &obj, const uint8_t *buf, uint32_t *key = nullptr) -> bool;
        auto work_memory() -> uint8_t *;

      private:
        uint8_t *work_mem_;
    };

    inline auto get_clzo() -> clzo & {
        static clzo instance;

        return instance;
    }
}
