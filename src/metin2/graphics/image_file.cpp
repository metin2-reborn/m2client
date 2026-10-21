#include "image_file.h"

#include <raylib.h>

#include <cstring>

// raylib compiles stb_image_write in (SUPPORT_IMAGE_EXPORT); ExportImage fixes the JPEG quality at 90,
// so the writer is called directly.
extern "C" int stbi_write_jpg(const char *filename, int w, int h, int comp, const void *data, int quality);

namespace metin2::inline graphics {

    // raylib reads and writes through libc, so the case-insensitive file layer applies.
    auto load_image_file(const char *path, bgra_image &image) -> bool {
        Image decoded = LoadImage(path);
        if (!decoded.data)
            return false;

        ImageFormat(&decoded, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        image.width = decoded.width;
        image.height = decoded.height;
        image.pixels.resize(static_cast<size_t>(decoded.width) * decoded.height);
        const auto *rgba = static_cast<const uint8_t *>(decoded.data);
        for (uint32_t &pixel : image.pixels) {
            pixel = (static_cast<uint32_t>(rgba[3]) << 24) | (static_cast<uint32_t>(rgba[0]) << 16) | (static_cast<uint32_t>(rgba[1]) << 8) | rgba[2];
            rgba += 4;
        }
        UnloadImage(decoded);
        return true;
    }

    namespace {
        auto sniff_extension(const uint8_t *d, size_t n) -> const char * {
            if (n >= 3 && d[0] == 0xff && d[1] == 0xd8 && d[2] == 0xff)
                return ".jpg";
            if (n >= 8 && d[0] == 0x89 && d[1] == 'P' && d[2] == 'N' && d[3] == 'G')
                return ".png";
            if (n >= 2 && d[0] == 'B' && d[1] == 'M')
                return ".bmp";
            return ".tga";
        }

        // One channel of a packed pixel scaled to 0..255; 255 for a missing channel.
        auto channel(uint32_t pixel, uint32_t mask) -> uint8_t {
            if (!mask)
                return 255;
            const int shift = __builtin_ctz(mask);
            const uint32_t max = mask >> shift;
            return static_cast<uint8_t>(((pixel & mask) >> shift) * 255 / max);
        }

        auto decode_dds(const uint8_t *d, size_t size, rgba_image &image) -> bool {
            constexpr uint32_t DDPF_ALPHAPIXELS = 0x1;
            constexpr uint32_t DDPF_RGB = 0x40;
            if (size < 128)
                return false;
            auto u32 = [d](size_t offset) {
                uint32_t v;
                memcpy(&v, d + offset, 4);
                return v;
            };
            const uint32_t height = u32(12);
            const uint32_t width = u32(16);
            const uint32_t flags = u32(80);
            const uint32_t bits = u32(88);
            const uint32_t masks[4] = {u32(92), u32(96), u32(100), (flags & DDPF_ALPHAPIXELS) ? u32(104) : 0};
            if (!(flags & DDPF_RGB) || (bits != 16 && bits != 24 && bits != 32) || width == 0 || height == 0 || width > 0x4000 || height > 0x4000)
                return false;
            const size_t bytes = bits / 8;
            if (128 + static_cast<size_t>(width) * height * bytes > size)
                return false;
            image.width = static_cast<int>(width);
            image.height = static_cast<int>(height);
            image.has_alpha = masks[3] != 0;
            image.pixels.resize(static_cast<size_t>(width) * height * 4);
            const uint8_t *src = d + 128;
            for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i, src += bytes) {
                uint32_t pixel = 0;
                memcpy(&pixel, src, bytes);
                for (int c = 0; c < 4; ++c)
                    image.pixels[i * 4 + c] = channel(pixel, masks[c]);
            }
            return true;
        }
    }

    auto decode_image(const void *data, size_t size, rgba_image &image) -> bool {
        const auto *bytes = static_cast<const uint8_t *>(data);
        if (size >= 4 && memcmp(bytes, "DDS ", 4) == 0)
            return decode_dds(bytes, size, image);

        Image decoded = LoadImageFromMemory(sniff_extension(bytes, size), bytes, static_cast<int>(size));
        if (!decoded.data)
            return false;
        image.has_alpha = decoded.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 || decoded.format == PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA || decoded.format == PIXELFORMAT_UNCOMPRESSED_R5G5B5A1 || decoded.format == PIXELFORMAT_UNCOMPRESSED_R4G4B4A4 || decoded.format == PIXELFORMAT_UNCOMPRESSED_R16G16B16A16 || decoded.format == PIXELFORMAT_UNCOMPRESSED_R32G32B32A32;
        ImageFormat(&decoded, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        image.width = decoded.width;
        image.height = decoded.height;
        const auto *rgba = static_cast<const uint8_t *>(decoded.data);
        image.pixels.assign(rgba, rgba + static_cast<size_t>(decoded.width) * decoded.height * 4);
        UnloadImage(decoded);
        return true;
    }

    namespace {
        // The file type follows the extension.
        auto export_image(const char *path, int width, int height, const uint32_t *pixels) -> bool {
            std::vector<uint8_t> rgba(static_cast<size_t>(width) * height * 4);
            for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i) {
                const uint32_t pixel = pixels[i];
                rgba[i * 4 + 0] = static_cast<uint8_t>(pixel >> 16);
                rgba[i * 4 + 1] = static_cast<uint8_t>(pixel >> 8);
                rgba[i * 4 + 2] = static_cast<uint8_t>(pixel);
                rgba[i * 4 + 3] = static_cast<uint8_t>(pixel >> 24);
            }
            Image image = {};
            image.data = rgba.data();
            image.width = width;
            image.height = height;
            image.mipmaps = 1;
            image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
            return ExportImage(image, path);
        }
    }

    auto save_tga_file(const char *path, int width, int height, const uint32_t *pixels) -> bool {
        return export_image(path, width, height, pixels);
    }

    auto save_png_file(const char *path, int width, int height, const uint32_t *pixels) -> bool {
        return export_image(path, width, height, pixels);
    }

    auto save_jpeg_file(const char *path, int width, int height, const uint8_t *rgb, int quality) -> bool {
        return stbi_write_jpg(path, width, height, 3, rgb, quality) != 0;
    }
}
