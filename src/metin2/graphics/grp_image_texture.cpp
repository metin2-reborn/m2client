#include "grp_image_texture.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <metin2/core/mapped_file.h>
#include <metin2/log/log.h>
#include <vector>
#include "image_file.h"
#include "resource_manager.h"

namespace metin2::inline graphics {

    namespace {
        auto full_mip_count(uint32_t w, uint32_t h) -> uint32_t {
            uint32_t count = 1;
            while (w > 1 || h > 1) {
                w = std::max(1u, w / 2);
                h = std::max(1u, h / 2);
                ++count;
            }
            return count;
        }

        // sRGB <-> linear through tables: the mips are averaged in linear light so they do not darken.
        auto srgb_to_linear_table() -> const std::array<float, 256> & {
            static const std::array<float, 256> table = [] {
                std::array<float, 256> t{};
                for (int i = 0; i < 256; ++i) {
                    const float c = static_cast<float>(i) / 255.0f;
                    t[i] = c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
                }
                return t;
            }();
            return table;
        }

        auto linear_to_srgb_byte(float linear) -> uint8_t {
            static const std::array<uint8_t, 4096> table = [] {
                std::array<uint8_t, 4096> t{};
                for (int i = 0; i < 4096; ++i) {
                    const float c = static_cast<float>(i) / 4095.0f;
                    const float s = c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
                    t[i] = static_cast<uint8_t>(std::lround(std::clamp(s, 0.0f, 1.0f) * 255.0f));
                }
                return t;
            }();
            return table[static_cast<size_t>(std::clamp(linear, 0.0f, 1.0f) * 4095.0f + 0.5f)];
        }

        // 2x2 box filter in linear light, the colour weighted by alpha so transparent texels (the
        // colour key leaves them black) do not darken the edges of the next level. A block without
        // any alpha takes the plain average: not every alpha channel is coverage (character skins
        // keep a specular mask there, mostly 0), and black would show through at a distance.
        void downsample(const std::vector<uint8_t> &src, uint32_t sw, uint32_t sh, std::vector<uint8_t> &dst, uint32_t dw, uint32_t dh) {
            const std::array<float, 256> &to_linear = srgb_to_linear_table();
            dst.assign(static_cast<size_t>(dw) * dh * 4, 0);
            for (uint32_t y = 0; y < dh; ++y)
                for (uint32_t x = 0; x < dw; ++x) {
                    const uint32_t xs[2] = {std::min(x * 2, sw - 1), std::min(x * 2 + 1, sw - 1)};
                    const uint32_t ys[2] = {std::min(y * 2, sh - 1), std::min(y * 2 + 1, sh - 1)};
                    float color[3] = {0.0f, 0.0f, 0.0f};
                    float plain[3] = {0.0f, 0.0f, 0.0f};
                    float alpha_sum = 0.0f;
                    for (uint32_t sy : ys)
                        for (uint32_t sx : xs) {
                            const uint8_t *p = &src[(static_cast<size_t>(sy) * sw + sx) * 4];
                            const float a = static_cast<float>(p[3]);
                            alpha_sum += a;
                            for (int c = 0; c < 3; ++c) {
                                color[c] += to_linear[p[c]] * a;
                                plain[c] += to_linear[p[c]];
                            }
                        }
                    uint8_t *out = &dst[(static_cast<size_t>(y) * dw + x) * 4];
                    for (int c = 0; c < 3; ++c)
                        out[c] = linear_to_srgb_byte(alpha_sum > 0.0f ? color[c] / alpha_sum : plain[c] / 4.0f);
                    out[3] = static_cast<uint8_t>((alpha_sum + 2.0f) / 4.0f);
                }
        }
    }

    auto graphic_image_texture::lock(int *ret_pitch, void **ret_pixels, int level) -> bool {
        gpu::locked_rect locked_rect;
        if (!texture_ || !texture_->lock(level, locked_rect))
            return false;

        *ret_pitch = static_cast<int>(locked_rect.pitch);
        *ret_pixels = locked_rect.bits;
        return true;
    }

