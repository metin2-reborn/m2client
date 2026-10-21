#include "dxtc_image.h"

#include <algorithm>
#include <cstring>

namespace metin2::inline graphics {

    namespace {

        constexpr uint32_t DDSD_MIPMAPCOUNT = 0x00020000;
        constexpr uint32_t DDPF_FOURCC = 0x00000004;

        constexpr auto make_four_cc(char a, char b, char c, char d) -> uint32_t {
            return uint32_t(uint8_t(a)) | (uint32_t(uint8_t(b)) << 8) | (uint32_t(uint8_t(c)) << 16) | (uint32_t(uint8_t(d)) << 24);
        }

        // The 124-byte DDS_HEADER that follows the "DDS " magic (DirectDraw's DDSURFACEDESC2).
        struct dds_pixel_format {
            uint32_t size;
            uint32_t flags_value;
            uint32_t four_cc;
            uint32_t rgb_bit_count;
            uint32_t r_bit_mask;
            uint32_t g_bit_mask;
            uint32_t b_bit_mask;
            uint32_t a_bit_mask;
        };

        struct dds_header {
            uint32_t size;
            uint32_t flags_value;
            uint32_t height;
            uint32_t width;
            uint32_t pitch_or_linear_size;
            uint32_t depth;
            uint32_t mip_map_count;
            uint32_t reserved1[11];
            dds_pixel_format ddspf;
            uint32_t caps;
            uint32_t caps2;
            uint32_t caps3;
            uint32_t caps4;
            uint32_t reserved2;
        };

        static_assert(sizeof(dds_pixel_format) == 32, "DDS pixel format layout");
        static_assert(sizeof(dds_header) == 124, "DDS header layout");

        constexpr size_t DDS_MAGIC_SIZE = 4;

        auto block_size(pix_format format) -> uint32_t {
            return format == PF_DXT1 ? 8 : 16;
        }

        // 5:6:5 to R, G, B, A bytes.
        void unpack_565(uint16_t v, uint8_t *out) {
            out[0] = static_cast<uint8_t>(((v >> 11) & 31) * 255 / 31);
            out[1] = static_cast<uint8_t>(((v >> 5) & 63) * 255 / 63);
            out[2] = static_cast<uint8_t>((v & 31) * 255 / 31);
            out[3] = 255;
        }

        // The four colours of a block; DXT1 blocks with c0 <= c1 have three and transparent black.
        void block_palette(const uint8_t *block, bool dxt1, uint8_t palette[4][4]) {
            uint16_t c0, c1;
            memcpy(&c0, block, 2);
            memcpy(&c1, block + 2, 2);
            unpack_565(c0, palette[0]);
            unpack_565(c1, palette[1]);
            for (int c = 0; c < 3; ++c) {
                if (!dxt1 || c0 > c1) {
                    palette[2][c] = static_cast<uint8_t>((2 * palette[0][c] + palette[1][c]) / 3);
                    palette[3][c] = static_cast<uint8_t>((palette[0][c] + 2 * palette[1][c]) / 3);
                } else {
                    palette[2][c] = static_cast<uint8_t>((palette[0][c] + palette[1][c]) / 2);
                    palette[3][c] = 0;
                }
            }
            palette[2][3] = 255;
            palette[3][3] = (dxt1 && c0 <= c1) ? 0 : 255;
        }

        // Byte size of one row of 4x4 blocks and the number of such rows for a mip level.
        void level_layout(int width_value, int height, int miplevel, pix_format format, uint32_t *row_bytes, uint32_t *rows) {
            const uint32_t w = std::max(1, width_value >> miplevel);
            const uint32_t h = std::max(1, height >> miplevel);
            *row_bytes = ((w + 3) / 4) * block_size(format);
            *rows = (h + 3) / 4;
        }

    } // namespace

    dxtc_image::dxtc_image() {
        initialize();
    }

    void dxtc_image::initialize() {
        comp_format = PF_UNKNOWN;
        mip_map_count = 0;
        width = 0;
        height = 0;

        for (int i = 0; i < MAX_MIPLEVELS; ++i)
            comp_buffer_by_levels_[i] = nullptr;
    }

