#include "map_outdoor.h"

#include "area_terrain.h"
#include "terrain_patch.h"

#include <metin2/core/timer.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/graphics/state_manager.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

// Grass: tufts of two crossed quads on the half cells (100 units) of grass and field tiles, built per
// terrain patch when the patch comes near the camera. Placement is a hash of the cell, so the grass
// is the same on every visit; the colour is the tile's average colour shaded by the baked terrain
// shadow, so it matches the ground it grows on.
namespace metin2::inline world::inline terrain {
    namespace {
        constexpr float GRASS_DISTANCE = 4000.0f; // tufts shrink away from 60% of this
        constexpr float PATCH_HALF_EXTENT = 1600.0f;
        constexpr int BUILDS_PER_FRAME = 2;
        constexpr uint32_t RELEASE_AFTER_FRAMES = 300;
        constexpr uint32_t MAX_TUFTS_PER_PATCH = 32 * 32 * 3;
        constexpr uint32_t MAX_QUADS_PER_PATCH = MAX_TUFTS_PER_PATCH * 2;
        const char *const GRASS_TEXTURE = "special:grass_tuft.tga";

        auto hash32(uint32_t a, uint32_t b, uint32_t c, uint32_t d) -> uint32_t {
            uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
            h ^= (c + 0x165667B1u) * 0xC2B2AE3Du;
            h ^= (d + 0x27D4EB2Fu) * 0x9E3779B1u;
            h ^= h >> 15;
            h *= 0x85EBCA77u;
            h ^= h >> 13;
            h *= 0xC2B2AE3Du;
            h ^= h >> 16;
            return h;
        }

        // A uniform value in [0, 1) from the next eight bits of a hash.
        struct hash_stream {
            uint32_t h;
            uint32_t salt = 0;
            auto next() -> float {
                h = hash32(h, ++salt, 0x51ED27u, 0xA341316Cu);
                return static_cast<float>(h >> 8) / 16777216.0f;
            }
        };

        auto channel(uint32_t color, int shift) -> float {
            return static_cast<float>((color >> shift) & 255) / 255.0f;
        }

        auto pack_color(float r, float g, float b, float a) -> uint32_t {
            auto c = [](float v) -> uint32_t { return static_cast<uint32_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
            return (c(a) << 24) | (c(r) << 16) | (c(g) << 8) | c(b);
        }
    }

