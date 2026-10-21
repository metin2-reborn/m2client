#include "mark_image.h"
#include <cstring>
#include <metin2/core/crc32.h>
#include <metin2/core/lzo.h>
#include <metin2/graphics/image_file.h>
#include <metin2/log/log.h>

#include <algorithm>

namespace metin2::inline client::inline guilds {

#define thecore_memcpy memcpy
#define itertype(cont) typeof(cont.begin())

    auto new_mark_image() -> guild_mark_image * {
        return new guild_mark_image;
    }

    void delete_mark_image(guild_mark_image *image) {
        delete image;
    }

    guild_mark_image::guild_mark_image() = default;

    guild_mark_image::~guild_mark_image() {
        destroy();
    }

    void guild_mark_image::destroy() {
        pixels_.clear();
    }

    void guild_mark_image::create() {
        if (pixels_.empty())
            pixels_.assign(WIDTH * HEIGHT, 0);
    }

    auto guild_mark_image::save(const char *file_name) -> bool {
        if (pixels_.empty())
            return false;
        return save_tga_file(file_name, WIDTH, HEIGHT, pixels_.data());
    }

    auto guild_mark_image::build(const char *file_name) -> bool {
        pixels_.assign(WIDTH * HEIGHT, 0);
        return save(file_name);
    }

    // A missing or unreadable file is replaced by an empty image.
    auto guild_mark_image::load(const char *file_name) -> bool {
        bgra_image image;
        if (!load_image_file(file_name, image)) {
            if (!build(file_name) || !load_image_file(file_name, image)) {
                metin2::log::error("Cannot open {} for writing", file_name);
                return false;
            }
        }

        if (image.width != WIDTH) {
            metin2::log::error("Guild mark image {} must be {} pixels wide", file_name, static_cast<int>(WIDTH));
            return false;
        }

        if (image.height != HEIGHT) {
            metin2::log::error("Guild mark image {} must be {} pixels high", file_name, static_cast<int>(HEIGHT));
            return false;
        }

        pixels_ = std::move(image.pixels);
        build_all_blocks();
        return true;
    }

    void guild_mark_image::put_data(uint32_t x, uint32_t y, uint32_t width_value, uint32_t height, void *data) {
        if (pixels_.empty() || x >= WIDTH || y >= HEIGHT)
            return;
        const uint32_t columns = std::min<uint32_t>(width_value, WIDTH - x);
        const auto *source = static_cast<const pixel *>(data);
        for (uint32_t row = 0; row < height && y + row < HEIGHT; ++row)
            std::copy_n(source + static_cast<size_t>(row) * width_value, columns, pixels_.begin() + (y + row) * WIDTH + x);
    }

    void guild_mark_image::get_data(uint32_t x, uint32_t y, uint32_t width_value, uint32_t height, void *data) {
        auto *destination = static_cast<pixel *>(data);
        std::fill_n(destination, static_cast<size_t>(width_value) * height, 0);
        if (pixels_.empty() || x >= WIDTH || y >= HEIGHT)
            return;
        const uint32_t columns = std::min<uint32_t>(width_value, WIDTH - x);
        for (uint32_t row = 0; row < height && y + row < HEIGHT; ++row)
            std::copy_n(pixels_.begin() + (y + row) * WIDTH + x, columns, destination + static_cast<size_t>(row) * width_value);
    }

    // The image is 512 x 512 and holds 8 x 10 blocks of 4 x 4 marks, each mark 16 x 12 pixels.

    auto guild_mark_image::save_mark(uint32_t pos_mark, uint8_t *image) -> bool {
        if (pos_mark >= MARK_TOTAL_COUNT) {
            metin2::log::error("Invalid guild mark position {}", pos_mark);
            return false;
        }

        uint32_t col_mark = pos_mark % MARK_COL_COUNT;
        uint32_t row_mark = pos_mark / MARK_COL_COUNT;

        printf("PutMark pos %u %ux%u\n", pos_mark, col_mark * guild_mark::WIDTH, row_mark * guild_mark::HEIGHT);
        put_data(col_mark * guild_mark::WIDTH, row_mark * guild_mark::HEIGHT, guild_mark::WIDTH, guild_mark::HEIGHT, image);

        uint32_t row_block = row_mark / guild_mark_block::MARK_PER_BLOCK_HEIGHT;
        uint32_t col_block = col_mark / guild_mark_block::MARK_PER_BLOCK_WIDTH;

        pixel apx_buf[guild_mark_block::SIZE];
        get_data(col_block * guild_mark_block::WIDTH, row_block * guild_mark_block::HEIGHT, guild_mark_block::WIDTH, guild_mark_block::HEIGHT, apx_buf);
        aak_block_[row_block][col_block].compress(apx_buf);
        return true;
    }

