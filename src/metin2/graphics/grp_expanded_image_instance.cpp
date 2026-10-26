#include "grp_expanded_image_instance.h"
#include <cmath>
#include <metin2/core/crc32.h>
#include <metin2/gpu/types.h>
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
