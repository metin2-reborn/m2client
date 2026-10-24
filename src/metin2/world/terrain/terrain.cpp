#include "terrain.h"

#include <metin2/core/timer.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>

#include <cstring>

namespace metin2::inline world::inline terrain {
    texture_set *terrain_impl::texture_set_value_ = nullptr;

    void terrain_impl::set_texture_set(texture_set *texture_set_value) {
        static texture_set empty_texture_set;

        if (!texture_set_value) {
            texture_set_value_ = &empty_texture_set;
        } else {
            texture_set_value_ = texture_set_value;
        }
    }

    auto terrain_impl::get_texture_set() -> texture_set * {
        if (!texture_set_value_) {
            set_texture_set(nullptr);
        }

        return texture_set_value_;
    }

    terrain_impl::terrain_impl() {
        initialize();
    }

    terrain_impl::~terrain_impl() {
        clear();
    }

    void terrain_impl::initialize() {
        memset(water_height_, -1, sizeof(water_height_));

        num_water_ = 0;
        memset(shadow_map_, 0xFF, sizeof(shadow_map_));
        for (auto &texture : alpha_texture_) {
            texture.reset();
        }

        view_radius_ = 0;

        height_scale_ = 0.0f;

        shadow_texture_ = nullptr;
    }

    void terrain_impl::clear() {
        for (uint32_t i = 0; i < get_texture_set()->texture_count(); ++i) {
            alpha_texture_[i].reset();
        }

        initialize();
    }

    auto terrain_impl::load_height_map(const char *file_name) -> bool {
        metin2::log::debug("Loading height map {}", file_name);

        mapped_file mapped_file;
        const void *lpcv_file_data;

        if (!get_file_manager().get(mapped_file, file_name, &lpcv_file_data)) {
            metin2::log::debug("Height map open failed");
            metin2::log::error("Cannot open height map {}", file_name);
            return false;
        }

        if (mapped_file.size() < sizeof(raw_height_map_)) {
            metin2::log::error("Height map {}: wrong file size", file_name);
            return false;
        }

        memcpy(raw_height_map_, lpcv_file_data, sizeof(raw_height_map_));

        return true;
    }

    auto terrain_impl::load_attr_map(const char *file_name) -> bool {
        uint32_t start = el_timer_get_m_sec();
        metin2::log::debug("Loading attribute map {}", file_name);

        mapped_file mapped_file;
        const void *lpcv_file_data;

        if (!get_file_manager().get(mapped_file, file_name, &lpcv_file_data)) {
            metin2::log::error("Cannot open attribute map {}", file_name);
            return false;
        }

        uint32_t file_size = mapped_file.size();
        const auto *file_data = (const uint8_t *)lpcv_file_data;

        {
#pragma pack(push)
#pragma pack(1)
            struct attr_map_header {
                uint16_t magic;
                uint16_t width;
                uint16_t height;
            };
#pragma pack(pop)

            static_assert(sizeof(attr_map_header) == 6, "attribute map header layout");

            if (file_size < sizeof(attr_map_header)) {
                metin2::log::error("Attribute map {}: wrong file size", file_name);
                return false;
            }

            attr_map_header attr_map_header;
            memcpy(&attr_map_header, file_data, sizeof(attr_map_header));

            const uint16_t attr_map_magic = 2634;
            if (attr_map_magic != attr_map_header.magic) {
                metin2::log::error("Attribute map {}: magic number {} does not match {}", file_name, attr_map_header.magic, attr_map_magic);
                return false;
            }

            if (ATTRMAP_XSIZE != attr_map_header.width) {
                metin2::log::error("Attribute map {}: width {} does not match {}", file_name, attr_map_header.width, ATTRMAP_XSIZE);
                return false;
            }

            if (ATTRMAP_YSIZE != attr_map_header.height) {
                metin2::log::error("Attribute map {}: height {} does not match {}", file_name, attr_map_header.height, ATTRMAP_YSIZE);
                return false;
            }

            uint32_t file_rest_size = file_size - sizeof(attr_map_header);
            uint32_t file_need_size = sizeof(attr_map_);
            if (file_rest_size != file_need_size) {
                metin2::log::error("Attribute map {}: {} bytes left, {} needed", file_name, file_rest_size, file_need_size);
                return false;
            }

            const uint8_t *src_attr_data = file_data + sizeof(attr_map_header);
            memcpy(attr_map_, src_attr_data, sizeof(attr_map_));
        }

        metin2::log::debug("Attribute map loaded in {} ms", el_timer_get_m_sec() - start);
        return true;
    }

