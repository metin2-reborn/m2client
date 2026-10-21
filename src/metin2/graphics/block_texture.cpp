#include "block_texture.h"
#include <algorithm>
#include <metin2/core/stl.h>
#include <metin2/gpu/types.h>
#include <metin2/log/log.h>
#include "grp_base.h"
#include "grp_dib.h"
#include "state_manager.h"

namespace metin2::inline graphics {

    void block_texture::set_clip_rect(const rectangle &rect_value) {
        clip_enable_ = true;
        clip_rect_ = rect_value;
    }

    void block_texture::render(int ix, int iy) {
        int isx = ix + rect_.left;
        int isy = iy + rect_.top;
        int iex = ix + rect_.left + width_;
        int iey = iy + rect_.top + height_;

        float su = 0.0f;
        float sv = 0.0f;
        float eu = 1.0f;
        float ev = 1.0f;

        if (clip_enable_) {
            if (isx > clip_rect_.right)
                return;
            if (iex < clip_rect_.left)
                return;

            if (isy > clip_rect_.bottom)
                return;
            if (iey < clip_rect_.top)
                return;

            if (clip_rect_.left > isx) {
                int idx = clip_rect_.left - isx;
                isx += idx;
                su += float(idx) / float(width_);
            }
            if (iex > clip_rect_.right) {
                int idx = iex - clip_rect_.right;
                iex -= idx;
                eu -= float(idx) / float(width_);
            }

            if (clip_rect_.top > isy) {
                int idy = clip_rect_.top - isy;
                isy += idy;
                sv += float(idy) / float(height_);
            }
            if (iey > clip_rect_.bottom) {
                int idy = iey - clip_rect_.bottom;
                iey -= idy;
                ev -= float(idy) / float(height_);
            }
        }

        pdt_vertex vertices[4];
        vertices[0].position_value.x = isx - 0.5f;
        vertices[0].position_value.y = isy - 0.5f;
        vertices[0].position_value.z = 0.0f;
        vertices[0].tex_coord = texture_coordinate(su, sv);
        vertices[0].diffuse_value = 0xffffffff;

        vertices[1].position_value.x = iex - 0.5f;
        vertices[1].position_value.y = isy - 0.5f;
        vertices[1].position_value.z = 0.0f;
        vertices[1].tex_coord = texture_coordinate(eu, sv);
        vertices[1].diffuse_value = 0xffffffff;

        vertices[2].position_value.x = isx - 0.5f;
        vertices[2].position_value.y = iey - 0.5f;
        vertices[2].position_value.z = 0.0f;
        vertices[2].tex_coord = texture_coordinate(su, ev);
        vertices[2].diffuse_value = 0xffffffff;

        vertices[3].position_value.x = iex - 0.5f;
        vertices[3].position_value.y = iey - 0.5f;
        vertices[3].position_value.z = 0.0f;
        vertices[3].tex_coord = texture_coordinate(eu, ev);
        vertices[3].diffuse_value = 0xffffffff;

        if (graphic_base::set_pdt_stream(vertices, 4)) {
            graphic_base::set_default_index_buffer(graphic_base::DEFAULT_IB_FILL_RECT);

            STATEMANAGER.set_texture(0, texture_.get());
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::TEX1 | gpu::fvf::DIFFUSE);
            STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, 4, 0, 2);
        }
    }

    void block_texture::invalidate_rect(const rectangle &rsrc_rect) {
        rectangle dst_rect = rect_;
        if (rsrc_rect.right < dst_rect.left ||
            rsrc_rect.left > dst_rect.right ||
            rsrc_rect.bottom < dst_rect.top ||
            rsrc_rect.top > dst_rect.bottom) {
            metin2::log::debug("Invalid rectangle to invalidate");
            return;
        }

        const rectangle clip_rect = {
            std::max<int32_t>(rsrc_rect.left - dst_rect.left, 0),
            std::max<int32_t>(rsrc_rect.top - dst_rect.top, 0),
            std::min<int32_t>(rsrc_rect.right - dst_rect.left, dst_rect.right - dst_rect.left),
            std::min<int32_t>(rsrc_rect.bottom - dst_rect.top, dst_rect.bottom - dst_rect.top),
        };

        uint32_t *src;
        src = (uint32_t *)dib_->pointer();
        src += dst_rect.left + dst_rect.top * dib_->width();

        gpu::locked_rect locked_rect;
        if (!texture_->lock(0, locked_rect, &clip_rect)) {
            metin2::log::debug("Cannot lock the block texture");
            return;
        }

        int iclip_width = clip_rect.right - clip_rect.left;
        int iclip_height = clip_rect.bottom - clip_rect.top;
        auto *dst = (uint32_t *)locked_rect.bits;
        uint32_t dst_width = locked_rect.pitch >> 2;
        uint32_t src_width = dib_->width();
        for (int i = 0; i < iclip_height; ++i) {
            for (int i = 0; i < iclip_width; ++i) {
                if (src[i])
                    dst[i] = src[i] | 0xff000000;
                else
                    dst[i] = 0;
            }
            dst += dst_width;
            src += src_width;
        }

        texture_->unlock(0);
    }

    auto block_texture::create(graphic_dib *dib, const rectangle &rect_value, uint32_t width_value, uint32_t height) -> bool {
        texture_ = device_->create_texture(width_value, height, 0, 0, gpu::format::A8R8G8B8);
        if (!texture_) {
            metin2::log::debug("Cannot create a block texture of {} by {}", width_value, height);
            return false;
        }

        dib_ = dib;
        rect_ = rect_value;
        width_ = width_value;
        height_ = height;
        clip_enable_ = false;

        return true;
    }

    block_texture::block_texture() {
        dib_ = nullptr;
    }

    block_texture::~block_texture() = default;
}
