#include "grp_screen.h"
#include <cassert>
#include <metin2/gpu/types.h>
#include <metin2/log/log.h>
#include "camera.h"
#include "state_manager.h"

namespace metin2::inline graphics {

    uint32_t screen::diffuse_color_ = 0xffffffff;
    uint32_t screen::clear_color_ = 0L;
    uint32_t screen::clear_stencil_ = 0L;
    float screen::clear_depth_ = 1.0f;
    frustum screen::frustum_value_;

    void screen::render_line3d(float sx, float sy, float sz, float ex, float ey, float ez) {
        assert(device_);

        pdt_vertex_raw vertices[2] =
            {
                {sx, sy, sz, diffuse_color_, 0.0f, 0.0f},
                {ex, ey, ez, diffuse_color_, 0.0f, 0.0f}
        };

        if (set_pdt_stream(vertices, 2)) {
            STATEMANAGER.set_texture(0, nullptr);
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.draw_primitive(gpu::primitive::LINE_LIST, 0, 1);
        }
    }

    void screen::render_box3d(float sx, float sy, float sz, float ex, float ey, float ez) {
        assert(device_);

        pdt_vertex_raw vertices[8] =
            {
                {sx,        sy, sz, diffuse_color_, 0.0f, 0.0f}, // 0
                {ex,        sy, sz, diffuse_color_, 0.0f, 0.0f}, // 1

                {sx,        sy, sz, diffuse_color_, 0.0f, 0.0f}, // 0
                {sx,        ey, ez, diffuse_color_, 0.0f, 0.0f}, // 2

                {ex,        sy, sz, diffuse_color_, 0.0f, 0.0f}, // 1
                {ex,        ey, ez, diffuse_color_, 0.0f, 0.0f}, // 3

                {sx,        ey, ez, diffuse_color_, 0.0f, 0.0f}, // 2
                {ex + 1.0f, ey, ez, diffuse_color_, 0.0f, 0.0f}  // 3, (3 with x increased by 1)
        };

        if (set_pdt_stream(vertices, 8)) {
            STATEMANAGER.set_texture(0, nullptr);
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.draw_primitive(gpu::primitive::LINE_LIST, 0, 4);
        }
    }

