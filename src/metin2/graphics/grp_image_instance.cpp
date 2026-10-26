#include "grp_image_instance.h"
#include "grp_nine_patch.h"
#include "state_manager.h"

#include <metin2/core/crc32.h>
#include <metin2/gpu/types.h>

#include <array>

namespace metin2::inline graphics {

    dynamic_pool<graphic_image_instance> graphic_image_instance::pool_value;

    void graphic_image_instance::create_system(uint32_t capacity) {
        pool_value.create(capacity);
    }

    void graphic_image_instance::destroy_system() {
        pool_value.destroy();
    }

    auto graphic_image_instance::new_value() -> graphic_image_instance * {
        return pool_value.alloc();
    }

    void graphic_image_instance::delete_value(graphic_image_instance *img_inst) {
        img_inst->destroy();
        pool_value.free(img_inst);
    }

    void graphic_image_instance::render() {
        if (is_empty())
            return;

        assert(!is_empty());

        on_render();
    }

    void graphic_image_instance::on_render() {
        if (is_nine_patch()) {
            render_nine_patch();
            return;
        }

        graphic_image *image = ro_image_.pointer();
        graphic_texture *texture = image->texture_pointer();

        float fimg_width = image->width();
        float fimg_height = image->height();

        const rectangle &rect_value = image->rect_reference();
        float tex_reverse_width = 1.0f / float(texture->width());
        float tex_reverse_height = 1.0f / float(texture->height());
        float su = rect_value.left * tex_reverse_width;
        float sv = rect_value.top * tex_reverse_height;
        float eu = (rect_value.left + (rect_value.right - rect_value.left)) * tex_reverse_width;
        float ev = (rect_value.top + (rect_value.bottom - rect_value.top)) * tex_reverse_height;

        // Half an interface pixel back (the D3D9 convention) in framebuffer pixels, so the quad
        // covers its texels exactly at every interface scale.
        const float half = graphic_base::half_pixel();

        pdt_vertex vertices[4];
        vertices[0].position_value.x = position_value_.x - half;
        vertices[0].position_value.y = position_value_.y - half;
        vertices[0].position_value.z = 0.0f;
        vertices[0].tex_coord = texture_coordinate(su, sv);
        vertices[0].diffuse_value = diffuse_color_;

        vertices[1].position_value.x = position_value_.x + fimg_width - half;
        vertices[1].position_value.y = position_value_.y - half;
        vertices[1].position_value.z = 0.0f;
        vertices[1].tex_coord = texture_coordinate(eu, sv);
        vertices[1].diffuse_value = diffuse_color_;

        vertices[2].position_value.x = position_value_.x - half;
        vertices[2].position_value.y = position_value_.y + fimg_height - half;
        vertices[2].position_value.z = 0.0f;
        vertices[2].tex_coord = texture_coordinate(su, ev);
        vertices[2].diffuse_value = diffuse_color_;

        vertices[3].position_value.x = position_value_.x + fimg_width - half;
        vertices[3].position_value.y = position_value_.y + fimg_height - half;
        vertices[3].position_value.z = 0.0f;
        vertices[3].tex_coord = texture_coordinate(eu, ev);
        vertices[3].diffuse_value = diffuse_color_;

        // set_pdt_stream fails while the device is lost; drawing anyway crashes.
        if (graphic_base::set_pdt_stream(vertices, 4)) {
            graphic_base::set_default_index_buffer(graphic_base::DEFAULT_IB_FILL_RECT);

            STATEMANAGER.set_texture(0, texture->gpu_texture());
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, 4, 0, 2);
        }
    }

    void graphic_image_instance::render_nine_patch() {
        auto *patch = static_cast<graphic_nine_patch *>(ro_image_.pointer());
        graphic_texture *texture = patch->texture_pointer();
        const rectangle &region = patch->rect_reference();
        const graphic_nine_patch::margins &margins = patch->patch_margins();

        // Cut positions in interface pixels (relative to the position) and the matching texels.
        const std::array<float, 4> xs = graphic_nine_patch::cuts(margins.left, margins.right, width());
        const std::array<float, 4> ys = graphic_nine_patch::cuts(margins.top, margins.bottom, height());
        const float tex_reverse_width = 1.0f / float(texture->width());
        const float tex_reverse_height = 1.0f / float(texture->height());
        const std::array<float, 4> us = {float(region.left) * tex_reverse_width, float(region.left + margins.left) * tex_reverse_width,
            float(region.right - margins.right) * tex_reverse_width, float(region.right) * tex_reverse_width};
        const std::array<float, 4> vs = {float(region.top) * tex_reverse_height, float(region.top + margins.top) * tex_reverse_height,
            float(region.bottom - margins.bottom) * tex_reverse_height, float(region.bottom) * tex_reverse_height};

        // Half an interface pixel back (the D3D9 convention) in framebuffer pixels, as for a plain
        // image; the cuts are whole pixels, so every corner texel lands on one framebuffer pixel.
        const float half = graphic_base::half_pixel();
        const float x0 = position_value_.x - half;
        const float y0 = position_value_.y - half;

        // Two triangles per patch (wound as the text quads are, the other way round is culled),
        // skipping the ones a margin of zero or a small size leaves empty.
        std::array<pdt_vertex, 9 * 6> vertices;
        size_t count = 0;
        auto vertex = [&](float x, float y, float u, float v) -> void {
            pdt_vertex &out = vertices[count++];
            out.position_value = {x0 + x, y0 + y, 0.0f};
            out.tex_coord = texture_coordinate(u, v);
            out.diffuse_value = diffuse_color_;
        };
        for (size_t row = 0; row < 3; ++row) {
            for (size_t column = 0; column < 3; ++column) {
                if (xs[column + 1] <= xs[column] || ys[row + 1] <= ys[row]) {
                    continue;
                }
                vertex(xs[column], ys[row], us[column], vs[row]);
                vertex(xs[column], ys[row + 1], us[column], vs[row + 1]);
                vertex(xs[column + 1], ys[row], us[column + 1], vs[row]);
                vertex(xs[column + 1], ys[row], us[column + 1], vs[row]);
                vertex(xs[column], ys[row + 1], us[column], vs[row + 1]);
                vertex(xs[column + 1], ys[row + 1], us[column + 1], vs[row + 1]);
            }
        }
        if (count == 0) {
            return;
        }

        STATEMANAGER.set_texture(0, texture->gpu_texture());
        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
        STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_LIST, static_cast<uint32_t>(count / 3), vertices.data(), sizeof(pdt_vertex));
    }

    auto graphic_image_instance::texture_reference() const -> const graphic_texture & {
        return ro_image_->texture_reference();
    }

    auto graphic_image_instance::texture_pointer() -> graphic_texture * {
        graphic_image *image = ro_image_.pointer();
        return image ? image->texture_pointer() : nullptr;
    }

    auto graphic_image_instance::graphic_image_pointer() -> graphic_image * {
        return ro_image_.pointer();
    }

    auto graphic_image_instance::width() -> int {
        if (is_empty())
            return 0;

        return width_ > 0 && is_nine_patch() ? width_ : ro_image_->width();
    }

    auto graphic_image_instance::height() -> int {
        if (is_empty())
            return 0;

        return height_ > 0 && is_nine_patch() ? height_ : ro_image_->height();
    }

    void graphic_image_instance::set_size(int width, int height) {
        width_ = width;
        height_ = height;
    }

    auto graphic_image_instance::is_nine_patch() const -> bool {
        return !ro_image_.is_null() && ro_image_->is_type(graphic_nine_patch::type());
    }

    void graphic_image_instance::set_diffuse_color(float fr, float fg, float fb, float fa) {
        diffuse_color_.r = fr;
        diffuse_color_.g = fg;
        diffuse_color_.b = fb;
        diffuse_color_.a = fa;
    }
    void graphic_image_instance::set_position(float fx, float fy) {
        position_value_.x = fx;
        position_value_.y = fy;
    }

    void graphic_image_instance::set_image_pointer(graphic_image *image) {
        ro_image_.set_pointer(image);

        on_set_image_pointer();
    }

    auto graphic_image_instance::is_empty() const -> bool {
        if (!ro_image_.is_null() && !ro_image_->is_empty())
            return false;

        return true;
    }

    auto graphic_image_instance::operator==(const graphic_image_instance &rhs) const -> bool {
        return (ro_image_.pointer() == rhs.ro_image_.pointer());
    }

    auto graphic_image_instance::type() -> uint32_t {
        static uint32_t type = get_crc32("CGraphicImageInstance", strlen("CGraphicImageInstance"));
        return (type);
    }

    auto graphic_image_instance::is_type(uint32_t type) -> bool {
        return on_is_type(type);
    }

    auto graphic_image_instance::on_is_type(uint32_t type) -> bool {
        if (graphic_image_instance::type() == type)
            return true;

        return false;
    }

    void graphic_image_instance::on_set_image_pointer() {
    }

    void graphic_image_instance::initialize() {
        diffuse_color_.r = diffuse_color_.g = diffuse_color_.b = diffuse_color_.a = 1.0f;
        position_value_.x = position_value_.y = 0.0f;
        width_ = height_ = 0;
    }

    void graphic_image_instance::destroy() {
        ro_image_.set_pointer(nullptr); // the CRef reference count must drop.
        initialize();
    }

    graphic_image_instance::graphic_image_instance() {
        initialize();
    }

    graphic_image_instance::~graphic_image_instance() {
        destroy();
    }
}
