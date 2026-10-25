//	CLensFlare Class
//	(c) 2003 IDV, Inc.
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//		IDV, Inc.
//		1233 Washington St. Suite 610
//		Columbia, SC 29201
//		Voice: (803) 799-1699
//		Fax:   (803) 931-0320
//		Web:   http://www.idvinc.com

#include "lens_flare.h"
#include "camera.h"
#include "resource_manager.h"
#include "state_manager.h"

#include <algorithm>
#include <math.h>
#include <metin2/core/timer.h>
#include <metin2/gpu/types.h>

namespace metin2::inline graphics {
    using namespace std;

    static string files[] =
        {
            "flare2.png",
            "flare1.png",
            "flare2.png",
            "flare1.png",
            "flare6.png",
            "flare4.png",
            "flare2.png",
            "flare3.png",
            ""};
    static float flare_positions[] =
        {
            -0.55f,
            -0.5f,
            -0.45f,
            0.2f,
            0.3f,
            0.95f,
            0.9f,
            1.0f};
    static float width[] =
        {
            20.0f,
            32.0f,
            20.0f,
            32.0f,
            100.0f,
            32.0f,
            20.0f,
            250.0f};

    static float colors[][4] =
        {
            {1.0f, 1.0f, 0.0f, 1.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
            {0.0f, 1.0f, 0.0f, 0.8f},
            {0.3f, 0.5f, 1.0f, 0.9f},
            {0.3f, 0.5f, 1.0f, 0.6f},
            {1.0f, 0.6f, 0.9f, 0.4f},
            {1.0f, 0.0f, 0.0f, 0.5f},
            {1.0f, 0.6f, 0.3f, 0.4f}
    };

    lens_flare::lens_flare() : before_bright_(0.0f),
                               after_bright_(0.0f),
                               flare_visible_(false),
                               draw_flare_(true),
                               draw_bright_screen_(true),
                               sun_size_(0),
                               enabled_(true),
                               show_main_flare_(true),
                               max_brightness_(1.0f) {
        color_value_[0] = color_value_[1] = color_value_[2] = 1.0f;
    }

    lens_flare::~lens_flare() {
        if (!device_)
            return;
        for (occlusion_probe &probe : probes_) {
            STATEMANAGER.destroy_occlusion_query(probe.total_query);
            STATEMANAGER.destroy_occlusion_query(probe.visible_query);
        }
    }

    auto lens_flare::interpolate(float start, float end, float percent) -> float {
        return start + (end - start) * percent;
    }

    void lens_flare::compute(const math::vector3 &light_direction) {
        float sun_pos[3];

        math::vector3 target = get_camera_manager().current_camera()->target();

        sun_pos[0] = target.x - light_direction.x * 99999999.0f;
        sun_pos[1] = target.y - light_direction.y * 99999999.0f;
        sun_pos[2] = target.z - light_direction.z * 99999999.0f;

        float x, y;
        project_position(sun_pos[0], sun_pos[1], sun_pos[2], &x, &y);

        set_flare_location(x, y);

        float sun_vector_magnitude = sqrtf(sun_pos[0] * sun_pos[0] +
                                           sun_pos[1] * sun_pos[1] +
                                           sun_pos[2] * sun_pos[2]);
        float sun_vector[3];
        sun_vector[0] = -sun_pos[0] / sun_vector_magnitude;
        sun_vector[1] = -sun_pos[1] / sun_vector_magnitude;
        sun_vector[2] = -sun_pos[2] / sun_vector_magnitude;

        float camera_direction[3];
        camera_direction[0] = view_._13;
        camera_direction[1] = view_._23;
        camera_direction[2] = view_._33;

        float dot_product =
            (sun_vector[0] * camera_direction[0]) +
            (sun_vector[1] * camera_direction[1]) +
            (sun_vector[2] * camera_direction[2]);

        if (acosf(dot_product) < 0.5f * math::PI)
            set_visible(true);
        else
            set_visible(false);

        x /= viewport_.width;
        y /= viewport_.height;

        float distance = sqrtf(((0.5f - x) * (0.5f - x)) + ((0.5f - y) * (0.5f - y)));
        float before_bright = interpolate(0.0f, c_fHalfMaxBright, 1.0f - (distance * c_fDistanceScale));
        float after_bright = interpolate(0.0f, 1.0f, 1.0f - (distance * c_fDistanceScale));

        set_brightnesses(before_bright, after_bright);
    }

    void lens_flare::draw_before_flare() {
        if (!flare_visible_ || !enabled_ || !show_main_flare_)
            return;

        if (sun_flare_image_instance_.is_empty())
            return;

        math::matrix proj;
        proj = math::matrix_ortho_off_center_rh(0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f);
        STATEMANAGER.save_transform(gpu::transform::PROJECTION, &proj);
        STATEMANAGER.save_transform(gpu::transform::VIEW, &identity_);

        math::matrix world;
        world = math::matrix_translation(flare_pos_[0], flare_pos_[1], 0.0f);
        STATEMANAGER.set_transform(gpu::transform::WORLD, &world);

        STATEMANAGER.save_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.save_render_state(gpu::state::Z_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
        STATEMANAGER.save_render_state(gpu::state::SHADE_MODE, gpu::shade::FLAT);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);
        STATEMANAGER.save_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);
        float aspect_ratio = viewport_.width / float(viewport_.height);
        float height = sun_size_ * aspect_ratio;
        math::color color(1.0f, 1.0f, 1.0f, 1.0f);

        vertex vertices[4];
        vertices[0].x = -sun_size_;
        vertices[0].y = -height;
        vertices[0].z = 0.0f;
        vertices[0].color_value = color;
        vertices[0].u = 0.0f;
        vertices[0].v = 0.0f;

        vertices[1].x = -sun_size_;
        vertices[1].y = height;
        vertices[1].z = 0.0f;
        vertices[1].color_value = color;
        vertices[1].u = 0.0f;
        vertices[1].v = 1.0f;

        vertices[2].x = sun_size_;
        vertices[2].y = -height;
        vertices[2].z = 0.0f;
        vertices[2].color_value = color;
        vertices[2].u = 1.0f;
        vertices[2].v = 0.0f;

        vertices[3].x = sun_size_;
        vertices[3].y = height;
        vertices[3].z = 0.0f;
        vertices[3].color_value = color;
        vertices[3].u = 1.0f;
        vertices[3].v = 1.0f;

        STATEMANAGER.set_texture(0, sun_flare_image_instance_.texture_pointer()->gpu_texture());
        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);

        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
        STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, vertices, sizeof(vertex));

        STATEMANAGER.restore_render_state(gpu::state::LIGHTING);
        STATEMANAGER.restore_render_state(gpu::state::Z_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
        STATEMANAGER.restore_render_state(gpu::state::SHADE_MODE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::SRC_BLEND);
        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);

        STATEMANAGER.restore_transform(gpu::transform::VIEW);
        STATEMANAGER.restore_transform(gpu::transform::PROJECTION);
    }

    void lens_flare::draw_after_flare() {
        if (enabled_ && after_bright_ != 0.0f && draw_bright_screen_) {
            set_diffuse_color(color_value_[0], color_value_[1], color_value_[2], after_bright_);
            render_bar2d(0.0f, 0.0f, 1024.0f, 1024.0f);
        }
    }

    void lens_flare::set_main_flare(string sun_file, float sun_size) {
        if (enabled_ && show_main_flare_) {
            sun_size_ = sun_size;
            resource *resource = get_resource_manager().resource_pointer(sun_file.c_str());

            if (!resource->is_type(graphic_image::type()))
                assert(false);

            sun_flare_image_instance_.set_image_pointer(static_cast<graphic_image *>(resource));
        }
    }

    void lens_flare::draw_flare() {
        if (enabled_ && flare_visible_ && draw_flare_ && after_bright_ != 0.0f) {
            STATEMANAGER.save_render_state(gpu::state::LIGHTING, false);
            STATEMANAGER.save_render_state(gpu::state::Z_ENABLE, false);
            STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
            STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, false);
            STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, true);

            math::matrix proj;
            proj = math::matrix_ortho_off_center_rh(0.0f, viewport_.width, viewport_.height, 0.0f, -1.0f, 1.0f);
            STATEMANAGER.save_transform(gpu::transform::PROJECTION, &proj);
            STATEMANAGER.save_transform(gpu::transform::VIEW, &identity_);

            STATEMANAGER.set_transform(gpu::transform::WORLD, &identity_);

            draw_after_flare();

            flare_value_.draw(after_bright_,
                viewport_.width,
                viewport_.height,
                static_cast<int>(flare_win_pos_[0]),
                static_cast<int>(flare_win_pos_[1]));

            STATEMANAGER.restore_render_state(gpu::state::LIGHTING);
            STATEMANAGER.restore_render_state(gpu::state::Z_ENABLE);
            STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
            STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
            STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);

            STATEMANAGER.restore_transform(gpu::transform::PROJECTION);
            STATEMANAGER.restore_transform(gpu::transform::VIEW);
        }
    }

    void lens_flare::characterize_flare(bool enabled, bool show_main_flare, float max_brightness, const math::color &color) {
        enabled_ = enabled;
        show_main_flare_ = show_main_flare;
        max_brightness_ = max_brightness;

        color_value_[0] = color.r;
        color_value_[1] = color.g;
        color_value_[2] = color.b;
    }

    void lens_flare::initialize(std::string path) {
        if (enabled_)
            flare_value_.init(path);
    }

    void lens_flare::set_flare_location(double x, double y) {
        if (enabled_) {
            flare_win_pos_[0] = float(x);
            flare_win_pos_[1] = float(y);

            flare_pos_[0] = float(x) / viewport_.width;
            flare_pos_[1] = float(y) / viewport_.height;
        }
    }

    void lens_flare::set_brightnesses(float before_bright, float after_bright) {
        if (enabled_) {
            before_bright_ = before_bright;
            after_bright_ = after_bright;

            clamp_brightness();
        }
    }

    void lens_flare::adjust_brightness() {
        if (!enabled_)
            return;
        if (flare_visible_)
            measure_occlusion();
        after_bright_ *= visibility_;
    }

    void lens_flare::measure_occlusion() {
        occlusion_probe &done = probes_[probe_index_ ^ 1];
        if (done.pending) {
            uint32_t total = 0;
            uint32_t visible = 0;
            if (STATEMANAGER.occlusion_query_result(done.total_query, total) && STATEMANAGER.occlusion_query_result(done.visible_query, visible)) {
                done.pending = false;
                // A probe clipped away entirely says nothing, so the last value stays.
                if (total > 0) {
                    const float target = static_cast<float>(visible) / static_cast<float>(total);
                    visibility_ += (target - visibility_) * std::min(1.0f, get_timer().elapsed_second() * 10.0f);
                }
            }
        }

        occlusion_probe &next = probes_[probe_index_];
        if (next.pending)
            return;
        if (!next.total_query) {
            next.total_query = STATEMANAGER.create_occlusion_query();
            next.visible_query = STATEMANAGER.create_occlusion_query();
        }

        math::matrix proj;
        proj = math::matrix_ortho_off_center_rh(0.0f, viewport_.width, viewport_.height, 0.0f, -1.0f, 1.0f);
        STATEMANAGER.save_transform(gpu::transform::PROJECTION, &proj);
        STATEMANAGER.save_transform(gpu::transform::VIEW, &identity_);
        STATEMANAGER.set_transform(gpu::transform::WORLD, &identity_);

        STATEMANAGER.save_render_state(gpu::state::COLOR_WRITE_ENABLE, 0);
        STATEMANAGER.save_render_state(gpu::state::Z_WRITE_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::Z_FUNC, gpu::compare::LESS_EQUAL);
        STATEMANAGER.save_render_state(gpu::state::LIGHTING, false);
        STATEMANAGER.save_render_state(gpu::state::FOG_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::CULL_MODE, gpu::cull::NONE);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_TEST_ENABLE, false);
        STATEMANAGER.save_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);
        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);

        STATEMANAGER.save_render_state(gpu::state::Z_ENABLE, false);
        STATEMANAGER.begin_occlusion_query(next.total_query);
        draw_occlusion_probe();
        STATEMANAGER.end_occlusion_query();
        STATEMANAGER.set_render_state(gpu::state::Z_ENABLE, true);
        STATEMANAGER.begin_occlusion_query(next.visible_query);
        draw_occlusion_probe();
        STATEMANAGER.end_occlusion_query();
        STATEMANAGER.restore_render_state(gpu::state::Z_ENABLE);

        STATEMANAGER.restore_render_state(gpu::state::ALPHA_BLEND_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::ALPHA_TEST_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::CULL_MODE);
        STATEMANAGER.restore_render_state(gpu::state::FOG_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::LIGHTING);
        STATEMANAGER.restore_render_state(gpu::state::Z_FUNC);
        STATEMANAGER.restore_render_state(gpu::state::Z_WRITE_ENABLE);
        STATEMANAGER.restore_render_state(gpu::state::COLOR_WRITE_ENABLE);
        STATEMANAGER.restore_transform(gpu::transform::VIEW);
        STATEMANAGER.restore_transform(gpu::transform::PROJECTION);

        next.pending = true;
        probe_index_ ^= 1;
    }

    void lens_flare::draw_occlusion_probe() {
        const float half = c_nDepthTestDimension * 0.5f;
        const float x = flare_win_pos_[0];
        const float y = flare_win_pos_[1];
        // Just inside the far plane of the orthographic projection (depth 0.9999), so everything the
        // scene drew in front of its own far plane covers the probe.
        const float z = -0.9998f;
        const vertex vertices[4] = {
            {x - half, y - half, z, 0xffffffff, 0.0f, 0.0f},
            {x - half, y + half, z, 0xffffffff, 0.0f, 1.0f},
            {x + half, y - half, z, 0xffffffff, 1.0f, 0.0f},
            {x + half, y + half, z, 0xffffffff, 1.0f, 1.0f},
        };
        STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, vertices, sizeof(vertex));
    }

    void lens_flare::clamp_brightness() {
        if (before_bright_ < 0.0f)
            before_bright_ = 0.0f;
        else if (before_bright_ > 1.0f)
            before_bright_ = 1.0f;

        before_bright_ *= max_brightness_;

        if (after_bright_ < 0.0f)
            after_bright_ = 0.0f;
        else if (after_bright_ > 1.0f)
            after_bright_ = 1.0f;

        after_bright_ *= max_brightness_;
    }

    flare::flare() = default;

    flare::~flare() = default;

    void flare::init(std::string path) {
        int i = 0;

        while (files[i] != "") {
            resource *resource = get_resource_manager().resource_pointer((path + string(files[i])).c_str());

            if (!resource->is_type(graphic_image::type()))
                assert(false);

            auto *piece = new flare_piece;

            piece->image_instance.set_image_pointer(static_cast<graphic_image *>(resource));
            piece->position_value = flare_positions[i];
            piece->width = width[i];
            piece->color_value = colors[i];

            v_flares_.push_back(piece);
            i++;
        }
    }

    void flare::draw(float bright_scale, int width_value, int height, int x, int y) {
        STATEMANAGER.save_render_state(gpu::state::DEST_BLEND, gpu::blend::ONE);

        float dx = float(x) - float(width_value) / 2.0f;
        float dy = float(y) - float(height) / 2.0f;

        STATEMANAGER.set_texture(1, nullptr);
        STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);

        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);

        for (unsigned int i = 0; i < v_flares_.size(); i++) {
            float center_x = float(x) - (v_flares_[i]->position_value + 1.0f) * dx;
            float center_y = float(y) - (v_flares_[i]->position_value + 1.0f) * dy;
            float w = v_flares_[i]->width;

            math::color flare_color(v_flares_[i]->color_value[0] * bright_scale,
                v_flares_[i]->color_value[1] * bright_scale,
                v_flares_[i]->color_value[2] * bright_scale,
                v_flares_[i]->color_value[3] * bright_scale);

            STATEMANAGER.set_texture(0, v_flares_[i]->image_instance.texture_pointer()->gpu_texture());

            vertex vertices[4];

            vertices[0].u = 0.0f;
            vertices[0].v = 0.0f;
            vertices[0].x = center_x - w;
            vertices[0].y = center_y - w;
            vertices[0].z = 0.0f;
            vertices[0].color_value = flare_color;

            vertices[1].u = 0.0f;
            vertices[1].v = 1.0f;
            vertices[1].x = center_x - w;
            vertices[1].y = center_y + w;
            vertices[1].z = 0.0f;
            vertices[1].color_value = flare_color;

            vertices[2].u = 1.0f;
            vertices[2].v = 0.0f;
            vertices[2].x = center_x + w;
            vertices[2].y = center_y - w;
            vertices[2].z = 0.0f;
            vertices[2].color_value = flare_color;

            vertices[3].u = 1.0f;
            vertices[3].v = 1.0f;
            vertices[3].x = center_x + w;
            vertices[3].y = center_y + w;
            vertices[3].z = 0.0f;
            vertices[3].color_value = flare_color;

            STATEMANAGER.draw_primitive_up(gpu::primitive::TRIANGLE_STRIP, 2, vertices, sizeof(vertex));
        }

        STATEMANAGER.restore_render_state(gpu::state::DEST_BLEND);
    }
}
