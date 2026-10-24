#include "texture_set.h"

#include "terrain_type.h"

#include <metin2/core/mapped_file.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/log/log.h>

#include <metin2/gpu/device.h>
#include <metin2/gpu/format.h>

#include <algorithm>
#include <cctype>
#include <cstring>

#include <toml++/toml.hpp>

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

        // A missing value keeps the default.
        auto read_pair(const toml::node *node, float &first, float &second) -> bool {
            if (!node) {
                return true;
            }
            const toml::array *pair = node->as_array();
            if (!pair || pair->size() != 2) {
                return false;
            }
            const auto a = (*pair)[0].value<double>();
            const auto b = (*pair)[1].value<double>();
            if (!a || !b) {
                return false;
            }
            first = static_cast<float>(*a);
            second = static_cast<float>(*b);
            return true;
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

    auto parse_texture_set(std::string_view text, std::string_view name) -> std::optional<std::vector<texture_set_entry>> {
        toml::parse_result parsed = toml::parse(text, name);
        if (!parsed) {
            const toml::parse_error &error = parsed.error();
            metin2::log::error("{}:{}: {}", name, error.source().begin.line, error.description());
            return std::nullopt;
        }

        std::vector<texture_set_entry> entries;
        for (const auto &[key, node] : parsed.table()) {
            if (key.str() != "texture") {
                metin2::log::warn("{}:{}: unknown key {}", name, node.source().begin.line, key.str());
            }
        }
        const toml::array *textures = parsed.table()["texture"].as_array();
        if (!textures) {
            return entries;
        }

        for (const toml::node &node : *textures) {
            const auto line = node.source().begin.line;
            const toml::table *table = node.as_table();
            if (!table) {
                metin2::log::error("{}:{}: texture must be a table ([[texture]])", name, line);
                continue;
            }
            for (const auto &[key, value] : *table) {
                if (key.str() != "index" && key.str() != "path" && key.str() != "uv_scale" && key.str() != "uv_offset") {
                    metin2::log::warn("{}:{}: unknown key {}", name, value.source().begin.line, key.str());
                }
            }

            texture_set_entry entry;
            const auto index = (*table)["index"].value_exact<int64_t>();
            if (!index || *index < 1 || *index >= MAXTERRAINTEXTURES) {
                metin2::log::error("{}:{}: index must be a whole number from 1 to {}", name, line, MAXTERRAINTEXTURES - 1);
                continue;
            }
            entry.index = static_cast<uint32_t>(*index);
            if (std::ranges::any_of(entries, [&](const texture_set_entry &other) { return other.index == entry.index; })) {
                metin2::log::error("{}:{}: index {} is used twice", name, line, entry.index);
                continue;
            }

            const auto path = (*table)["path"].value_exact<std::string>();
            if (!path || path->empty()) {
                metin2::log::error("{}:{}: path must be the texture's file name", name, line);
                continue;
            }
            entry.path = *path;

            if (!read_pair((*table)["uv_scale"].node(), entry.u_scale, entry.v_scale)) {
                metin2::log::error("{}:{}: uv_scale must be two numbers", name, line);
                continue;
            }
            if (!read_pair((*table)["uv_offset"].node(), entry.u_offset, entry.v_offset)) {
                metin2::log::error("{}:{}: uv_offset must be two numbers", name, line);
                continue;
            }
            entries.push_back(std::move(entry));
        }
        return entries;
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

        mapped_file file;
        const void *data = nullptr;
        if (!get_file_manager().get(file, texture_set_file_name, &data)) {
            metin2::log::error("Cannot load texture set {}", texture_set_file_name);
            return false;
        }

        const auto entries = parse_texture_set(std::string_view(static_cast<const char *>(data), data ? file.size() : 0), texture_set_file_name);
        if (!entries) {
            return false;
        }

        create();

        uint32_t count = 0;
        for (const texture_set_entry &entry : *entries) {
            count = std::max(count, entry.index);
        }
        textures_.resize(count + 1);

        for (const texture_set_entry &entry : *entries) {
            if (!set_texture(entry.index, entry.path.c_str(), entry.u_scale, entry.v_scale, entry.u_offset, entry.v_offset, terrain_tex_coord_base)) {
                metin2::log::error("Cannot set texture {}", entry.path);
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
