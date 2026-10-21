#include "dib_bar.h"
#include <cassert>
#include <metin2/log/log.h>
#include "block_texture.h"

namespace metin2::inline graphics {

    void dib_bar::invalidate() {
        rectangle rect_value = {static_cast<int32_t>(0), static_cast<int32_t>(0), static_cast<int32_t>(width_), static_cast<int32_t>(height_)};

        auto itor = vec_block_texture_.begin();
        for (; itor != vec_block_texture_.end(); ++itor) {
            block_texture *texture = *itor;
            texture->invalidate_rect(rect_value);
        }
    }

    void dib_bar::set_clip_rect(const rectangle &rect_value) {
        auto itor = vec_block_texture_.begin();
        for (; itor != vec_block_texture_.end(); ++itor) {
            block_texture *texture = *itor;
            assert(texture);
            if (!texture)
                continue;
            texture->set_clip_rect(rect_value);
        }
    }

    void dib_bar::clear_bar() {
        auto *buf = (uint32_t *)dib_.pointer();
        memset(buf, 0, dib_.width() * dib_.height() * 4);
        invalidate();
    }

    void dib_bar::render(int ix, int iy) {
        auto itor = vec_block_texture_.begin();
        for (; itor != vec_block_texture_.end(); ++itor) {
            block_texture *texture = *itor;
            texture->render(ix, iy);
        }
    }

    auto dib_bar::near_texture_size(uint32_t size) -> uint32_t {
        if ((size & (size - 1)) == 0)
            return size;

        uint32_t ret = 2;
        while (ret < size) {
            ret <<= 1;
        }

        return ret;
    }

    void dib_bar::divide_texture_size(uint32_t size, uint32_t max_value, uint32_t *pdwx_step, uint32_t *pdwx_count, uint32_t *pdwx_rest) {
        if (size < max_value) {
            *pdwx_step = max_value;
            *pdwx_count = 0;
            *pdwx_rest = size % max_value;
            return;
        }

        *pdwx_step = max_value;
        *pdwx_count = size / max_value;
        *pdwx_rest = size % max_value;
    }

    auto dib_bar::build_texture_block(uint32_t dwx_pos, uint32_t dwy_pos, uint32_t image_width, uint32_t image_height, uint32_t texture_width, uint32_t texture_height) -> block_texture * {
        if (texture_width == 0 || texture_height == 0)
            return nullptr;

        rectangle pos_rect = {static_cast<int32_t>(dwx_pos), static_cast<int32_t>(dwy_pos), static_cast<int32_t>(dwx_pos + image_width), static_cast<int32_t>(dwy_pos + image_height)};

        auto *block_texture_value = new block_texture;
        if (!block_texture_value->create(&dib_, pos_rect, texture_width, texture_height)) {
            delete block_texture_value;
            return nullptr;
        }

        return block_texture_value;
    }

    void dib_bar::build_texture_block_list(uint32_t width_value, uint32_t height, uint32_t max_value) {
        uint32_t dwx_step, dwy_step;
        uint32_t dwx_count, dwy_count;
        uint32_t dwx_rest, dwy_rest;
        divide_texture_size(width_value, max_value, &dwx_step, &dwx_count, &dwx_rest);
        divide_texture_size(height, max_value, &dwy_step, &dwy_count, &dwy_rest);
        uint32_t dwx_tex_rest = near_texture_size(dwx_rest);
        uint32_t dwy_tex_rest = near_texture_size(dwy_rest);

        for (uint32_t y = 0; y < dwy_count; ++y) {
            for (uint32_t x = 0; x < dwx_count; ++x) {
                block_texture *texture = build_texture_block(x * dwx_step, y * dwy_step,
                    dwx_step, dwy_step,
                    max_value, max_value);
                if (texture)
                    vec_block_texture_.push_back(texture);
            }

            block_texture *texture = build_texture_block(dwx_count * dwx_step, y * dwy_step,
                dwx_rest, dwy_step,
                dwx_tex_rest, max_value);

            if (texture)
                vec_block_texture_.push_back(texture);
        }

        for (uint32_t x = 0; x < dwx_count; ++x) {
            block_texture *texture = build_texture_block(x * dwx_step, dwy_count * dwy_step,
                dwx_step, dwy_rest,
                max_value, dwy_tex_rest);
            if (texture)
                vec_block_texture_.push_back(texture);
        }

        if (dwx_rest > 0 && dwy_rest > 0) {
            block_texture *texture = build_texture_block(dwx_count * dwx_step, dwy_count * dwy_step,
                dwx_rest, dwy_rest,
                dwx_tex_rest, dwy_tex_rest);
            if (texture)
                vec_block_texture_.push_back(texture);
        }
    }

    auto dib_bar::create(uint32_t width_value, uint32_t height) -> bool {
        if (!dib_.create(width_value, height)) {
            metin2::log::debug("Cannot create the DIB bar");
            return false;
        }

        width_ = width_value;
        height_ = height;

        build_texture_block_list(width_value, height);
        on_create();

        return true;
    }

    dib_bar::dib_bar() = default;
    dib_bar::~dib_bar() = default;
}
