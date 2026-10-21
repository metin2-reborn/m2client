#include "render_target.h"
#include <metin2/log/log.h>

namespace metin2::inline graphics {

    render_target::~render_target() {
        destroy();
    }

    auto render_target::create(uint32_t width_value, uint32_t height, gpu::format color, gpu::format depth) -> bool {
        destroy();

        color_ = device_->create_texture(width_value, height, 1, gpu::usage::RENDER_TARGET, color);
        if (!color_) {
            metin2::log::error("Cannot create a {}x{} render target", width_value, height);
            return false;
        }

        if (depth != gpu::format::UNKNOWN) {
            depth_ = device_->create_depth_buffer(width_value, height, depth);
            if (!depth_) {
                metin2::log::error("Cannot create a {}x{} depth buffer", width_value, height);
                color_.reset();
                return false;
            }
        }
        return true;
    }

    void render_target::destroy() {
        if (active_)
            end();
        color_.reset();
        depth_.reset();
    }

    auto render_target::begin(const gpu::viewport *view_port) -> bool {
        if (!color_ || active_)
            return false;

        gpu::texture *current_color = device_->render_target();
        gpu::depth_buffer *current_depth = device_->depth_target();
        saved_color_ = current_color ? current_color->shared_from_this() : nullptr;
        saved_depth_ = current_depth ? current_depth->shared_from_this() : nullptr;
        saved_viewport_ = device_->get_viewport();

        if (!device_->set_render_target(color_.get(), depth_.get())) {
            saved_color_.reset();
            saved_depth_.reset();
            return false;
        }
        if (view_port)
            device_->set_viewport(*view_port);
        active_ = true;
        return true;
    }

    void render_target::end() {
        if (!active_)
            return;

        device_->set_render_target(saved_color_.get(), saved_depth_.get());
        device_->set_viewport(saved_viewport_);
        saved_color_.reset();
        saved_depth_.reset();
        active_ = false;
    }
}
