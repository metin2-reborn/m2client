#include "terrain_patch.h"
#include <metin2/gpu/types.h>

namespace metin2::inline world::inline terrain {

    terrain_patch::software_transform_patch::software_transform_patch() {
        initialize_impl();
    }

    terrain_patch::software_transform_patch::~software_transform_patch() {
        destroy();
    }

    void terrain_patch::software_transform_patch::create() {
        assert(nullptr == terrain_vertex);
        terrain_vertex = std::make_unique<software_transform_patch_source_vertex[]>(TERRAIN_VERTEX_COUNT);
    }

    void terrain_patch::software_transform_patch::destroy() {
        if (terrain_vertex)
            terrain_vertex.reset();

        initialize_impl();
    }

    void terrain_patch::software_transform_patch::initialize_impl() {
        terrain_vertex.reset();
    }

    bool terrain_patch::SOFTWARE_TRANSFORM_PATCH_ENABLE = true;

    void terrain_patch::clear() {
        ht_.vb.destroy();
        st_.destroy();

        water_vertex_buffer_.destroy();

        water_exist_ = false;
        need_update_ = true;

        water_pri_count_ = 0;
        type_value_ = PATCH_TYPE_PLAIN;

        min_x_ = max_x_ = min_y_ = max_y_ = min_z_ = max_z_ = 0.0f;

        version_ = 0;
    }

    void terrain_patch::build_water_vertex_buffer(water_vertex *src_vertex, uint32_t water_vertex_count) {
        graphic_vertex_buffer &vb = water_vertex_buffer_;

        if (!vb.create(water_vertex_count, gpu::fvf::XYZ | gpu::fvf::DIFFUSE, gpu::usage::WRITE_ONLY))
            return;

        water_vertex *dst_water_vertex;
        if (vb.lock((void **)&dst_water_vertex)) {
            uint32_t vb_size = sizeof(water_vertex) * water_vertex_count;
            memcpy(dst_water_vertex, src_vertex, vb_size);
            water_pri_count_ = water_vertex_count / 3;

            vb.unlock();
        }
    }

    void terrain_patch::build_terrain_vertex_buffer(hardware_transform_patch_source_vertex *src_vertex) {
        if (SOFTWARE_TRANSFORM_PATCH_ENABLE)
            build_software_terrain_vertex_buffer(src_vertex);
        else
            build_hardware_terrain_vertex_buffer(src_vertex);
    }

    void terrain_patch::build_software_terrain_vertex_buffer(hardware_transform_patch_source_vertex *src_vertex) {

        st_.create();

        software_transform_patch_source_vertex *dst_vertex = software_transform_patch_get_terrain_vertex_data_ptr();
        for (uint32_t index = 0; index != TERRAIN_VERTEX_COUNT; ++index) {
            *((hardware_transform_patch_source_vertex *)(dst_vertex + index)) = *(src_vertex + index);
            dst_vertex[index].diffuse_value = 0xFFFFFFFF;
        }
    }

    void terrain_patch::build_hardware_terrain_vertex_buffer(hardware_transform_patch_source_vertex *src_vertex) {

        graphic_vertex_buffer &vb = ht_.vb;
        if (!vb.create(TERRAIN_VERTEX_COUNT, gpu::fvf::XYZ | gpu::fvf::NORMAL, gpu::usage::WRITE_ONLY))
            return;

        hardware_transform_patch_source_vertex *dst_vertex;
        if (vb.lock((void **)&dst_vertex)) {
            uint32_t vb_size = sizeof(hardware_transform_patch_source_vertex) * TERRAIN_VERTEX_COUNT;

            memcpy(dst_vertex, src_vertex, vb_size);
            vb.unlock();
        }
    }

    void terrain_patch::software_transform_patch_update_terrain_lighting(uintptr_t version, const gpu::light &light, const gpu::material &mtrl) {
        if (version_ == version)
            return;

        version_ = version;

        software_transform_patch_source_vertex *src_vertex = software_transform_patch_get_terrain_vertex_data_ptr();
        if (!src_vertex)
            return;

        math::vector3 light_dir = light.direction;

        uint32_t dw_dot;
        uint32_t ambient_r = (mtrl.ambient.r * light.ambient.r + mtrl.emissive.r) * 255.0f;
        uint32_t ambient_g = (mtrl.ambient.g * light.ambient.g + mtrl.emissive.g) * 255.0f;
        uint32_t ambient_b = (mtrl.ambient.b * light.ambient.b + mtrl.emissive.b) * 255.0f;
        uint32_t diffuse_r = (mtrl.diffuse.r * light.diffuse.r) * 255.0f;
        uint32_t diffuse_g = (mtrl.diffuse.g * light.diffuse.g) * 255.0f;
        uint32_t diffuse_b = (mtrl.diffuse.b * light.diffuse.b) * 255.0f;

        if (diffuse_r > 255 - ambient_r)
            diffuse_r = 255 - ambient_r;

        if (diffuse_g + ambient_g > 255)
            diffuse_g = 255 - ambient_g;

        if (diffuse_b + ambient_b > 255)
            diffuse_b = 255 - ambient_b;

        for (uint32_t index = 0; index != terrain_patch::TERRAIN_VERTEX_COUNT; ++index) {
            float f_dot = math::dot(src_vertex[index].normal_value, light_dir);

            const float n = 0xffffff;
            const int s = 24;
            if (f_dot < 0.0f)
                dw_dot = (n * -f_dot);
            else
                dw_dot = (n * +f_dot);

            src_vertex[index].diffuse_value = (0xff000000) |
                                              (((diffuse_r * dw_dot >> s) + ambient_r) << 16) |
                                              (((diffuse_g * dw_dot >> s) + ambient_g) << 8) |
                                              ((diffuse_b * dw_dot >> s) + ambient_b);
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

        if (dist < check)
            return true;

        return false;
    }

    void terrain_patch_proxy::software_transform_patch_update_terrain_lighting(uintptr_t version, const gpu::light &light, const gpu::material &mtrl) {
        if (terrain_patch_value_)
            terrain_patch_value_->software_transform_patch_update_terrain_lighting(version, light, mtrl);
    }

    auto terrain_patch_proxy::hardware_transform_patch_get_vertex_buffer_ptr() -> graphic_vertex_buffer * {
        if (terrain_patch_value_)
            return terrain_patch_value_->hardware_transform_patch_get_vertex_buffer_ptr();

        return nullptr;
    }

    auto terrain_patch_proxy::software_transform_patch_get_terrain_vertex_data_ptr() -> software_transform_patch_source_vertex * {
        if (terrain_patch_value_)
            return terrain_patch_value_->software_transform_patch_get_terrain_vertex_data_ptr();

        return nullptr;
    }

    auto terrain_patch_proxy::water_face_count() -> uint32_t {
        if (terrain_patch_value_)
            return terrain_patch_value_->water_face_count();

        return 0;
    }

    void terrain_patch_proxy::clear() {
        used_ = false;
        patch_num_ = 0;
        terrain_num_ = 0xFF;

        terrain_patch_value_ = nullptr;
    }
}
