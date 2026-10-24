#include "texture_set.h"

#include "terrain_type.h"

#include <metin2/core/parse.h>
#include <metin2/core/stl.h>
#include <metin2/core/util.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/log/log.h>

#include <metin2/gpu/device.h>
#include <metin2/gpu/format.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace metin2::inline world::inline terrain {
    namespace {
        auto is_grass_texture(std::string name) -> bool {
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (name.find("seagrass") != std::string::npos) {
                return false;
            }
            return name.find("grass") != std::string::npos || name.find("field") != std::string::npos;
        }

        auto rgb565_to_rgb(uint16_t c, int rgb[3]) {
            rgb[0] = ((c >> 11) & 31) * 255 / 31;
            rgb[1] = ((c >> 5) & 63) * 255 / 63;
            rgb[2] = (c & 31) * 255 / 31;
        }

        // The average colour of a 4x4 DXT colour block (the second half of a DXT3/5 block).
        auto dxt_block_average(const uint8_t *block, bool dxt1) -> uint32_t {
            uint16_t c0, c1;
            memcpy(&c0, block, 2);
            memcpy(&c1, block + 2, 2);
            int p[4][3];
            rgb565_to_rgb(c0, p[0]);
            rgb565_to_rgb(c1, p[1]);
            const bool four = !dxt1 || c0 > c1;
            for (int i = 0; i < 3; ++i) {
                p[2][i] = four ? (2 * p[0][i] + p[1][i]) / 3 : (p[0][i] + p[1][i]) / 2;
                p[3][i] = four ? (p[0][i] + 2 * p[1][i]) / 3 : 0;
            }
            uint32_t bits;
            memcpy(&bits, block + 4, 4);
            int sum[3] = {0, 0, 0};
            for (int t = 0; t < 16; ++t) {
                const int *c = p[(bits >> (2 * t)) & 3];
                for (int i = 0; i < 3; ++i) {
                    sum[i] += c[i];
                }
            }
            return 0xff000000u | (static_cast<uint32_t>(sum[0] / 16) << 16) | (static_cast<uint32_t>(sum[1] / 16) << 8) | static_cast<uint32_t>(sum[2] / 16);
        }

        // From the smallest mip level, read back from the GPU.
        auto texture_average_color(gpu::texture *texture, uint32_t fallback) -> uint32_t {
            if (!texture) {
                return fallback;
            }
            const uint32_t level = texture->level_count() - 1;
            gpu::locked_rect locked;
            if (!texture->lock(level, locked, nullptr, gpu::lock::READ_ONLY)) {
                return fallback;
            }
            uint32_t color = fallback;
            const auto fmt = texture->pixel_format();
            if (fmt == gpu::format::DXT1) {
                color = dxt_block_average(locked.bits, true);
            } else if (fmt == gpu::format::DXT3 || fmt == gpu::format::DXT5) {
                color = dxt_block_average(locked.bits + 8, false);
            } else if (const gpu::format_info *info = gpu::get_format_info(fmt); info && !info->compressed) {
                uint8_t rgba[4];
                gpu::convert_to_rgba8(*info, locked.bits, locked.pitch, 1, 1, rgba);
                color = 0xff000000u | (static_cast<uint32_t>(rgba[0]) << 16) | (static_cast<uint32_t>(rgba[1]) << 8) | rgba[2];
            }
            texture->unlock(level);
            return color;
        }
    }

    texture_set::texture_set() = default;

    texture_set::~texture_set() {
        clear();
    }

    void texture_set::create() {
        auto *resource = get_resource_manager().resource_pointer("d:/ymir work/special/error.tga");
        error_texture_.image_instance.set_image_pointer(static_cast<graphic_image *>(resource));
        add_empty_texture(); // The eraser texture has to be the first entry.
    }

    auto texture_set::load(const char *texture_set_file_name, float terrain_tex_coord_base) -> bool {
        clear();

        token_vector_map token_vector_map;

        if (!load_multiple_text_data(texture_set_file_name, token_vector_map)) {
            metin2::log::error("Cannot load texture set {}", texture_set_file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("textureset")) {
            metin2::log::error("Texture set {}: no TextureSet header", texture_set_file_name);
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("texturecount")) {
            metin2::log::error("Texture set {}: no TextureCount entry", texture_set_file_name);
            return false;
        }

        create();

        const std::string &c_rstr_count = token_vector_map["texturecount"][0];

        int i_count = metin2::to_number<int>(c_rstr_count.c_str());
        if (i_count < 0 || i_count >= MAXTERRAINTEXTURES) {
            metin2::log::error("Texture count {} out of range in texture set {}", i_count, texture_set_file_name);
            return false;
        }
        char texture_name[32 + 1];

        textures_.resize(i_count + 1);

        for (int i = 0; i < i_count; ++i) {
            snprintf(texture_name, sizeof(texture_name), "texture%03d", i + 1);

            if (token_vector_map.end() == token_vector_map.find(texture_name)) {
                continue;
            }

            const auto &vector = token_vector_map[texture_name];
            if (vector.size() < 8) {
                metin2::log::error("Texture {} needs 8 values in texture set {}", texture_name, texture_set_file_name);
                continue;
            }

            const std::string &file_name = vector[0];
            const std::string &u_scale = vector[1];
            const std::string &v_scale = vector[2];
            const std::string &u_offset = vector[3];
            const std::string &v_offset = vector[4];
            const std::string &rstrb_splat = vector[5];
            const std::string &begin = vector[6];
            const std::string &end = vector[7];

            float fu_scale, fv_scale, fu_offset, fv_offset;
            bool splat;
            unsigned short us_begin, us_end;

            fu_scale = float(metin2::to_number<double>(u_scale.c_str()));
            fv_scale = float(metin2::to_number<double>(v_scale.c_str()));
            fu_offset = float(metin2::to_number<double>(u_offset.c_str()));
            fv_offset = float(metin2::to_number<double>(v_offset.c_str()));
            splat = 0 != metin2::to_number<int>(rstrb_splat.c_str());
            us_begin = static_cast<unsigned short>(metin2::to_number<int>(begin.c_str()));
            us_end = static_cast<unsigned short>(metin2::to_number<int>(end.c_str()));

            if (!set_texture(i + 1, file_name.c_str(), fu_scale, fv_scale, fu_offset, fv_offset, splat, us_begin, us_end, terrain_tex_coord_base)) {
                metin2::log::error("Cannot set texture {}", file_name.c_str());
            }
        }

        file_name_.assign(texture_set_file_name);

        return true;
    }

    void texture_set::clear() {
        error_texture_.image_instance.destroy();
        textures_.clear();
        file_name_.clear();
    }

    void texture_set::add_empty_texture() {
        terrain_texture eraser;
        textures_.push_back(eraser);
    }

    auto texture_set::texture_count() -> uint32_t {
        return textures_.size();
    }

    auto texture_set::texture(uint32_t index) -> terrain_texture & {
        if (texture_count() <= index) {
            return error_texture_;
        }

        return textures_[index];
    }

    auto texture_set::set_texture(uint32_t index,
        const char *file_name,
        float fu_scale,
        float fv_scale,
        float fu_offset,
        float fv_offset,
        bool splat,
        uint16_t us_begin,
        uint16_t us_end,
        float terrain_tex_coord_base) -> bool {

        if (index >= textures_.size()) {
            metin2::log::error("Texture index {} out of range ({} textures)", index, textures_.size());
            return false;
        }

        auto *resource = get_resource_manager().resource_pointer(file_name);
        if (!resource || !resource->is_type(graphic_image::type())) {
            metin2::log::error("Texture {} is not an image file", file_name);
            return false;
        }

        terrain_texture &tex = textures_[index];

        tex.filename = file_name;
        tex.u_scale = fu_scale;
        tex.v_scale = fv_scale;
        tex.u_offset = fu_offset;
        tex.v_offset = fv_offset;
        tex.splat = splat;
        tex.begin = us_begin;
        tex.end = us_end;
        tex.image_instance.set_image_pointer(static_cast<graphic_image *>(resource));
        tex.gpu_texture = tex.image_instance.texture_pointer()->gpu_texture();
        tex.transform = math::matrix_scaling(terrain_tex_coord_base * tex.u_scale, -terrain_tex_coord_base * tex.v_scale, 0.0f);
        tex.transform._41 = tex.u_offset;
        tex.transform._42 = -tex.v_offset;
        tex.grass = is_grass_texture(tex.filename);
        if (tex.grass) {
            tex.average_color = texture_average_color(tex.gpu_texture, tex.average_color);
        }

        return true;
    }
}
