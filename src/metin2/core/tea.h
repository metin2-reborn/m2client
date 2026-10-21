#pragma once

#include <cstdint>

namespace metin2::inline core {

    // TEA (Wheeler and Needham): a 64-bit block cipher with a 128-bit key.

#define TEA_KEY_LENGTH 16

    // Both return the number of bytes processed, i.e. size rounded up to a multiple of 8.
    // dest must have room for that many bytes.
    auto tea_encrypt(uint32_t *dest, const uint32_t *src, const uint32_t *key, int size) -> int;
    auto tea_decrypt(uint32_t *dest, const uint32_t *src, const uint32_t *key, int size) -> int;
}
