#include "grp_base.h"
#include <algorithm>
#include <metin2/core/random.h>
#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/gpu/types.h>
#include "camera.h"
#include "state_manager.h"

#include <cmath>

namespace metin2::inline graphics {

    void pixel_position_to_vector3(const math::vector3 &p_pos_src, math::vector3 *dst) {
        dst->x = +p_pos_src.x;
        dst->y = -p_pos_src.y;
        dst->z = +p_pos_src.z;
    }

    std::unique_ptr<gpu::device> graphic_base::device_;
    math::matrix_stack graphic_base::matrix_stack_;
    gpu::viewport graphic_base::viewport_;

    int graphic_base::width_;
    int graphic_base::height_;

    uint32_t graphic_base::face_count_ = 0;

    uint32_t graphic_base::vs_ = 0;
    uint32_t graphic_base::pnt_vs_ = 0;
    uint32_t graphic_base::pnt2vs_ = 0;

    math::matrix graphic_base::identity_;
    float graphic_base::interface_pixel_scale_ = 1.0f;
    std::vector<rectangle> graphic_base::clip_stack_;

    math::matrix graphic_base::view_;
    math::matrix graphic_base::proj_;
    math::matrix graphic_base::inverse_view_;
    math::matrix graphic_base::inverse_view_y_axis_;

    math::matrix graphic_base::screen0_;
    math::matrix graphic_base::screen1_;
    math::matrix graphic_base::screen2_;

    math::vector3 graphic_base::vt_pick_ray_orig_;
    math::vector3 graphic_base::vt_pick_ray_dir_;

    float graphic_base::field_of_view_;
    float graphic_base::near_y_;
    float graphic_base::far_y_;
    float graphic_base::aspect_;

    uint32_t graphic_base::waving_end_time_;
    uint32_t graphic_base::waving_duration_;
    int graphic_base::waving_power_;
    uint32_t graphic_base::flashing_end_time_;
    math::color graphic_base::flashing_color_;

    // Terrain picking ray built from the camera; should be merged with the pick ray in screen.
    ray graphic_base::ray_value_;
    bool graphic_base::support_dxt_ = true;
    bool graphic_base::is_low_texture_memory_ = false;
    bool graphic_base::is_high_texture_memory_ = false;

    gpu::mesh graphic_base::sphere_mesh_;
    gpu::mesh graphic_base::cylinder_mesh_;

    std::shared_ptr<gpu::vertex_buffer> graphic_base::pdt_vertex_buffers_[PDT_VERTEXBUFFER_NUM];
    std::shared_ptr<gpu::index_buffer> graphic_base::default_index_buffers_[DEFAULT_IB_NUM];

    auto graphic_base::is_low_texture_memory() -> bool {
        return is_low_texture_memory_;
    }

    void graphic_base::back_buffer_size(uint32_t *width_value, uint32_t *height) {
        *width_value = device_->back_buffer_width();
        *height = device_->back_buffer_height();
    }

    void graphic_base::set_default_index_buffer(uint32_t def_ib) {
        if (def_ib >= DEFAULT_IB_NUM)
            return;

        STATEMANAGER.set_indices(default_index_buffers_[def_ib].get(), 0);
    }

    auto graphic_base::set_pdt_stream(pdt_vertex *vertices, uint32_t vtx_count) -> bool {
        return set_pdt_stream((pdt_vertex_raw *)vertices, vtx_count);
    }

    auto graphic_base::set_pdt_stream(pdt_vertex_raw *src_vertices, uint32_t vtx_count) -> bool {
        if (!vtx_count)
            return false;

        static uint32_t vb_pos = 0;

        if (vb_pos >= PDT_VERTEXBUFFER_NUM)
            vb_pos = 0;

        gpu::vertex_buffer *buffer = pdt_vertex_buffers_[vb_pos].get();
        ++vb_pos;

        assert(PDT_VERTEX_NUM >= vtx_count);
        if (vtx_count >= PDT_VERTEX_NUM)
            return false;

        void *dst_vertices;
        if (!buffer->lock(0, sizeof(pdt_vertex) * vtx_count, &dst_vertices, gpu::lock::DISCARD)) {
            STATEMANAGER.set_stream_source(0, nullptr, 0);
            return false;
        }

        memcpy(dst_vertices, src_vertices, sizeof(pdt_vertex) * vtx_count);

        buffer->unlock();

        STATEMANAGER.set_stream_source(0, buffer, sizeof(pdt_vertex));

        return true;
    }

    auto graphic_base::available_texture_memory() -> uint32_t {
        assert(device_);

        static uint32_t next_update_time = 0;
        static uint32_t tex_mem_size = 0;

        uint32_t cur_time = el_timer_get_m_sec();
        if (next_update_time < cur_time) {
            next_update_time = cur_time + 5000;
            tex_mem_size = device_->available_texture_memory();
        }

        return tex_mem_size;
    }

    auto graphic_base::view_matrix() -> const math::matrix & {
        return view_;
    }

