#include "area_terrain.h"

#include <metin2/core/timer.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>

#include "map_outdoor.h"

namespace metin2::inline world::inline terrain {
    dynamic_pool<terrain_area> terrain_area::pool_value;

    void terrain_area::destroy_system() {
        pool_value.destroy();
    }

    auto terrain_area::new_value() -> terrain_area * {
        return pool_value.alloc();
    }

    void terrain_area::delete_value(terrain_area *terrain) {
        terrain->clear();
        pool_value.free(terrain);
    }

    terrain_area::terrain_area() {
        memset(&alpha_texture_, 0, sizeof(alpha_texture_));
        marked_texture_.reset();
        initialize();
    }

    terrain_area::~terrain_area() {
        deallocate_marked_splats();
        raw_deallocate_splats();
        clear();
    }

    void terrain_area::set_map_out_door(map_outdoor *owner_outdoor_map) {
        owner_outdoor_map_ = owner_outdoor_map;
    }

    void terrain_area::clear() {
        deallocate_marked_splats();
        terrain_impl::clear();
        initialize();
    }

    auto terrain_area::initialize() -> bool {
        set_ready(false);
        name_value_ = "";
        x_ = y_ = 0xFFFF;
        ready_ = false;
        marked_ = false;

        for (uint8_t y = 0; y < PATCH_YCOUNT; ++y) {
            for (uint8_t x = 0; x < PATCH_XCOUNT; ++x) {
                terrain_patch_list_[y * PATCH_XCOUNT + x].clear();
            }
        }

        return true;
    }

    void terrain_area::load_mini_map_texture(const char *pch_mini_map_file_name) {
        uint32_t start = el_timer_get_m_sec();
        auto *image = (graphic_image *)get_resource_manager().resource_pointer(pch_mini_map_file_name);
        mini_map_graphic_image_instance_.set_image_pointer(image);

        if (!mini_map_graphic_image_instance_.texture_pointer()->is_empty()) {
            mini_map_texture_ = mini_map_graphic_image_instance_.texture_pointer()->gpu_texture();
            metin2::log::debug("Minimap texture loaded in {} ms", el_timer_get_m_sec() - start);
        } else {
            metin2::log::debug("Minimap texture missing");
            mini_map_texture_ = nullptr;
        }
    }

    void terrain_area::load_shadow_texture(const char *shadow_file_name) {
        uint32_t start = el_timer_get_m_sec();
        auto *image = (graphic_image *)get_resource_manager().resource_pointer(shadow_file_name);
        shadow_graphic_image_instance_.set_image_pointer(image);

        if (!shadow_graphic_image_instance_.texture_pointer()->is_empty()) {
            shadow_texture_ = shadow_graphic_image_instance_.texture_pointer()->gpu_texture();
        } else {
            metin2::log::error("Shadow texture is empty");
            shadow_texture_ = nullptr;
        }
        metin2::log::debug("Shadow texture loaded in {} ms", el_timer_get_m_sec() - start);
    }

    auto terrain_area::load_shadow_map(const char *file_name) -> bool {
        uint32_t start = el_timer_get_m_sec();
        metin2::log::debug("Loading shadow map {}", file_name);

        mapped_file file;
        const void *data;

        if (!get_file_manager().get(file, file_name, &data)) {
            metin2::log::error("Cannot open shadow map {}", file_name);
            return false;
        }

        uint32_t shadow_map_size = sizeof(uint16_t) * 256 * 256;

        if (file.size() != shadow_map_size) {
            metin2::log::error("Shadow map {}: wrong file size", file_name);
            return false;
        }

        memcpy(shadow_map_, data, shadow_map_size);

        metin2::log::debug("Shadow map loaded in {} ms", el_timer_get_m_sec() - start);
        return true;
    }

    void terrain_area::copy_setting_from_global_setting() {
        view_radius_ = owner_outdoor_map_->view_radius();
        height_scale_ = owner_outdoor_map_->height_scale();
    }

    auto terrain_area::get_normal(int ix, int iy, math::vector3 *normal) -> bool {
        int32_t map_width = XSIZE * CELLSCALE;
        int32_t map_height = YSIZE * CELLSCALE;
        while (ix < 0) {
            ix += map_width;
        }

        while (iy < 0) {
            iy += map_height;
        }

        while (ix > map_width) {
            ix -= map_width;
        }

        while (iy > map_height) {
            iy -= map_height;
        }

        ix /= CELLSCALE;
        iy /= CELLSCALE;

        char *n = (char *)&normal_map_[(iy * NORMALMAP_XSIZE + ix) * 3];
        normal->x = -((float)*n++) * 0.007874016f;
        normal->y = ((float)*n++) * 0.007874016f;
        normal->z = ((float)*n++) * 0.007874016f;

        return true;
    }

