#include "python_graphic.h"
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/image_file.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>

namespace metin2::inline ui {

    void python_graphic::destroy() {
    }

    auto python_graphic::ortho_depth() -> float {
        return ortho_depth_;
    }

    void python_graphic::set_interface_render_state() {
        STATEMANAGER.set_transform(gpu::transform::PROJECTION, &identity_);
        STATEMANAGER.set_transform(gpu::transform::VIEW, &identity_);
        STATEMANAGER.set_transform(gpu::transform::WORLD, &identity_);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::MIN_FILTER, gpu::filter::NONE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::MAG_FILTER, gpu::filter::NONE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::MIP_FILTER, gpu::filter::NONE);

        STATEMANAGER.set_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.set_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.set_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);

        get_python_graphic().set_blend_operation();
        // The interface's logical size over the whole window; point sampling keeps the magnified
        // pixels crisp.
        get_python_graphic().set_ortho2d(static_cast<float>(width_ / interface_scale_), static_cast<float>(height_ / interface_scale_), ortho_depth());

        STATEMANAGER.set_render_state(gpu::state::LIGHTING, false);
    }

    void python_graphic::project_position_to_interface(float x, float y, float z, float *px, float *py, float *pz) {
        float depth = 0.0f;
        project_position(x, y, z, px, py, &depth);
        *px /= static_cast<float>(interface_scale_);
        *py /= static_cast<float>(interface_scale_);
        if (pz)
            *pz = depth;
    }

    void python_graphic::set_game_render_state() {
        STATEMANAGER.set_best_filtering(0);

        STATEMANAGER.set_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);
        STATEMANAGER.set_render_state(gpu::state::LIGHTING, true);
    }

    void python_graphic::set_cursor_position(int x, int y) {
        screen::set_cursor_position(x * interface_scale_, y * interface_scale_, width_, height_);
    }

    void python_graphic::set_omni_light() {
        gpu::material material;
        material.ambient = math::color(0.3f, 0.3f, 0.3f, 1.0f);
        material.diffuse = math::color(1.0f, 1.0f, 1.0f, 1.0f);
        material.emissive = math::color(0.1f, 0.1f, 0.1f, 1.0f);
        STATEMANAGER.set_material(&material);

        gpu::light light;
        light.type = gpu::light_type::SPOT;
        light.position = math::vector3(50.0f, 150.0f, 350.0f);
        light.direction = math::vector3(-0.15f, -0.3f, -0.9f);
        light.theta = math::to_radian(30.0f);
        light.phi = math::to_radian(45.0f);
        light.falloff = 1.0f;
        light.attenuation0 = 0.0f;
        light.attenuation1 = 0.005f;
        light.attenuation2 = 0.0f;
        light.diffuse.r = 1.0f;
        light.diffuse.g = 1.0f;
        light.diffuse.b = 1.0f;
        light.diffuse.a = 1.0f;
        light.ambient.r = 1.0f;
        light.ambient.g = 1.0f;
        light.ambient.b = 1.0f;
        light.ambient.a = 1.0f;
        light.range = 500.0f;
        STATEMANAGER.set_light(0, &light);
        STATEMANAGER.enable_light(0, true);

        light.type = gpu::light_type::POINT;
        light.position = math::vector3(0.0f, 200.0f, 200.0f);
        light.attenuation0 = 0.1f;
        light.attenuation1 = 0.01f;
        light.attenuation2 = 0.0f;
        STATEMANAGER.set_light(1, &light);
        STATEMANAGER.enable_light(1, true);
    }

    void python_graphic::set_viewport(float fx, float fy, float width_value, float height) {
        backup_viewport_ = device_->get_viewport();
        device_->set_viewport({static_cast<uint32_t>(fx), static_cast<uint32_t>(fy), static_cast<uint32_t>(width_value), static_cast<uint32_t>(height), 0.0f, 1.0f});
    }

    void python_graphic::restore_viewport() {
        device_->set_viewport(backup_viewport_);
    }

    auto python_graphic::save_screen_shot(const char *file_name) -> bool {
        std::vector<uint8_t> bgra;
        if (!device_->read_back_buffer(bgra)) {
            metin2::log::error("Cannot read the back buffer");
            return false;
        }

        const uint32_t width_value = device_->back_buffer_width();
        const uint32_t height = device_->back_buffer_height();
        std::vector<uint8_t> buffer_value(size_t(width_value) * height * 3);
        for (size_t i = 0; i < size_t(width_value) * height; ++i) {
            buffer_value[i * 3 + 0] = bgra[i * 4 + 2];
            buffer_value[i * 3 + 1] = bgra[i * 4 + 1];
            buffer_value[i * 3 + 2] = bgra[i * 4 + 0];
        }

        bool saved = save_jpeg_file(file_name, static_cast<int>(width_value), static_cast<int>(height), buffer_value.data(), 100);

        if (saved == false) {
            metin2::log::error("Cannot save JPEG file {} ({} by {})", file_name, width_value, height);
            return false;
        }

        return true;
    }

    void python_graphic::push_state() {
        state cur_state;

        cur_state.proj = proj_;
        cur_state.view = view_;

        state_stack_.push(cur_state);
    }

    void python_graphic::pop_state() {
        if (state_stack_.empty()) {
            assert(!"PythonGraphic::PopState StateStack is EMPTY");
            return;
        }

        state &state_value = state_stack_.top();

        proj_ = state_value.proj;
        view_ = state_value.view;

        update_pipe_line_matrix();

        state_stack_.pop();
    }

    void python_graphic::render_cool_time_box(float fx_center, float fy_center, float radius, float time) {
        if (time >= 1.0f)
            return;

        time = std::max(0.0f, time);

        static math::color color = math::color(0.0f, 0.0f, 0.0f, 0.5f);
        static math::vector2 box_pos[8] =
            {
                math::vector2(-1.0f, -1.0f),
                math::vector2(-1.0f, 0.0f),
                math::vector2(-1.0f, +1.0f),
                math::vector2(0.0f, +1.0f),
                math::vector2(+1.0f, +1.0f),
                math::vector2(+1.0f, 0.0f),
                math::vector2(+1.0f, -1.0f),
                math::vector2(0.0f, -1.0f),
        };

        int tri_count = int(8 - 8.0f * time);
        float last_percentage = (8 - 8.0f * time) - tri_count;

        std::vector<pdt_vertex> vertices;
        pdt_vertex vertex;
        vertex.position_value.x = fx_center;
        vertex.position_value.y = fy_center;
        vertex.position_value.z = 0.0f;
        vertex.diffuse_value = color;
        vertex.tex_coord.x = 0.0f;
        vertex.tex_coord.x = 0.0f;
        vertices.push_back(vertex);
        vertex.position_value.x = fx_center;
        vertex.position_value.y = fy_center - radius;
        vertex.position_value.z = 0.0f;
        vertex.diffuse_value = color;
        vertex.tex_coord.x = 0.0f;
        vertex.tex_coord.x = 0.0f;
        vertices.push_back(vertex);

        for (int j = 0; j < tri_count; ++j) {
            vertex.position_value.x = fx_center + box_pos[j].x * radius;
            vertex.position_value.y = fy_center + box_pos[j].y * radius;
            vertices.push_back(vertex);
        }

        if (last_percentage > 0.0f) {
            math::vector2 *pos;
            math::vector2 *last_pos;

            assert((tri_count - 1 + 8) % 8 >= 0 && (tri_count - 1 + 8) % 8 < 8);
            assert((tri_count + 8) % 8 >= 0 && (tri_count + 8) % 8 < 8);
            last_pos = &box_pos[(tri_count - 1 + 8) % 8];
            pos = &box_pos[(tri_count + 8) % 8];

            vertex.position_value.x = fx_center + ((pos->x - last_pos->x) * last_percentage + last_pos->x) * radius;
            vertex.position_value.y = fy_center + ((pos->y - last_pos->y) * last_percentage + last_pos->y) * radius;
            vertices.push_back(vertex);
            ++tri_count;
        }

        if (vertices.empty())
            return;

        if (set_pdt_stream(&vertices[0], vertices.size())) {
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::DIFFUSE);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::DIFFUSE);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.set_texture(0, nullptr);
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_FAN, 0, tri_count);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG1);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_OP);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_OP);
        }
    }

    auto python_graphic::generate_color(float r, float g, float b, float a) -> int32_t {
        return get_color(r, g, b, a);
    }

    python_graphic::python_graphic() {
        light_color_ = get_color(1.0f, 1.0f, 1.0f);
        dark_color_ = get_color(0.0f, 0.0f, 0.0f);

        ortho_depth_ = 1000.0f;
    }

    python_graphic::~python_graphic() {
        metin2::log::debug("Python graphic destroyed");
    }
}