    auto dxtc_image::load_header_from_memory(const uint8_t *map, size_t size) -> bool {
        initialize();

        if (!map || size < DDS_MAGIC_SIZE + sizeof(dds_header))
            return false;

        uint32_t magic;
        memcpy(&magic, map, sizeof(magic));
        if (magic != make_four_cc('D', 'D', 'S', ' '))
            return false;

        dds_header ddsd;
        memcpy(&ddsd, map + DDS_MAGIC_SIZE, sizeof(ddsd));

        if (!(ddsd.ddspf.flags_value & DDPF_FOURCC))
            return false;

        switch (ddsd.ddspf.four_cc) {
        case make_four_cc('D', 'X', 'T', '1'):
            comp_format = PF_DXT1;
            break;
        case make_four_cc('D', 'X', 'T', '2'):
        case make_four_cc('D', 'X', 'T', '3'):
            comp_format = PF_DXT3;
            break;
        case make_four_cc('D', 'X', 'T', '4'):
        case make_four_cc('D', 'X', 'T', '5'):
            comp_format = PF_DXT5;
            break;
        default:
            // uncompressed files are decoded by decode_image (image_file.h)
            return false;
        }

        if (ddsd.width == 0 || ddsd.height == 0 || ddsd.width > 0x10000 || ddsd.height > 0x10000)
            return false;

        width = int(ddsd.width);
        height = int(ddsd.height);

        uint32_t mip_map_count_value = (ddsd.flags_value & DDSD_MIPMAPCOUNT) ? ddsd.mip_map_count : 1;
        mip_map_count = std::min<uint32_t>(std::max<uint32_t>(1, mip_map_count_value), MAX_MIPLEVELS);

        // every level is stored contiguously, smallest last
        size_t offset = DDS_MAGIC_SIZE + sizeof(dds_header);
        for (uint32_t i = 0; i < mip_map_count; ++i) {
            uint32_t row_bytes, rows;
            level_layout(width, height, int(i), comp_format, &row_bytes, &rows);
            const size_t level_size = size_t(row_bytes) * rows;
            if (offset + level_size > size) {
                // truncated file: keep the levels that are complete
                if (i == 0) {
                    initialize();
                    return false;
                }
                mip_map_count = i;
                break;
            }
            comp_buffer_by_levels_[i] = map + offset;
            offset += level_size;
        }

        return true;
    }

    auto dxtc_image::copy(int miplevel, uint8_t *dest, int32_t dest_pitch) const -> bool {
        if (miplevel < 0 || uint32_t(miplevel) >= mip_map_count || !comp_buffer_by_levels_[miplevel])
            return false;

        uint32_t row_bytes, rows;
        level_layout(width, height, miplevel, comp_format, &row_bytes, &rows);

        const uint8_t *src = comp_buffer_by_levels_[miplevel];

        if (dest_pitch <= 0 || uint32_t(dest_pitch) == row_bytes) {
            memcpy(dest, src, size_t(row_bytes) * rows);
            return true;
        }

        for (uint32_t y = 0; y < rows; ++y) {
            memcpy(dest, src, row_bytes);
            src += row_bytes;
            dest += dest_pitch;
        }
        return true;
    }

    auto dxtc_image::decompress(int miplevel, uint8_t *rgba) const -> bool {
        if (miplevel < 0 || uint32_t(miplevel) >= mip_map_count || !comp_buffer_by_levels_[miplevel])
            return false;

        const int w = level_width(miplevel);
        const int h = level_height(miplevel);
        const uint8_t *block = comp_buffer_by_levels_[miplevel];
        for (int by = 0; by < h; by += 4) {
            for (int bx = 0; bx < w; bx += 4, block += block_size(comp_format)) {
                const uint8_t *color_block = comp_format == PF_DXT1 ? block : block + 8;
                uint8_t palette[4][4];
                block_palette(color_block, comp_format == PF_DXT1, palette);
                uint32_t indices;
                memcpy(&indices, color_block + 4, 4);

                uint8_t alpha[16];
                if (comp_format == PF_DXT3) {
                    for (int i = 0; i < 16; ++i)
                        alpha[i] = static_cast<uint8_t>(((block[i / 2] >> ((i & 1) * 4)) & 15) * 17);
                } else if (comp_format == PF_DXT5) {
                    uint8_t a[8];
                    a[0] = block[0];
                    a[1] = block[1];
                    for (int i = 0; i < 6; ++i)
                        a[2 + i] = a[0] > a[1] ? static_cast<uint8_t>(((6 - i) * a[0] + (1 + i) * a[1]) / 7) : (i < 4 ? static_cast<uint8_t>(((4 - i) * a[0] + (1 + i) * a[1]) / 5) : (i == 4 ? 0 : 255));
                    uint64_t bits = 0;
                    for (int i = 0; i < 6; ++i)
                        bits |= static_cast<uint64_t>(block[2 + i]) << (8 * i);
                    for (int i = 0; i < 16; ++i)
                        alpha[i] = a[(bits >> (3 * i)) & 7];
                }

                for (int i = 0; i < 16; ++i) {
                    const int x = bx + (i & 3);
                    const int y = by + (i >> 2);
                    if (x >= w || y >= h)
                        continue;
                    uint8_t *out = rgba + (static_cast<size_t>(y) * w + x) * 4;
                    memcpy(out, palette[(indices >> (2 * i)) & 3], 4);
                    if (comp_format != PF_DXT1)
                        out[3] = alpha[i];
                }
            }
        }
        return true;
    }
}
