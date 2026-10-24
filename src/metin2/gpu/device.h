#pragma once

#include "fixed_function.h"
#include "format.h"
#include "types.h"
#include "vertex_layout.h"

#include <metin2/core/rectangle.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// The renderer interface. It keeps the Direct3D fixed-function model the game is written against:
// render and texture stage states, transforms, lights and a material, vertex formats, buffers that
// are locked and written by the CPU. A backend turns that state into its own objects; the shared
// parts (state defaults, the uniform block, the shader source) are in fixed_function.h.
//
// Resources are shared: the device holds a reference while one is bound, like Direct3D did, so a
// resource released while bound stays alive until it is unbound.
namespace metin2::gpu {

    class device;

    struct locked_rect {
        uint32_t pitch = 0;
        uint8_t *bits = nullptr;
    };

    class texture : public std::enable_shared_from_this<texture> {
      public:
        texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, format fmt)
            : width_(width), height_(height), levels_(levels), usage_(usage), format_(fmt) {
        }
        texture(const texture &) = delete;
        auto operator=(const texture &) -> texture & = delete;
        virtual ~texture() = default;

        auto width() const -> uint32_t { return width_; }
        auto height() const -> uint32_t { return height_; }
        auto level_count() const -> uint32_t { return levels_; }
        auto usage() const -> uint32_t { return usage_; }
        auto pixel_format() const -> format { return format_; }
        auto level_width(uint32_t level) const -> uint32_t { return width_ >> level ? width_ >> level : 1; }
        auto level_height(uint32_t level) const -> uint32_t { return height_ >> level ? height_ >> level : 1; }

        // Pixels in the texture's format; area is in level pixels (whole level when null).
        virtual auto lock(uint32_t level, locked_rect &out, const rectangle *area = nullptr, uint32_t flags = 0) -> bool = 0;
        virtual void unlock(uint32_t level) = 0;
        // A whole level, in the texture's format or as RGBA bytes converted to it.
        virtual void upload(uint32_t level, const void *data) = 0;
        virtual void upload_rgba8(uint32_t level, const uint8_t *rgba) = 0;
        virtual void generate_mipmaps() = 0;

