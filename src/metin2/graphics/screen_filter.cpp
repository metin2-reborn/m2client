#include "screen_filter.h"
#include <metin2/gpu/types.h>
#include "state_manager.h"

namespace metin2::inline graphics {

    void screen_filter::render() {
        if (!enable_)
            return;

        STATEMANAGER.save_transform(gpu::transform::PROJECTION, &identity_);
        STATEMANAGER.save_transform(gpu::transform::VIEW, &identity_);
        STATEMANAGER.set_transform(gpu::transform::WORLD, &identity_);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, src_type_);
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, dest_type_);
        STATEMANAGER.save_render_state(gpu::state::Z_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::FOG_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::LIGHTING, false);

        set_ortho2d(screen::width_, screen::height_, 400.0f);
        set_diffuse_color(color_value_.r, color_value_.g, color_value_.b, color_value_.a);
        render_bar2d(0, 0, screen::width_, screen::height_);

        STATEMANAGER.restore_render_state(gpu::state::LIGHTING);
        STATEMANAGER.restore_render_state(gpu::state::FOG_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::Z_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
        STATEMANAGER.restore_transform(gpu::transform::VIEW);
        STATEMANAGER.restore_transform(gpu::transform::PROJECTION);
    }

    void screen_filter::set_enable(bool flag) {
        enable_ = flag;
    }

    void screen_filter::set_blend_type(uint8_t src_type, uint8_t dest_type) {
        src_type_ = src_type;
        dest_type_ = dest_type;
    }
    void screen_filter::set_color(const math::color &color) {
        color_value_ = color;
    }

    screen_filter::screen_filter() {
        enable_ = false;
        src_type_ = gpu::blend::SRC_ALPHA;
        dest_type_ = gpu::blend::INV_SRC_ALPHA;
        color_value_ = math::color(0.0f, 0.0f, 0.0f, 0.0f);
    }
    screen_filter::~screen_filter() = default;
}