    // Returns the height of the terrain at the given world coordinate.
    auto terrain_area::height(int x, int y) -> float {
        x -= x_ * XSIZE * CELLSCALE;
        y -= y_ * YSIZE * CELLSCALE;

        if (x < 0 || y < 0 || x > XSIZE * CELLSCALE || y > XSIZE * CELLSCALE) {
            return 0.0f;
        }

        int32_t xdist;
        int32_t ydist;
        float xslope, yslope;

        float h1, h2, h3;
        int32_t x2, y2;
        float ooscale;

        // Distance from the top-left vertex of the tile.
        xdist = x % CELLSCALE;
        ydist = y % CELLSCALE;

        ooscale = 1.0f / ((float)CELLSCALE);
        x /= CELLSCALE;
        y /= CELLSCALE;

        x2 = x;
        y2 = y;
        h1 = (float)height_map_value(x2, y2) * height_scale_;

        x2 = x + 1;
        y2 = y + 1;

        h2 = (float)height_map_value(x2, y2) * height_scale_;

        // Left triangle.
        if (xdist <= ydist) {
            x2 = x;
            y2 = y + 1;

            h3 = (float)height_map_value(x2, y2) * height_scale_;

            xslope = (h2 - h3) * ooscale;
            yslope = (h3 - h1) * ooscale;

            return (h1 + (xdist * xslope + ydist * yslope));
        }

        // Right triangle.
        x2 = x + 1;
        y2 = y;

        h3 = (float)height_map_value(x2, y2) * height_scale_;

        xslope = (h3 - h1) * ooscale;
        yslope = (h2 - h3) * ooscale;

        return (h1 + (xdist * xslope + ydist * yslope));
    }

    void terrain_area::calculate_normal(int32_t x, int32_t y) {
        math::vector3 normal;

        normal.x = -height_scale_ * ((float)height_map_value((x - 1), y) - (float)height_map_value((x + 1), y));
        normal.y = -height_scale_ * ((float)height_map_value(x, (y - 1)) - (float)height_map_value(x, (y + 1)));

        normal.z = 2.0f * static_cast<float>(CELLSCALE);
        normal *= 127.0f / math::length(normal);

        int ix, iy, iz;
        PR_FLOAT_TO_INT(normal.x, ix);
        PR_FLOAT_TO_INT(normal.y, iy);
        PR_FLOAT_TO_INT(normal.z, iz);

        char *n = (char *)&normal_map_[(y * NORMALMAP_XSIZE + x) * 3];

        *n++ = (char)ix;
        *n++ = (char)iy;
        *n++ = (char)iz;
    }

    auto terrain_area::raw_load_tile_map(const char *file_name) -> bool {
        terrain_impl::raw_load_tile_map(file_name);
        uint32_t start = el_timer_get_m_sec();
        raw_allocate_splats();
        metin2::log::debug("Splats allocated in {} ms", el_timer_get_m_sec() - start);
        return true;
    }

    auto terrain_area::load_height_map(const char *file_name) -> bool {
        terrain_impl::load_height_map(file_name);

        uint32_t start = el_timer_get_m_sec();
        for (uint16_t y = 0; y < NORMALMAP_YSIZE; ++y) {
            for (uint16_t x = 0; x < NORMALMAP_XSIZE; ++x) {
                calculate_normal(x, y);
            }
        }

        metin2::log::debug("Height map normals calculated in {} ms", el_timer_get_m_sec() - start);
        return true;
    }

    auto terrain_area::load_attr_map(const char *file_name) -> bool {
        return terrain_impl::load_attr_map(file_name);
    }

    auto terrain_area::is_attr_on(uint16_t coord_x, uint16_t coord_y, uint8_t attr_flag) -> bool {
        if (coord_x >= ATTRMAP_XSIZE || coord_y >= ATTRMAP_YSIZE) {
            metin2::log::debug("Attribute cell ({}, {}) out of range (limit {} x {}), treated as off", coord_x, coord_y, ATTRMAP_XSIZE, ATTRMAP_YSIZE);
            return false;
        }

        uint8_t map_attr = attr_map_[coord_y * ATTRMAP_XSIZE + coord_x];

        if (attr_flag < 16) {
            return (map_attr & attr_flag) ? true : false;
        } else {
            if (attr_flag / 16 == map_attr / 16) {
                return true;
            } else {
                return false;
            }
        }
    }

