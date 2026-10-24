// Copyright (C) 1999, 2000 NVIDIA Corporation
//
// This file is provided without support, instruction, or implied warranty of any
// kind.  NVIDIA makes no guarantee of its fitness for a particular purpose and is
// not liable under any circumstances for any damages or loss whatsoever arising
// from the use or inability to use this file or items derived from it.
//
// Render state cache. Use the save_* functions to deviate from the current state and
// restore_* to return to the last save. Sets, saves and restores are tracked for
// redundancy, and the flush before rendering only sends state that differs from what the
// device already has.

#pragma once

#include <metin2/gpu/device.h>
#include <vector>

namespace metin2::inline graphics {

    static const uint32_t STATEMANAGER_MAX_RENDERSTATES = 256;
    static const uint32_t STATEMANAGER_MAX_TEXTURESTATES = 128;
    static const uint32_t STATEMANAGER_MAX_STAGES = 8;
    static const uint32_t STATEMANAGER_MAX_TRANSFORMSTATES = 300; // gpu::transform::WORLD1 is 257.
    static const uint32_t STATEMANAGER_MAX_STREAMS = 16;

    class stream_data {
      public:
        stream_data(gpu::vertex_buffer *stream_data = nullptr, uint32_t stride_value = 0) : stream_data_value(stream_data), stride(stride_value) {
        }

        auto operator==(const stream_data &rhs) const -> bool {
            return ((stream_data_value == rhs.stream_data_value) && (stride == rhs.stride));
        }

        gpu::vertex_buffer *stream_data_value;
        uint32_t stride;
    };

    class index_data {
      public:
        index_data(gpu::index_buffer *index_data = nullptr, uint32_t base_vertex_index_value = 0)
            : index_data_value(index_data),
              base_vertex_index(base_vertex_index_value) {
        }

        auto operator==(const index_data &rhs) const -> bool {
            return ((index_data_value == rhs.index_data_value) && (base_vertex_index == rhs.base_vertex_index));
        }

        gpu::index_buffer *index_data_value;
        uint32_t base_vertex_index;
    };

    enum e_state_type {
        STATE_MATERIAL = 0,
        STATE_RENDER,
        STATE_TEXTURE,
        STATE_TEXTURESTAGE,
        STATE_VSHADER,
        STATE_TRANSFORM,
        STATE_STREAM,
        STATE_INDEX
    };

    class state_id {
      public:
        state_id(e_state_type type, uint32_t value0 = 0, uint32_t value1 = 0)
            : type_value(type),
              value0(value0),
              value1(value1) {
        }

        state_id(e_state_type type, uint32_t stage, gpu::stage_state stage_type)
            : type_value(type),
              stage(stage),
              texture_stage_state_type(stage_type) {
        }

        state_id(e_state_type type, gpu::state render_type)
            : type_value(type),
              render_state_type(render_type) {
        }

        e_state_type type_value;

        union {
            uint32_t value0;
            uint32_t stage;
            gpu::state render_state_type;
            gpu::transform transform_state_type;
        };

        union {
            uint32_t value1;
            gpu::stage_state texture_stage_state_type;
        };
    };

    using state_id_type = std::vector<state_id>;

    class state_manager_state {
      public:
        state_manager_state() = default;

        void reset_state() {
            uint32_t i, y;

            for (i = 0; i < STATEMANAGER_MAX_RENDERSTATES; i++)
                render_states[i] = 0x7FFFFFFF;

            for (i = 0; i < STATEMANAGER_MAX_STAGES; i++)
                for (y = 0; y < STATEMANAGER_MAX_TEXTURESTATES; y++)
                    texture_states[i][y] = 0x7FFFFFFF;

            for (i = 0; i < STATEMANAGER_MAX_STREAMS; i++)
                stream_data_value[i] = stream_data();

            index_data_value = index_data();

            for (i = 0; i < STATEMANAGER_MAX_STAGES; i++)
                textures[i] = nullptr;

            // Matrices are not cached, just restored. They start zeroed, as they always did.
            for (i = 0; i < STATEMANAGER_MAX_TRANSFORMSTATES; i++)
                matrices[i] = math::matrix();

            vertex_shader = gpu::fvf::XYZ;
        }