    void screen::render_bar3d(float sx, float sy, float sz, float ex, float ey, float ez) {
        assert(device_);

        pdt_vertex_raw vertices[4] =
            {
                {sx, sy, sz, diffuse_color_, 0.0f, 0.0f},
                {sx, ey, ez, diffuse_color_, 0.0f, 0.0f},
                {ex, sy, sz, diffuse_color_, 0.0f, 0.0f},
                {ex, ey, ez, diffuse_color_, 0.0f, 0.0f},
        };

        if (set_pdt_stream(vertices, 4)) {
            STATEMANAGER.set_texture(0, nullptr);
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_STRIP, 0, 2);
        }
    }

    void screen::render_bar3d(const math::vector3 *positions) {
        assert(device_);

        pdt_vertex_raw vertices[4] =
            {
                {positions[0].x, positions[0].y, positions[0].z, diffuse_color_, 0.0f, 0.0f},
                {positions[2].x, positions[2].y, positions[2].z, diffuse_color_, 0.0f, 0.0f},
                {positions[1].x, positions[1].y, positions[1].z, diffuse_color_, 0.0f, 0.0f},
                {positions[3].x, positions[3].y, positions[3].z, diffuse_color_, 0.0f, 0.0f},
        };

        if (set_pdt_stream(vertices, 4)) {
            STATEMANAGER.set_texture(0, nullptr);
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_STRIP, 0, 2);
        }
    }

    void screen::render_gradation_bar3d(float sx, float sy, float sz, float ex, float ey, float ez, uint32_t start_color, uint32_t end_color) {
        assert(device_);
        if (sx == ex) return;
        if (sy == ey) return;

        pdt_vertex_raw vertices[4] =
            {
                {sx, sy, sz, start_color, 0.0f, 0.0f},
                {sx, ey, ez, end_color,   0.0f, 0.0f},
                {ex, sy, sz, start_color, 0.0f, 0.0f},
                {ex, ey, ez, end_color,   0.0f, 0.0f},
        };

        if (set_pdt_stream(vertices, 4)) {
            STATEMANAGER.set_texture(0, nullptr);
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.draw_primitive(gpu::primitive::TRIANGLE_STRIP, 0, 2);
        }
    }

    void screen::render_cube(float sx, float sy, float sz, float ex, float ey, float ez) {
        pdt_vertex_raw vertices[8] =
            {
                {sx, sy, sz, diffuse_color_, 0.0f, 0.0f},
                {ex, sy, sz, diffuse_color_, 0.0f, 0.0f},
                {sx, ey, sz, diffuse_color_, 0.0f, 0.0f},
                {ex, ey, sz, diffuse_color_, 0.0f, 0.0f},
                {sx, sy, ez, diffuse_color_, 0.0f, 0.0f},
                {ex, sy, ez, diffuse_color_, 0.0f, 0.0f},
                {sx, ey, ez, diffuse_color_, 0.0f, 0.0f},
                {ex, ey, ez, diffuse_color_, 0.0f, 0.0f},
        };

        if (set_pdt_stream(vertices, 8)) {
            STATEMANAGER.set_texture(0, nullptr);
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.set_transform(gpu::transform::WORLD, &matrix_stack_.top());

            set_default_index_buffer(DEFAULT_IB_FILL_CUBE);
            STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, 8, 0, 4 * 3);
        }
    }

    void screen::render_cube(float sx, float sy, float sz, float ex, float ey, float ez, math::matrix rotation) {
        math::vector3 center = math::vector3((sx + ex) * 0.5f, (sy + ey) * 0.5f, (sz + ez) * 0.5f);
        math::vector3 vertex[8] =
            {
                math::vector3(sx, sy, sz),
                math::vector3(ex, sy, sz),
                math::vector3(sx, ey, sz),
                math::vector3(ex, ey, sz),
                math::vector3(sx, sy, ez),
                math::vector3(ex, sy, ez),
                math::vector3(sx, ey, ez),
                math::vector3(ex, ey, ez),
        };
        pdt_vertex_raw vertices[8];

        for (int i = 0; i < 8; i++) {
            vertex[i] = vertex[i] - center;
            vertex[i] = math::transform_coord(vertex[i], rotation);
            vertex[i] = vertex[i] + center;
            vertices[i].px = vertex[i].x;
            vertices[i].py = vertex[i].y;
            vertices[i].pz = vertex[i].z;
            vertices[i].diffuse_value = diffuse_color_;
            vertices[i].u = 0.0f;
            vertices[i].v = 0.0f;
        }

        if (set_pdt_stream(vertices, 8)) {
            STATEMANAGER.set_texture(0, nullptr);
            STATEMANAGER.set_texture(1, nullptr);
            STATEMANAGER.set_vertex_shader(gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1);
            STATEMANAGER.set_transform(gpu::transform::WORLD, &matrix_stack_.top());

            set_default_index_buffer(DEFAULT_IB_FILL_CUBE);
            STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, 8, 0, 4 * 3);
        }
    }

    void screen::render_line2d(float sx, float sy, float ex, float ey, float z) {
        render_line3d(sx, sy, z, ex, ey, z);
    }

    void screen::render_box2d(float sx, float sy, float ex, float ey, float z) {
        render_box3d(sx, sy, z, ex, ey, z);
    }

    void screen::render_bar2d(float sx, float sy, float ex, float ey, float z) {
        render_bar3d(sx, sy, z, ex, ey, z);
    }

    void screen::render_gradation_bar2d(float sx, float sy, float ex, float ey, uint32_t start_color, uint32_t end_color, float ez) {
        render_gradation_bar3d(sx, sy, ez, ex, ey, ez, start_color, end_color);
    }

    void screen::render_circle2d(float fx, float fy, float fz, float radius, int step) {
        int count;
        float theta, delta;
        float x, y, z;
        std::vector<math::vector3> pts;

        pts.clear();
        pts.resize(step);

        theta = 0.0;
        delta = 2 * math::PI / float(step);

        for (count = 0; count < step; count++) {
            x = fx + radius * cosf(theta);
            y = fy + radius * sinf(theta);
            z = fz;

            pts[count] = math::vector3(x, y, z);

            theta += delta;
        }
        for (count = 0; count < step - 1; count++) {
            render_line3d(pts[count].x, pts[count].y, pts[count].z, pts[count + 1].x, pts[count + 1].y, pts[count + 1].z);
        }
        render_line3d(pts[step - 1].x, pts[step - 1].y, pts[step - 1].z, pts[0].x, pts[0].y, pts[0].z);
    }

    void screen::render_circle3d(float fx, float fy, float fz, float radius, int step) {
        int count;
        float theta, delta;
        std::vector<math::vector3> pts;

        pts.clear();
        pts.resize(step);

        theta = 0.0;
        delta = 2 * math::PI / float(step);

        const math::matrix &inv_view = get_camera_manager().current_camera()->billboard_matrix();

        for (count = 0; count < step; count++) {
            pts[count] = math::vector3(radius * cosf(theta), radius * sinf(theta), 0.0f);
            pts[count] = math::transform_coord(pts[count], inv_view);

            theta += delta;
        }
        for (count = 0; count < step - 1; count++) {
            render_line3d(fx + pts[count].x, fy + pts[count].y, fz + pts[count].z,
                fx + pts[count + 1].x, fy + pts[count + 1].y, fz + pts[count + 1].z);
        }
        render_line3d(fx + pts[step - 1].x, fy + pts[step - 1].y, fz + pts[step - 1].z,
            fx + pts[0].x, fy + pts[0].y, fz + pts[0].z);
    }

    class mesh_rendering_option : public screen {
      public:
        mesh_rendering_option(uint32_t fill_mode, const math::matrix &world) {
            STATEMANAGER.save_vertex_shader(gpu::mesh::VERTEX_FORMAT);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TFACTOR);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
            STATEMANAGER.save_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
            STATEMANAGER.set_render_state(gpu::state::FILL_MODE, fill_mode);
            STATEMANAGER.save_transform(gpu::transform::WORLD, &world);

            STATEMANAGER.set_texture(0, nullptr);
            STATEMANAGER.set_texture(1, nullptr);
        }

        ~mesh_rendering_option() override {
            STATEMANAGER.restore_vertex_shader();
            STATEMANAGER.restore_transform(gpu::transform::WORLD);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_ARG1);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::COLOR_OP);
            STATEMANAGER.restore_texture_stage_state(0, gpu::stage_state::ALPHA_OP);
            STATEMANAGER.set_render_state(gpu::state::FILL_MODE, gpu::fill::SOLID);
        }
    };

    void screen::render_mesh(const gpu::mesh &mesh, const math::matrix *c_pmat_world, float fx, float fy, float fz, float radius, uint32_t fill_mode) {
        if (!mesh.vertices || !mesh.indices)
            return;

        math::matrix mat_world = math::matrix_scaling(radius, radius, radius) * math::matrix_translation(fx, fy, fz);
        if (c_pmat_world)
            mat_world *= *c_pmat_world;

        mesh_rendering_option set_rendering_option(fill_mode, mat_world);
        STATEMANAGER.set_indices(mesh.indices.get(), 0);
        STATEMANAGER.set_stream_source(0, mesh.vertices.get(), gpu::mesh::VERTEX_SIZE);
        STATEMANAGER.draw_indexed_primitive(gpu::primitive::TRIANGLE_LIST, 0, mesh.vertex_count, 0, mesh.face_count);
    }

    void screen::render_sphere(const math::matrix *world, float fx, float fy, float fz, float radius, uint32_t fill_mode) {
        render_mesh(sphere_mesh_, world, fx, fy, fz, radius, fill_mode);
    }

    void screen::render_cylinder(const math::matrix *world, float fx, float fy, float fz, float radius, float /*fLength*/, uint32_t fill_mode) {
        render_mesh(cylinder_mesh_, world, fx, fy, fz, radius, fill_mode);
    }

    void screen::set_cursor_position(int x, int y, int hres, int vres) {
        math::vector3 v;
        v.x = -(((2.0f * x) / hres) - 1) / proj_._11;
        v.y = (((2.0f * y) / vres) - 1) / proj_._22;
        v.z = 1.0f;

        math::matrix view_inverse = inverse_view_;

        vt_pick_ray_dir_.x = v.x * view_inverse._11 +
                             v.y * view_inverse._21 +
                             v.z * view_inverse._31;

        vt_pick_ray_dir_.y = v.x * view_inverse._12 +
                             v.y * view_inverse._22 +
                             v.z * view_inverse._32;

        vt_pick_ray_dir_.z = v.x * view_inverse._13 +
                             v.y * view_inverse._23 +
                             v.z * view_inverse._33;

        vt_pick_ray_orig_.x = view_inverse._41;
        vt_pick_ray_orig_.y = view_inverse._42;
        vt_pick_ray_orig_.z = view_inverse._43;

        // Terrain picking ray; should be merged with the pick ray above.
        ray_value_.set_start_point(vt_pick_ray_orig_);
        ray_value_.set_direction(-vt_pick_ray_dir_, 51200.0f);
    }

    auto screen::cursor_position(float *px, float *py, float *pz) -> bool {
        if (!cursor_xy_position(px, py)) return false;
        if (!cursor_z_position(pz)) return false;

        return true;
    }

    auto screen::cursor_xy_position(float *px, float *py) -> bool {
        math::vector3 eye = get_camera_manager().current_camera()->eye();

        position pos_vertices[4];
        pos_vertices[0] = position(eye.x - 90000000.0f, eye.y + 90000000.0f, 0.0f);
        pos_vertices[1] = position(eye.x - 90000000.0f, eye.y - 90000000.0f, 0.0f);
        pos_vertices[2] = position(eye.x + 90000000.0f, eye.y + 90000000.0f, 0.0f);
        pos_vertices[3] = position(eye.x + 90000000.0f, eye.y - 90000000.0f, 0.0f);

        static const uint16_t fill_rect_indices[6] = {
            0,
            2,
            1,
            2,
            3,
            1,
        };

        float u, v, t;
        for (int i = 0; i < 2; ++i) {
            if (intersect_triangle(vt_pick_ray_orig_, vt_pick_ray_dir_,
                    pos_vertices[fill_rect_indices[i * 3 + 0]],
                    pos_vertices[fill_rect_indices[i * 3 + 1]],
                    pos_vertices[fill_rect_indices[i * 3 + 2]],
                    &u, &v, &t)) {
                *px = vt_pick_ray_orig_.x + vt_pick_ray_dir_.x * t;
                *py = vt_pick_ray_orig_.y + vt_pick_ray_dir_.y * t;
                return true;
            }
        }
        return false;
    }

    auto screen::cursor_z_position(float *pz) -> bool {
        math::vector3 eye = get_camera_manager().current_camera()->eye();

        position pos_vertices[4];
        pos_vertices[0] = position(eye.x - 90000000.0f, 0.0f, eye.z + 90000000.0f);
        pos_vertices[1] = position(eye.x - 90000000.0f, 0.0f, eye.z - 90000000.0f);
        pos_vertices[2] = position(eye.x + 90000000.0f, 0.0f, eye.z + 90000000.0f);
        pos_vertices[3] = position(eye.x + 90000000.0f, 0.0f, eye.z - 90000000.0f);

        static const uint16_t fill_rect_indices[6] = {
            0,
            2,
            1,
            2,
            3,
            1,
        };

        float u, v, t;
        for (int i = 0; i < 2; ++i) {
            if (intersect_triangle(vt_pick_ray_orig_, vt_pick_ray_dir_,
                    pos_vertices[fill_rect_indices[i * 3 + 0]],
                    pos_vertices[fill_rect_indices[i * 3 + 1]],
                    pos_vertices[fill_rect_indices[i * 3 + 2]],
                    &u, &v, &t)) {
                *pz = vt_pick_ray_orig_.z + vt_pick_ray_dir_.z * t;
                return true;
            }
        }
        return false;
    }

    void screen::set_diffuse_color(uint32_t diffuse_color) {
        diffuse_color_ = diffuse_color;
    }

    void screen::set_diffuse_color(float r, float g, float b, float a) {
        diffuse_color_ = get_color(r, g, b, a);
    }

    void screen::clear_depth_buffer() {
        assert(device_);
        device_->clear({}, gpu::clear::Z_BUFFER, clear_color_, clear_depth_, clear_stencil_);
    }

    void screen::clear() {
        assert(device_);
        device_->clear({}, gpu::clear::TARGET | gpu::clear::Z_BUFFER, clear_color_, clear_depth_, clear_stencil_);
    }

    auto screen::begin() -> bool {
        assert(device_);
        reset_face_count();

        if (!STATEMANAGER.begin_scene()) {
            metin2::log::debug("Cannot begin the scene");
            return false;
        }

        return true;
    }

    void screen::end() {
        STATEMANAGER.end_scene();
    }

    void screen::show() {
        assert(device_);
        device_->present();
    }

    void screen::project_position(float x, float y, float z, float *pf_x, float *pf_y) {
        math::vector3 input(x, y, z);
        math::vector3 output;
        output = math::project(input, {static_cast<float>(viewport_.x), static_cast<float>(viewport_.y), static_cast<float>(viewport_.width), static_cast<float>(viewport_.height), viewport_.min_z, viewport_.max_z}, &proj_, &view_, &world_);

        *pf_x = output.x;
        *pf_y = output.y;
    }

    void screen::project_position(float x, float y, float z, float *pf_x, float *pf_y, float *pf_z) {
        math::vector3 input(x, y, z);
        math::vector3 output;
        output = math::project(input, {static_cast<float>(viewport_.x), static_cast<float>(viewport_.y), static_cast<float>(viewport_.width), static_cast<float>(viewport_.height), viewport_.min_z, viewport_.max_z}, &proj_, &view_, &world_);

        *pf_x = output.x;
        *pf_y = output.y;
        *pf_z = output.z;
    }

    void screen::set_color_operation() {
        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::SELECT_ARG1);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
    }

    void screen::set_diffuse_operation() {
        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);

        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
    }

    void screen::set_blend_operation() {
        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::CURRENT);
        STATEMANAGER.set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::MODULATE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        STATEMANAGER.set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
    }

    void screen::identity() {
        STATEMANAGER.set_transform(gpu::transform::WORLD, &identity_);
    }

    screen::screen() = default;

    screen::~screen() = default;

    void screen::build_view_frustum() {
        const math::vector3 &eye = get_camera_manager().current_camera()->eye();
        const math::vector3 &view = get_camera_manager().current_camera()->view();
        auto vv = view_ * proj_;
        frustum_value_.build_view_frustum2(
            vv,
            near_y_,
            far_y_,
            field_of_view_,
            aspect_,
            eye, view);
    }
}
