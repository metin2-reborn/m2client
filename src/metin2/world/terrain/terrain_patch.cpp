#include "terrain_patch.h"

#include <metin2/gpu/types.h>

namespace metin2::inline world::inline terrain {
    void terrain_patch::release_grass() {
        grass_vb.destroy();
        grass_quad_count = 0;
        grass_generation = 0;
    }

    void terrain_patch::clear() {
        release_grass();
        ht_.vb.destroy();

        water_vertex_buffer_.destroy();

        water_exist_ = false;
        need_update_ = true;

        water_pri_count_ = 0;
        type_value_ = PATCH_TYPE_PLAIN;

        min_x_ = max_x_ = min_y_ = max_y_ = min_z_ = max_z_ = 0.0f;
    }

    void terrain_patch::build_water_vertex_buffer(water_vertex *src_vertex, uint32_t water_vertex_count) {
        graphic_vertex_buffer &vb = water_vertex_buffer_;

        if (!vb.create(water_vertex_count, gpu::fvf::XYZ | gpu::fvf::DIFFUSE, gpu::usage::WRITE_ONLY)) {
            return;
        }

        water_vertex *dst_water_vertex;
        if (vb.lock((void **)&dst_water_vertex)) {
            uint32_t vb_size = sizeof(water_vertex) * water_vertex_count;
            memcpy(dst_water_vertex, src_vertex, vb_size);
            water_pri_count_ = water_vertex_count / 3;

            vb.unlock();
        }
    }

    void terrain_patch::build_terrain_vertex_buffer(hardware_transform_patch_source_vertex *src_vertex) {
        graphic_vertex_buffer &vb = ht_.vb;
        if (!vb.create(TERRAIN_VERTEX_COUNT, gpu::fvf::XYZ | gpu::fvf::NORMAL, gpu::usage::WRITE_ONLY)) {
            return;
        }

        hardware_transform_patch_source_vertex *dst_vertex;
        if (vb.lock((void **)&dst_vertex)) {
            uint32_t vb_size = sizeof(hardware_transform_patch_source_vertex) * TERRAIN_VERTEX_COUNT;

            memcpy(dst_vertex, src_vertex, vb_size);
            vb.unlock();
        }
    }

    auto terrain_patch::water_face_count() -> uint32_t {
        return water_pri_count_;
    }

    terrain_patch_proxy::terrain_patch_proxy() {
        clear();
    }

    terrain_patch_proxy::~terrain_patch_proxy() {
        clear();
    }

    void terrain_patch_proxy::set_center_position(const math::vector3 &center) {
        center_ = center;
    }

    auto terrain_patch_proxy::is_in(const math::vector3 &target, float radius) -> bool {
        float dx = center_.x - target.x;
        float dy = center_.y - target.y;
        float dist = dx * dx + dy * dy;
        float check = radius * radius;

        return dist < check;
    }

    auto terrain_patch_proxy::hardware_transform_patch_get_vertex_buffer_ptr() -> graphic_vertex_buffer * {
        if (terrain_patch_value_) {
            return terrain_patch_value_->hardware_transform_patch_get_vertex_buffer_ptr();
        }

        return nullptr;
    }

    auto terrain_patch_proxy::water_face_count() -> uint32_t {
        if (terrain_patch_value_) {
            return terrain_patch_value_->water_face_count();
        }

        return 0;
    }

    void terrain_patch_proxy::clear() {
        used_ = false;
        patch_num_ = 0;
        terrain_num_ = 0xFF;

        terrain_patch_value_ = nullptr;
    }
}