        uint32_t render_states[STATEMANAGER_MAX_RENDERSTATES];

        uint32_t texture_states[STATEMANAGER_MAX_STAGES][STATEMANAGER_MAX_TEXTURESTATES];

        gpu::texture *textures[STATEMANAGER_MAX_STAGES];

        uint32_t vertex_shader;

        math::matrix matrices[STATEMANAGER_MAX_TRANSFORMSTATES];

        gpu::material material_value;

        stream_data stream_data_value[STATEMANAGER_MAX_STREAMS];
        index_data index_data_value;
    };

    class state_manager {
      public:
        explicit state_manager(gpu::device &device);
        ~state_manager();

        void set_default_state();

        auto begin_scene() -> bool;
        void end_scene();

        void save_material();
        void save_material(const gpu::material *material);
        void set_material(const gpu::material *material);
        void get_material(gpu::material *material);

        void set_light(uint32_t index, const gpu::light *light);
        void enable_light(uint32_t index, bool enable);
        void get_light(uint32_t index, gpu::light *light);

        void save_render_state(gpu::state type, uint32_t value);
        void restore_render_state(gpu::state type);
        void set_render_state(gpu::state type, uint32_t value);
        void render_state(gpu::state type, uint32_t *value);

        void set_texture(uint32_t stage, gpu::texture *texture);
        void texture(uint32_t stage, gpu::texture **texture);

        void save_texture_stage_state(uint32_t stage, gpu::stage_state type, uint32_t value);
        void restore_texture_stage_state(uint32_t stage, gpu::stage_state type);
        void set_texture_stage_state(uint32_t stage, gpu::stage_state type, uint32_t value);
        void texture_stage_state(uint32_t stage, gpu::stage_state type, uint32_t *value);
        void set_best_filtering(uint32_t stage);

        void save_vertex_shader(uint32_t shader);
        void restore_vertex_shader();
        void set_vertex_shader(uint32_t shader);
        void vertex_shader(uint32_t *shader);

        // Transforms are cached, but not protected from multiple sends of the same value.
        void save_transform(gpu::transform transform, const math::matrix *matrix);
        void restore_transform(gpu::transform transform);

        void set_transform(gpu::transform type, const math::matrix *matrix);
        void get_transform(gpu::transform type, math::matrix *matrix);

        void set_stream_source(uint32_t stream_number, gpu::vertex_buffer *stream_data, uint32_t stride);

        void set_indices(gpu::index_buffer *index_data, uint32_t base_vertex_index);

        auto draw_primitive(gpu::primitive primitive_type, uint32_t start_vertex, uint32_t primitive_count) -> bool;
        auto draw_primitive_up(gpu::primitive primitive_type, uint32_t primitive_count, const void *vertex_stream_zero_data, uint32_t vertex_stream_zero_stride) -> bool;
        auto draw_indexed_primitive(gpu::primitive primitive_type, uint32_t min_index, uint32_t num_vertices, uint32_t start_index, uint32_t prim_count) -> bool;
        auto draw_indexed_primitive_up(gpu::primitive primitive_type, uint32_t min_vertex_index, uint32_t num_vertex_indices, uint32_t primitive_count, const void *index_data, gpu::format index_data_format, const void *vertex_stream_zero_data, uint32_t vertex_stream_zero_stride) -> bool;

        auto create_occlusion_query() -> uint32_t;
        void destroy_occlusion_query(uint32_t query);
        void begin_occlusion_query(uint32_t query);
        void end_occlusion_query();
        auto occlusion_query_result(uint32_t query, uint32_t &samples) -> bool;

        auto render_state(gpu::state type) -> uint32_t;

      private:
        state_manager_state chip_state_;
        state_manager_state current_state_;
        state_manager_state copy_state_;
        state_id_type dirty_states_;
        bool force_;
        bool scene_;
        uint32_t best_min_filter_;
        uint32_t best_mag_filter_;
        uint32_t max_anisotropy_;
        gpu::device &device_;
    };

    auto get_state_manager() -> state_manager &;

#define STATEMANAGER (get_state_manager())
}