    auto terrain_area::attr(uint16_t coord_x, uint16_t coord_y) -> uint8_t {
        if (coord_x >= ATTRMAP_XSIZE || coord_y >= ATTRMAP_YSIZE) {
            metin2::log::debug("Attribute cell ({}, {}) out of range (limit {} x {}), returning 0", coord_x, coord_y, ATTRMAP_XSIZE, ATTRMAP_YSIZE);
            return 0;
        }

        return attr_map_[coord_y * ATTRMAP_XSIZE + coord_x];
    }

    void terrain_area::water_height(uint8_t water_num, int32_t *pl_water_height) {
        if (water_num > num_water_) {
            metin2::log::debug("Water {} out of range ({} waters)", water_num, num_water_);
            return;
        }
        *pl_water_height = water_height_[water_num];
    }

    auto terrain_area::water_height(uint16_t coord_x, uint16_t coord_y, int32_t *pl_water_height) -> bool {
        uint8_t water_num = *(water_map_ + (coord_y * WATERMAP_XSIZE) + coord_x);
        if (water_num > num_water_) {
            metin2::log::debug("No water height at ({}, {}) ({} waters)", coord_x, coord_y, num_water_);
            return false;
        }
        *pl_water_height = water_height_[water_num] / 2;
        return true;
    }

    void terrain_area::raw_deallocate_splats() {
        for (uint32_t i = 1; i < get_texture_set()->texture_count(); ++i) {
            terain_splat &splat = terrain_splat_patch_value_.splats[i];

            alpha_texture_[i].reset();
            splat.gpu_texture = nullptr;
        }

        memset(&terrain_splat_patch_value_, 0, sizeof(terrain_splat_patch_value_));
    }

    void terrain_area::raw_allocate_splats() {
        raw_deallocate_splats();
        uint32_t tex_count = get_texture_set()->texture_count();

        terrain_splat_patch_value_.needs_update = true;

        for (uint32_t t = 0; t < tex_count; ++t) {
            terrain_splat_patch_value_.splats[t].needs_update = 1;
        }

        raw_count_tiles();
        raw_generate_splat();

        terrain_splat_patch_value_.needs_update = false;
    }

    void terrain_area::raw_count_tiles() {
        for (int32_t y = 0; y < TILEMAP_RAW_YSIZE; ++y) {
            int32_t patch_index_y = std::min(std::max((y - 1) / PATCH_TILE_YSIZE, 0), PATCH_YCOUNT - 1);
            for (int32_t x = 0; x < TILEMAP_RAW_XSIZE; ++x) {
                int32_t patch_index_x = std::min(std::max((x - 1) / (PATCH_TILE_XSIZE), 0), PATCH_XCOUNT - 1);
                uint8_t tilenum = tile_map_[y * TILEMAP_RAW_XSIZE + x];

                ++terrain_splat_patch_value_.patch_tile_count[patch_index_y * PATCH_XCOUNT + patch_index_x][tilenum];

                if (0 == y % PATCH_TILE_YSIZE && 0 != y && (TILEMAP_RAW_YSIZE - 2) != y) {
                    ++terrain_splat_patch_value_.patch_tile_count[std::min(PATCH_YCOUNT - 1, patch_index_y + 1) * PATCH_XCOUNT + patch_index_x][tilenum];
                    if (0 == x % PATCH_TILE_XSIZE && 0 != x && (TILEMAP_RAW_XSIZE - 2) != x) {
                        ++terrain_splat_patch_value_.patch_tile_count[patch_index_y * PATCH_XCOUNT + std::min(PATCH_XCOUNT - 1, patch_index_x + 1)][tilenum];
                        ++terrain_splat_patch_value_.patch_tile_count[std::min(PATCH_YCOUNT - 1, patch_index_y + 1) * PATCH_XCOUNT + std::min(PATCH_XCOUNT - 1, patch_index_x + 1)][tilenum];
                    } else if (1 == x % PATCH_TILE_XSIZE && (TILEMAP_RAW_XSIZE - 1) != x && 1 != x) {
                        ++terrain_splat_patch_value_.patch_tile_count[patch_index_y * PATCH_XCOUNT + std::max(0, patch_index_x - 1)][tilenum];
                        ++terrain_splat_patch_value_.patch_tile_count[std::min(PATCH_YCOUNT - 1, patch_index_y + 1) * PATCH_XCOUNT + std::max(0, patch_index_x - 1)][tilenum];
                    }
                } else if (1 == y % PATCH_TILE_YSIZE && (TILEMAP_RAW_YSIZE - 1) != y && 1 != y) {
                    ++terrain_splat_patch_value_.patch_tile_count[std::max(0, patch_index_y - 1) * PATCH_XCOUNT + patch_index_x][tilenum];
                    if (0 == x % PATCH_TILE_XSIZE && 0 != x && (TILEMAP_RAW_XSIZE - 2) != x) {
                        ++terrain_splat_patch_value_.patch_tile_count[patch_index_y * PATCH_XCOUNT + std::min(PATCH_XCOUNT - 1, patch_index_x + 1)][tilenum];
                        ++terrain_splat_patch_value_.patch_tile_count[std::max(0, patch_index_y - 1) * PATCH_XCOUNT + std::min(PATCH_XCOUNT - 1, patch_index_x + 1)][tilenum];
                    } else if (1 == x % PATCH_TILE_XSIZE && (TILEMAP_RAW_XSIZE - 1) != x && 1 != x) {
                        ++terrain_splat_patch_value_.patch_tile_count[patch_index_y * PATCH_XCOUNT + std::max(0, patch_index_x - 1)][tilenum];
                        ++terrain_splat_patch_value_.patch_tile_count[std::max(0, patch_index_y - 1) * PATCH_XCOUNT + std::max(0, patch_index_x - 1)][tilenum];
                    }
                } else {
                    if (0 == x % PATCH_TILE_XSIZE && 0 != x && (TILEMAP_RAW_XSIZE - 2) != x) {
                        ++terrain_splat_patch_value_.patch_tile_count[patch_index_y * PATCH_XCOUNT + std::min(PATCH_XCOUNT - 1, patch_index_x + 1)][tilenum];
                    } else if (1 == x % PATCH_TILE_XSIZE && (TILEMAP_RAW_XSIZE - 1) != x && 1 != x) {
                        ++terrain_splat_patch_value_.patch_tile_count[patch_index_y * PATCH_XCOUNT + std::max(0, patch_index_x - 1)][tilenum];
                    }
                }

                ++terrain_splat_patch_value_.tile_count[tilenum];
            }
        }
    }

