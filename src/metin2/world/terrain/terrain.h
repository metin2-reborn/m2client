#pragma once

#include <cstdint>

#include <metin2/gpu/types.h>

#include "terrain_type.h"
#include "texture_set.h"

namespace metin2::inline world::inline terrain {

    class terrain_impl {
      public:
        enum {
            XSIZE = TERRAIN_SIZE,
            YSIZE = TERRAIN_SIZE,
            HEIGHTMAP_XSIZE = XSIZE + 1,
            HEIGHTMAP_YSIZE = YSIZE + 1,
            HEIGHTMAP_RAW_XSIZE = XSIZE + 3,
            HEIGHTMAP_RAW_YSIZE = YSIZE + 3,
            ATTRMAP_XSIZE = XSIZE * 2,
            ATTRMAP_YSIZE = YSIZE * 2,
            ATTRMAP_ZCOUNT = 8,
            TILEMAP_XSIZE = XSIZE * 2,
            TILEMAP_YSIZE = YSIZE * 2,
            TILEMAP_RAW_XSIZE = XSIZE * 2 + 2,
            TILEMAP_RAW_YSIZE = YSIZE * 2 + 2,
            WATERMAP_XSIZE = XSIZE,
            WATERMAP_YSIZE = YSIZE,
            NORMALMAP_XSIZE = XSIZE + 1,
            NORMALMAP_YSIZE = YSIZE + 1,
            SHADOWMAP_XSIZE = XSIZE * 2,
            SHADOWMAP_YSIZE = YSIZE * 2,
            SPLATALPHA_RAW_XSIZE = XSIZE * 2 + 2,
            SPLATALPHA_RAW_YSIZE = YSIZE * 2 + 2,
            SPLATALPHA_ZCOUNT = 7,
            HEIGHT_TILE_XRATIO = TILEMAP_XSIZE / XSIZE,
            HEIGHT_TILE_YRATIO = TILEMAP_XSIZE / XSIZE,

            PATCH_XSIZE = TERRAIN_PATCHSIZE,
            PATCH_YSIZE = TERRAIN_PATCHSIZE,
            PATCH_TILE_XSIZE = PATCH_XSIZE * HEIGHT_TILE_XRATIO,
            PATCH_TILE_YSIZE = PATCH_YSIZE * HEIGHT_TILE_YRATIO,
            PATCH_XCOUNT = TERRAIN_PATCHCOUNT,
            PATCH_YCOUNT = TERRAIN_PATCHCOUNT,

            CELLSCALE = 200,
            CELLSCALE_IN_METER = CELLSCALE / 100,
            HALF_CELLSCALE = CELLSCALE / 2,
            HALF_CELLSCALE_IN_METER = HALF_CELLSCALE / 100,
            TERRAIN_XSIZE = XSIZE * CELLSCALE,
            TERRAIN_YSIZE = YSIZE * CELLSCALE,
        };

        enum {
            MAX_WATER_NUM = 255,
        };

        enum {
            ATTRIBUTE_BLOCK = (1 << 0),
            ATTRIBUTE_WATER = (1 << 1),
            ATTRIBUTE_BANPK = (1 << 2),
        };

      public:
        static void set_texture_set(texture_set *texture_set);
        static auto get_texture_set() -> texture_set *; // Never fails.

      protected:
        static texture_set *texture_set_value_;

      public:
        terrain_impl();
        virtual ~terrain_impl();

        auto get_terrain_splat_patch() -> terrain_splat_patch & { return terrain_splat_patch_value_; }
        auto num_textures() -> uint32_t { return texture_set_value_->texture_count(); }
        auto texture(uint32_t texture_num) -> terrain_texture & { return texture_set_value_->texture(texture_num); }

        auto load_water_map(const char *water_map_name) -> bool;
        auto load_water_map_file(const char *water_map_name) -> bool;

        auto shadow_texture() -> gpu::texture * { return shadow_texture_; }

        auto shadow_map_color(float fx, float fy) -> uint32_t;

      protected:
        void initialize();
        virtual void clear();

        auto load_height_map(const char *file_name) -> bool;
        auto raw_load_tile_map(const char *file_name) -> bool;
        auto load_attr_map(const char *file_name) -> bool;
        inline auto height_map_value(short sx, short sy) -> uint16_t;

      protected:
        std::shared_ptr<gpu::texture> alpha_texture_[MAXTERRAINTEXTURES];
        uint16_t raw_height_map_[HEIGHTMAP_RAW_YSIZE * HEIGHTMAP_RAW_XSIZE];

        uint8_t tile_map_[TILEMAP_RAW_YSIZE * TILEMAP_RAW_XSIZE];
        uint8_t attr_map_[ATTRMAP_YSIZE * ATTRMAP_XSIZE];
        uint8_t water_map_[WATERMAP_YSIZE * WATERMAP_XSIZE];
        char normal_map_[NORMALMAP_YSIZE * NORMALMAP_XSIZE * 3];

        int32_t view_radius_;
        float height_scale_;

        terrain_splat_patch terrain_splat_patch_value_;

        uint8_t num_water_;
        int32_t water_height_[MAX_WATER_NUM + 1];

        gpu::texture *shadow_texture_;
        uint16_t shadow_map_[SHADOWMAP_YSIZE * SHADOWMAP_XSIZE]; // 16-bit R5G6B5.
    };

    inline auto terrain_impl::height_map_value(short sx, short sy) -> uint16_t {
        return raw_height_map_[(sy + 1) * HEIGHTMAP_RAW_XSIZE + sx + 1];
    }
}