    auto guild_mark_image::delete_mark(uint32_t pos_mark) -> bool {
        pixel image[guild_mark::SIZE];
        memset(&image, 0, sizeof(image));
        return save_mark(pos_mark, (uint8_t *)&image);
    }

    auto guild_mark_image::save_block_from_compressed_data(uint32_t pos_block, const uint8_t *comp, uint32_t comp_size) -> bool {
        if (pos_block >= BLOCK_TOTAL_COUNT)
            return false;

        pixel apx_buf[guild_mark_block::SIZE];
        size_t size_buf = sizeof(apx_buf);

        if (LZO_E_OK != lzo1x_decompress_safe(comp, comp_size, (uint8_t *)apx_buf, (lzo_uint *)&size_buf, get_clzo().work_memory())) {
            metin2::log::error("Cannot decompress the guild mark block (compressed size {})", comp_size);
            return false;
        }

        if (size_buf != sizeof(apx_buf)) {
            metin2::log::error("Guild mark block is corrupted (decompressed size {})", size_buf);
            return false;
        }

        uint32_t row_block = pos_block / BLOCK_COL_COUNT;
        uint32_t col_block = pos_block % BLOCK_COL_COUNT;

        put_data(col_block * guild_mark_block::WIDTH, row_block * guild_mark_block::HEIGHT, guild_mark_block::WIDTH, guild_mark_block::HEIGHT, apx_buf);

        aak_block_[row_block][col_block].copy_from(comp, comp_size, get_crc32((const char *)apx_buf, sizeof(pixel) * guild_mark_block::SIZE));
        return true;
    }

    void guild_mark_image::build_all_blocks() {
        pixel apx_buf[guild_mark_block::SIZE];
        metin2::log::debug("Building all guild mark blocks");

        for (uint32_t row = 0; row < BLOCK_ROW_COUNT; ++row)
            for (uint32_t col = 0; col < BLOCK_COL_COUNT; ++col) {
                get_data(col * guild_mark_block::WIDTH, row * guild_mark_block::HEIGHT, guild_mark_block::WIDTH, guild_mark_block::HEIGHT, apx_buf);
                aak_block_[row][col].compress(apx_buf);
            }
    }

    void guild_mark_image::diff_blocks(const uint32_t *crc_list, std::map<uint8_t, const guild_mark_block *> &diff_blocks) {
        uint8_t pos_block = 0;

        for (uint32_t row = 0; row < BLOCK_ROW_COUNT; ++row)
            for (uint32_t col = 0; col < BLOCK_COL_COUNT; ++col) {
                if (aak_block_[row][col].crc != *crc_list) {
                    diff_blocks.insert(std::map<uint8_t, const guild_mark_block *>::value_type(pos_block, &aak_block_[row][col]));
                }
                ++crc_list;
                ++pos_block;
            }
    }

    void guild_mark_image::block_crc_list(uint32_t *crc_list) {
        for (uint32_t row = 0; row < BLOCK_ROW_COUNT; ++row)
            for (uint32_t col = 0; col < BLOCK_COL_COUNT; ++col)
                *(crc_list++) = aak_block_[row][col].get_crc();
    }

    void guild_mark::clear() {
        for (uint32_t pixel = 0; pixel < SIZE; ++pixel)
            apx_buf[pixel] = 0xff000000;
    }

    auto guild_mark::is_empty() -> bool {
        for (uint32_t pixel = 0; pixel < SIZE; ++pixel)
            if (apx_buf[pixel] != 0x00000000)
                return false;

        return true;
    }

    auto guild_mark_block::get_crc() const -> uint32_t {
        return crc;
    }

    void guild_mark_block::copy_from(const uint8_t *comp_buf_value, uint32_t comp_size, uint32_t crc_value) {
        if (comp_size > MAX_COMP_SIZE)
            return;

        size_comp_buf = comp_size;
        thecore_memcpy(comp_buf, comp_buf_value, comp_size);
        crc = crc_value;
    }

    void guild_mark_block::compress(const pixel *px_buf) {
        size_comp_buf = MAX_COMP_SIZE;
        lzo_uint size_comp_buf_value = size_comp_buf; // lzo_uint is 64-bit on Linux; m_sizeCompBuf is 32-bit

        if (LZO_E_OK != lzo1x_999_compress((const uint8_t *)px_buf,
                            sizeof(pixel) * guild_mark_block::SIZE, comp_buf,
                            &size_comp_buf_value,
                            get_clzo().work_memory())) {
            metin2::log::error("Guild mark block needs {} bytes but the compression buffer holds {}", sizeof(pixel) * guild_mark_block::SIZE, size_comp_buf);
            return;
        }

        size_comp_buf = (uint32_t)size_comp_buf_value;
        crc = get_crc32((const char *)px_buf, sizeof(pixel) * guild_mark_block::SIZE);
    }
}