    void terrain_area::raw_generate_splat() {
        if (!terrain_splat_patch_value_.needs_update) {
            return;
        }

        terrain_splat_patch_value_.needs_update = false;

        uint8_t alpha_map[SPLATALPHA_RAW_XSIZE * SPLATALPHA_RAW_YSIZE];
        uint8_t *aptr;

        for (uint32_t i = 1; i < get_texture_set()->texture_count(); ++i) {
            terain_splat &splat = terrain_splat_patch_value_.splats[i];

            if (splat.needs_update) {
                if (terrain_splat_patch_value_.tile_count[i] > 0) {
                    if (splat.active) // An existing alpha map needs to be updated.
                    {
                        alpha_texture_[i].reset();
                        splat.gpu_texture = nullptr;
                    }

                    splat.active = 1;
                    splat.needs_update = 0;

                    aptr = alpha_map;

                    for (int32_t y = 0; y < SPLATALPHA_RAW_YSIZE; ++y) {
                        for (int32_t x = 0; x < SPLATALPHA_RAW_XSIZE; ++x) {
                            int32_t tile_map_offset = y * TILEMAP_RAW_XSIZE + x;

                            uint8_t tile_num = tile_map_[tile_map_offset];
                            if (tile_num == i) {
                                *aptr = 0xFF;
                            } else if (tile_num > i) {
                                uint8_t tile_tl, tile_tr, tile_bl, tile_br, tile_t, tile_b, tile_l, tile_r;

                                if (x > 0 && y > 0) {
                                    tile_tl = tile_map_[tile_map_offset - TILEMAP_RAW_YSIZE - 1];
                                } else {
                                    tile_tl = 0;
                                }

                                if (x < (SPLATALPHA_RAW_XSIZE - 1) && y > 0) {
                                    tile_tr = tile_map_[tile_map_offset - TILEMAP_RAW_YSIZE + 1];
                                } else {
                                    tile_tr = 0;
                                }

                                if (x > 0 && y < (SPLATALPHA_RAW_YSIZE - 1)) {
                                    tile_bl = tile_map_[tile_map_offset + TILEMAP_RAW_YSIZE - 1];
                                } else {
                                    tile_bl = 0;
                                }

                                if (x < (SPLATALPHA_RAW_XSIZE - 1) && y < (SPLATALPHA_RAW_YSIZE - 1)) {
                                    tile_br = tile_map_[tile_map_offset + TILEMAP_RAW_YSIZE + 1];
                                } else {
                                    tile_br = 0;
                                }

                                if (y > 0) {
                                    tile_t = tile_map_[tile_map_offset - TILEMAP_RAW_YSIZE];
                                } else {
                                    tile_t = 0;
                                }

                                if (y < (SPLATALPHA_RAW_YSIZE - 1)) {
                                    tile_b = tile_map_[tile_map_offset + TILEMAP_RAW_YSIZE];
                                } else {
                                    tile_b = 0;
                                }

                                if (x > 0) {
                                    tile_l = tile_map_[tile_map_offset - 1];
                                } else {
                                    tile_l = 0;
                                }

                                if (x < (SPLATALPHA_RAW_XSIZE - 1)) {
                                    tile_r = tile_map_[tile_map_offset + 1];
                                } else {
                                    tile_r = 0;
                                }

                                if (tile_tl == i || tile_tr == i || tile_bl == i || tile_br == i ||
                                    tile_t == i || tile_b == i || tile_l == i || tile_r == i) {
                                    *aptr = 0xFF;
                                } else {
                                    *aptr = 0x00;
                                }
                            } else {
                                *aptr = 0x00;
                            }

                            ++aptr;
                        }
                    }

                    splat.gpu_texture = add_texture32(i, alpha_map, SPLATALPHA_RAW_XSIZE, SPLATALPHA_RAW_YSIZE);
                } else {
                    if (splat.active) {
                        alpha_texture_[i].reset();
                        splat.gpu_texture = nullptr;
                    }

                    splat.needs_update = 0;
                    splat.active = 0;
                }
            }
        }
    }

