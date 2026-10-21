#include "grp_device.h"
#include <cassert>
#include <memory>
#include <metin2/core/stl.h>
#include <metin2/gpu/types.h>
#include <metin2/log/log.h>

namespace metin2::inline graphics {

    // The state cache lives as long as the device it mirrors.
    static std::unique_ptr<state_manager> state_manager_instance;

    auto get_state_manager() -> state_manager & {
        assert(state_manager_instance);
        return *state_manager_instance;
    }

    bool graphics_caps_software_tiling = false;

    graphic_device::graphic_device() {
        initialize_impl();
    }

    graphic_device::~graphic_device() {
        destroy();
    }

    void graphic_device::initialize_impl() {
        waving_end_time_ = 0;
        flashing_end_time_ = 0;
    }

    auto graphic_device::resize_back_buffer(uint32_t width_value, uint32_t height) -> bool {
        if (!device_)
            return false;

        // the 2D projection, the cursor ray and the screen filter use the screen size
        width_ = static_cast<int>(width_value);
        height_ = static_cast<int>(height);

        if (device_->back_buffer_width() != width_value || device_->back_buffer_height() != height) {
            gpu::device_settings settings;
            settings.width = width_value;
            settings.height = height;
            if (!device_->reset(settings))
                return false;
            // names and the lens flare project with the cached viewport
            viewport_ = device_->get_viewport();

            STATEMANAGER.set_default_state();
        }

        return true;
    }

    static uint32_t max_texture_width, max_texture_height;

    auto get_max_texture_width() -> uint32_t {
        return max_texture_width;
    }

    auto get_max_texture_height() -> uint32_t {
        return max_texture_height;
    }

    auto graphic_device::create(int hres, int vres, const char *renderer) -> bool {
        destroy();

        width_ = hres;
        height_ = vres;

        gpu::device_settings settings;
        settings.width = static_cast<uint32_t>(hres);
        settings.height = static_cast<uint32_t>(vres);
        device_ = gpu::create_device(renderer, settings);
        if (!device_)
            return false;

        metin2::log::info("Renderer: {} on {}", device_->backend_name(), device_->adapter_name());

        const gpu::device_caps &caps = device_->caps();
        support_dxt_ = caps.compressed_textures;
        max_texture_width = caps.max_texture_size;
        max_texture_height = caps.max_texture_size;

        viewport_ = device_->get_viewport();

        state_manager_instance = std::make_unique<state_manager>(*device_);

        matrix_stack_.load_identity();

        static constexpr gpu::vertex_element pt_layout[] = {
            {0, gpu::vertex_usage::POSITION,  gpu::vertex_type::FLOAT3},
            {1, gpu::vertex_usage::TEXCOORD0, gpu::vertex_type::FLOAT2},
        };
        static constexpr gpu::vertex_element pnt_layout[] = {
            {0, gpu::vertex_usage::POSITION,  gpu::vertex_type::FLOAT3},
            {0, gpu::vertex_usage::NORMAL,    gpu::vertex_type::FLOAT3},
            {0, gpu::vertex_usage::TEXCOORD0, gpu::vertex_type::FLOAT2},
        };
        static constexpr gpu::vertex_element pnt2_layout[] = {
            {0, gpu::vertex_usage::POSITION,  gpu::vertex_type::FLOAT3},
            {0, gpu::vertex_usage::NORMAL,    gpu::vertex_type::FLOAT3},
            {0, gpu::vertex_usage::TEXCOORD0, gpu::vertex_type::FLOAT2},
            {0, gpu::vertex_usage::TEXCOORD1, gpu::vertex_type::FLOAT2},
        };
        vs_ = device_->create_vertex_layout(pt_layout);
        pnt_vs_ = device_->create_vertex_layout(pnt_layout);
        pnt2vs_ = device_->create_vertex_layout(pnt2_layout);

        identity_ = math::matrix_identity();
        view_ = math::matrix_identity();
        proj_ = math::matrix_identity();
        inverse_view_ = math::matrix_identity();
        inverse_view_y_axis_ = math::matrix_identity();
        screen0_ = math::matrix_identity();
        screen1_ = math::matrix_identity();
        screen2_ = math::matrix_identity();

        screen0_._11 = 1;
        screen0_._22 = -1;

        screen1_._41 = 1;
        screen1_._42 = 1;

        screen2_._11 = (float)hres / 2;
        screen2_._22 = (float)vres / 2;

        sphere_mesh_ = gpu::make_sphere(*device_, 1.0f, 32, 32);
        cylinder_mesh_ = gpu::make_cylinder(*device_, 1.0f, 1.0f, 1.0f, 8, 8);

        device_->clear({}, gpu::clear::TARGET | gpu::clear::Z_BUFFER, 0xff000000, 1.0f, 0);

        if (!create_default_index_buffer_list())
            return false;

        if (!create_pdt_vertex_buffer_list())
            return false;

        const uint32_t tex_mem_size = available_texture_memory();
        is_low_texture_memory_ = tex_mem_size < 64 * 1024 * 1024;
        is_high_texture_memory_ = tex_mem_size > 100 * 1024 * 1024;

        return true;
    }