    void terrain_area::build_grass_patch(uint8_t patch_num_x, uint8_t patch_num_y, int density, std::vector<grass_vertex> &out) {
        out.clear();
        if (density <= 0 || !texture_set_value_) {
            return;
        }
        const int start_hx = patch_num_x * PATCH_TILE_XSIZE;
        const int start_hy = patch_num_y * PATCH_TILE_YSIZE;
        constexpr auto TERRAIN_EXTENT = static_cast<float>(static_cast<int>(XSIZE) * static_cast<int>(CELLSCALE));
        constexpr auto HALF_CELL = static_cast<float>(static_cast<int>(HALF_CELLSCALE));
        const float base_x = static_cast<float>(x_) * TERRAIN_EXTENT;
        const float base_y = static_cast<float>(y_) * TERRAIN_EXTENT;
        const auto is_grass = [this](int hx, int hy) -> bool {
            const uint8_t tile = tile_map_[(hy + 1) * TILEMAP_RAW_XSIZE + hx + 1];
            return tile != 0 && tile < texture_set_value_->texture_count() && texture_set_value_->texture(tile).grass;
        };

        for (int hy = start_hy; hy < start_hy + PATCH_TILE_YSIZE; ++hy) {
            for (int hx = start_hx; hx < start_hx + PATCH_TILE_XSIZE; ++hx) {
                if (!is_grass(hx, hy)) {
                    continue;
                }
                if (attr_map_[hy * ATTRMAP_XSIZE + hx] & (ATTRIBUTE_BLOCK | ATTRIBUTE_WATER)) {
                    continue;
                }
                const uint8_t water = water_map_[(hy / 2) * WATERMAP_XSIZE + hx / 2];
                if (water != 0xFF && water <= num_water_ && water_height_[water] != -1 &&
                    water_height_[water] >= static_cast<int32_t>(height_map_value(static_cast<short>(hx / 2), static_cast<short>(hy / 2)))) {
                    continue;
                }

                // Fewer tufts where the grass meets another tile, so its edge is not a hard line.
                const bool edge = (hx > 0 && !is_grass(hx - 1, hy)) || (hx + 1 < TILEMAP_XSIZE && !is_grass(hx + 1, hy)) ||
                                  (hy > 0 && !is_grass(hx, hy - 1)) || (hy + 1 < TILEMAP_YSIZE && !is_grass(hx, hy + 1));

                const uint8_t tile = tile_map_[(hy + 1) * TILEMAP_RAW_XSIZE + hx + 1];
                const uint32_t tile_color = texture_set_value_->texture(tile).average_color;
                const uint16_t shadow = shadow_map_[hy * SHADOWMAP_XSIZE + hx];
                const float shade = 0.35f + 0.65f * static_cast<float>((shadow >> 5) & 63) / 63.0f;

                const int tufts = density == 1 ? 1 : density;
                const float chance = (density == 1 ? 0.6f : 1.0f) * (edge ? 0.5f : 1.0f);
                hash_stream rng{hash32(x_, y_, static_cast<uint32_t>(hx), static_cast<uint32_t>(hy))};
                for (int k = 0; k < tufts; ++k) {
                    if (rng.next() >= chance) {
                        continue;
                    }
                    const float local_x = static_cast<float>(hx) * HALF_CELL + rng.next() * HALF_CELL;
                    const float local_y = static_cast<float>(hy) * HALF_CELL + rng.next() * HALF_CELL;
                    const float yaw = rng.next() * 3.14159265f;
                    const float height_value = 22.0f + 18.0f * rng.next();
                    const float width_value = 40.0f + 20.0f * rng.next();
                    const float variation = 0.9f + 0.2f * rng.next();

                    math::vector3 normal;
                    get_normal(static_cast<int>(local_x), static_cast<int>(local_y), &normal);
                    if (normal.z < 0.82f) {
                        continue;
                    }
                    const float world_x = base_x + local_x;
                    const float world_y = base_y + local_y;
                    // Slightly into the ground so a tuft on a slope does not float.
                    const float ground = height(static_cast<int>(world_x), static_cast<int>(world_y)) - 4.0f;

                    // A little lighter and greener than the tile, as blades catch more light than soil.
                    const float r = channel(tile_color, 16) * shade * variation * 1.30f;
                    const float g = channel(tile_color, 8) * shade * variation * 1.40f;
                    const float b = channel(tile_color, 0) * shade * variation * 1.10f;
                    const uint32_t color = pack_color(r, g, b, height_value / 128.0f);

                    for (int quad = 0; quad < 2; ++quad) {
                        const float angle = yaw + static_cast<float>(quad) * 1.5707963f;
                        const float dx = std::cos(angle) * width_value * 0.5f;
                        const float dy = std::sin(angle) * width_value * 0.5f;
                        const math::vector3 bottom_left(world_x - dx, -(world_y - dy), ground);
                        const math::vector3 bottom_right(world_x + dx, -(world_y + dy), ground);
                        const math::vector3 up(0.0f, 0.0f, height_value + 4.0f);
                        out.push_back({bottom_left, normal, color, 0.0f, 1.0f});
                        out.push_back({bottom_left + up, normal, color, 0.0f, 0.0f});
                        out.push_back({bottom_right, normal, color, 1.0f, 1.0f});
                        out.push_back({bottom_right + up, normal, color, 1.0f, 0.0f});
                    }
                }
            }
        }
    }

    void map_outdoor::set_grass_density(int density) {
        density = std::clamp(density, 0, 3);
        if (density == grass_density_) {
            return;
        }
        grass_density_ = density;
        ++grass_generation_; // every patch rebuilds with the new density
    }

    auto map_outdoor::grass_texture() -> gpu::texture * {
        if (!grass_resources_tried_) {
            grass_resources_tried_ = true;
            auto *image = static_cast<graphic_image *>(get_resource_manager().resource_pointer(GRASS_TEXTURE));
            if (image) {
                grass_image_instance_.set_image_pointer(image);
            }

            // The quads of one patch share this index pattern: 0 1 2, 2 1 3 per quad.
            if (grass_index_buffer_.create(static_cast<int>(MAX_QUADS_PER_PATCH * 6), gpu::format::INDEX16)) {
                uint16_t *indices;
                if (grass_index_buffer_.lock(reinterpret_cast<void **>(&indices))) {
                    for (uint32_t q = 0; q < MAX_QUADS_PER_PATCH; ++q) {
                        const auto base = static_cast<uint16_t>(q * 4);
                        uint16_t *i = indices + q * 6;
                        i[0] = base;
                        i[1] = static_cast<uint16_t>(base + 1);
                        i[2] = static_cast<uint16_t>(base + 2);
                        i[3] = static_cast<uint16_t>(base + 2);
                        i[4] = static_cast<uint16_t>(base + 1);
                        i[5] = static_cast<uint16_t>(base + 3);
                    }
                    grass_index_buffer_.unlock();
                }
            }
        }
        graphic_texture *texture = grass_image_instance_.texture_pointer();
        return texture ? texture->gpu_texture() : nullptr;
    }