    auto terrain_area::add_texture32(uint8_t image_num, uint8_t *image, int32_t texture_width, int32_t texture_height) -> gpu::texture * {
        assert(!alpha_texture_[image_num]);

        alpha_texture_[image_num].reset();

        gpu::format format;

        if (support_dxt_) {
            format = gpu::format::A8R8G8B8;
        } else {
            format = gpu::format::A4R4G4B4;
        }

        bool resized_and_success = false;

        uint32_t new_width = 256;
        uint32_t new_height = 256;
        std::shared_ptr<gpu::texture> tex = device_->create_texture(new_width, new_height, 5, 0, format);
        if (!tex) {
            metin2::log::error("Cannot create 32-bit texture");
            return nullptr;
        }

        uint8_t resize_image[256 * 256];
        {
            uint8_t *dst_pixel = resize_image;
            uint8_t *src_pixel;
            uint8_t *cur_line = image;
            for (uint32_t y = 0; y < 256; ++y, cur_line += 258) {
                for (uint32_t x = 0; x < 256; ++x) {
                    src_pixel = cur_line + x;
                    *dst_pixel++ =
                        (((src_pixel[0] + src_pixel[1] + src_pixel[2] +
                              src_pixel[258] + src_pixel[260] +
                              src_pixel[258 * 2] + src_pixel[258 * 2 + 1] + src_pixel[258 * 2 + 2]) >>
                             3) +
                            src_pixel[259]) >>
                        1;
                }
            }

            gpu::locked_rect locked;
            if (!tex->lock(0, locked)) {
                return nullptr;
            }

            if (support_dxt_) {
                put_image32(resize_image, locked.bits, 256, static_cast<int32_t>(locked.pitch), 256, 256, resized_and_success);
            } else {
                put_image16(resize_image, locked.bits, 256, static_cast<int32_t>(locked.pitch), 256, 256, resized_and_success);
            }

            tex->unlock(0);
        }

        uint8_t resize_image2[128 * 128];

        uint8_t *src_buffer = resize_image;
        uint8_t *dst_buffer = resize_image2;

        uint32_t src_size = 256;

        for (uint32_t mip_map_level = 1; mip_map_level != tex->level_count(); ++mip_map_level) {
            uint32_t dst_size = src_size >> 1;

            uint8_t *dst_pixel = dst_buffer;
            uint8_t *src_pixel;
            uint8_t *cur_line = src_buffer;
            for (uint32_t y = 0; y != src_size; y += 2, cur_line += src_size * 2) {
                for (uint32_t x = 0; x != src_size; x += 2) {
                    src_pixel = cur_line + x;
                    *dst_pixel++ = (src_pixel[0] + src_pixel[1] + src_pixel[src_size + 0] + src_pixel[src_size + 1]) >> 2;
                }
            }

            gpu::locked_rect locked;

            if (!tex->lock(mip_map_level, locked)) {
                continue;
            }

            if (support_dxt_) {
                put_image32(dst_buffer, locked.bits, dst_size, static_cast<int32_t>(locked.pitch), dst_size, dst_size, resized_and_success);
            } else {
                put_image16(dst_buffer, locked.bits, dst_size, static_cast<int32_t>(locked.pitch), dst_size, dst_size, resized_and_success);
            }

            tex->unlock(mip_map_level);

            std::swap(src_buffer, dst_buffer);
            src_size = dst_size;
        }

        alpha_texture_[image_num] = tex;

        return tex.get();
    }

