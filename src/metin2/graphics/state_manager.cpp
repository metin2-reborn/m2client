#include "state_manager.h"
#include <cassert>
#include <metin2/log/log.h>

#include <algorithm>

namespace metin2::inline graphics {

#define StateManager_Assert(a) assert(a)

    namespace {
        constexpr auto index(gpu::state s) -> uint32_t { return static_cast<uint32_t>(s); }
        constexpr auto index(gpu::stage_state s) -> uint32_t { return static_cast<uint32_t>(s); }
        constexpr auto index(gpu::transform t) -> uint32_t { return static_cast<uint32_t>(t); }

    }

    struct light_state {
        enum {
            LIGHT_NUM = 8,
        };
        gpu::light lights[LIGHT_NUM];
    } default_light_data;

    void state_manager::set_light(uint32_t index, const gpu::light *light) {
        assert(index < light_state::LIGHT_NUM);
        default_light_data.lights[index] = *light;

        device_.set_light(index, *light);
    }

    void state_manager::enable_light(uint32_t index, bool enable) {
        device_.enable_light(index, enable);
    }

    void state_manager::get_light(uint32_t index, gpu::light *light) {
        assert(index < 8);
        *light = default_light_data.lights[index];
    }

    auto state_manager::begin_scene() -> bool {
        scene_ = true;

        math::matrix m4_proj;
        math::matrix m4_view;
        math::matrix m4_world;
        get_transform(gpu::transform::WORLD, &m4_world);
        get_transform(gpu::transform::PROJECTION, &m4_proj);
        get_transform(gpu::transform::VIEW, &m4_view);
        set_transform(gpu::transform::WORLD, &m4_world);
        set_transform(gpu::transform::PROJECTION, &m4_proj);
        set_transform(gpu::transform::VIEW, &m4_view);
        return true;
    }

    void state_manager::end_scene() {
        scene_ = false;
    }

    state_manager::state_manager(gpu::device &device) : device_(device) {
        scene_ = false;

        const gpu::device_caps &caps = device_.caps();
        best_mag_filter_ = caps.anisotropic_filter ? gpu::filter::ANISOTROPIC : gpu::filter::LINEAR;
        best_min_filter_ = caps.anisotropic_filter ? gpu::filter::ANISOTROPIC : gpu::filter::LINEAR;
        max_anisotropy_ = std::min(caps.max_anisotropy, 16u);

        set_default_state();
    }

    state_manager::~state_manager() = default;

    void state_manager::set_best_filtering(uint32_t stage) {
        set_texture_stage_state(stage, gpu::stage_state::MIN_FILTER, best_min_filter_);
        set_texture_stage_state(stage, gpu::stage_state::MAG_FILTER, best_mag_filter_);
        set_texture_stage_state(stage, gpu::stage_state::MIP_FILTER, gpu::filter::LINEAR);
    }

