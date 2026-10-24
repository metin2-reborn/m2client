#include "map_outdoor.h"

#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/camera.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>

namespace metin2::inline world::inline terrain {
    static int recreate = false;

    void map_outdoor::set_shadow_texture_size(uint16_t size) {
        if (shadow_map_size_ != size) {
            recreate = true;
            metin2::log::debug("Shadow texture size changed from {} to {}", shadow_map_size_, size);
        }

        shadow_map_size_ = size;
    }

    void map_outdoor::create_character_shadow_texture() {

        release_character_shadow_texture();

        if (is_low_texture_memory()) {
            set_shadow_texture_size(128);
        }

        shadow_map_viewport_ = {1, 1, static_cast<uint32_t>(shadow_map_size_ - 2), static_cast<uint32_t>(shadow_map_size_ - 2), 0.0f, 1.0f};

        // 8 bits per channel: the blurred edge of a shadow banded visibly in R5G6B5.
        if (!character_shadow_raw_target_.create(shadow_map_size_, shadow_map_size_, gpu::format::X8R8G8B8, gpu::format::D16) ||
            !character_shadow_target_.create(shadow_map_size_, shadow_map_size_, gpu::format::X8R8G8B8, gpu::format::UNKNOWN)) {
            metin2::log::error("Cannot create the character shadow render target");
        }
    }

    void map_outdoor::release_character_shadow_texture() {
        character_shadow_raw_target_.destroy();
        character_shadow_target_.destroy();
    }

    uint32_t light_enable = false;

    auto map_outdoor::begin_render_character_shadow_to_texture() -> bool {
        math::matrix light_view, light_proj;

        camera *current_camera = get_camera_manager().current_camera();

        if (!current_camera) {
            return false;
        }

        if (recreate) {
            create_character_shadow_texture();
            recreate = false;
        }

        math::vector3 target = current_camera->target();

        const auto vv = math::vector3(0.0f, 0.0f, 1.0f);
        light_view = math::matrix_look_at_rh(shadow_light_eye(target), target, vv);

        light_proj = math::matrix_ortho_rh(2550.0f, 2550.0f, 1.0f, 15000.0f);

        STATEMANAGER.save_transform(gpu::transform::VIEW, &light_view);
        STATEMANAGER.save_transform(gpu::transform::PROJECTION, &light_proj);

        light_enable = STATEMANAGER.render_state(gpu::state::LIGHTING);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);

        STATEMANAGER.save_render_state(gpu::state::TEXTURE_FACTOR, 0xFF808080);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);

        if (!character_shadow_raw_target_.begin()) {
            metin2::log::error("Cannot set the shadow map render target");
            return false;
        }

        device_->clear({}, gpu::clear::TARGET | gpu::clear::Z_BUFFER, 0xffffffff, 1.0f, 0);
        device_->set_viewport(shadow_map_viewport_);
        return true;
    }

    void map_outdoor::end_render_character_shadow_to_texture() {
        character_shadow_raw_target_.end();

        STATEMANAGER.restore_transform(gpu::transform::VIEW);
        STATEMANAGER.restore_transform(gpu::transform::PROJECTION);

        STATEMANAGER.set_render_state(gpu::state::LIGHTING, light_enable);
        STATEMANAGER.restore_render_state(gpu::state::TEXTURE_FACTOR);

        blur_character_shadow_texture();
    }

    // Four quarter-weight copies of the raw map, each shifted by a texel diagonally, sum to a tent
    // filter that softens the shadow edges by about a texel. The one-texel border stays white: the
    // terrain clamps the map, so anything in the border would be drawn out to the horizon.
    void map_outdoor::blur_character_shadow_texture() {
        if (!character_shadow_target_.is_created() || !character_shadow_target_.begin()) {
            return;
        }

        device_->clear({}, gpu::clear::TARGET, 0xffffffff, 1.0f, 0);
        device_->set_viewport(shadow_map_viewport_);

        const float size = static_cast<float>(shadow_map_size_);
        const float texel = 1.0f / size;
        const float inset = static_cast<float>(shadow_map_viewport_.x);
        const float extent = size - inset;
        const math::matrix proj = math::matrix_ortho_off_center_rh(inset, extent, extent, inset, -1.0f, 1.0f);
        const math::matrix identity = math::matrix_identity();
        STATEMANAGER.save_transform(gpu::transform::PROJECTION, &proj);
        STATEMANAGER.save_transform(gpu::transform::VIEW, &identity);
        STATEMANAGER.set_transform(gpu::transform::WORLD, &identity);

        STATEMANAGER.save_render_state(gpu::state::Z_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.save_render_state(gpu::state::FOG_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::ONE);
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::ONE);
        STATEMANAGER.save_render_state(gpu::state::TEXTURE_FACTOR, 0xff404040);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::CLAMP);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::CLAMP);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX, 0);
        STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, gpu::texture_transform_flags::DISABLE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::TFACTOR);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture(0, character_shadow_raw_target_.texture());
        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);

        const float offsets[4][2] = {
            {-texel, -texel},
            {texel,  -texel},
            {-texel, texel },
            {texel,  texel }
        };

        const float uv_lo = inset * texel;
        const float uv_hi = extent * texel;
        for (int pass = 0; pass < 4; ++pass) {
            const float u = offsets[pass][0];
            const float v = offsets[pass][1];
            const vertex vertices[4] = {
                {inset,  inset,  0.0f, 0xffffffff, uv_lo + u, uv_lo + v},
                {inset,  extent, 0.0f, 0xffffffff, uv_lo + u, uv_hi + v},
                {extent, inset,  0.0f, 0xffffffff, uv_hi + u, uv_lo + v},
                {extent, extent, 0.0f, 0xffffffff, uv_hi + u, uv_hi + v},
            };

            if (pass == 1) {
                STATEMANAGER.set_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
            }

            STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, vertices, sizeof(vertex));
        }

        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ADDRESS_U);
        STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ADDRESS_V);
        STATEMANAGER.restore_render_state(gpu::state::TEXTURE_FACTOR);
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
        STATEMANAGER.restore_render_state(gpu::state::FOG_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::LIGHTING);
        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::Z_ENABLE);
        STATEMANAGER.restore_transform(gpu::transform::VIEW);
        STATEMANAGER.restore_transform(gpu::transform::PROJECTION);

        character_shadow_target_.end();
    }
}
