#pragma once

#include "gl.h"

#include <metin2/gpu/device.h>

#include <memory>
#include <vector>

// The renderer on OpenGL 3.3 core, on the context raylib created with the window. The fixed-function
// pipeline runs as one shader program fed by the uniform block of fixed_function.h; buffers and
// textures keep a CPU copy for locks.
namespace metin2::gpu::opengl {

    class gl_device;

    class gl_vertex_buffer final : public vertex_buffer {
      public:
        gl_vertex_buffer(uint32_t size, uint32_t usage);
        ~gl_vertex_buffer() override;

        auto lock(uint32_t offset, uint32_t size, void **data, uint32_t flags) -> bool override;
        void unlock() override;

        GLuint name = 0;

      private:
        std::vector<uint8_t> shadow_;
        bool locked_ = false;
        bool read_only_ = false;
        uint32_t lock_offset_ = 0;
        uint32_t lock_size_ = 0;
        // As for textures: a static buffer drops its CPU copy after the upload and reads it back if it
        // is ever locked again (then keeps it).
        bool uploaded_ = false;
        bool keep_shadow_ = false;
    };

    class gl_index_buffer final : public index_buffer {
      public:
        gl_index_buffer(uint32_t size, uint32_t usage, format fmt);
        ~gl_index_buffer() override;

        auto lock(uint32_t offset, uint32_t size, void **data, uint32_t flags) -> bool override;
        void unlock() override;

        GLuint name = 0;

      private:
        std::vector<uint8_t> shadow_;
        bool locked_ = false;
        bool read_only_ = false;
        uint32_t lock_offset_ = 0;
        uint32_t lock_size_ = 0;
        // As for textures: a static buffer drops its CPU copy after the upload and reads it back if it
        // is ever locked again (then keeps it).
        bool uploaded_ = false;
        bool keep_shadow_ = false;
    };

    class gl_texture final : public texture {
      public:
        gl_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, format fmt);
        ~gl_texture() override;

        auto lock(uint32_t level, locked_rect &out, const rectangle *area, uint32_t flags) -> bool override;
        void unlock(uint32_t level) override;
        void upload(uint32_t level, const void *data) override;
        void upload_rgba8(uint32_t level, const uint8_t *rgba) override;
        void generate_mipmaps() override;

        // The framebuffer that renders into level 0; the CPU copy is dropped since the GPU now owns it.
        auto framebuffer() -> GLuint;

        GLuint name = 0;

      private:
        // The CPU copy of a level exists while it is locked. A static texture drops it after the
        // upload; one locked again gets its contents read back and keeps the copy from then on.
        struct level_data {
            std::vector<uint8_t> shadow;
            bool locked = false;
            bool read_only = false;
            bool uploaded = false;
            bool keep_shadow = false;
        };

        void read_back(uint32_t level);

