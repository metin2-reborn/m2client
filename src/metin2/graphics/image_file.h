#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace metin2::inline graphics {

    // An image in memory: 32-bit pixels 0xAARRGGBB (B, G, R, A bytes), rows top-down.
    struct bgra_image {
        int width = 0;
        int height = 0;
        std::vector<uint32_t> pixels;
    };

    // Decodes a TGA, PNG, BMP or JPEG file.
    auto load_image_file(const char *path, bgra_image &image) -> bool;

    // An image in memory as R, G, B, A bytes, rows top-down.
    struct rgba_image {
        int width = 0;
        int height = 0;
        bool has_alpha = false; // the file has an alpha channel
        std::vector<uint8_t> pixels;
    };

    // Decodes a TGA, PNG, BMP or JPEG file held in memory, or an uncompressed 16, 24 or 32-bit DDS
    // (only its first level; the DXT formats are read by dxtc_image).
    auto decode_image(const void *data, size_t size, rgba_image &image) -> bool;

    auto save_tga_file(const char *path, int width, int height, const uint32_t *pixels) -> bool;
    auto save_png_file(const char *path, int width, int height, const uint32_t *pixels) -> bool;

    // rgb: 3 bytes per pixel (R, G, B), rows top-down; quality 1 to 100.
    auto save_jpeg_file(const char *path, int width, int height, const uint8_t *rgb, int quality) -> bool;
}
