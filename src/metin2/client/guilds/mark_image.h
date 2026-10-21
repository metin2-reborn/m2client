#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace metin2::inline client::inline guilds {

    using pixel = uint32_t; // 32-bit ARGB, as in the mark files and the server's blocks

    struct guild_mark {
        enum {
            WIDTH = 16,
            HEIGHT = 12,
            SIZE = WIDTH * HEIGHT,
        };

        pixel apx_buf[SIZE];

        void clear();
        auto is_empty() -> bool;
    };

    struct guild_mark_block {
        enum {
            MARK_PER_BLOCK_WIDTH = 4,
            MARK_PER_BLOCK_HEIGHT = 4,

            WIDTH = guild_mark::WIDTH * MARK_PER_BLOCK_WIDTH,
            HEIGHT = guild_mark::HEIGHT * MARK_PER_BLOCK_HEIGHT,

            SIZE = WIDTH * HEIGHT,
            MAX_COMP_SIZE = (SIZE * sizeof(pixel)) + ((SIZE * sizeof(pixel)) >> 4) + 64 + 3
        };

        pixel apx_buf[SIZE];

        uint8_t comp_buf[MAX_COMP_SIZE];
        size_t size_comp_buf;
        uint32_t crc; // CRC of the compressed data

        auto get_crc() const -> uint32_t;

        void copy_from(const uint8_t *comp_buf_value, uint32_t comp_size, uint32_t crc_value);
        void compress(const pixel *px_buf);
    };

    class guild_mark_image {
      public:
        enum {
            WIDTH = 512,
            HEIGHT = 512,

            BLOCK_ROW_COUNT = HEIGHT / guild_mark_block::HEIGHT, // 10
            BLOCK_COL_COUNT = WIDTH / guild_mark_block::WIDTH,   // 8

            BLOCK_TOTAL_COUNT = BLOCK_ROW_COUNT * BLOCK_COL_COUNT, // 80

            MARK_ROW_COUNT = BLOCK_ROW_COUNT * guild_mark_block::MARK_PER_BLOCK_HEIGHT, // 40
            MARK_COL_COUNT = BLOCK_COL_COUNT * guild_mark_block::MARK_PER_BLOCK_WIDTH,  // 32

            MARK_TOTAL_COUNT = MARK_ROW_COUNT * MARK_COL_COUNT, // 1280

            INVALID_MARK_POSITION = 0xffffffff,
        };

        guild_mark_image();
        virtual ~guild_mark_image();

        void create();
        void destroy();

        auto build(const char *file_name) -> bool;
        auto save(const char *file_name) -> bool;
        auto load(const char *file_name) -> bool;

        void put_data(uint32_t x, uint32_t y, uint32_t width_value, uint32_t height, void *data);
        void get_data(uint32_t x, uint32_t y, uint32_t width_value, uint32_t height, void *data);

        auto save_mark(uint32_t pos_mark, uint8_t *mark_image) -> bool;
        auto delete_mark(uint32_t pos_mark) -> bool;
        auto save_block_from_compressed_data(uint32_t pos_block, const uint8_t *comp, uint32_t comp_size) -> bool; // server -> client

        void block_crc_list(uint32_t *crc_list);
        void diff_blocks(const uint32_t *crc_list, std::map<uint8_t, const guild_mark_block *> &diff_blocks);

      private:
        void build_all_blocks();

        guild_mark_block aak_block_[BLOCK_ROW_COUNT][BLOCK_COL_COUNT];

        std::vector<pixel> pixels_; // WIDTH x HEIGHT, empty until created
    };
}
