#include "grp_mark_instance.h"
#include <metin2/log/log.h>
#include "resource_manager.h"
#include "state_manager.h"

#include <metin2/core/crc32.h>
#include <metin2/gpu/types.h>

namespace metin2::inline graphics {

    dynamic_pool<graphic_mark_instance> graphic_mark_instance::pool_value;

    void graphic_mark_instance::set_image_file_name(const char *file_name) {
        image_file_name_ = file_name;
    }

    auto graphic_mark_instance::image_file_name() -> const std::string & {
        return image_file_name_;
    }

    void graphic_mark_instance::create_system(uint32_t capacity) {
        pool_value.create(capacity);
    }

    void graphic_mark_instance::destroy_system() {
        pool_value.destroy();
    }

    auto graphic_mark_instance::new_value() -> graphic_mark_instance * {
        return pool_value.alloc();
    }

    void graphic_mark_instance::delete_value(graphic_mark_instance *img_inst) {
        img_inst->destroy();
        pool_value.free(img_inst);
    }

    void graphic_mark_instance::render() {
        if (is_empty())
            return;

        assert(!is_empty());

        on_render();
    }

    void graphic_mark_instance::on_render() {
        graphic_image *image = ro_image_.pointer();
        graphic_texture *texture = image->texture_pointer();

        uint32_t col_count = image->width() / MARK_WIDTH;

        if (col_count == 0)
            return;

        uint32_t col = index_value_ % col_count;
        uint32_t row = index_value_ / col_count;

        rectangle rect_value;
        rect_value.left = col * MARK_WIDTH;
        rect_value.top = row * MARK_HEIGHT;
        rect_value.right = rect_value.left + MARK_WIDTH;
        rect_value.bottom = rect_value.top + MARK_HEIGHT;

        float tex_reverse_width = 1.0f / float(texture->width());
        float tex_reverse_height = 1.0f / float(texture->height());
        float su = rect_value.left * tex_reverse_width;
        float sv = rect_value.top * tex_reverse_height;
        float eu = rect_value.right * tex_reverse_width;
        float ev = rect_value.bottom * tex_reverse_height;

        float render_width = static_cast<float>(MARK_WIDTH) * scale_;
        float render_height = static_cast<float>(MARK_HEIGHT) * scale_;

        pdt_vertex vertices[4];
        vertices[0].position_value.x = position_value_.x - 0.5f;
        vertices[0].position_value.y = position_value_.y - 0.5f;
        vertices[0].position_value.z = 0.0f;
        vertices[0].tex_coord = texture_coordinate(su, sv);
        vertices[0].diffuse_value = diffuse_color_;

        vertices[1].position_value.x = position_value_.x + render_width - 0.5f;
        vertices[1].position_value.y = position_value_.y - 0.5f;
        vertices[1].position_value.z = 0.0f;
        vertices[1].tex_coord = texture_coordinate(eu, sv);
        vertices[1].diffuse_value = diffuse_color_;

        vertices[2].position_value.x = position_value_.x - 0.5f;
        vertices[2].position_value.y = position_value_.y + render_height - 0.5f;
        vertices[2].position_value.z = 0.0f;
        vertices[2].tex_coord = texture_coordinate(su, ev);
        vertices[2].diffuse_value = diffuse_color_;

        vertices[3].position_value.x = position_value_.x + render_width - 0.5f;
        vertices[3].position_value.y = position_value_.y + render_height - 0.5f;
        vertices[3].position_value.z = 0.0f;
        vertices[3].tex_coord = texture_coordinate(eu, ev);
        vertices[3].diffuse_value = diffuse_color_;

        if (graphic_base::set_pdt_stream(vertices, 4)) {
            graphic_base::set_default_index_buffer(graphic_base::DEFAULT_IB_FILL_RECT);

            STATEMANAGER.set_texture(0, texture->gpu_texture());
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, 4, 0, 2);
        }
    }

    auto graphic_mark_instance::texture_reference() const -> const graphic_texture & {
        return ro_image_->texture_reference();
    }

    auto graphic_mark_instance::texture_pointer() -> graphic_texture * {
        return ro_image_->texture_pointer();
    }

    auto graphic_mark_instance::graphic_image_pointer() -> graphic_image * {
        return ro_image_.pointer();
    }

    void graphic_mark_instance::set_scale(float scale) {
        scale_ = scale;
    }

    void graphic_mark_instance::set_index(uint32_t index) {
        index_value_ = index;
    }

    auto graphic_mark_instance::width() -> int {
        if (is_empty())
            return 0;

        return 16;
    }

    auto graphic_mark_instance::height() -> int {
        if (is_empty())
            return 0;

        return 12;
    }

    void graphic_mark_instance::set_diffuse_color(float fr, float fg, float fb, float fa) {
        diffuse_color_.r = fr;
        diffuse_color_.g = fg;
        diffuse_color_.b = fb;
        diffuse_color_.a = fa;
    }

    void graphic_mark_instance::set_position(float fx, float fy) {
        position_value_.x = fx;
        position_value_.y = fy;
    }

    void graphic_mark_instance::load() {
        if (image_file_name().empty())
            return;

        resource *resource = get_resource_manager().resource_pointer(image_file_name().c_str());

        if (!resource) {
            metin2::log::error("Mark image {} not found", image_file_name().c_str());
            return;
        }

        if (resource->is_type(graphic_image::type()))
            set_image_pointer(static_cast<graphic_image *>(resource));
    }

    void graphic_mark_instance::set_image_pointer(graphic_image *image) {
        ro_image_.set_pointer(image);

        on_set_image_pointer();
    }

    auto graphic_mark_instance::is_empty() const -> bool {
        if (!ro_image_.is_null() && !ro_image_->is_empty())
            return false;

        return true;
    }

    auto graphic_mark_instance::operator==(const graphic_mark_instance &rhs) const -> bool {
        return (ro_image_.pointer() == rhs.ro_image_.pointer());
    }

    auto graphic_mark_instance::type() -> uint32_t {
        static uint32_t type = get_crc32("CGraphicMarkInstance", strlen("CGraphicMarkInstance"));
        return (type);
    }

    auto graphic_mark_instance::is_type(uint32_t type) -> bool {
        return on_is_type(type);
    }

    auto graphic_mark_instance::on_is_type(uint32_t type) -> bool {
        if (graphic_mark_instance::type() == type)
            return true;

        return false;
    }

    void graphic_mark_instance::on_set_image_pointer() {
    }

    void graphic_mark_instance::initialize() {
        diffuse_color_.r = diffuse_color_.g = diffuse_color_.b = diffuse_color_.a = 1.0f;
        position_value_.x = position_value_.y = 0.0f;
        index_value_ = 0;
        scale_ = 1.0f;
    }

    void graphic_mark_instance::destroy() {
        ro_image_.set_pointer(nullptr); // the CRef reference count must drop.
        initialize();
    }

    graphic_mark_instance::graphic_mark_instance() {
        initialize();
    }

    graphic_mark_instance::~graphic_mark_instance() {
        destroy();
    }
}
