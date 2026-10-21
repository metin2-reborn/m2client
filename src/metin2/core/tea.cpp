#include "tea.h"

#include <cstring>

namespace metin2::inline core {

    // tea_code/tea_decode work on one 8-byte block with a 16-byte key; sz and sy are the two
    // 4-byte halves of the block in reverse order (tea_code(src[1], src[0], key, dest)).
    namespace {
        // The round count and delta are part of the data format: changing them breaks existing packs.
        const uint32_t TEA_ROUND = 32;
        const uint32_t DELTA = 0x9E3779B9;

        void tea_code(const uint32_t sz, const uint32_t sy, const uint32_t *key, uint32_t *dest) {
            uint32_t y = sy, z = sz, sum = 0;
            uint32_t n = TEA_ROUND;

            while (n-- > 0) {
                y += ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);
                sum += DELTA;
                z += ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);
            }

            *(dest++) = y;
            *dest = z;
        }

        void tea_decode(const uint32_t sz, const uint32_t sy, const uint32_t *key, uint32_t *dest) {
            uint32_t y = sy, z = sz, sum = DELTA * TEA_ROUND;
            uint32_t n = TEA_ROUND;

            while (n-- > 0) {
                z -= ((y << 4 ^ y >> 5) + y) ^ (sum + key[sum >> 11 & 3]);
                sum -= DELTA;
                y -= ((z << 4 ^ z >> 5) + z) ^ (sum + key[sum & 3]);
            }

            *(dest++) = y;
            *dest = z;
        }
    } // namespace

    auto tea_encrypt(uint32_t *dest, const uint32_t *src, const uint32_t *key, int size) -> int {
        const int resize = size % 8 != 0 ? size + 8 - (size % 8) : size;
        const int full_blocks = size / 8;

        for (int i = 0; i < full_blocks; i++, dest += 2, src += 2)
            tea_code(*(src + 1), *src, key, dest);

        if (resize != size) {
            // Last partial block: zero-padded in a local copy instead of writing past the source.
            uint32_t tail[2] = {0, 0};
            memcpy(tail, src, size % 8);
            tea_code(tail[1], tail[0], key, dest);
        }

        return resize;
    }

    auto tea_decrypt(uint32_t *dest, const uint32_t *src, const uint32_t *key, int size) -> int {
        const int resize = size % 8 != 0 ? size + 8 - (size % 8) : size;

        for (int i = 0; i < resize >> 3; i++, dest += 2, src += 2)
            tea_decode(*(src + 1), *src, key, dest);

        return resize;
    }
}