    void terrain_area::put_image32(uint8_t *src, uint8_t *dst, int32_t src_pitch, int32_t dst_pitch, int32_t texturewidth, int32_t textureheight, bool resize) {
        for (int y = 0; y < textureheight; ++y) {
            for (int x = 0; x < texturewidth; ++x) {
                uint32_t packed_pixel = src[x] << 24;
                *((uint32_t *)(dst + x * 4)) = packed_pixel;
            }

            dst += dst_pitch;
            src += src_pitch;
        }
    }

    void terrain_area::put_image16(uint8_t *src, uint8_t *dst, int32_t src_pitch, int32_t dst_pitch, int32_t texturewidth, int32_t textureheight, bool resize) {
        for (int y = 0; y < textureheight; ++y) {
            for (int x = 0; x < texturewidth; ++x) {
                uint16_t packed_pixel = src[x] << 8;
                // Deliberately unmasked; even one & was too costly here.
                *((uint16_t *)(dst + x * 2)) = packed_pixel;
            }

            dst += dst_pitch;
            src += src_pitch;
        }
    }

    void terrain_area::set_coordinate(uint16_t coord_x, uint16_t coord_y) {
        x_ = coord_x;
        y_ = coord_y;
    }

    void terrain_area::calculate_terrain_patch() {
        for (uint8_t patch_num_y = 0; patch_num_y < PATCH_YCOUNT; ++patch_num_y) {
            for (uint8_t patch_num_x = 0; patch_num_x < PATCH_XCOUNT; ++patch_num_x) {
                calculate_terrain_patch_impl(patch_num_x, patch_num_y);
            }
        }
    }

    auto terrain_area::terrain_patch_ptr(uint8_t patch_num_x, uint8_t patch_num_y) -> terrain_patch * {
        if (patch_num_x >= PATCH_XCOUNT || patch_num_y >= PATCH_YCOUNT) {
            return nullptr;
        }

        return &terrain_patch_list_[patch_num_y * PATCH_XCOUNT + patch_num_x];
    }

