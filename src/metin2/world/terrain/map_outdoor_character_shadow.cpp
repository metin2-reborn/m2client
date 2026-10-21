#include <metin2/graphics/camera.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>

#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include "map_outdoor.h"

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

        if (is_low_texture_memory())
            set_shadow_texture_size(128);

        shadow_map_viewport_ = {1, 1, static_cast<uint32_t>(shadow_map_size_ - 2), static_cast<uint32_t>(shadow_map_size_ - 2), 0.0f, 1.0f};

        if (!character_shadow_target_.create(shadow_map_size_, shadow_map_size_, gpu::format::R5G6B5, gpu::format::D16))
            metin2::log::error("Cannot create the character shadow render target");
    }

    void map_outdoor::release_character_shadow_texture() {
        character_shadow_target_.destroy();
    }

    uint32_t light_enable = false;

    auto map_outdoor::begin_render_character_shadow_to_texture() -> bool {
        math::matrix light_view, light_proj;

        camera *current_camera = get_camera_manager().current_camera();

        if (!current_camera)
            return false;

        if (recreate) {
            create_character_shadow_texture();
            recreate = false;
        }

        math::vector3 target = current_camera->target();

        math::vector3 eye(target.x - 1.732f * 1250.0f,
            target.y - 1250.0f,
            target.z + 2.0f * 1.732f * 1250.0f);

        const auto vv = math::vector3(0.0f, 0.0f, 1.0f);
        light_view = math::matrix_look_at_rh(eye, target, vv);

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

        if (!character_shadow_target_.begin()) {
            metin2::log::error("Cannot set the shadow map render target");
            return false;
        }

        device_->clear({}, gpu::clear::TARGET | gpu::clear::Z_BUFFER, 0xffffffff, 1.0f, 0);
        device_->set_viewport(shadow_map_viewport_);
        return true;
    }

    void map_outdoor::end_render_character_shadow_to_texture() {
        character_shadow_target_.end();

        STATEMANAGER.restore_transform(gpu::transform::VIEW);
        STATEMANAGER.restore_transform(gpu::transform::PROJECTION);

        STATEMANAGER.set_render_state(gpu::state::LIGHTING, light_enable);
        STATEMANAGER.restore_render_state(gpu::state::TEXTURE_FACTOR);
    }
}