    auto terrain_impl::raw_load_tile_map(const char *file_name) -> bool {
        metin2::log::debug("Loading tile map {}", file_name);

        mapped_file mapped_file;
        const void *lpcv_file_data;

        if (!get_file_manager().get(mapped_file, file_name, &lpcv_file_data)) {
            metin2::log::debug("Tile map open failed");
            metin2::log::error("Cannot open tile map {}", file_name);
            return false;
        }

        if (mapped_file.size() < sizeof(tile_map_)) {
            metin2::log::error("Tile map {}: wrong file size", file_name);
            return false;
        }

        memcpy(tile_map_, lpcv_file_data, sizeof(tile_map_));

        return true;
    }

    auto terrain_impl::load_water_map(const char *file_name) -> bool {
        uint32_t start = el_timer_get_m_sec();

        if (!load_water_map_file(file_name)) {
            memset(water_map_, 0xFF, sizeof(water_map_));

            num_water_ = 0;
            memset(water_height_, -1, sizeof(water_height_));

            metin2::log::error("Cannot load water map {}", file_name);

            return false;
        }

        metin2::log::debug("Water map {} loaded in {} ms", file_name, el_timer_get_m_sec() - start);
        return true;
    }

    auto terrain_impl::load_water_map_file(const char *file_name) -> bool {
        mapped_file mapped_file;
        const void *lpcv_file_data;

        if (!get_file_manager().get(mapped_file, file_name, &lpcv_file_data)) {
            metin2::log::debug("Water map open failed");
            metin2::log::error("Cannot open water map {}", file_name);
            return false;
        }

        uint32_t file_size = mapped_file.size();
        const auto *file_data = (const uint8_t *)lpcv_file_data;

        {
#pragma pack(push)
#pragma pack(1)
            struct water_map_header {
                uint16_t magic;
                uint16_t width;
                uint16_t height;
                uint8_t layer_count;
            };
#pragma pack(pop)
            static_assert(sizeof(water_map_header) == 7, "water map header layout");

            if (file_size < sizeof(water_map_header)) {
                metin2::log::error("Water map {}: wrong file size", file_name);
                return false;
            }

            water_map_header water_map_header;
            memcpy(&water_map_header, file_data, sizeof(water_map_header));

            const uint16_t water_map_magic = 5426;

            if (water_map_magic != water_map_header.magic) {
                metin2::log::error("Water map {}: magic number {} does not match {}", file_name, water_map_header.magic, water_map_magic);
                return false;
            }

            if (WATERMAP_XSIZE != water_map_header.width) {
                metin2::log::error("Water map {}: width {} does not match {}", file_name, water_map_header.width, WATERMAP_XSIZE);
                return false;
            }

            if (WATERMAP_YSIZE != water_map_header.height) {
                metin2::log::error("Water map {}: height {} does not match {}", file_name, water_map_header.height, WATERMAP_YSIZE);
                return false;
            }

            num_water_ = water_map_header.layer_count;

            // The water heights follow the map as 16-bit or 32-bit values, depending on the file version.
            uint32_t file_rest_size = file_size - sizeof(water_map_header);
            uint32_t file_need_size32 = sizeof(water_map_) + sizeof(int32_t) * num_water_;
            uint32_t file_need_size16 = sizeof(water_map_) + sizeof(uint16_t) * num_water_;

            if (file_rest_size != file_need_size16 && file_rest_size != file_need_size32) {
                metin2::log::error("Water map {}: {} bytes left, {} needed", file_name, file_rest_size, file_need_size32);
                return false;
            }

            const uint8_t *src_water_data = file_data + sizeof(water_map_header);
            memcpy(water_map_, src_water_data, sizeof(water_map_));

            const uint8_t *src_water_height = src_water_data + sizeof(water_map_);

            if (file_rest_size == file_need_size16) {
                for (int i = 0; i < num_water_; ++i) {
                    uint16_t height;
                    memcpy(&height, src_water_height + i * sizeof(uint16_t), sizeof(height));
                    water_height_[i] = height;
                }
            } else if (num_water_) {
                memcpy(water_height_, src_water_height, sizeof(int32_t) * num_water_);
            }
        }

        return true;
    }

    auto terrain_impl::shadow_map_color(float fx, float fy) -> uint32_t {
        auto map_size = (float)(TERRAIN_XSIZE);

        float foo_map_size = 1.0f / map_size;
        if (fx < 0 || fy < 0 || fx >= map_size || fy >= map_size) {
            return 0xFFFFFFFF;
        }

        fx = fx * foo_map_size * (float)(SHADOWMAP_XSIZE - 1);
        fy = fy * foo_map_size * (float)(SHADOWMAP_YSIZE - 1);

        int ix, iy;
        PR_FLOAT_TO_INT(fx, ix);
        PR_FLOAT_TO_INT(fy, iy);

        uint16_t w = shadow_map_[iy * SHADOWMAP_XSIZE + ix];

        uint32_t b = (w & 0x1f) << 3;
        uint32_t g = ((w >> 5) & 0x3f) << 2;
        uint32_t r = ((w >> 11) & 0x1f) << 3;

        return 0xff000000 | (r << 16) | (g << 8) | b;
    }
}