        const format_info *info_;
        const gl_format *gl_;
        std::vector<level_data> level_data_;
        GLuint framebuffer_ = 0;
    };

    class gl_pixel_shader final : public pixel_shader {
      public:
        ~gl_pixel_shader() override;

        GLuint program = 0;
        GLint time_location = -1;
        GLint size_location = -1;
        GLint user_location = -1;
    };

    class gl_depth_buffer final : public depth_buffer {
      public:
        gl_depth_buffer(uint32_t width, uint32_t height, format fmt);
        ~gl_depth_buffer() override;

        GLuint name = 0;
        bool stencil = false;
    };

    class gl_device final : public device {
      public:
        explicit gl_device(const device_settings &settings);
        ~gl_device() override;

        auto initialize() -> bool;

        auto backend_name() const -> std::string_view override { return "opengl"; }
        auto adapter_name() const -> std::string override;
        auto caps() const -> const device_caps & override { return caps_; }
        auto check_format(format fmt, uint32_t usage) const -> bool override;
        auto available_texture_memory() const -> uint32_t override;

        auto create_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, format fmt) -> std::shared_ptr<texture> override;
        auto create_vertex_buffer(uint32_t size, uint32_t usage) -> std::shared_ptr<vertex_buffer> override;
        auto create_index_buffer(uint32_t size, uint32_t usage, format fmt) -> std::shared_ptr<index_buffer> override;
        auto create_depth_buffer(uint32_t width, uint32_t height, format fmt) -> std::shared_ptr<depth_buffer> override;
        auto create_vertex_layout(std::span<const vertex_element> elements) -> uint32_t override;
        void destroy_vertex_layout(uint32_t handle) override;
        auto create_pixel_shader(std::string_view body, std::string &error) -> std::shared_ptr<pixel_shader> override;

        auto reset(const device_settings &settings) -> bool override;
        auto back_buffer_width() const -> uint32_t override { return settings_.width; }
        auto back_buffer_height() const -> uint32_t override { return settings_.height; }
        void present() override;
        auto read_back_buffer(std::vector<uint8_t> &bgra) -> bool override;

        void set_render_state(state s, uint32_t value) override;
        auto render_state(state s) const -> uint32_t override { return state_.render_state(s); }
        void set_texture_stage_state(uint32_t stage, stage_state s, uint32_t value) override;
        auto texture_stage_state(uint32_t stage, stage_state s) const -> uint32_t override;
        void set_texture(uint32_t stage, texture *tex) override;
        void set_transform(transform t, const math::matrix &m) override;
        auto get_transform(transform t) const -> math::matrix override;
        void set_material(const material &m) override;
        void set_light(uint32_t index, const light &l) override;
        void enable_light(uint32_t index, bool enable) override;
        void set_viewport(const viewport &vp) override;
        auto get_viewport() const -> viewport override { return state_.view_port; }
        void set_scissor(const rectangle *rect) override;
        void set_vertex_format(uint32_t format_or_handle) override;
        void set_stream(uint32_t stream, vertex_buffer *buffer, uint32_t stride) override;
        void set_indices(index_buffer *buffer, uint32_t base_vertex) override;
        void set_pixel_shader(pixel_shader *shader, const pixel_shader_params &params) override;

        auto set_render_target(texture *color, depth_buffer *depth) -> bool override;
        auto render_target() const -> texture * override { return target_.get(); }
        auto depth_target() const -> depth_buffer * override { return depth_target_.get(); }

        void clear(std::span<const rectangle> areas, uint32_t flags, uint32_t color, float z, uint32_t stencil) override;
        auto draw(primitive type, uint32_t start_vertex, uint32_t primitive_count) -> bool override;
        auto draw_indexed(primitive type, uint32_t min_index, uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count) -> bool override;
        auto draw_user(primitive type, uint32_t primitive_count, const void *vertices, uint32_t stride) -> bool override;
        auto draw_indexed_user(primitive type, uint32_t min_index, uint32_t vertex_count, uint32_t primitive_count, const void *indices, format index_format, const void *vertices, uint32_t stride) -> bool override;

        auto create_occlusion_query() -> uint32_t override;
        void destroy_occlusion_query(uint32_t query) override;
        void begin_occlusion_query(uint32_t query) override;
        void end_occlusion_query() override;
        auto occlusion_query_result(uint32_t query, uint32_t &samples) -> bool override;

        void blit(texture *source, const blit_params &params) override;

      private:
        struct stream {
            std::shared_ptr<gl_vertex_buffer> buffer;
            uint32_t stride = 0;
        };

        auto create_program() -> bool;
        auto create_blit_program() -> bool;
        auto current_layout() -> const vertex_layout *;
        void bind_target();
        void apply_viewport();
        void apply_scissor();
        void apply_pipeline_state();
        void apply_sampler(int stage);
        void prepare_draw(const vertex_layout &layout, const GLuint *buffers, const uint32_t *strides, const uint32_t *offsets);
        void submit(primitive type, uint32_t primitive_count, GLenum index_type, GLuint indices, size_t index_offset, int32_t base_vertex, uint32_t first_vertex);
        auto stream_user_data(GLenum target, GLuint buffer, size_t capacity, size_t &offset, const void *data, size_t size, size_t alignment) -> size_t;

        device_settings settings_;
        device_caps caps_;
        fixed_function_state state_;
        fixed_function_uniforms uniforms_ = {};
        std::shared_ptr<gl_texture> textures_[fixed_function_state::STAGES];

        std::vector<std::unique_ptr<vertex_layout>> layouts_;
        uint32_t vertex_format_ = 0;
        vertex_layout fvf_layout_;
        uint32_t fvf_layout_code_ = 0xffffffff;
        const vertex_layout *uniform_layout_ = nullptr;

        stream streams_[4];
        std::shared_ptr<gl_index_buffer> indices_;
        uint32_t base_vertex_ = 0;

        std::shared_ptr<gl_texture> target_;
        std::shared_ptr<gl_depth_buffer> depth_target_;
        GLuint target_framebuffer_ = 0;
        uint32_t target_width_ = 0;
        uint32_t target_height_ = 0;
        bool flip_y_ = false;
        bool scissor_ = false;
        rectangle scissor_rect_ = {};

        GLuint program_ = 0;
        GLuint vertex_shader_ = 0; // kept to link the pixel shaders with
        std::shared_ptr<gl_pixel_shader> pixel_shader_;
        pixel_shader_params pixel_shader_params_;
        bool pixel_shader_params_dirty_ = false;
        GLuint uniform_buffer_ = 0;
        GLuint vertex_array_ = 0;
        GLuint user_vertices_ = 0;
        GLuint user_indices_ = 0;
        size_t user_vertices_offset_ = 0;
        size_t user_indices_offset_ = 0;
        GLuint samplers_[2] = {0, 0};
        GLuint white_texture_ = 0;
        GLuint blit_program_ = 0;
        GLuint blit_sampler_ = 0;
        GLuint blit_vertex_array_ = 0;
        GLint blit_gamma_location_ = -1;
        GLint blit_antialias_location_ = -1;
        GLint blit_texel_location_ = -1;

        // What prepare_draw last bound, so unchanged bindings are not sent again (see binding_epoch).
        struct attribute_binding {
            GLuint buffer = 0;
            uint32_t offset = 0;
            GLsizei stride = 0;
            int kind = -1;
            int components = 0;
            bool enabled = false;
        };
        uint64_t binding_epoch_seen_ = ~uint64_t{0};
        GLuint bound_program_ = 0;
        GLuint bound_vertex_array_ = 0;
        GLuint bound_textures_[2] = {0, 0};
        GLuint bound_samplers_[2] = {0, 0};
        attribute_binding attributes_[7];
        void forget_bindings();

        // An identical fixed-function block is not uploaded again. (A ring of blocks bound by range
        // measured slower than rewriting one buffer on radeonsi.)
        fixed_function_uniforms uploaded_uniforms_ = {};
        bool uniforms_uploaded_ = false;

        bool uniforms_dirty_ = true;
        bool pipeline_dirty_ = true;
        bool sampler_dirty_[2] = {true, true};
        bool viewport_dirty_ = true;
    };
}