      protected:
        uint32_t width_;
        uint32_t height_;
        uint32_t levels_;
        uint32_t usage_;
        format format_;
    };

    class vertex_buffer : public std::enable_shared_from_this<vertex_buffer> {
      public:
        vertex_buffer(uint32_t size, uint32_t usage) : size_(size), usage_(usage) {
        }
        vertex_buffer(const vertex_buffer &) = delete;
        auto operator=(const vertex_buffer &) -> vertex_buffer & = delete;
        virtual ~vertex_buffer() = default;

        auto size() const -> uint32_t { return size_; }
        auto usage() const -> uint32_t { return usage_; }

        // size 0 locks to the end.
        virtual auto lock(uint32_t offset, uint32_t size, void **data, uint32_t flags = 0) -> bool = 0;
        virtual void unlock() = 0;

      protected:
        uint32_t size_;
        uint32_t usage_;
    };

    class index_buffer : public std::enable_shared_from_this<index_buffer> {
      public:
        index_buffer(uint32_t size, uint32_t usage, format fmt) : size_(size), usage_(usage), format_(fmt) {
        }
        index_buffer(const index_buffer &) = delete;
        auto operator=(const index_buffer &) -> index_buffer & = delete;
        virtual ~index_buffer() = default;

        auto size() const -> uint32_t { return size_; }
        auto usage() const -> uint32_t { return usage_; }
        auto index_format() const -> format { return format_; }

        virtual auto lock(uint32_t offset, uint32_t size, void **data, uint32_t flags = 0) -> bool = 0;
        virtual void unlock() = 0;

      protected:
        uint32_t size_;
        uint32_t usage_;
        format format_;
    };

    class depth_buffer : public std::enable_shared_from_this<depth_buffer> {
      public:
        depth_buffer(uint32_t width, uint32_t height, format fmt) : width_(width), height_(height), format_(fmt) {
        }
        depth_buffer(const depth_buffer &) = delete;
        auto operator=(const depth_buffer &) -> depth_buffer & = delete;
        virtual ~depth_buffer() = default;

        auto width() const -> uint32_t { return width_; }
        auto height() const -> uint32_t { return height_; }
        auto depth_format() const -> format { return format_; }

      protected:
        uint32_t width_;
        uint32_t height_;
        format format_;
    };

    // What the backend implements; the game checks these instead of assuming.
    struct device_caps {
        uint32_t max_texture_size = 2048;
        uint32_t max_anisotropy = 1;
        uint32_t max_texture_stages = 2;
        uint32_t max_lights = 8;
        bool compressed_textures = false; // DXT1-5
        bool anisotropic_filter = false;
        bool render_to_texture = true;
        bool index32 = true;
        bool stencil = false;
    };

    struct device_settings {
        uint32_t width = 800;
        uint32_t height = 600;
        bool depth_buffer = true;
        bool vsync = false;
    };

    // Handles from create_vertex_layout have this bit set; set_vertex_format takes either a handle or
    // a gpu::fvf vertex format.
    constexpr uint32_t VERTEX_LAYOUT_HANDLE = 0x40000000u;

    class device {
      public:
        device() = default;
        device(const device &) = delete;
        auto operator=(const device &) -> device & = delete;
        virtual ~device() = default;

        virtual auto backend_name() const -> std::string_view = 0;
        virtual auto adapter_name() const -> std::string = 0;
        virtual auto caps() const -> const device_caps & = 0;
        // Whether textures (or render targets / depth buffers, by usage) of this format can be made.
        virtual auto check_format(format fmt, uint32_t usage) const -> bool = 0;
        virtual auto available_texture_memory() const -> uint32_t = 0;

        // levels 0 is the full chain.
        virtual auto create_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, format fmt) -> std::shared_ptr<texture> = 0;
        virtual auto create_vertex_buffer(uint32_t size, uint32_t usage) -> std::shared_ptr<vertex_buffer> = 0;
        virtual auto create_index_buffer(uint32_t size, uint32_t usage, format fmt) -> std::shared_ptr<index_buffer> = 0;
        virtual auto create_depth_buffer(uint32_t width, uint32_t height, format fmt) -> std::shared_ptr<depth_buffer> = 0;
        virtual auto create_vertex_layout(std::span<const vertex_element> elements) -> uint32_t = 0;
        virtual void destroy_vertex_layout(uint32_t handle) = 0;

        // The back buffer follows the window; reset after it changed size.
        virtual auto reset(const device_settings &settings) -> bool = 0;
        virtual auto back_buffer_width() const -> uint32_t = 0;
        virtual auto back_buffer_height() const -> uint32_t = 0;
        virtual void present() = 0;
        // Top-down rows of B, G, R, A bytes.
        virtual auto read_back_buffer(std::vector<uint8_t> &bgra) -> bool = 0;

        virtual void set_render_state(state s, uint32_t value) = 0;
        virtual auto render_state(state s) const -> uint32_t = 0;
        virtual void set_texture_stage_state(uint32_t stage, stage_state s, uint32_t value) = 0;
        virtual auto texture_stage_state(uint32_t stage, stage_state s) const -> uint32_t = 0;
        virtual void set_texture(uint32_t stage, texture *tex) = 0;
        virtual void set_transform(transform t, const math::matrix &m) = 0;
        virtual auto get_transform(transform t) const -> math::matrix = 0;
        virtual void set_material(const material &m) = 0;
        virtual void set_light(uint32_t index, const light &l) = 0;
        virtual void enable_light(uint32_t index, bool enable) = 0;
        virtual void set_viewport(const viewport &vp) = 0;
        virtual auto get_viewport() const -> viewport = 0;
        virtual void set_vertex_format(uint32_t format_or_handle) = 0;
        virtual void set_stream(uint32_t stream, vertex_buffer *buffer, uint32_t stride) = 0;
        virtual void set_indices(index_buffer *buffer, uint32_t base_vertex) = 0;

        // A null colour target is the back buffer; a null depth buffer with the back buffer is its own
        // depth. The viewport is reset to cover the target.
        virtual auto set_render_target(texture *color, depth_buffer *depth) -> bool = 0;
        virtual auto render_target() const -> texture * = 0;
        virtual auto depth_target() const -> depth_buffer * = 0;

        virtual void clear(std::span<const rectangle> areas, uint32_t flags, uint32_t color, float z, uint32_t stencil) = 0;
        virtual auto draw(primitive type, uint32_t start_vertex, uint32_t primitive_count) -> bool = 0;
        virtual auto draw_indexed(primitive type, uint32_t min_index, uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count) -> bool = 0;
        virtual auto draw_user(primitive type, uint32_t primitive_count, const void *vertices, uint32_t stride) -> bool = 0;
        virtual auto draw_indexed_user(primitive type, uint32_t min_index, uint32_t vertex_count, uint32_t primitive_count, const void *indices, format index_format, const void *vertices, uint32_t stride) -> bool = 0;

        // An occlusion query counts the samples the draws between begin and end leave after the depth
        // test. The result is read without waiting: occlusion_query_result is false until the GPU has
        // finished those draws, usually a frame later, so callers keep a query per frame in flight.
        virtual auto create_occlusion_query() -> uint32_t = 0;
        virtual void destroy_occlusion_query(uint32_t query) = 0;
        virtual void begin_occlusion_query(uint32_t query) = 0;
        virtual void end_occlusion_query() = 0;
        virtual auto occlusion_query_result(uint32_t query, uint32_t &samples) -> bool = 0;

        // Called around the buffer swap in present (the dev tools and window code hook in here).
        void set_present_hooks(std::function<void()> before_swap, std::function<void()> after_swap) {
            before_swap_ = std::move(before_swap);
            after_swap_ = std::move(after_swap);
        }

      protected:
        std::function<void()> before_swap_;
        std::function<void()> after_swap_;
    };

    // The backends built into this client, best first.
    auto available_backends() -> std::span<const std::string_view>;
    // Creates the named backend on the current window; unknown or failing names fall back to the
    // first available backend that works. Null when none does.
    auto create_device(std::string_view backend, const device_settings &settings) -> std::unique_ptr<device>;

    auto primitive_vertex_count(primitive type, uint32_t primitive_count) -> uint32_t;
}
