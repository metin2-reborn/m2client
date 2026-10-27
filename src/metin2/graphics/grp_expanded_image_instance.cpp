#include "grp_expanded_image_instance.h"
#include <algorithm>
#include <cmath>
#include <metin2/core/crc32.h>
#include <metin2/core/timer.h>
#include <metin2/gpu/types.h>
#include <utility>
#include <vector>
#include "state_manager.h"

namespace metin2::inline graphics {

    dynamic_pool<graphic_expanded_image_instance> graphic_expanded_image_instance::pool_value;

    void graphic_expanded_image_instance::create_system(uint32_t capacity) {
        pool_value.create(capacity);
    }

    void graphic_expanded_image_instance::destroy_system() {
        pool_value.destroy();
    }

    auto graphic_expanded_image_instance::new_value() -> graphic_expanded_image_instance * {
        return pool_value.alloc();
    }

    void graphic_expanded_image_instance::delete_value(graphic_expanded_image_instance *img_inst) {
        img_inst->destroy();
        pool_value.free(img_inst);
    }

    auto graphic_expanded_image_instance::fit(fit_mode mode, float image_width, float image_height, float width, float height) -> fit_rect {
        fit_rect result;
        if (image_width <= 0.0f || image_height <= 0.0f || width <= 0.0f || height <= 0.0f)
            return result;
        float scale_x = width / image_width;
        float scale_y = height / image_height;
        if (mode == FIT_COVER)
            scale_x = scale_y = std::max(scale_x, scale_y);
        else if (mode == FIT_CONTAIN)
            scale_x = scale_y = std::min(scale_x, scale_y);
        const float drawn_width = image_width * scale_x;
        const float drawn_height = image_height * scale_y;
        // The part of the box the image covers (contain leaves margins) on whole pixels, and the
        // part of the image that is shown (cover crops both sides alike).
        result.width = std::min(drawn_width, width);
        result.height = std::min(drawn_height, height);
        result.x = std::floor((width - result.width) / 2.0f);
        result.y = std::floor((height - result.height) / 2.0f);
        const float crop_u = (1.0f - result.width / drawn_width) / 2.0f;
        const float crop_v = (1.0f - result.height / drawn_height) / 2.0f;
        result.u0 = crop_u;
        result.v0 = crop_v;
        result.u1 = 1.0f - crop_u;
        result.v1 = 1.0f - crop_v;
        return result;
    }

