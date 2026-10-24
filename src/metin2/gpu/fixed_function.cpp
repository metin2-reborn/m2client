#include "fixed_function.h"

#include <cmath>
#include <cstring>

namespace metin2::gpu {

    namespace {
        auto float_bits(float f) -> uint32_t {
            uint32_t bits;
            memcpy(&bits, &f, sizeof(bits));
            return bits;
        }

        void color_to_floats(uint32_t argb, float *out) {
            out[0] = static_cast<float>((argb >> 16) & 255) / 255.0f;
            out[1] = static_cast<float>((argb >> 8) & 255) / 255.0f;
            out[2] = static_cast<float>(argb & 255) / 255.0f;
            out[3] = static_cast<float>((argb >> 24) & 255) / 255.0f;
        }

        // Inverse transpose of the upper 3x3 of world * view. Matrices are uploaded as they are, so the
        // shader's M * v is the row-vector v * M; the normal needs v * inverse(M)^T.
        void normal_matrix(const math::matrix &world_view, float *out) {
            float a[3][3];
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    a[i][j] = world_view.m[i][j];
            const float det = a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
            float inv[3][3];
            if (fabsf(det) < 1e-20f) {
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        inv[i][j] = i == j ? 1.0f : 0.0f;
            } else {
                const float id = 1.0f / det;
                inv[0][0] = (a[1][1] * a[2][2] - a[1][2] * a[2][1]) * id;
                inv[0][1] = (a[0][2] * a[2][1] - a[0][1] * a[2][2]) * id;
                inv[0][2] = (a[0][1] * a[1][2] - a[0][2] * a[1][1]) * id;
                inv[1][0] = (a[1][2] * a[2][0] - a[1][0] * a[2][2]) * id;
                inv[1][1] = (a[0][0] * a[2][2] - a[0][2] * a[2][0]) * id;
                inv[1][2] = (a[0][2] * a[1][0] - a[0][0] * a[1][2]) * id;
                inv[2][0] = (a[1][0] * a[2][1] - a[1][1] * a[2][0]) * id;
                inv[2][1] = (a[0][1] * a[2][0] - a[0][0] * a[2][1]) * id;
                inv[2][2] = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) * id;
            }
            memset(out, 0, 64);
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    out[i * 4 + j] = inv[j][i];
            out[15] = 1.0f;
        }
    }

    void fixed_function_state::reset(uint32_t target_width, uint32_t target_height, bool depth_buffer) {
        memset(render_states, 0, sizeof(render_states));
        auto set = [this](state s, uint32_t value) { render_states[static_cast<uint32_t>(s)] = value; };
        set(state::Z_ENABLE, depth_buffer ? 1 : 0);
        set(state::FILL_MODE, fill::SOLID);
        set(state::SHADE_MODE, shade::GOURAUD);
        set(state::Z_WRITE_ENABLE, 1);
        set(state::LAST_PIXEL, 1);
        set(state::SRC_BLEND, blend::ONE);
        set(state::DEST_BLEND, blend::ZERO);
        set(state::CULL_MODE, cull::CCW);
        set(state::Z_FUNC, compare::LESS_EQUAL);
        set(state::ALPHA_FUNC, compare::ALWAYS);
        set(state::FOG_TABLE_MODE, fog::NONE);
        set(state::FOG_START, float_bits(0.0f));
        set(state::FOG_END, float_bits(1.0f));
        set(state::FOG_DENSITY, float_bits(1.0f));
        set(state::STENCIL_MASK, 0xffffffff);
        set(state::STENCIL_WRITE_MASK, 0xffffffff);
        set(state::TEXTURE_FACTOR, 0xffffffff);
        set(state::LIGHTING, 1);
        set(state::FOG_VERTEX_MODE, fog::NONE);
        set(state::COLOR_VERTEX, 1);
        set(state::LOCAL_VIEWER, 1);
        set(state::DIFFUSE_MATERIAL_SOURCE, material_source::COLOR1);
        set(state::SPECULAR_MATERIAL_SOURCE, material_source::COLOR2);
        set(state::AMBIENT_MATERIAL_SOURCE, material_source::MATERIAL);
        set(state::EMISSIVE_MATERIAL_SOURCE, material_source::MATERIAL);
        set(state::COLOR_WRITE_ENABLE, 0xf);
        set(state::BLEND_OP, blend_op::ADD);
        set(state::POINT_SIZE, float_bits(1.0f));
        set(state::CLIPPING, 1);

        memset(stage_states, 0, sizeof(stage_states));
        for (int s = 0; s < STAGES; ++s) {
            uint32_t *st = stage_states[s];
            st[static_cast<uint32_t>(gpu::stage_state::COLOR_OP)] = s == 0 ? texture_op::MODULATE : texture_op::DISABLE;
            st[static_cast<uint32_t>(gpu::stage_state::COLOR_ARG1)] = texture_arg::TEXTURE;
            st[static_cast<uint32_t>(gpu::stage_state::COLOR_ARG2)] = texture_arg::CURRENT;
            st[static_cast<uint32_t>(gpu::stage_state::ALPHA_OP)] = s == 0 ? texture_op::SELECT_ARG1 : texture_op::DISABLE;
            st[static_cast<uint32_t>(gpu::stage_state::ALPHA_ARG1)] = texture_arg::TEXTURE;
            st[static_cast<uint32_t>(gpu::stage_state::ALPHA_ARG2)] = texture_arg::CURRENT;
            st[static_cast<uint32_t>(gpu::stage_state::TEXCOORD_INDEX)] = static_cast<uint32_t>(s);
            st[static_cast<uint32_t>(gpu::stage_state::ADDRESS_U)] = address::WRAP;
            st[static_cast<uint32_t>(gpu::stage_state::ADDRESS_V)] = address::WRAP;
            st[static_cast<uint32_t>(gpu::stage_state::ADDRESS_W)] = address::WRAP;
            st[static_cast<uint32_t>(gpu::stage_state::MAG_FILTER)] = filter::POINT;
            st[static_cast<uint32_t>(gpu::stage_state::MIN_FILTER)] = filter::POINT;
            st[static_cast<uint32_t>(gpu::stage_state::MIP_FILTER)] = filter::NONE;
            st[static_cast<uint32_t>(gpu::stage_state::MAX_ANISOTROPY)] = 1;
            st[static_cast<uint32_t>(gpu::stage_state::TEXTURE_TRANSFORM_FLAGS)] = texture_transform_flags::DISABLE;
        }

        for (bool &bound : texture_bound)
            bound = false;
        world = view = projection = math::matrix_identity();
        for (math::matrix &m : texture_matrices)
            m = math::matrix_identity();
        mat = {};
        mat.diffuse = {1.0f, 1.0f, 1.0f, 1.0f};
        mat.ambient = {1.0f, 1.0f, 1.0f, 1.0f};
        view_port = {0, 0, target_width, target_height, 0.0f, 1.0f};
    }

    auto fixed_function_state::render_state_float(state s) const -> float {
        float f;
        memcpy(&f, &render_states[static_cast<uint32_t>(s)], sizeof(f));
        return f;
    }

    auto fixed_function_state::transform_slot(transform t) -> math::matrix * {
        switch (t) {
        case transform::VIEW:
            return &view;
        case transform::PROJECTION:
            return &projection;
        case transform::WORLD:
            return &world;
        default:
            break;
        }
        const auto index = static_cast<uint32_t>(t);
        if (index >= static_cast<uint32_t>(transform::TEXTURE0) && index <= static_cast<uint32_t>(transform::TEXTURE7))
            return &texture_matrices[index - static_cast<uint32_t>(transform::TEXTURE0)];
        return nullptr;
    }

    auto fixed_function_state::transform_slot(transform t) const -> const math::matrix * {
        return const_cast<fixed_function_state *>(this)->transform_slot(t);
    }

    auto is_pipeline_state(state s) -> bool {
        switch (s) {
        case state::Z_ENABLE:
        case state::FILL_MODE:
        case state::Z_WRITE_ENABLE:
        case state::SRC_BLEND:
        case state::DEST_BLEND:
        case state::CULL_MODE:
        case state::Z_FUNC:
        case state::DITHER_ENABLE:
        case state::ALPHA_BLEND_ENABLE:
        case state::STENCIL_ENABLE:
        case state::COLOR_WRITE_ENABLE:
        case state::BLEND_OP:
        case state::Z_BIAS:
            return true;
        default:
            return false;
        }
    }

    auto is_sampler_state(stage_state s) -> bool {
        switch (s) {
        case stage_state::ADDRESS_U:
        case stage_state::ADDRESS_V:
        case stage_state::ADDRESS_W:
        case stage_state::BORDER_COLOR:
        case stage_state::MAG_FILTER:
        case stage_state::MIN_FILTER:
        case stage_state::MIP_FILTER:
        case stage_state::MIPMAP_LOD_BIAS:
        case stage_state::MAX_MIP_LEVEL:
        case stage_state::MAX_ANISOTROPY:
            return true;
        default:
            return false;
        }
    }

    void pack_uniforms(const fixed_function_state &st, const vertex_layout &layout, const uniform_options &options, fixed_function_uniforms &u) {
        memcpy(u.world, &st.world, 64);
        memcpy(u.view, &st.view, 64);
        memcpy(u.projection, &st.projection, 64);
        normal_matrix(st.world * st.view, u.normal);
        memcpy(u.texture_matrices[0], &st.texture_matrices[0], 64);
        memcpy(u.texture_matrices[1], &st.texture_matrices[1], 64);
        u.viewport[0] = static_cast<float>(st.view_port.width);
        u.viewport[1] = static_cast<float>(st.view_port.height);
        u.viewport[2] = options.flip_y ? -1.0f : 1.0f;
        u.viewport[3] = options.half_pixel ? 1.0f : 0.0f;
        memcpy(u.material_diffuse, &st.mat.diffuse, 16);
        memcpy(u.material_ambient, &st.mat.ambient, 16);
        memcpy(u.material_specular, &st.mat.specular, 16);
        memcpy(u.material_emissive, &st.mat.emissive, 16);
        color_to_floats(st.render_state(state::AMBIENT), u.global_ambient);
        u.global_ambient[3] = st.mat.power;
        u.light_flags[0] = st.render_state(state::LIGHTING) ? 1 : 0;
        u.light_flags[1] = st.render_state(state::COLOR_VERTEX) ? 1 : 0;
        u.light_flags[2] = st.render_state(state::SPECULAR_ENABLE) ? 1 : 0;
        u.light_flags[3] = st.render_state(state::LOCAL_VIEWER) ? 1 : 0;
        const bool color_vertex = st.render_state(state::COLOR_VERTEX) != 0;
        u.material_source[0] = color_vertex ? static_cast<int32_t>(st.render_state(state::DIFFUSE_MATERIAL_SOURCE)) : 0;
        u.material_source[1] = color_vertex ? static_cast<int32_t>(st.render_state(state::AMBIENT_MATERIAL_SOURCE)) : 0;
        u.material_source[2] = color_vertex ? static_cast<int32_t>(st.render_state(state::SPECULAR_MATERIAL_SOURCE)) : 0;
        u.material_source[3] = color_vertex ? static_cast<int32_t>(st.render_state(state::EMISSIVE_MATERIAL_SOURCE)) : 0;
        u.fog_modes[0] = st.render_state(state::FOG_ENABLE) ? 1 : 0;
        u.fog_modes[1] = static_cast<int32_t>(st.render_state(state::FOG_VERTEX_MODE));
        u.fog_modes[2] = static_cast<int32_t>(st.render_state(state::FOG_TABLE_MODE));
        u.fog_modes[3] = st.render_state(state::RANGE_FOG_ENABLE) ? 1 : 0;
        // Fog without a mode is linear vertex fog.
        if (u.fog_modes[0] && u.fog_modes[1] == fog::NONE && u.fog_modes[2] == fog::NONE)
            u.fog_modes[1] = fog::LINEAR;
        u.fog_parameters[0] = st.render_state_float(state::FOG_START);
        u.fog_parameters[1] = st.render_state_float(state::FOG_END);
        u.fog_parameters[2] = st.render_state_float(state::FOG_DENSITY);
        u.fog_parameters[3] = 0.0f;
        color_to_floats(st.render_state(state::FOG_COLOR), u.fog_color);
        color_to_floats(st.render_state(state::TEXTURE_FACTOR), u.texture_factor);
        u.misc[0] = static_cast<float>(st.render_state(state::ALPHA_REF) & 255) / 255.0f;
        u.misc[1] = layout.pre_transformed ? 1.0f : 0.0f;
        u.misc[2] = layout.has_diffuse ? 1.0f : 0.0f;
        u.misc[3] = layout.has_specular ? 1.0f : 0.0f;
        u.alpha[0] = st.render_state(state::ALPHA_TEST_ENABLE) ? 1 : 0;
        u.alpha[1] = static_cast<int32_t>(st.render_state(state::ALPHA_FUNC));
        u.alpha[2] = static_cast<int32_t>(st.view_port.x);
        u.alpha[3] = static_cast<int32_t>(st.view_port.y);
        u.effect[0] = static_cast<float>(st.render_state(state::EFFECT_MODE));
        u.effect[1] = st.render_state_float(state::EFFECT_TIME);
        u.effect[2] = 0.0f;
        u.effect[3] = 0.0f;
        for (int s = 0; s < 2; ++s) {
            u.stage_a[s][0] = static_cast<int32_t>(st.stage_state(s, stage_state::COLOR_OP));
            u.stage_a[s][1] = static_cast<int32_t>(st.stage_state(s, stage_state::COLOR_ARG1));
            u.stage_a[s][2] = static_cast<int32_t>(st.stage_state(s, stage_state::COLOR_ARG2));
            u.stage_a[s][3] = static_cast<int32_t>(st.stage_state(s, stage_state::ALPHA_OP));
            u.stage_b[s][0] = static_cast<int32_t>(st.stage_state(s, stage_state::ALPHA_ARG1));
            u.stage_b[s][1] = static_cast<int32_t>(st.stage_state(s, stage_state::ALPHA_ARG2));
            const uint32_t texcoord_index = st.stage_state(s, stage_state::TEXCOORD_INDEX);
            u.stage_b[s][2] = static_cast<int32_t>(texcoord_index & 0xffff);
            u.stage_b[s][3] = static_cast<int32_t>((texcoord_index >> 16) & 0xf);
            const uint32_t flags = st.stage_state(s, stage_state::TEXTURE_TRANSFORM_FLAGS);
            u.stage_c[s][0] = static_cast<int32_t>(flags & 0xff);
            u.stage_c[s][1] = st.texture_bound[s] ? 1 : 0;
            const int index = u.stage_b[s][2];
            u.stage_c[s][2] = (index >= 0 && index < 3 && layout.texcoord_dimension[index]) ? layout.texcoord_dimension[index] : 2;
            u.stage_c[s][3] = (flags & texture_transform_flags::PROJECTED) ? 1 : 0;
        }
        for (int i = 0; i < fixed_function_state::LIGHTS; ++i) {
            fixed_function_light &out = u.lights[i];
            if (!st.light_enabled[i]) {
                memset(&out, 0, sizeof(out));
                continue;
            }
            const light &l = st.lights[i];
            memcpy(out.diffuse, &l.diffuse, 16);
            memcpy(out.specular, &l.specular, 16);
            memcpy(out.ambient, &l.ambient, 16);
            // to view space (row vectors: p' = p * view)
            const math::matrix &v = st.view;
            const math::vector3 &p = l.position;
            out.position[0] = p.x * v._11 + p.y * v._21 + p.z * v._31 + v._41;
            out.position[1] = p.x * v._12 + p.y * v._22 + p.z * v._32 + v._42;
            out.position[2] = p.x * v._13 + p.y * v._23 + p.z * v._33 + v._43;
            out.position[3] = static_cast<float>(l.type);
            const math::vector3 &d = l.direction;
            out.direction[0] = d.x * v._11 + d.y * v._21 + d.z * v._31;
            out.direction[1] = d.x * v._12 + d.y * v._22 + d.z * v._32;
            out.direction[2] = d.x * v._13 + d.y * v._23 + d.z * v._33;
            out.direction[3] = l.range > 0.0f ? l.range : 1e30f;
            out.attenuation[0] = l.attenuation0;
            out.attenuation[1] = l.attenuation1;
            out.attenuation[2] = l.attenuation2;
            out.attenuation[3] = l.falloff;
            out.spot[0] = cosf(l.theta * 0.5f);
            out.spot[1] = cosf(l.phi * 0.5f);
            out.spot[2] = out.spot[3] = 0.0f;
        }
    }
}