    void graphic_base::set_position_camera(float fx, float fy, float fz, float distance, float pitch, float roll) {
        const uint32_t now = get_timer().current_millisecond();
        if (waving_end_time_ > now && waving_power_ > 0 && waving_duration_ > 0) {
            // A shake around the true position that dies out over the event, instead of a one-sided
            // offset that stops abruptly.
            const float left = static_cast<float>(waving_end_time_ - now) / static_cast<float>(waving_duration_);
            const float amplitude = static_cast<float>(waving_power_) / 10.0f * std::min(1.0f, left);
            fx += amplitude * static_cast<float>(metin2::random::number(-1000, 1000)) / 1000.0f;
            fy += amplitude * static_cast<float>(metin2::random::number(-1000, 1000)) / 1000.0f;
            fz += amplitude * static_cast<float>(metin2::random::number(-1000, 1000)) / 1000.0f;
        }

        camera *camera = get_camera_manager().current_camera();
        if (!camera)
            return;

        camera->set_view_params(math::vector3(0.0f, -distance, 0.0f), math::vector3(0.0f, 0.0f, 0.0f), math::vector3(0.0f, 0.0f, 1.0f));
        pitch = f_min(80.0f, f_max(-80.0f, pitch));
        camera->rotate_eye_around_target(pitch, roll);
        camera->move(math::vector3(fx, fy, fz));

        update_view_matrix();
    }

    void graphic_base::set_ortho2d(float hres, float vres, float zres) {
        proj_ = math::matrix_ortho_off_center_rh(0, hres, vres, 0, 0, zres);
        update_proj_matrix();
    }

    void graphic_base::set_perspective(float fov, float aspect, float nearz, float farz) {
        field_of_view_ = fov;

        aspect_ = aspect;

        near_y_ = nearz;
        far_y_ = farz;

        proj_ = math::matrix_perspective_fov_rh(math::to_radian(fov), aspect_, nearz, farz);
        update_proj_matrix();
    }

    void graphic_base::update_proj_matrix() {
        STATEMANAGER.set_transform(gpu::transform::PROJECTION, &proj_);
    }

    void graphic_base::update_view_matrix() {
        camera *camera = get_camera_manager().current_camera();
        if (!camera)
            return;

        view_ = camera->view_matrix();
        STATEMANAGER.set_transform(gpu::transform::VIEW, &view_);

        math::inverse(view_, inverse_view_);
        inverse_view_y_axis_._11 = inverse_view_._11;
        inverse_view_y_axis_._12 = inverse_view_._12;
        inverse_view_y_axis_._21 = inverse_view_._21;
        inverse_view_y_axis_._22 = inverse_view_._22;
    }

    void graphic_base::update_pipe_line_matrix() {
        update_proj_matrix();
        update_view_matrix();
    }

    void graphic_base::set_viewport(uint32_t x, uint32_t y, uint32_t width_value, uint32_t height, float min_z, float max_z) {
        viewport_ = {x, y, width_value, height, min_z, max_z};
    }

    void graphic_base::scale(float x, float y, float z) {
        matrix_stack_.multiply(math::matrix_scaling(x, y, z));
    }

    void graphic_base::mult_matrix(const math::matrix *mat) {
        matrix_stack_.multiply(*mat);
    }

    void graphic_base::mult_matrix_local(const math::matrix *mat) {
        matrix_stack_.multiply_local(*mat);
    }

    void graphic_base::rotate_yaw_pitch_roll_local(float yaw, float pitch, float roll) {
        matrix_stack_.multiply_local(math::matrix_rotation_yaw_pitch_roll(math::to_radian(yaw), math::to_radian(pitch), math::to_radian(roll)));
    }

    void graphic_base::load_matrix(const math::matrix &src_matrix) {
        matrix_stack_.load(src_matrix);
    }

    auto graphic_base::get_color(float r, float g, float b, float a) -> uint32_t {
        uint8_t argb[4] =
            {
                (uint8_t)(255.0f * b),
                (uint8_t)(255.0f * g),
                (uint8_t)(255.0f * r),
                (uint8_t)(255.0f * a)};

        return *((uint32_t *)argb);
    }

    void graphic_base::init_screen_effect() {
        waving_end_time_ = 0;
        flashing_end_time_ = 0;
        waving_power_ = 0;
        flashing_color_ = math::color(0.0f, 0.0f, 0.0f, 0.0f);
    }

    void graphic_base::set_screen_effect_waving(float during_time, int power) {
        waving_duration_ = static_cast<uint32_t>(during_time * 1000.0f);
        waving_end_time_ = get_timer().current_millisecond() + waving_duration_;
        waving_power_ = power;
    }

    auto graphic_base::face_count() -> uint32_t {
        return face_count_;
    }

    void graphic_base::reset_face_count() {
        face_count_ = 0;
    }

    graphic_base::graphic_base() = default;

    graphic_base::~graphic_base() = default;

    auto graphic_base::push_clip(const rectangle &rect) -> bool {
        rectangle clip = rect;
        if (!clip_stack_.empty()) {
            const rectangle &outer = clip_stack_.back();
            clip.left = std::max(clip.left, outer.left);
            clip.top = std::max(clip.top, outer.top);
            clip.right = std::min(clip.right, outer.right);
            clip.bottom = std::min(clip.bottom, outer.bottom);
        }
        clip.right = std::max(clip.right, clip.left);
        clip.bottom = std::max(clip.bottom, clip.top);
        clip_stack_.push_back(clip);

        const float scale = interface_pixel_scale_;
        const rectangle pixels = {
            static_cast<int32_t>(std::floor(clip.left * scale)),
            static_cast<int32_t>(std::floor(clip.top * scale)),
            static_cast<int32_t>(std::ceil(clip.right * scale)),
            static_cast<int32_t>(std::ceil(clip.bottom * scale)),
        };
        device().set_scissor(&pixels);
        return clip.right > clip.left && clip.bottom > clip.top;
    }

    void graphic_base::pop_clip() {
        if (clip_stack_.empty())
            return;
        clip_stack_.pop_back();
        if (clip_stack_.empty()) {
            device().set_scissor(nullptr);
            return;
        }
        // Back to the enclosing clip.
        const rectangle outer = clip_stack_.back();
        clip_stack_.pop_back();
        push_clip(outer);
    }
}
