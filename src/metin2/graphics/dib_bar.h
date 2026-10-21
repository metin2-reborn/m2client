#pragma once

#include <cstdint>
#include <metin2/core/rectangle.h>
#include <metin2/gpu/types.h>
#include <vector>
#include "grp_dib.h"

namespace metin2::inline graphics {

    class block_texture;

    class dib_bar {
      public:
        dib_bar();
        virtual ~dib_bar();

        auto create(uint32_t width_value, uint32_t height) -> bool;
        void invalidate();
        void set_clip_rect(const rectangle &rect_value);
        void clear_bar();
        void render(int ix, int iy);

      protected:
        auto near_texture_size(uint32_t size) -> uint32_t;
        void divide_texture_size(uint32_t size, uint32_t max_value, uint32_t *pdwx_step, uint32_t *pdwx_count, uint32_t *pdwx_rest);
        auto build_texture_block(uint32_t dwx_pos, uint32_t dwy_pos, uint32_t image_width, uint32_t image_height, uint32_t texture_width, uint32_t texture_height) -> block_texture *;
        void build_texture_block_list(uint32_t width_value, uint32_t height, uint32_t max_value = 256);

        virtual void on_create() {}

      protected:
        graphic_dib dib_;
        std::vector<block_texture *> vec_block_texture_;

        uint32_t width_;
        uint32_t height_;
    };
}
