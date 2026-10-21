#pragma once

#include "types.h"
#include "vertex_layout.h"

#include <cstdint>

namespace metin2::gpu {

    struct viewport {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        float min_z = 0.0f;
        float max_z = 1.0f;
    };

    // Everything the fixed-function pipeline reads, as the game set it. Backends keep one and turn it
    // into their own state and the uniform block below.
    struct fixed_function_state {
        static constexpr int STAGES = 8;
        static constexpr int EVALUATED_STAGES = 2; // the game combines at most two textures
        static constexpr int LIGHTS = 8;

        uint32_t render_states[256];
        uint32_t stage_states[STAGES][32];
        bool texture_bound[STAGES];
        math::matrix world;
        math::matrix view;
        math::matrix projection;
        math::matrix texture_matrices[STAGES];
        material mat;
        light lights[LIGHTS];
        bool light_enabled[LIGHTS];
        viewport view_port;

        // The Direct3D defaults; the viewport covers the render target.
        void reset(uint32_t target_width, uint32_t target_height, bool depth_buffer);

        auto render_state(state s) const -> uint32_t { return render_states[static_cast<uint32_t>(s)]; }
        auto render_state_float(state s) const -> float;
        auto stage_state(int stage, gpu::stage_state s) const -> uint32_t { return stage_states[stage][static_cast<uint32_t>(s)]; }
        // null for transforms the pipeline does not use
        auto transform_slot(transform t) -> math::matrix *;
        auto transform_slot(transform t) const -> const math::matrix *;
    };

    // Render states that become pipeline state (depth, blending, culling...) rather than uniforms.
    auto is_pipeline_state(state s) -> bool;
    // Texture stage states that belong to the sampler.
    auto is_sampler_state(stage_state s) -> bool;

    // The uniform block of the fixed-function shader (std140, 1600 bytes).
    struct alignas(16) fixed_function_light {
        float diffuse[4];
        float specular[4];
        float ambient[4];
        float position[4];  // view space; w is the light type (0 when off)
        float direction[4]; // view space; w is the range
        float attenuation[4];
        float spot[4]; // cos(theta / 2), cos(phi / 2)
    };

    struct alignas(16) fixed_function_uniforms {
        float world[16];
        float view[16];
        float projection[16];
        float normal[16];
        float texture_matrices[2][16];
        float viewport[4]; // width, height, -1 when rendering to a texture, 1 for the half-pixel offset
        float material_diffuse[4];
        float material_ambient[4];
        float material_specular[4];
        float material_emissive[4];
        float global_ambient[4]; // w is the material's specular power
        int32_t light_flags[4];  // lighting, colour vertex, specular, local viewer
        int32_t material_source[4];
        int32_t fog_modes[4]; // enable, vertex mode, table mode, range fog
        float fog_parameters[4];
        float fog_color[4];
        float texture_factor[4];
        float misc[4];         // alpha reference, pre-transformed, has diffuse, has specular
        int32_t alpha[4];      // alpha test enable, compare function; viewport x, y (pre-transformed vertices are in target pixels)
        int32_t stage_a[2][4]; // colour op, colour arg 1, colour arg 2, alpha op
        int32_t stage_b[2][4]; // alpha arg 1, alpha arg 2, texture coordinate index, generation mode
        int32_t stage_c[2][4]; // transform count, texture bound, input dimension, projected
        fixed_function_light lights[8];
    };
    static_assert(sizeof(fixed_function_uniforms) == 1600, "std140 layout of the FFP block");

    struct uniform_options {
        bool flip_y = false;    // rendering into a texture: the shader flips y
        bool half_pixel = true; // Direct3D samples pixel centres at integer coordinates
    };

    void pack_uniforms(const fixed_function_state &state, const vertex_layout &layout, const uniform_options &options, fixed_function_uniforms &uniforms);

    // GLSL 3.30 sources of the shader that evaluates the block.
    extern const char *const FIXED_FUNCTION_VERTEX_SHADER;
    extern const char *const FIXED_FUNCTION_FRAGMENT_SHADER;
}
