#pragma once

#include <memory>
#include <metin2/geometry/ray.h>
#include <metin2/gpu/device.h>
#include <metin2/gpu/mesh.h>
#include <vector>

namespace metin2::inline graphics {

    void pixel_position_to_vector3(const math::vector3 &p_pos_src, math::vector3 *dst);

    class graphic_texture;

    using index = uint16_t;

    struct face {
        index indices[3];
    };

    using position = math::vector3;

    using normal = math::vector3;

    using texture_coordinate = math::vector2;

    using diffuse = uint32_t;
    using ambient = uint32_t;
    using specular = uint32_t;

    struct vertex {
        float x, y, z;
        uint32_t color_value;
        float u, v;
    };

    struct t_vertex {
        float x, y, z, rhw;
    };

    struct p_vertex {
        float x, y, z;
    };

    struct pd_vertex {
        float x, y, z;
        uint32_t color_value;
    };

    struct pdt_vertex_raw {
        float px, py, pz;
        uint32_t diffuse_value;
        float u, v;
    };

    struct pt_vertex {
        position position_value;
        texture_coordinate tex_coord;
    };

    struct pdt_vertex {
        position position_value;
        diffuse diffuse_value;
        texture_coordinate tex_coord;
    };

    struct pnt_vertex {
        position position_value;
        normal normal_value;
        texture_coordinate tex_coord;
    };

    struct spnt2_vertex {
        position position_value;
        normal normal_value;
        texture_coordinate tex_coord;
        texture_coordinate tex_coord2;
    };
    using tpnt2_vertex = spnt2_vertex;

    struct spdt2_vertex {
        position position_value;
        uint32_t diffuse_value;
        texture_coordinate tex_coord;
        texture_coordinate tex_coord2;
    };
    using tpdt2_vertex = spdt2_vertex;

    struct bound_box {
        float sx, sy, sz;
        float ex, ey, ez;
        int mesh_index;
        int bone_index;
    };

    const uint16_t c_FillRectIndices[6] = {0, 2, 1, 2, 3, 1};

    class graphic_base {
      public:
        static auto available_texture_memory() -> uint32_t;
        static auto view_matrix() -> const math::matrix &;

        enum {
            DEFAULT_IB_LINE,
            DEFAULT_IB_LINE_TRI,
            DEFAULT_IB_LINE_RECT,
            DEFAULT_IB_LINE_CUBE,
            DEFAULT_IB_FILL_TRI,
            DEFAULT_IB_FILL_RECT,
            DEFAULT_IB_FILL_CUBE,
            DEFAULT_IB_NUM,
        };

      public:
        graphic_base();
        virtual ~graphic_base();

        void set_position_camera(float fx, float fy, float fz, float distance, float pitch, float rotation);

        void set_ortho2d(float hres, float vres, float zres);
        void set_perspective(float fov, float aspect, float nearz, float farz);
        void clip_plane(float *near_y, float *far_y) {
            *near_y = near_y_;
            *far_y = far_y_;
        }

        void mult_matrix(const math::matrix *mat);
        void mult_matrix_local(const math::matrix *mat);

        void rotate_yaw_pitch_roll_local(float yaw, float pitch, float roll);
        void scale(float x, float y, float z);
        void load_matrix(const math::matrix &src_matrix);

        void init_screen_effect();
        void set_screen_effect_waving(float during_time, int power);

        auto get_color(float r, float g, float b, float a = 1.0f) -> uint32_t;

        auto face_count() -> uint32_t;
        void reset_face_count();

        void update_proj_matrix();
        void update_view_matrix();

        void set_viewport(uint32_t x, uint32_t y, uint32_t width_value, uint32_t height, float min_z, float max_z);
        static void back_buffer_size(uint32_t *width_value, uint32_t *height);
        static auto is_low_texture_memory() -> bool;
        static auto device() -> gpu::device & { return *device_; }

        static void set_default_index_buffer(uint32_t def_ib);
        static auto set_pdt_stream(pdt_vertex_raw *vertices, uint32_t vtx_count) -> bool;
        static auto set_pdt_stream(pdt_vertex *vertices, uint32_t vtx_count) -> bool;

        // Framebuffer pixels per interface pixel: the interface is laid out in its own pixels and
        // drawn magnified by this factor. Text is rasterized at this scale, and 2D quads place
        // themselves on framebuffer pixels with it (the vertex shader adds the D3D9 half pixel in
        // framebuffer units, so a quad subtracts half an interface pixel / scale).
        static void set_interface_pixel_scale(float scale) { interface_pixel_scale_ = scale > 0.0f ? scale : 1.0f; }
        static auto interface_pixel_scale() -> float { return interface_pixel_scale_; }
        static auto half_pixel() -> float { return 0.5f / interface_pixel_scale_; }

      protected:
        static math::matrix identity_;
        static float interface_pixel_scale_;

        static math::matrix view_;
        static math::matrix proj_;
        static math::matrix inverse_view_;
        static math::matrix inverse_view_y_axis_;

      protected:
        void update_pipe_line_matrix();

      protected:
        // debug shapes (collision data, areas)
        static gpu::mesh sphere_mesh_;
        static gpu::mesh cylinder_mesh_;

      protected:
        static int width_;
        static int height_;

        static std::unique_ptr<gpu::device> device_;
        static math::matrix_stack matrix_stack_;
        static gpu::viewport viewport_;

        static uint32_t face_count_;

        // vertex layouts: position + texture coordinates in two streams, position + normal +
        // texture coordinates, and the same with a second set of texture coordinates
        static uint32_t vs_;
        static uint32_t pnt_vs_;
        static uint32_t pnt2vs_;

        static math::matrix screen0_;
        static math::matrix screen1_;
        static math::matrix screen2_;

        static math::vector3 vt_pick_ray_orig_;
        static math::vector3 vt_pick_ray_dir_;

        static float field_of_view_;
        static float aspect_;
        static float near_y_;
        static float far_y_;

        // Screen Effect - Waving, Flashing and so on..
        static uint32_t waving_end_time_;
        static uint32_t waving_duration_;
        static int waving_power_;
        static uint32_t flashing_end_time_;
        static math::color flashing_color_;

        // Ray for terrain picking... CCamera-based version.. should be merged with the existing ray...
        static ray ray_value_;

        static bool support_dxt_;
        static bool is_low_texture_memory_;
        static bool is_high_texture_memory_;

        enum {
            PDT_VERTEX_NUM = 16,
            PDT_VERTEXBUFFER_NUM = 100,
        };

        static std::shared_ptr<gpu::vertex_buffer> pdt_vertex_buffers_[PDT_VERTEXBUFFER_NUM];
        static std::shared_ptr<gpu::index_buffer> default_index_buffers_[DEFAULT_IB_NUM];
    };
}