    void graphic_device::destroy_pdt_vertex_buffer_list() {
        for (auto &buffer : pdt_vertex_buffers_)
            buffer.reset();
    }

    auto graphic_device::create_pdt_vertex_buffer_list() -> bool {
        for (auto &buffer : pdt_vertex_buffers_) {
            buffer = device_->create_vertex_buffer(sizeof(pdt_vertex) * PDT_VERTEX_NUM, gpu::usage::DYNAMIC | gpu::usage::WRITE_ONLY);
            if (!buffer)
                return false;
        }
        return true;
    }

    void graphic_device::destroy_default_index_buffer_list() {
        for (auto &buffer : default_index_buffers_)
            buffer.reset();
    }

    auto graphic_device::create_default_index_buffer(uint32_t def_ib, uint32_t idx_count, const uint16_t *indices) -> bool {
        assert(!default_index_buffers_[def_ib]);

        auto buffer = device_->create_index_buffer(sizeof(uint16_t) * idx_count, gpu::usage::WRITE_ONLY, gpu::format::INDEX16);
        if (!buffer)
            return false;

        void *dst_indices;
        if (!buffer->lock(0, 0, &dst_indices))
            return false;

        memcpy(dst_indices, indices, sizeof(uint16_t) * idx_count);

        buffer->unlock();

        default_index_buffers_[def_ib] = std::move(buffer);
        return true;
    }

    auto graphic_device::create_default_index_buffer_list() -> bool {
        static const uint16_t line_indices[2] = {
            0,
            1,
        };
        static const uint16_t line_tri_indices[6] = {
            0,
            1,
            0,
            2,
            1,
            2,
        };
        static const uint16_t line_rect_indices[8] = {
            0,
            1,
            0,
            2,
            1,
            3,
            2,
            3,
        };
        static const uint16_t line_cube_indices[24] = {
            0,
            1,
            0,
            2,
            1,
            3,
            2,
            3,
            0,
            4,
            1,
            5,
            2,
            6,
            3,
            7,
            4,
            5,
            4,
            6,
            5,
            7,
            6,
            7,
        };
        static const uint16_t fill_tri_indices[3] = {
            0,
            1,
            2,
        };
        static const uint16_t fill_rect_indices[6] = {
            0,
            2,
            1,
            2,
            3,
            1,
        };
        static const uint16_t fill_cube_indices[36] = {
            0,
            1,
            2,
            1,
            3,
            2,
            2,
            0,
            6,
            0,
            4,
            6,
            0,
            1,
            4,
            1,
            5,
            4,
            1,
            3,
            5,
            3,
            7,
            5,
            3,
            2,
            7,
            2,
            6,
            7,
            4,
            5,
            6,
            5,
            7,
            6,
        };

        if (!create_default_index_buffer(DEFAULT_IB_LINE, 2, line_indices))
            return false;
        if (!create_default_index_buffer(DEFAULT_IB_LINE_TRI, 6, line_tri_indices))
            return false;
        if (!create_default_index_buffer(DEFAULT_IB_LINE_RECT, 8, line_rect_indices))
            return false;
        if (!create_default_index_buffer(DEFAULT_IB_LINE_CUBE, 24, line_cube_indices))
            return false;
        if (!create_default_index_buffer(DEFAULT_IB_FILL_TRI, 3, fill_tri_indices))
            return false;
        if (!create_default_index_buffer(DEFAULT_IB_FILL_RECT, 6, fill_rect_indices))
            return false;
        if (!create_default_index_buffer(DEFAULT_IB_FILL_CUBE, 36, fill_cube_indices))
            return false;

        return true;
    }

    void graphic_device::destroy() {
        destroy_pdt_vertex_buffer_list();
        destroy_default_index_buffer_list();

        if (device_) {
            for (uint32_t *layout : {&vs_, &pnt_vs_, &pnt2vs_}) {
                if (*layout)
                    device_->destroy_vertex_layout(*layout);
                *layout = 0;
            }
        }

        sphere_mesh_ = {};
        cylinder_mesh_ = {};

        state_manager_instance.reset();
        device_.reset();

        initialize_impl();
    }
}