    void map_outdoor::release_all_grass() {
        auto release = [](terrain_area *terrain) -> void {
            for (uint8_t py = 0; py < terrain_impl::PATCH_YCOUNT; ++py) {
                for (uint8_t px = 0; px < terrain_impl::PATCH_XCOUNT; ++px) {
                    if (terrain_patch *patch = terrain->terrain_patch_ptr(px, py)) {
                        patch->release_grass();
                    }
                }
            }
        };
        for (terrain_area *terrain : terrain_vector_) {
            release(terrain);
        }
        for (terrain_area *terrain : terrain_delete_vector_) {
            release(terrain);
        }
    }

    void map_outdoor::render_grass() {
        ++grass_frame_;
        if (grass_density_ <= 0 || !terrain_patch_proxy_list_) {
            return;
        }
        gpu::texture *texture = grass_texture();
        if (!texture || !grass_index_buffer_.gpu_buffer()) {
            return;
        }

        // Patches near the camera, nearest first (patch_vector_ is sorted and culled by render_terrain).
        static std::vector<grass_vertex> scratch;
        int builds = 0;
        std::vector<terrain_patch *> visible;
        for (const auto &[distance, proxy_index] : patch_vector_) {
            if (distance > GRASS_DISTANCE + PATCH_HALF_EXTENT) {
                break;
            }
            terrain_patch_proxy &proxy = terrain_patch_proxy_list_[proxy_index];
            if (!proxy.is_used() || proxy.patch_num() < 0) {
                continue;
            }
            terrain_area *terrain;
            if (!terrain_pointer(proxy.terrain_num(), &terrain)) {
                continue;
            }
            const auto patch_x = static_cast<uint8_t>(proxy.patch_num() % terrain_impl::PATCH_XCOUNT);
            const auto patch_y = static_cast<uint8_t>(proxy.patch_num() / terrain_impl::PATCH_XCOUNT);
            terrain_patch *patch = terrain->terrain_patch_ptr(patch_x, patch_y);
            if (!patch) {
                continue;
            }
            if (patch->grass_generation != grass_generation_) {
                if (builds >= BUILDS_PER_FRAME) {
                    continue;
                }
                ++builds;
                patch->release_grass();
                terrain->build_grass_patch(patch_x, patch_y, grass_density_, scratch);
                patch->grass_generation = grass_generation_;
                if (!scratch.empty() && patch->grass_vb.create(static_cast<int>(scratch.size()), gpu::fvf::XYZ | gpu::fvf::NORMAL | gpu::fvf::DIFFUSE | gpu::fvf::TEX1, gpu::usage::WRITE_ONLY)) {
                    void *dst;
                    if (patch->grass_vb.lock(&dst)) {
                        memcpy(dst, scratch.data(), scratch.size() * sizeof(grass_vertex));
                        patch->grass_vb.unlock();
                        patch->grass_quad_count = static_cast<uint32_t>(scratch.size() / 4);
                    }
                }
            }
            patch->grass_last_used_frame = grass_frame_;
            if (patch->grass_quad_count > 0) {
                visible.push_back(patch);
            }
        }

        if (!visible.empty()) {
            const float time = get_timer().current_second();
            const float wind = get_speed_tree_forest_direct_x8().wind_strength();
            const float distance = GRASS_DISTANCE;
            uint32_t time_bits, wind_bits, distance_bits;
            memcpy(&time_bits, &time, 4);
            memcpy(&wind_bits, &wind, 4);
            memcpy(&distance_bits, &distance, 4);

            // Lit like the ground: the vertex colour is the diffuse and ambient reflectance, and no
            // emissive term from the environment's material washes it towards grey.
            gpu::material previous_material;
            STATEMANAGER.get_material(&previous_material);
            gpu::material grass_material;
            grass_material.diffuse = math::color(1.0f, 1.0f, 1.0f, 1.0f);
            grass_material.ambient = math::color(1.0f, 1.0f, 1.0f, 1.0f);
            grass_material.specular = math::color(0.0f, 0.0f, 0.0f, 0.0f);
            grass_material.emissive = math::color(0.0f, 0.0f, 0.0f, 0.0f);
            STATEMANAGER.set_material(&grass_material);

            STATEMANAGER.save_render_state(gpu::state::EFFECT_MODE, 3);
            STATEMANAGER.save_render_state(gpu::state::EFFECT_TIME, time_bits);
            STATEMANAGER.save_render_state(gpu::state::EFFECT_PARAM0, wind_bits);
            STATEMANAGER.save_render_state(gpu::state::EFFECT_PARAM1, distance_bits);
            STATEMANAGER.save_render_state(gpu::state::LIGHTING, true);
            STATEMANAGER.save_render_state(gpu::state::COLOR_VERTEX, true);
            STATEMANAGER.save_render_state(gpu::state::DIFFUSE_MATERIAL_SOURCE, gpu::material_source::COLOR1);
            STATEMANAGER.save_render_state(gpu::state::AMBIENT_MATERIAL_SOURCE, gpu::material_source::COLOR1);
            STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);
            STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, true);
            STATEMANAGER.save_render_state(gpu::state::ALPHA_REF, 0x60);
            STATEMANAGER.save_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER);
            STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
            STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, true);

            STATEMANAGER.set_texture(0, texture);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
            // The vertex alpha is the tuft height for the vertex shader, not a transparency.
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX, 0);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::DISABLE);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);

            // The character shadow falls on the grass as it does on the ground.
            const bool character_shadow = draw_shadow_ && draw_chr_shadow_;
            if (character_shadow) {
                STATEMANAGER.save_transform(gpu::transform::TEXTURE1, &dynamic_shadow_);
                STATEMANAGER.set_texture(1, character_shadow_target_.texture());
                STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX, gpu::texcoord::CAMERA_SPACE_POSITION);
                STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);
                STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
                STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
                STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
                STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
                STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
                STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);
            } else {
                STATEMANAGER.set_texture(1, nullptr);
                STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
                STATEMANAGER.save_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
            }

            const math::matrix identity = math::matrix_identity();
            STATEMANAGER.set_transform(gpu::transform::WORLD, &identity);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::NORMAL | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.set_indices(grass_index_buffer_.gpu_buffer(), 0);
            for (terrain_patch *patch : visible) {
                STATEMANAGER.set_stream_source(0, patch->grass_vb.gpu_buffer(), sizeof(grass_vertex));
                STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, patch->grass_quad_count * 4, 0, patch->grass_quad_count * 2);
            }

            if (character_shadow) {
                STATEMANAGER.restore_transform(gpu::transform::TEXTURE1);
                STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX);
                STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);
                STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::COLOR_ARG1);
                STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::COLOR_ARG2);
                STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_U);
                STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ADDRESS_V);
            }
            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::COLOR_OP);
            STATEMANAGER.restore_texture_stage_state(1, gpu::stage_state::ALPHA_OP);
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_texture(0, nullptr);

            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ADDRESS_V);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ADDRESS_U);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_OP);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_OP);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG2);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG1);

            STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
            STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
            STATEMANAGER.restore_render_state(gpu::state::ALPHA_FUNC);
            STATEMANAGER.restore_render_state(gpu::state::ALPHA_REF);
            STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
            STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
            STATEMANAGER.restore_render_state(gpu::state::AMBIENT_MATERIAL_SOURCE);
            STATEMANAGER.restore_render_state(gpu::state::DIFFUSE_MATERIAL_SOURCE);
            STATEMANAGER.restore_render_state(gpu::state::COLOR_VERTEX);
            STATEMANAGER.restore_render_state(gpu::state::LIGHTING);
            STATEMANAGER.restore_render_state(gpu::state::EFFECT_PARAM1);
            STATEMANAGER.restore_render_state(gpu::state::EFFECT_PARAM0);
            STATEMANAGER.restore_render_state(gpu::state::EFFECT_TIME);
            STATEMANAGER.restore_render_state(gpu::state::EFFECT_MODE);
            STATEMANAGER.set_material(&previous_material);
        }

        // Patches that have not been drawn for a while give their grass back.
        if (grass_frame_ % 60 == 0) {
            for (uint8_t i = 0; i < AROUND_AREA_NUM; ++i) {
                terrain_area *terrain;
                if (!terrain_pointer(i, &terrain)) {
                    continue;
                }
                for (uint8_t py = 0; py < terrain_impl::PATCH_YCOUNT; ++py) {
                    for (uint8_t px = 0; px < terrain_impl::PATCH_XCOUNT; ++px) {
                        terrain_patch *patch = terrain->terrain_patch_ptr(px, py);
                        if (patch && patch->grass_generation != 0 && grass_frame_ - patch->grass_last_used_frame > RELEASE_AFTER_FRAMES) {
                            patch->release_grass();
                        }
                    }
                }
            }
        }
    }
}