    void terrain_area::calculate_terrain_patch_impl(uint8_t patch_num_x, uint8_t patch_num_y) {
        uint32_t patch_num = patch_num_y * PATCH_XCOUNT + patch_num_x;

        terrain_patch &terrain_patch = terrain_patch_list_[patch_num];
        if (!terrain_patch.need_update()) {
            return;
        }

        const float opaque_water_depth = owner_outdoor_map_->opaque_water_depth();
        const float oo_opaque_water_depth = 1.0f / opaque_water_depth;
        const float transparent_water_depth = 0.8f * opaque_water_depth;

        terrain_patch.clear();

        hardware_transform_patch_source_vertex src_terrain_vertex[terrain_patch::TERRAIN_VERTEX_COUNT];
        water_vertex src_water_vertex[PATCH_XSIZE * PATCH_YSIZE * 6];

        uint32_t normal_width = terrain_impl::NORMALMAP_XSIZE * 3;
        uint32_t start_x = patch_num_x * PATCH_XSIZE;
        uint32_t start_y = patch_num_y * PATCH_YSIZE;

        uint16_t *orig_raw_height_ptr = raw_height_map_ + ((start_y + 1) * HEIGHTMAP_RAW_XSIZE) + start_x + 1;
        char *orig_normal_ptr = normal_map_ + (start_y * normal_width) + start_x * 3;
        uint8_t *orig_water_ptr = water_map_ + (start_y * WATERMAP_XSIZE) + start_x;

        float f_x, f_y, orig_x;
        orig_x = f_x = (float)(x_ * XSIZE * CELLSCALE) + (float)(start_x * CELLSCALE);
        f_y = (float)(y_ * YSIZE * CELLSCALE) + (float)(start_y * CELLSCALE);

        terrain_patch.set_min_x(f_x);
        terrain_patch.set_max_x(f_x + (float)(PATCH_XSIZE * CELLSCALE));
        terrain_patch.set_min_y(f_y);
        terrain_patch.set_max_y(f_y + (float)(PATCH_YSIZE * CELLSCALE));

        float min_z = 999999.0f;
        float max_z = -999999.0f;
        uint16_t num_plain_type = 0;
        uint16_t num_hill_type = 0;
        uint16_t num_cliff_type = 0;

        bool water_exist = false;

        water_vertex *water_vertex = src_water_vertex;
        uint32_t water_vertex_count = 0;

        hardware_transform_patch_source_vertex *terrain_vertex = src_terrain_vertex;
        [[maybe_unused]] uint32_t terrain_vertex_count = 0; // Checked by the assert below.

        math::vector3 normal;
        math::vector3 position;
        for (uint32_t dw_y = start_y; dw_y <= start_y + PATCH_YSIZE; ++dw_y) {
            uint16_t *raw_height = orig_raw_height_ptr;
            char *pch_normal = orig_normal_ptr;
            uint8_t *water = orig_water_ptr;
            f_x = orig_x;

            for (uint32_t dw_x = start_x; dw_x <= start_x + PATCH_XSIZE; ++dw_x) {
                uint16_t hgt = (*raw_height++);

                normal.x = -(*pch_normal++) * 0.0078740f;
                normal.y = (*pch_normal++) * 0.0078740f;
                normal.z = (*pch_normal++) * 0.0078740f;

                position.x = +f_x;
                position.y = -f_y;
                position.z = (float)(hgt)*height_scale_;
                terrain_vertex->position_value = position;
                terrain_vertex->normal_value = normal;

                if (0.5f > normal.z) { // Slope steeper than 60 degrees: cliff.
                    ++num_cliff_type;
                } else if (0.8660254f > normal.z) { // Slope steeper than 30 degrees: hill.
                    ++num_hill_type;
                } else { // Anything else: plain.
                    ++num_plain_type;
                }

                if (position.z > max_z) {
                    max_z = position.z;
                }
                if (position.z < min_z) {
                    min_z = position.z;
                }

                if (XSIZE > dw_x && YSIZE > dw_y &&
                    (start_x + PATCH_XSIZE) != dw_x && (start_y + PATCH_YSIZE) != dw_y) {
                    uint8_t num_water = (*water++);

                    if (num_water != 0xFF) {
                        int32_t water_height = water_height_[num_water];
                        if (-1 != water_height) {
                            auto water_terrain_height_difference0 = (float)(water_height - (int32_t)hgt);
                            if (water_terrain_height_difference0 >= transparent_water_depth) {
                                water_terrain_height_difference0 = transparent_water_depth;
                            }
                            if (water_terrain_height_difference0 <= 0.0f) {
                                water_terrain_height_difference0 = 0.0f;
                            }

                            auto water_terrain_height_difference1 = (float)(water_height - (int32_t)(*(raw_height + terrain_impl::HEIGHTMAP_RAW_XSIZE - 1)));
                            if (water_terrain_height_difference1 >= transparent_water_depth) {
                                water_terrain_height_difference1 = transparent_water_depth;
                            }
                            if (water_terrain_height_difference1 <= 0.0f) {
                                water_terrain_height_difference1 = 0.0f;
                            }

                            auto water_terrain_height_difference2 = (float)(water_height - (int32_t)(*(raw_height)));
                            if (water_terrain_height_difference2 >= transparent_water_depth) {
                                water_terrain_height_difference2 = transparent_water_depth;
                            }
                            if (water_terrain_height_difference2 <= 0.0f) {
                                water_terrain_height_difference2 = 0.0f;
                            }

                            auto water_terrain_height_difference3 = (float)(water_height - (int32_t)(*(raw_height + terrain_impl::HEIGHTMAP_RAW_XSIZE)));
                            if (water_terrain_height_difference3 >= transparent_water_depth) {
                                water_terrain_height_difference3 = transparent_water_depth;
                            }
                            if (water_terrain_height_difference3 <= 0.0f) {
                                water_terrain_height_difference3 = 0.0f;
                            }

                            uint32_t alpha0;
                            uint32_t alpha1;
                            uint32_t alpha2;
                            uint32_t alpha3;

                            PR_FLOAT_TO_INT(water_terrain_height_difference0 * oo_opaque_water_depth * 255.0f, alpha0);
                            PR_FLOAT_TO_INT(water_terrain_height_difference1 * oo_opaque_water_depth * 255.0f, alpha1);
                            PR_FLOAT_TO_INT(water_terrain_height_difference2 * oo_opaque_water_depth * 255.0f, alpha2);
                            PR_FLOAT_TO_INT(water_terrain_height_difference3 * oo_opaque_water_depth * 255.0f, alpha3);

                            uint32_t alpha_key = (alpha0 << 24) | (alpha1 << 16) | (alpha2 << 8) | alpha3;
                            if (alpha_key != 0) {
                                assert(water_vertex < src_water_vertex + PATCH_XSIZE * PATCH_YSIZE * 6);
                                water_vertex->x = f_x;
                                water_vertex->y = -f_y;
                                water_vertex->z = (float)water_height * height_scale_;
                                water_vertex->diffuse_value = ((alpha0 << 24) & 0xFF000000) | 0x000000FF;
                                water_vertex++;

                                water_vertex->x = f_x;
                                water_vertex->y = -f_y - float(CELLSCALE);
                                water_vertex->z = (float)water_height * height_scale_;
                                water_vertex->diffuse_value = ((alpha1 << 24) & 0xFF000000) | 0x00FFFFFF;
                                water_vertex++;

                                water_vertex->x = f_x + float(CELLSCALE);
                                water_vertex->y = -f_y;
                                water_vertex->z = (float)water_height * height_scale_;
                                water_vertex->diffuse_value = ((alpha2 << 24) & 0xFF000000) | 0x00FFFFFF;
                                water_vertex++;

                                water_vertex->x = f_x + float(CELLSCALE);
                                water_vertex->y = -f_y;
                                water_vertex->z = (float)water_height * height_scale_;
                                water_vertex->diffuse_value = ((alpha2 << 24) & 0xFF000000) | 0x00FFFFFF;
                                water_vertex++;

                                water_vertex->x = f_x;
                                water_vertex->y = -f_y - float(CELLSCALE);
                                water_vertex->z = (float)water_height * height_scale_;
                                water_vertex->diffuse_value = ((alpha1 << 24) & 0xFF000000) | 0x00FFFFFF;
                                water_vertex++;

                                water_vertex->x = f_x + float(CELLSCALE);
                                water_vertex->y = -f_y - float(CELLSCALE);
                                water_vertex->z = (float)water_height * height_scale_;
                                water_vertex->diffuse_value = ((alpha3 << 24) & 0xFF0000FF) | 0x00FFFFFF;
                                water_vertex++;

                                water_vertex_count += 6;
                                water_exist = true;
                            }
                        }
                    }
                }

                ++terrain_vertex;
                ++terrain_vertex_count;
                f_x += float(CELLSCALE);
            }

            orig_raw_height_ptr += terrain_impl::HEIGHTMAP_RAW_XSIZE;
            orig_normal_ptr += normal_width;
            orig_water_ptr += terrain_impl::XSIZE;
            f_y += float(CELLSCALE);
        }

        if (num_plain_type <= std::max(num_hill_type, num_cliff_type)) {
            if (num_cliff_type <= num_hill_type) {
                terrain_patch.set_type(terrain_patch::PATCH_TYPE_HILL);
            } else {
                terrain_patch.set_type(terrain_patch::PATCH_TYPE_CLIFF);
            }
        }

        terrain_patch.set_water_exist(water_exist);

        terrain_patch.set_min_z(min_z);
        terrain_patch.set_max_z(max_z);

        assert((PATCH_XSIZE + 1) * (PATCH_YSIZE + 1) == terrain_vertex_count);
        terrain_patch.build_terrain_vertex_buffer(src_terrain_vertex);

        if (water_exist) {
            terrain_patch.build_water_vertex_buffer(src_water_vertex, water_vertex_count);
        }

        terrain_patch.need_update(false);
    }

    void terrain_area::allocate_marked_splats(uint8_t *alpha_map) {
        terain_splat &attr_splat = marked_splat_patch_.splats[0];

        marked_texture_ = device_->create_texture(ATTRMAP_XSIZE, ATTRMAP_YSIZE, 1, 0, gpu::format::A8R8G8B8);
        if (!marked_texture_) {
            return;
        }

        gpu::locked_rect locked;
        if (marked_texture_->lock(0, locked)) {
            put_image32(alpha_map, locked.bits, ATTRMAP_XSIZE, static_cast<int32_t>(locked.pitch), ATTRMAP_XSIZE, ATTRMAP_YSIZE);
            marked_texture_->unlock(0);
        }

        attr_splat.gpu_texture = marked_texture_.get();
        marked_ = true;
    }

    void terrain_area::deallocate_marked_splats() {
        terain_splat &splat = marked_splat_patch_.splats[0];

        splat.gpu_texture = nullptr;
        marked_texture_.reset();
        marked_ = false;

        memset(&marked_splat_patch_, 0, sizeof(marked_splat_patch_));
    }
}
