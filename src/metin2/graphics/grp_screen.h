#pragma once

#include <metin2/geometry/frustum.h>
#include <metin2/geometry/grp_collision_object.h>
#include <metin2/gpu/types.h>

namespace metin2::inline graphics {

    class screen : public graphic_collision_object {
      public:
        screen();
        ~screen() override;

        void clear_depth_buffer();
        void clear();
        auto begin() -> bool;
        void end();
        void show();

        void render_line2d(float sx, float sy, float ex, float ey, float z = 0.0f);
        void render_box2d(float sx, float sy, float ex, float ey, float z = 0.0f);
        void render_bar2d(float sx, float sy, float ex, float ey, float z = 0.0f);
        void render_gradation_bar2d(float sx, float sy, float ex, float ey, uint32_t start_color, uint32_t end_color, float ez = 0.0f);
        void render_circle2d(float fx, float fy, float fz, float radius, int step = 50);
        void render_circle3d(float fx, float fy, float fz, float radius, int step = 50);

        void render_line3d(float sx, float sy, float sz, float ex, float ey, float ez);
        void render_box3d(float sx, float sy, float sz, float ex, float ey, float ez);
        void render_bar3d(float sx, float sy, float sz, float ex, float ey, float ez);
        void render_bar3d(const math::vector3 *positions);
        void render_gradation_bar3d(float sx, float sy, float sz, float ex, float ey, float ez, uint32_t start_color, uint32_t end_color);

        void render_cube(float sx, float sy, float sz, float ex, float ey, float ez);
        void render_cube(float sx, float sy, float sz, float ex, float ey, float ez, math::matrix rotation);

        void render_mesh(const gpu::mesh &mesh, const math::matrix *world, float fx, float fy, float fz, float radius, uint32_t fill_mode);
        void render_sphere(const math::matrix *world, float fx, float fy, float fz, float radius, uint32_t fill_mode = gpu::fill::SOLID);
        void render_cylinder(const math::matrix *world, float fx, float fy, float fz, float radius, float length, uint32_t fill_mode = gpu::fill::SOLID);

        void set_color_operation();
        void set_diffuse_operation();
        void set_blend_operation();
        void set_diffuse_color(uint32_t diffuse_color);
        void set_diffuse_color(float r, float g, float b, float a = 1.0f);

        void set_cursor_position(int x, int y, int hres, int vres); // creates picking ray
        auto cursor_position(float *px, float *py, float *pz) -> bool;
        auto cursor_xy_position(float *px, float *py) -> bool;
        auto cursor_z_position(float *pz) -> bool;
        void project_position(float x, float y, float z, float *pf_x, float *pf_y);
        void project_position(float x, float y, float z, float *pf_x, float *pf_y, float *pf_z);

        void build_view_frustum();

        static void identity();
        static auto get_frustum() -> frustum & { return frustum_value_; }

      protected:
        static uint32_t diffuse_color_;
        static uint32_t clear_color_;
        static uint32_t clear_stencil_;
        static float clear_depth_;

        static frustum frustum_value_;
    };
}