    void state_manager::set_default_state() {
        current_state_.reset_state();
        copy_state_.reset_state();
        chip_state_.reset_state();

        scene_ = false;
        force_ = true;

        // A device reset clears the anisotropy with the rest of the state.
        for (uint32_t i = 0; i < STATEMANAGER_MAX_STAGES; ++i)
            device_.set_texture_stage_state(i, gpu::stage_state::MAX_ANISOTROPY, max_anisotropy_);

        math::matrix identity;
        identity = math::matrix_identity();

        set_transform(gpu::transform::WORLD, &identity);
        set_transform(gpu::transform::VIEW, &identity);
        set_transform(gpu::transform::PROJECTION, &identity);

        gpu::material default_mat;
        default_mat = {};

        default_mat.diffuse.r = 1.0f;
        default_mat.diffuse.g = 1.0f;
        default_mat.diffuse.b = 1.0f;
        default_mat.diffuse.a = 1.0f;
        default_mat.ambient.r = 1.0f;
        default_mat.ambient.g = 1.0f;
        default_mat.ambient.b = 1.0f;
        default_mat.ambient.a = 1.0f;
        default_mat.emissive.r = 0.0f;
        default_mat.emissive.g = 0.0f;
        default_mat.emissive.b = 0.0f;
        default_mat.emissive.a = 0.0f;
        default_mat.specular.r = 0.0f;
        default_mat.specular.g = 0.0f;
        default_mat.specular.b = 0.0f;
        default_mat.specular.a = 0.0f;
        default_mat.power = 0.0f;

        set_material(&default_mat);

        set_render_state(gpu::state::DIFFUSE_MATERIAL_SOURCE, gpu::material_source::MATERIAL);
        set_render_state(gpu::state::SPECULAR_MATERIAL_SOURCE, gpu::material_source::MATERIAL);
        set_render_state(gpu::state::AMBIENT_MATERIAL_SOURCE, gpu::material_source::MATERIAL);
        set_render_state(gpu::state::EMISSIVE_MATERIAL_SOURCE, gpu::material_source::MATERIAL);

        set_render_state(gpu::state::LINE_PATTERN, 0xFFFFFFFF);
        set_render_state(gpu::state::LAST_PIXEL, false);
        set_render_state(gpu::state::ALPHA_REF, 1);
        set_render_state(gpu::state::ALPHA_FUNC, gpu::compare::GREATER_EQUAL);
        set_render_state(gpu::state::Z_VISIBLE, false);
        set_render_state(gpu::state::FOG_START, 0);
        set_render_state(gpu::state::FOG_END, 0);
        set_render_state(gpu::state::FOG_DENSITY, 0);
        set_render_state(gpu::state::EDGE_ANTIALIAS, false);
        set_render_state(gpu::state::Z_BIAS, 0);
        set_render_state(gpu::state::STENCIL_WRITE_MASK, 0xFFFFFFFF);
        set_render_state(gpu::state::AMBIENT, 0x00000000);
        set_render_state(gpu::state::LOCAL_VIEWER, false);
        set_render_state(gpu::state::NORMALIZE_NORMALS, false);
        set_render_state(gpu::state::VERTEX_BLEND, gpu::vertex_blend::DISABLE);
        set_render_state(gpu::state::CLIP_PLANE_ENABLE, 0);
        set_render_state(gpu::state::SOFTWARE_VERTEX_PROCESSING, false);
        set_render_state(gpu::state::MULTISAMPLE_ANTIALIAS, false);
        set_render_state(gpu::state::MULTISAMPLE_MASK, 0xFFFFFFFF);
        set_render_state(gpu::state::INDEXED_VERTEX_BLEND_ENABLE, false);
        set_render_state(gpu::state::COLOR_WRITE_ENABLE, 0xFFFFFFFF);
        set_render_state(gpu::state::FILL_MODE, gpu::fill::SOLID);
        set_render_state(gpu::state::SHADE_MODE, gpu::shade::GOURAUD);
        set_render_state(gpu::state::CULL_MODE, gpu::cull::CW);
        set_render_state(gpu::state::ALPHA_BLEND_ENABLE, false);
        set_render_state(gpu::state::BLEND_OP, gpu::blend_op::ADD);
        set_render_state(gpu::state::SRC_BLEND, gpu::blend::SRC_ALPHA);
        set_render_state(gpu::state::DEST_BLEND, gpu::blend::INV_SRC_ALPHA);
        set_render_state(gpu::state::FOG_ENABLE, false);
        set_render_state(gpu::state::FOG_COLOR, 0xFF000000);
        set_render_state(gpu::state::FOG_TABLE_MODE, gpu::fog::NONE);
        set_render_state(gpu::state::FOG_VERTEX_MODE, gpu::fog::LINEAR);
        set_render_state(gpu::state::RANGE_FOG_ENABLE, false);
        set_render_state(gpu::state::Z_ENABLE, true);
        set_render_state(gpu::state::Z_FUNC, gpu::compare::LESS_EQUAL);
        set_render_state(gpu::state::Z_WRITE_ENABLE, true);
        set_render_state(gpu::state::DITHER_ENABLE, true);
        set_render_state(gpu::state::STENCIL_ENABLE, false);
        set_render_state(gpu::state::ALPHA_TEST_ENABLE, false);
        set_render_state(gpu::state::CLIPPING, true);
        set_render_state(gpu::state::LIGHTING, false);
        set_render_state(gpu::state::SPECULAR_ENABLE, false);
        set_render_state(gpu::state::COLOR_VERTEX, false);
        set_render_state(gpu::state::WRAP0, 0);
        set_render_state(gpu::state::WRAP1, 0);
        set_render_state(gpu::state::WRAP2, 0);
        set_render_state(gpu::state::WRAP3, 0);
        set_render_state(gpu::state::WRAP4, 0);
        set_render_state(gpu::state::WRAP5, 0);
        set_render_state(gpu::state::WRAP6, 0);
        set_render_state(gpu::state::WRAP7, 0);

        set_texture_stage_state(0, gpu::stage_state::COLOR_OP, gpu::texture_op::MODULATE);
        set_texture_stage_state(0, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(0, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::CURRENT);
        set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(0, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::CURRENT);
        set_texture_stage_state(0, gpu::stage_state::ALPHA_OP, gpu::texture_op::SELECT_ARG1);

        set_texture_stage_state(1, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(1, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(1, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        set_texture_stage_state(1, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(1, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);

        set_texture_stage_state(2, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(2, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(2, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        set_texture_stage_state(2, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(2, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(2, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);

        set_texture_stage_state(3, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(3, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(3, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        set_texture_stage_state(3, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(3, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(3, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);

        set_texture_stage_state(4, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(4, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(4, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        set_texture_stage_state(4, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(4, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(4, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);

        set_texture_stage_state(5, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(5, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(5, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        set_texture_stage_state(5, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(5, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(5, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);

        set_texture_stage_state(6, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(6, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(6, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        set_texture_stage_state(6, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(6, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(6, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);

        set_texture_stage_state(7, gpu::stage_state::COLOR_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(7, gpu::stage_state::COLOR_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(7, gpu::stage_state::COLOR_ARG2, gpu::texture_arg::DIFFUSE);
        set_texture_stage_state(7, gpu::stage_state::ALPHA_OP, gpu::texture_op::DISABLE);
        set_texture_stage_state(7, gpu::stage_state::ALPHA_ARG1, gpu::texture_arg::TEXTURE);
        set_texture_stage_state(7, gpu::stage_state::ALPHA_ARG2, gpu::texture_arg::DIFFUSE);

        set_texture_stage_state(0, gpu::stage_state::TEXCOORD_INDEX, 0);
        set_texture_stage_state(1, gpu::stage_state::TEXCOORD_INDEX, 1);
        set_texture_stage_state(2, gpu::stage_state::TEXCOORD_INDEX, 2);
        set_texture_stage_state(3, gpu::stage_state::TEXCOORD_INDEX, 3);
        set_texture_stage_state(4, gpu::stage_state::TEXCOORD_INDEX, 4);
        set_texture_stage_state(5, gpu::stage_state::TEXCOORD_INDEX, 5);
        set_texture_stage_state(6, gpu::stage_state::TEXCOORD_INDEX, 6);
        set_texture_stage_state(7, gpu::stage_state::TEXCOORD_INDEX, 7);

        set_best_filtering(0);
        set_best_filtering(1);

        set_texture_stage_state(2, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(2, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(2, gpu::stage_state::MIP_FILTER, gpu::filter::LINEAR);

        set_texture_stage_state(3, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(3, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(3, gpu::stage_state::MIP_FILTER, gpu::filter::LINEAR);

        set_texture_stage_state(4, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(4, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(4, gpu::stage_state::MIP_FILTER, gpu::filter::LINEAR);

        set_texture_stage_state(5, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(5, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(5, gpu::stage_state::MIP_FILTER, gpu::filter::LINEAR);

        set_texture_stage_state(6, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(6, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(6, gpu::stage_state::MIP_FILTER, gpu::filter::LINEAR);

        set_texture_stage_state(7, gpu::stage_state::MIN_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(7, gpu::stage_state::MAG_FILTER, gpu::filter::LINEAR);
        set_texture_stage_state(7, gpu::stage_state::MIP_FILTER, gpu::filter::LINEAR);

        set_texture_stage_state(0, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        set_texture_stage_state(0, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);
        set_texture_stage_state(1, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        set_texture_stage_state(1, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);
        set_texture_stage_state(2, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        set_texture_stage_state(2, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);
        set_texture_stage_state(3, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        set_texture_stage_state(3, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);
        set_texture_stage_state(4, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        set_texture_stage_state(4, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);
        set_texture_stage_state(5, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        set_texture_stage_state(5, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);
        set_texture_stage_state(6, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        set_texture_stage_state(6, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);
        set_texture_stage_state(7, gpu::stage_state::ADDRESS_U, gpu::address::WRAP);
        set_texture_stage_state(7, gpu::stage_state::ADDRESS_V, gpu::address::WRAP);

        set_texture_stage_state(0, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, 0);
        set_texture_stage_state(1, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, 0);
        set_texture_stage_state(2, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, 0);
        set_texture_stage_state(3, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, 0);
        set_texture_stage_state(4, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, 0);
        set_texture_stage_state(5, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, 0);
        set_texture_stage_state(6, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, 0);
        set_texture_stage_state(7, gpu::stage_state::TEXTURE_TRANSFORM_FLAGS, 0);

        set_texture(0, nullptr);
        set_texture(1, nullptr);
        set_texture(2, nullptr);
        set_texture(3, nullptr);
        set_texture(4, nullptr);
        set_texture(5, nullptr);
        set_texture(6, nullptr);
        set_texture(7, nullptr);

        set_vertex_shader(gpu::fvf::XYZ);

        force_ = false;
    }

    void state_manager::save_material() {
        copy_state_.material_value = current_state_.material_value;
    }

    void state_manager::save_material(const gpu::material *material) {
        copy_state_.material_value = current_state_.material_value;
        set_material(material);
    }

    void state_manager::set_material(const gpu::material *material) {
        device_.set_material(*material);
        current_state_.material_value = *material;
    }

    void state_manager::get_material(gpu::material *material) {
        *material = current_state_.material_value;
    }

    auto state_manager::render_state(gpu::state type) -> uint32_t {
        return current_state_.render_states[index(type)];
    }

    void state_manager::save_render_state(gpu::state type, uint32_t value) {

        copy_state_.render_states[index(type)] = current_state_.render_states[index(type)];
        set_render_state(type, value);
    }

    void state_manager::restore_render_state(gpu::state type) {

        set_render_state(type, copy_state_.render_states[index(type)]);
    }

    void state_manager::set_render_state(gpu::state type, uint32_t value) {
        if (current_state_.render_states[index(type)] == value)
            return;

        device_.set_render_state(type, value);
        current_state_.render_states[index(type)] = value;
    }

    void state_manager::render_state(gpu::state type, uint32_t *value) {
        *value = current_state_.render_states[index(type)];
    }

    void state_manager::set_texture(uint32_t stage, gpu::texture *texture) {
        if (texture == current_state_.textures[stage])
            return;

        device_.set_texture(stage, texture);
        current_state_.textures[stage] = texture;
    }

    void state_manager::texture(uint32_t stage, gpu::texture **texture) {
        *texture = current_state_.textures[stage];
    }

    void state_manager::save_texture_stage_state(uint32_t stage, gpu::stage_state type, uint32_t value) {
        copy_state_.texture_states[stage][index(type)] = current_state_.texture_states[stage][index(type)];
        set_texture_stage_state(stage, type, value);
    }

    void state_manager::restore_texture_stage_state(uint32_t stage, gpu::stage_state type) {
        set_texture_stage_state(stage, type, copy_state_.texture_states[stage][index(type)]);
    }

    void state_manager::set_texture_stage_state(uint32_t stage, gpu::stage_state type, uint32_t value) {
        if (current_state_.texture_states[stage][index(type)] == value)
            return;

        device_.set_texture_stage_state(stage, type, value);
        current_state_.texture_states[stage][index(type)] = value;
    }

    void state_manager::texture_stage_state(uint32_t stage, gpu::stage_state type, uint32_t *value) {
        *value = current_state_.texture_states[stage][index(type)];
    }

    void state_manager::save_vertex_shader(uint32_t shader) {
        copy_state_.vertex_shader = current_state_.vertex_shader;
        set_vertex_shader(shader);
    }

    void state_manager::restore_vertex_shader() {
        set_vertex_shader(copy_state_.vertex_shader);
    }

    void state_manager::set_vertex_shader(uint32_t shader) {
        if (current_state_.vertex_shader == shader)
            return;

        device_.set_vertex_format(shader);
        current_state_.vertex_shader = shader;
    }

    void state_manager::vertex_shader(uint32_t *shader) {
        *shader = current_state_.vertex_shader;
    }

    // Transforms are cached, but not protected from multiple sends of the same value.
    void state_manager::save_transform(gpu::transform type, const math::matrix *matrix) {

        copy_state_.matrices[index(type)] = current_state_.matrices[index(type)];
        set_transform(type, (math::matrix *)matrix);
    }

    void state_manager::restore_transform(gpu::transform type) {

        set_transform(type, &copy_state_.matrices[index(type)]);
    }

    void state_manager::set_transform(gpu::transform type, const math::matrix *matrix) {
        if (scene_) {
            device_.set_transform(type, *matrix);
        } else {
            assert(gpu::transform::VIEW == type || gpu::transform::PROJECTION == type || gpu::transform::WORLD == type);
        }

        current_state_.matrices[index(type)] = *matrix;
    }

    void state_manager::get_transform(gpu::transform type, math::matrix *matrix) {
        *matrix = current_state_.matrices[index(type)];
    }

    void state_manager::set_stream_source(uint32_t stream_number, gpu::vertex_buffer *p_stream_data, uint32_t stride) {
        stream_data k_stream_data(p_stream_data, stride);
        if (current_state_.stream_data_value[stream_number] == k_stream_data)
            return;

        device_.set_stream(stream_number, p_stream_data, stride);
        current_state_.stream_data_value[stream_number] = k_stream_data;
    }

    void state_manager::set_indices(gpu::index_buffer *p_index_data, uint32_t base_vertex_index) {
        index_data k_index_data(p_index_data, base_vertex_index);

        if (current_state_.index_data_value == k_index_data)
            return;

        device_.set_indices(p_index_data, base_vertex_index);
        current_state_.index_data_value = k_index_data;
    }

    auto state_manager::draw_primitive(gpu::primitive primitive_type, uint32_t start_vertex, uint32_t primitive_count) -> bool {
        return device_.draw(primitive_type, start_vertex, primitive_count);
    }

    auto state_manager::draw_primitive_up(gpu::primitive primitive_type, uint32_t primitive_count, const void *vertex_stream_zero_data, uint32_t vertex_stream_zero_stride) -> bool {
        current_state_.stream_data_value[0] = nullptr;
        return device_.draw_user(primitive_type, primitive_count, vertex_stream_zero_data, vertex_stream_zero_stride);
    }

    auto state_manager::draw_indexed_primitive(gpu::primitive primitive_type, uint32_t min_index, uint32_t num_vertices, uint32_t start_index, uint32_t prim_count) -> bool {
        return device_.draw_indexed(primitive_type, min_index, num_vertices, start_index, prim_count);
    }

    auto state_manager::create_occlusion_query() -> uint32_t {
        return device_.create_occlusion_query();
    }

    void state_manager::destroy_occlusion_query(uint32_t query) {
        device_.destroy_occlusion_query(query);
    }

    void state_manager::begin_occlusion_query(uint32_t query) {
        device_.begin_occlusion_query(query);
    }

    void state_manager::end_occlusion_query() {
        device_.end_occlusion_query();
    }

    auto state_manager::occlusion_query_result(uint32_t query, uint32_t &samples) -> bool {
        return device_.occlusion_query_result(query, samples);
    }

    auto state_manager::draw_indexed_primitive_up(gpu::primitive primitive_type, uint32_t min_vertex_index, uint32_t num_vertex_indices, uint32_t primitive_count, const void *index_data, gpu::format index_data_format, const void *vertex_stream_zero_data, uint32_t vertex_stream_zero_stride) -> bool {
        current_state_.index_data_value = nullptr;
        current_state_.stream_data_value[0] = nullptr;
        return device_.draw_indexed_user(primitive_type, min_vertex_index, num_vertex_indices, primitive_count, index_data, index_data_format, vertex_stream_zero_data, vertex_stream_zero_stride);
    }
}