    void graphic_image_texture::unlock(int level) {
        assert(texture_);
        texture_->unlock(level);
    }

    void graphic_image_texture::initialize() {
        graphic_texture::initialize();

        file_name_ = "";

        format_ = gpu::format::UNKNOWN;
    }

    void graphic_image_texture::destroy() {
        graphic_texture::destroy();

        initialize();
    }

    auto graphic_image_texture::create_device_objects() -> bool {
        assert(device_);
        assert(!texture_);

        if (file_name_.empty()) {
            // No file name: an empty font texture.
            texture_ = device_->create_texture(width_, height_, 1, 0, format_);
            if (!texture_)
                return false;
        } else {
            mapped_file mapped_file;
            const void *map;

            if (!resource_manager::file(mapped_file, file_name_.c_str(), &map))
                return false;

            return create_from_memory_file(mapped_file.size(), map, format_);
        }

        empty_ = false;
        return true;
    }

    auto graphic_image_texture::create(uint32_t width_value, uint32_t height, gpu::format fmt) -> bool {
        assert(device_);
        destroy();

        width_ = width_value;
        height_ = height;
        format_ = fmt;

        return create_device_objects();
    }

    void graphic_image_texture::create_from_texture_pointer(const graphic_texture *src_texture) {
        width_ = src_texture->width();
        height_ = src_texture->height();
        texture_ = src_texture->shared_texture();

        empty_ = false;
    }

    auto graphic_image_texture::create_dds_texture(dxtc_image &image) -> bool {
        int mipmap_count = image.mip_map_count == 0 ? 1 : image.mip_map_count;

        // With little texture memory the largest level is skipped.
        int min_mip_map_index = 0;
        if (is_low_texture_memory() && mipmap_count > 1) {
            min_mip_map_index = 1;
            mipmap_count -= 1;
        }

        const uint32_t w = image.level_width(min_mip_map_index);
        const uint32_t h = image.level_height(min_mip_map_index);

        if (support_dxt_) {
            gpu::format format = gpu::format::DXT1;
            if (image.comp_format == PF_DXT5)
                format = gpu::format::DXT5;
            else if (image.comp_format == PF_DXT3)
                format = gpu::format::DXT3;

            texture_ = device_->create_texture(w, h, mipmap_count, 0, format);
            if (!texture_) {
                metin2::log::error("Cannot create the DDS texture");
                return false;
            }

            for (int i = 0; i < mipmap_count; ++i) {
                gpu::locked_rect locked_rect;
                if (!texture_->lock(i, locked_rect)) {
                    metin2::log::error("Cannot lock the DDS texture");
                } else {
                    image.copy(i + min_mip_map_index, locked_rect.bits, static_cast<int32_t>(locked_rect.pitch));
                    texture_->unlock(i);
                }
            }
        } else {
            // No compressed textures on this device: decode on the CPU.
            texture_ = device_->create_texture(w, h, mipmap_count, 0, gpu::format::A8R8G8B8);
            if (!texture_) {
                metin2::log::error("Cannot create the DDS texture");
                return false;
            }

            std::vector<uint8_t> rgba;
            for (int i = 0; i < mipmap_count; ++i) {
                const int level = i + min_mip_map_index;
                rgba.resize(static_cast<size_t>(image.level_width(level)) * image.level_height(level) * 4);
                if (image.decompress(level, rgba.data()))
                    texture_->upload_rgba8(i, rgba.data());
            }
        }

        width_ = static_cast<int>(w);
        height_ = static_cast<int>(h);
        empty_ = false;

        return true;
    }