    void graphic_expanded_image_instance::render_tiled(graphic_image &image, graphic_texture &texture) {
        // The image repeated at its own size from the top left of the box, a partial tile at the
        // far sides; a negative scale mirrors every tile.
        const rectangle &rect_value = image.rect_reference();
        const float tile_width = float(rect_value.right - rect_value.left);
        const float tile_height = float(rect_value.bottom - rect_value.top);
        if (tile_width <= 0.0f || tile_height <= 0.0f || fit_width_ <= 0.0f || fit_height_ <= 0.0f)
            return;
        const float tex_reverse_width = 1.0f / float(texture.width());
        const float tex_reverse_height = 1.0f / float(texture.height());
        const float half = graphic_base::half_pixel();
        const float x0 = position_value_.x - half;
        const float y0 = position_value_.y - half;

        // Two triangles per tile, wound as the nine patch quads are.
        static std::vector<pdt_vertex> vertices;
        vertices.clear();
        auto vertex = [&](float x, float y, float u, float v) -> void {
            pdt_vertex &out = vertices.emplace_back();
            out.position_value = {x0 + x, y0 + y, depth_};
            out.tex_coord = texture_coordinate(u * tex_reverse_width, v * tex_reverse_height);
            out.diffuse_value = diffuse_color_;
        };
        for (float y = 0.0f; y < fit_height_; y += tile_height) {
            const float h = std::min(tile_height, fit_height_ - y);
            float v0 = float(rect_value.top);
            float v1 = v0 + h;
            if (scale_.y < 0.0f) {
                v0 = float(rect_value.bottom);
                v1 = v0 - h;
            }
            for (float x = 0.0f; x < fit_width_; x += tile_width) {
                const float w = std::min(tile_width, fit_width_ - x);
                float u0 = float(rect_value.left);
                float u1 = u0 + w;
                if (scale_.x < 0.0f) {
                    u0 = float(rect_value.right);
                    u1 = u0 - w;
                }
                vertex(x, y, u0, v0);
                vertex(x, y + h, u0, v1);
                vertex(x + w, y, u1, v0);
                vertex(x + w, y, u1, v0);
                vertex(x, y + h, u0, v1);
                vertex(x + w, y + h, u1, v1);
            }
        }

        STATEMANAGER.set_texture(0, texture.gpu_texture());
        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
        // Longer runs than the user stream holds go in slices (each a whole number of triangles).
        constexpr size_t MAX_VERTICES = 4096 * 6;
        for (size_t at = 0; at < vertices.size(); at += MAX_VERTICES) {
            const size_t count = std::min(MAX_VERTICES, vertices.size() - at);
            STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_LIST, static_cast<uint32_t>(count / 3), vertices.data() + at, sizeof(pdt_vertex));
        }
    }

    void graphic_expanded_image_instance::on_render() {
        graphic_image *image = ro_image_.pointer();
        graphic_texture *texture = image->texture_pointer();

        const rectangle &rect_value = image->rect_reference();
        float tex_reverse_width = 1.0f / float(texture->width());
        float tex_reverse_height = 1.0f / float(texture->height());
        float su = (rect_value.left - rendering_rect_.left) * tex_reverse_width;
        float sv = (rect_value.top - rendering_rect_.top) * tex_reverse_height;
        float eu = (rect_value.left + rendering_rect_.right + (rect_value.right - rect_value.left)) * tex_reverse_width;
        float ev = (rect_value.top + rendering_rect_.bottom + (rect_value.bottom - rect_value.top)) * tex_reverse_height;

        if (fit_mode_ == FIT_TILE && rotation_ == 0.0f) {
            render_tiled(*image, *texture);
            return;
        }
        if (fit_mode_ != FIT_NONE && rotation_ == 0.0f) {
            // The image fitted into the box of fit_width_ x fit_height_ at the position; the scale
            // and the rendering rect do not apply, only the sign of the scale (a mirror).
            const fit_rect box = fit(static_cast<fit_mode>(fit_mode_), float(image->width()), float(image->height()), fit_width_, fit_height_);
            const float image_width = float(rect_value.right - rect_value.left);
            const float image_height = float(rect_value.bottom - rect_value.top);
            su = (float(rect_value.left) + box.u0 * image_width) * tex_reverse_width;
            eu = (float(rect_value.left) + box.u1 * image_width) * tex_reverse_width;
            sv = (float(rect_value.top) + box.v0 * image_height) * tex_reverse_height;
            ev = (float(rect_value.top) + box.v1 * image_height) * tex_reverse_height;
            if (scale_.x < 0.0f)
                std::swap(su, eu);
            if (scale_.y < 0.0f)
                std::swap(sv, ev);
            const float half = graphic_base::half_pixel();
            const float x0 = position_value_.x + box.x - half;
            const float y0 = position_value_.y + box.y - half;
            const float x1 = x0 + box.width;
            const float y1 = y0 + box.height;
            pdt_vertex vertices[4];
            for (pdt_vertex &v : vertices) {
                v.position_value.z = depth_;
                v.diffuse_value = diffuse_color_;
            }
            vertices[0].position_value.x = x0;
            vertices[0].position_value.y = y0;
            vertices[0].tex_coord = texture_coordinate(su, sv);
            vertices[1].position_value.x = x1;
            vertices[1].position_value.y = y0;
            vertices[1].tex_coord = texture_coordinate(eu, sv);
            vertices[2].position_value.x = x0;
            vertices[2].position_value.y = y1;
            vertices[2].tex_coord = texture_coordinate(su, ev);
            vertices[3].position_value.x = x1;
            vertices[3].position_value.y = y1;
            vertices[3].tex_coord = texture_coordinate(eu, ev);
            gpu::pixel_shader *shader = shader_.is_null() ? nullptr : shader_->shader();
            if (graphic_base::set_pdt_stream(vertices, 4)) {
                graphic_base::set_default_index_buffer(graphic_base::DEFAULT_IB_FILL_RECT);
                STATEMANAGER.set_texture(0, texture->gpu_texture());
                graphic_texture *mask = shader && !mask_.is_null() ? mask_->texture_pointer() : nullptr;
                STATEMANAGER.set_texture(1, mask ? mask->gpu_texture() : nullptr);
                if (shader) {
                    gpu::pixel_shader_params params;
                    params.time = float(el_timer_get_m_sec() % 3600000) / 1000.0f;
                    params.width = box.width;
                    params.height = box.height;
                    graphic_base::device().set_pixel_shader(shader, params);
                }
                STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
                STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, 4, 0, 2);
                if (shader) {
                    graphic_base::device().set_pixel_shader(nullptr, {});
                    STATEMANAGER.set_texture(1, nullptr);
                }
            }
            return;
        }

        // Half an interface pixel back (the D3D9 convention) in framebuffer pixels, so the quad
        // covers its texels exactly at every interface scale.
        const float half = graphic_base::half_pixel();

        pdt_vertex vertices[4];
        vertices[0].position_value.x = position_value_.x - half;
        vertices[0].position_value.y = position_value_.y - half;
        vertices[0].position_value.z = depth_;
        vertices[0].tex_coord = texture_coordinate(su, sv);
        vertices[0].diffuse_value = diffuse_color_;

        vertices[1].position_value.x = position_value_.x - half;
        vertices[1].position_value.y = position_value_.y - half;
        vertices[1].position_value.z = depth_;
        vertices[1].tex_coord = texture_coordinate(eu, sv);
        vertices[1].diffuse_value = diffuse_color_;

        vertices[2].position_value.x = position_value_.x - half;
        vertices[2].position_value.y = position_value_.y - half;
        vertices[2].position_value.z = depth_;
        vertices[2].tex_coord = texture_coordinate(su, ev);
        vertices[2].diffuse_value = diffuse_color_;

        vertices[3].position_value.x = position_value_.x - half;
        vertices[3].position_value.y = position_value_.y - half;
        vertices[3].position_value.z = depth_;
        vertices[3].tex_coord = texture_coordinate(eu, ev);
        vertices[3].diffuse_value = diffuse_color_;

        if (0.0f == rotation_) {
            float fimg_width = float(image->width()) * scale_.x;
            float fimg_height = float(image->height()) * scale_.y;

            vertices[0].position_value.x -= rendering_rect_.left;
            vertices[0].position_value.y -= rendering_rect_.top;
            vertices[1].position_value.x += fimg_width + rendering_rect_.right;
            vertices[1].position_value.y -= rendering_rect_.top;
            vertices[2].position_value.x -= rendering_rect_.left;
            vertices[2].position_value.y += fimg_height + rendering_rect_.bottom;
            vertices[3].position_value.x += fimg_width + rendering_rect_.right;
            vertices[3].position_value.y += fimg_height + rendering_rect_.bottom;
            if ((0.0f < scale_.x && 0.0f > scale_.y) || (0.0f > scale_.x && 0.0f < scale_.y)) {
                STATEMANAGER.set_render_state(gpu::state::CULL_MODE, gpu::cull::CCW);
            }
        } else {
            float fimg_half_width = float(image->width()) / 2.0f * scale_.x;
            float fimg_half_height = float(image->height()) / 2.0f * scale_.y;

            for (int i = 0; i < 4; ++i) {
                vertices[i].position_value.x += origin_.x;
                vertices[i].position_value.y += origin_.y;
            }

            float radian = math::to_radian(rotation_);
            vertices[0].position_value.x += (-fimg_half_width * cosf(radian)) - (-fimg_half_height * sinf(radian));
            vertices[0].position_value.y += (-fimg_half_width * sinf(radian)) + (-fimg_half_height * cosf(radian));
            vertices[1].position_value.x += (+fimg_half_width * cosf(radian)) - (-fimg_half_height * sinf(radian));
            vertices[1].position_value.y += (+fimg_half_width * sinf(radian)) + (-fimg_half_height * cosf(radian));
            vertices[2].position_value.x += (-fimg_half_width * cosf(radian)) - (+fimg_half_height * sinf(radian));
            vertices[2].position_value.y += (-fimg_half_width * sinf(radian)) + (+fimg_half_height * cosf(radian));
            vertices[3].position_value.x += (+fimg_half_width * cosf(radian)) - (+fimg_half_height * sinf(radian));
            vertices[3].position_value.y += (+fimg_half_width * sinf(radian)) + (+fimg_half_height * cosf(radian));
        }

        switch (rendering_mode_value_) {
        case RENDERING_MODE_SCREEN:
        case RENDERING_MODE_COLOR_DODGE:
            STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::INV_DEST_COLOR);
            STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::ONE);
            break;
        case RENDERING_MODE_MODULATE:
            STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::ZERO);
            STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::SRC_COLOR);
            break;
        }

        // set_pdt_stream fails while the device is lost; drawing anyway crashes.
        if (graphic_base::set_pdt_stream(vertices, 4)) {
            graphic_base::set_default_index_buffer(graphic_base::DEFAULT_IB_FILL_RECT);

            STATEMANAGER.set_texture(0, texture->gpu_texture());
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, 4, 0, 2);
        }

        switch (rendering_mode_value_) {
        case RENDERING_MODE_SCREEN:
        case RENDERING_MODE_COLOR_DODGE:
        case RENDERING_MODE_MODULATE:
            STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
            STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
            break;
        }
        STATEMANAGER.set_render_state(gpu::state::CULL_MODE, gpu::cull::CW);
    }

    void graphic_expanded_image_instance::set_depth(float depth) {
        depth_ = depth;
    }

    void graphic_expanded_image_instance::set_origin() {
        set_origin(float(width()) / 2.0f, float(height()) / 2.0f);
    }

    void graphic_expanded_image_instance::set_origin(float fx, float fy) {
        origin_.x = fx;
        origin_.y = fy;
    }

    void graphic_expanded_image_instance::set_rotation(float rotation) {
        rotation_ = rotation;
    }

    void graphic_expanded_image_instance::set_scale(float fx, float fy) {
        scale_.x = fx;
        scale_.y = fy;
    }

    void graphic_expanded_image_instance::set_rendering_rect(float left, float top, float right, float bottom) {
        if (is_empty())
            return;

        auto width_value = float(width());
        auto height_value = float(height());

        rendering_rect_.left = width_value * left;
        rendering_rect_.top = height_value * top;
        rendering_rect_.right = width_value * right;
        rendering_rect_.bottom = height_value * bottom;
    }

    void graphic_expanded_image_instance::set_rendering_mode(int mode) {
        rendering_mode_value_ = mode;
    }

    void graphic_expanded_image_instance::set_shader(graphic_pixel_shader *shader, graphic_image *mask) {
        shader_.set_pointer(shader);
        mask_.set_pointer(shader ? mask : nullptr);
    }

    void graphic_expanded_image_instance::set_fit(int mode, float width, float height) {
        fit_mode_ = mode;
        fit_width_ = width;
        fit_height_ = height;
    }

    auto graphic_expanded_image_instance::type() -> uint32_t {
        static uint32_t type = get_crc32("CGraphicExpandedImageInstance", strlen("CGraphicExpandedImageInstance"));
        return (type);
    }

    void graphic_expanded_image_instance::on_set_image_pointer() {
        if (is_empty())
            return;

        set_origin(float(width()) / 2.0f, float(height()) / 2.0f);
    }

    auto graphic_expanded_image_instance::on_is_type(uint32_t type) -> bool {
        if (graphic_expanded_image_instance::type() == type)
            return true;

        return graphic_image_instance::is_type(type);
    }

    void graphic_expanded_image_instance::initialize() {
        rendering_mode_value_ = RENDERING_MODE_NORMAL;
        depth_ = 0.0f;
        origin_.x = origin_.y = 0.0f;
        scale_.x = scale_.y = 1.0f;
        rotation_ = 0.0f;
        rendering_rect_ = {};
        fit_mode_ = FIT_NONE;
        fit_width_ = fit_height_ = 0.0f;
        shader_.clear();
        mask_.clear();
    }

    void graphic_expanded_image_instance::destroy() {
        graphic_image_instance::destroy();
        initialize();
    }

    graphic_expanded_image_instance::graphic_expanded_image_instance() {
        initialize();
    }

    graphic_expanded_image_instance::~graphic_expanded_image_instance() {
        destroy();
    }
}
