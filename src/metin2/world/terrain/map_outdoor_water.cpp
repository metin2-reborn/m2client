#include "map_outdoor.h"

#include "terrain_patch.h"

#include <metin2/core/random.h>
#include <metin2/core/timer.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/graphics/state_manager.h>

#include <algorithm>
#include <cstring>

namespace metin2::inline world::inline terrain {

    void map_outdoor::load_water_texture() {
        unload_water_texture();
        char buf[256];
        for (int i = 0; i < 30; ++i) {
            snprintf(buf, sizeof(buf), "d:/ymir Work/special/water/%02d.dds", i + 1);
            water_instances_[i].set_image_pointer((graphic_image *)get_resource_manager().resource_pointer(buf));
        }
    }

    void map_outdoor::unload_water_texture() {
        for (int i = 0; i < 30; ++i) {
            water_instances_[i].destroy();
        }
    }

    void map_outdoor::render_water() {
        if (patch_vector_.empty()) {
            return;
        }

        if (!is_visible_part(PART_WATER)) {
            return;
        }

        math::matrix tex_transform_water;

        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
        STATEMANAGER.save_render_state(gpu::state::DIFFUSE_MATERIAL_SOURCE, gpu::material_source::COLOR1);
        STATEMANAGER.save_render_state(gpu::state::COLOR_VERTEX, true);
        const float water_time = get_timer().current_second();
        uint32_t water_time_bits;
        std::memcpy(&water_time_bits, &water_time, sizeof(water_time_bits));
        STATEMANAGER.save_render_state(gpu::state::EFFECT_MODE, 1);
        STATEMANAGER.save_render_state(gpu::state::EFFECT_TIME, water_time_bits);

        STATEMANAGER.set_texture(0, water_instances_[((el_timer_get_m_sec() / 70) % 30)].texture_pointer()->gpu_texture());

        tex_transform_water = math::matrix_scaling(water_tex_coord_base_, -water_tex_coord_base_, 0.0f);
        tex_transform_water = view_inverse_ * tex_transform_water;

        STATEMANAGER.save_transform(gpu::transform::TEXTURE0, &tex_transform_water);
        STATEMANAGER.save_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE);

        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX, gpu::texcoord::CAMERA_SPACE_POSITION);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::COUNT2);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::MIP_FILTER, gpu::filter::LINEAR);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);

        // Animate the water up and down.
        static float water_height_current = 0;
        static float water_height_begin = 0;
        static float water_height_end = 0;
        static uint32_t last_height_change_time = get_timer().current_millisecond();
        static uint32_t blendtime = 300;

        // A new target height is picked every 1 to 3 seconds.
        if ((get_timer().current_millisecond() - last_height_change_time) > blendtime) {
            blendtime = metin2::random::number(1000, 3000);

            if (water_height_end == 0) {
                water_height_end = -metin2::random::number(0, 15);
            } else {
                water_height_end = 0;
            }

            water_height_begin = water_height_current;
            last_height_change_time = get_timer().current_millisecond();
        }

        // Eased in and out, so the surface does not reverse with a jolt.
        const float progress = std::clamp(static_cast<float>(get_timer().current_millisecond() - last_height_change_time) / static_cast<float>(blendtime), 0.0f, 1.0f);
        const float eased = progress * progress * (3.0f - 2.0f * progress);
        water_height_current = water_height_begin + (water_height_end - water_height_begin) * eased;
        world_for_common_use_._43 = water_height_current;

        world_for_common_use_._41 = 0.0f;
        world_for_common_use_._42 = 0.0f;
        STATEMANAGER.set_transform(gpu::transform::WORLD, &world_for_common_use_);

        float fog_distance_value = fog_distance();

        std::vector<std::pair<float, int32_t>>::iterator i;

        for (i = patch_vector_.begin(); i != patch_vector_.end(); ++i) {
            if (i->first < fog_distance_value) {
                draw_water(i->second);
            }
        }

        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);

        for (i = patch_vector_.begin(); i != patch_vector_.end(); ++i) {
            if (i->first >= fog_distance_value) {
                draw_water(i->second);
            }
        }

        world_for_common_use_._43 = 0.0f;

        STATEMANAGER.restore_render_state(gpu::state::EFFECT_TIME);
        STATEMANAGER.restore_render_state(gpu::state::EFFECT_MODE);
        STATEMANAGER.restore_vertex_shader();
        STATEMANAGER.restore_transform(gpu::transform::TEXTURE0);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MIN_FILTER);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MAG_FILTER);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::MIP_FILTER);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ADDRESS_U);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ADDRESS_V);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);

        STATEMANAGER.restore_render_state(gpu::state::DIFFUSE_MATERIAL_SOURCE);
        STATEMANAGER.restore_render_state(gpu::state::COLOR_VERTEX);
        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
    }

    void map_outdoor::draw_water(int32_t patchnum) {
        assert(nullptr != terrain_patch_proxy_list_);
        if (!terrain_patch_proxy_list_) {
            return;
        }

        auto &terrain_patch_proxy = terrain_patch_proxy_list_[patchnum];
        if (!terrain_patch_proxy.is_used()) {
            return;
        }

        if (!terrain_patch_proxy.is_water_exists()) {
            return;
        }

        graphic_vertex_buffer *vb = terrain_patch_proxy.water_vertex_buffer_pointer();
        if (!vb) {
            return;
        }

        if (!vb->gpu_buffer()) {
            return;
        }

        uint32_t pri_count = terrain_patch_proxy.water_face_count();
        if (!pri_count) {
            return;
        }

        STATEMANAGER.set_stream_source(0, vb->gpu_buffer(), sizeof(water_vertex));
        STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_LIST, 0, pri_count);

        face_count_ += pri_count;
    }
}