    auto graphic_image_texture::create_from_memory_file(uint32_t buf_size, const void *buf, gpu::format fmt) -> bool {
        assert(device_);
        assert(!texture_);

        dxtc_image image;
        if (image.load_header_from_memory((const uint8_t *)buf, buf_size))
            return create_dds_texture(image);

        rgba_image decoded;
        if (!decode_image(buf, buf_size, decoded) || decoded.width <= 0 || decoded.height <= 0) {
            metin2::log::error("Cannot create a texture from the image in memory");
            return false;
        }

        // Magenta is the colour key of files without alpha.
        bool keyed = false;
        for (size_t i = 0; i + 3 < decoded.pixels.size(); i += 4) {
            uint8_t *p = &decoded.pixels[i];
            if (p[0] == 0xff && p[1] == 0x00 && p[2] == 0xff && (p[3] == 0xff || !decoded.has_alpha)) {
                p[3] = 0;
                keyed = true;
            }
        }

        // The keyed texels are invisible but still filtered into their neighbours, which gave sprites
        // a magenta fringe: give them the colour of the opaque texels around them.
        if (keyed) {
            const int w = decoded.width;
            const int h = decoded.height;
            std::vector<uint8_t> &px = decoded.pixels;
            const std::vector<uint8_t> source = px;
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    uint8_t *p = &px[(static_cast<size_t>(y) * w + x) * 4];
                    if (p[3] != 0) {
                        continue;
                    }
                    int sum[3] = {0, 0, 0};
                    int count = 0;
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            const int nx = x + dx;
                            const int ny = y + dy;
                            if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
                                continue;
                            }
                            const uint8_t *n = &source[(static_cast<size_t>(ny) * w + nx) * 4];
                            if (n[3] == 0) {
                                continue;
                            }
                            sum[0] += n[0];
                            sum[1] += n[1];
                            sum[2] += n[2];
                            ++count;
                        }
                    }
                    if (count > 0) {
                        p[0] = static_cast<uint8_t>(sum[0] / count);
                        p[1] = static_cast<uint8_t>(sum[1] / count);
                        p[2] = static_cast<uint8_t>(sum[2] / count);
                    } else {
                        p[0] = p[1] = p[2] = 0;
                    }
                }
            }
        }

        const bool alpha = decoded.has_alpha || keyed;
        if (is_low_texture_memory())
            fmt = alpha ? gpu::format::A4R4G4B4 : gpu::format::A1R5G5B5;
        else if (fmt == gpu::format::UNKNOWN)
            fmt = alpha ? gpu::format::A8R8G8B8 : gpu::format::X8R8G8B8;
        const gpu::format_info *info = gpu::get_format_info(fmt);
        if (!info || info->compressed || info->depth)
            fmt = gpu::format::A8R8G8B8;

        const auto w = static_cast<uint32_t>(decoded.width);
        const auto h = static_cast<uint32_t>(decoded.height);
        const uint32_t levels = full_mip_count(w, h);
        texture_ = device_->create_texture(w, h, levels, 0, fmt);
        if (!texture_) {
            metin2::log::error("Cannot create a texture from the image in memory");
            return false;
        }

        texture_->upload_rgba8(0, decoded.pixels.data());
        std::vector<uint8_t> current = std::move(decoded.pixels);
        std::vector<uint8_t> next;
        uint32_t cw = w;
        uint32_t ch = h;
        for (uint32_t level = 1; level < levels; ++level) {
            const uint32_t nw = std::max(1u, cw / 2);
            const uint32_t nh = std::max(1u, ch / 2);
            downsample(current, cw, ch, next, nw, nh);
            texture_->upload_rgba8(level, next.data());
            current.swap(next);
            cw = nw;
            ch = nh;
        }

        width_ = static_cast<int>(w);
        height_ = static_cast<int>(h);
        empty_ = false;
        return true;
    }

    void graphic_image_texture::set_file_name(const char *file_name) {
        file_name_ = file_name;
    }

    graphic_image_texture::graphic_image_texture() {
        initialize();
    }

    graphic_image_texture::~graphic_image_texture() {
        destroy();
    }
}
