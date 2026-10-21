#pragma once

#include <metin2/core/rectangle.h>
#include "grp_base.h"

namespace metin2::inline graphics {

    class graphic_dib;

    class block_texture : public graphic_base {
      public:
        block_texture();
        ~block_texture() override;

        auto create(graphic_dib *dib, const rectangle &rect_value, uint32_t width_value, uint32_t height) -> bool;
        void set_clip_rect(const rectangle &rect_value);
        void render(int ix, int iy);
        void invalidate_rect(const rectangle &rsrc_rect);

      protected:
        graphic_dib *dib_;
        rectangle rect_;
        rectangle clip_rect_;
        bool clip_enable_;
        uint32_t width_;
        uint32_t height_;
        std::shared_ptr<gpu::texture> texture_;
    };
}
