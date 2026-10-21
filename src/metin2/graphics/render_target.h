#pragma once

#include "grp_base.h"

namespace metin2::inline graphics {

    // A texture to render into, with its own depth buffer. begin saves the current target and
    // viewport, end puts them back.
    class render_target : public graphic_base {
      public:
        render_target() = default;
        ~render_target() override;

        auto create(uint32_t width_value, uint32_t height, gpu::format color, gpu::format depth = gpu::format::D16) -> bool;
        void destroy();

        auto texture() const -> gpu::texture * { return color_.get(); }
        auto is_created() const -> bool { return color_ != nullptr; }

        // The viewport defaults to the whole target.
        auto begin(const gpu::viewport *view_port = nullptr) -> bool;
        void end();

      private:
        std::shared_ptr<gpu::texture> color_;
        std::shared_ptr<gpu::depth_buffer> depth_;

        std::shared_ptr<gpu::texture> saved_color_;
        std::shared_ptr<gpu::depth_buffer> saved_depth_;
        gpu::viewport saved_viewport_;
        bool active_ = false;
    };
}
