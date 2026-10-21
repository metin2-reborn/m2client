#include "check.h"

#include <metin2/gpu/fixed_function.h>
#include <metin2/gpu/format.h>
#include <metin2/gpu/vertex_layout.h>
#include <metin2/graphics/dxtc_image.h>
#include <metin2/graphics/image_file.h>

#include <cmath>
#include <cstring>
#include <vector>

using namespace metin2;

namespace {
    auto near(float a, float b) -> bool {
        return std::fabs(a - b) < 1e-5f;
    }

    void put32(std::vector<uint8_t> &data, size_t offset, uint32_t value) {
        memcpy(data.data() + offset, &value, 4);
    }

    // A DDS header (128 bytes including the magic) for the given pixel format.
    auto dds_header(uint32_t width, uint32_t height, uint32_t four_cc, uint32_t bits = 0, uint32_t r = 0, uint32_t g = 0, uint32_t b = 0, uint32_t a = 0) -> std::vector<uint8_t> {
        std::vector<uint8_t> data(128, 0);
        memcpy(data.data(), "DDS ", 4);
        put32(data, 4, 124);
        put32(data, 12, height);
        put32(data, 16, width);
        put32(data, 76, 32);
        put32(data, 80, four_cc ? 0x4 : (0x40 | (a ? 0x1 : 0)));
        put32(data, 84, four_cc);
        put32(data, 88, bits);
        put32(data, 92, r);
        put32(data, 96, g);
        put32(data, 100, b);
        put32(data, 104, a);
        return data;
    }
}

TEST(formats_have_sizes_and_pitches) {
    const gpu::format_info *argb = gpu::get_format_info(gpu::format::A8R8G8B8);
    CHECK(argb != nullptr);
    CHECK_EQ(gpu::format_pitch(*argb, 10), 40u);
    const gpu::format_info *dxt1 = gpu::get_format_info(gpu::format::DXT1);
    CHECK(dxt1 != nullptr && dxt1->compressed);
    CHECK_EQ(gpu::format_level_size(*dxt1, 5, 5), 32u); // 2x2 blocks of 8 bytes
    CHECK_EQ(gpu::format_level_size(*gpu::get_format_info(gpu::format::DXT5), 4, 4), 16u);
    CHECK(gpu::get_format_info(gpu::format::D24S8)->depth);
    CHECK(gpu::get_format_info(gpu::format::INDEX16) == nullptr);
    CHECK(gpu::format_is_renderable(gpu::format::R5G6B5));
    CHECK(!gpu::format_is_renderable(gpu::format::DXT1));
}

TEST(pixel_conversions_round_trip) {
    const uint8_t rgba[8] = {0x11, 0x22, 0x33, 0x44, 0xff, 0x80, 0x00, 0xff};
    for (gpu::format fmt : {gpu::format::A8R8G8B8, gpu::format::R8G8B8}) {
        const gpu::format_info &info = *gpu::get_format_info(fmt);
        uint8_t packed[8] = {};
        uint8_t back[8] = {};
        gpu::convert_from_rgba8(info, rgba, 2, 1, packed, gpu::format_pitch(info, 2));
        gpu::convert_to_rgba8(info, packed, gpu::format_pitch(info, 2), 2, 1, back);
        for (int i = 0; i < 8; ++i)
            CHECK_EQ(back[i], (fmt == gpu::format::R8G8B8 && i % 4 == 3) ? uint8_t(255) : rgba[i]);
    }
    // A8R8G8B8 is 0xAARRGGBB in memory order B, G, R, A
    const gpu::format_info &argb = *gpu::get_format_info(gpu::format::A8R8G8B8);
    uint32_t pixel = 0;
    gpu::convert_from_rgba8(argb, rgba, 1, 1, reinterpret_cast<uint8_t *>(&pixel), 4);
    CHECK_EQ(pixel, 0x44112233u);
    // the X formats read back opaque
    const gpu::format_info &xrgb = *gpu::get_format_info(gpu::format::X8R8G8B8);
    uint8_t opaque[4];
    gpu::convert_to_rgba8(xrgb, reinterpret_cast<const uint8_t *>(&pixel), 4, 1, 1, opaque);
    CHECK_EQ(opaque[3], uint8_t(255));
}

TEST(vertex_formats_become_layouts) {
    gpu::vertex_layout layout;
    CHECK(gpu::layout_from_fvf(gpu::fvf::XYZ | gpu::fvf::NORMAL | gpu::fvf::DIFFUSE | gpu::fvf::TEX2, layout));
    CHECK_EQ(layout.stream_stride[0], 12u + 12u + 4u + 8u + 8u);
    CHECK(layout.has_normal && layout.has_diffuse && !layout.has_specular && !layout.pre_transformed);
    CHECK_EQ(layout.attributes.size(), size_t(5));
    CHECK_EQ(layout.attributes[2].offset, 24);
    CHECK(layout.attributes[2].data == gpu::layout_attribute::kind::COLOR);
    CHECK_EQ(layout.texcoord_dimension[1], 2);
    CHECK_EQ(gpu::vertex_size(gpu::fvf::XYZ | gpu::fvf::NORMAL | gpu::fvf::DIFFUSE | gpu::fvf::TEX2), layout.stream_stride[0]);

    CHECK(gpu::layout_from_fvf(gpu::fvf::XYZRHW | gpu::fvf::DIFFUSE | gpu::fvf::SPECULAR | gpu::fvf::TEX1, layout));
    CHECK(layout.pre_transformed && layout.has_specular);
    CHECK_EQ(layout.stream_stride[0], 16u + 4u + 4u + 8u);

    CHECK(!gpu::layout_from_fvf(gpu::fvf::DIFFUSE, layout));
}

TEST(vertex_elements_span_streams) {
    const gpu::vertex_element elements[] = {
        {0, gpu::vertex_usage::POSITION,  gpu::vertex_type::FLOAT3},
        {1, gpu::vertex_usage::TEXCOORD0, gpu::vertex_type::FLOAT2},
    };
    gpu::vertex_layout layout;
    CHECK(gpu::layout_from_elements(elements, layout));
    CHECK_EQ(layout.stream_stride[0], 12u);
    CHECK_EQ(layout.stream_stride[1], 8u);
    CHECK_EQ(layout.attributes[1].stream, 1);
    CHECK_EQ(layout.attributes[1].offset, 0);
    CHECK_EQ(layout.attributes[1].slot, 4);
}

TEST(state_defaults_match_direct3d) {
    gpu::fixed_function_state state;
    state.reset(640, 480, true);
    CHECK_EQ(state.render_state(gpu::state::Z_ENABLE), 1u);
    CHECK_EQ(state.render_state(gpu::state::CULL_MODE), uint32_t(gpu::cull::CCW));
    CHECK_EQ(state.render_state(gpu::state::LIGHTING), 1u);
    CHECK(near(state.render_state_float(gpu::state::FOG_END), 1.0f));
    CHECK_EQ(state.stage_state(0, gpu::stage_state::COLOR_OP), uint32_t(gpu::texture_op::MODULATE));
    CHECK_EQ(state.stage_state(1, gpu::stage_state::COLOR_OP), uint32_t(gpu::texture_op::DISABLE));
    CHECK_EQ(state.stage_state(3, gpu::stage_state::TEXCOORD_INDEX), 3u);
    CHECK_EQ(state.view_port.width, 640u);
    CHECK(state.transform_slot(gpu::transform::WORLD) == &state.world);
    CHECK(state.transform_slot(gpu::texture_transform(1)) == &state.texture_matrices[1]);
    CHECK(state.transform_slot(gpu::transform::WORLD1) == nullptr);
    CHECK(gpu::is_pipeline_state(gpu::state::SRC_BLEND));
    CHECK(!gpu::is_pipeline_state(gpu::state::FOG_COLOR));
    CHECK(gpu::is_sampler_state(gpu::stage_state::MIN_FILTER));
    CHECK(!gpu::is_sampler_state(gpu::stage_state::COLOR_OP));
}

TEST(uniforms_pack_the_state) {
    gpu::fixed_function_state state;
    state.reset(800, 600, true);
    for (bool &enabled : state.light_enabled)
        enabled = false;
    state.view_port = {10, 20, 300, 200, 0.0f, 1.0f};
    state.render_states[static_cast<uint32_t>(gpu::state::FOG_ENABLE)] = 1;
    state.render_states[static_cast<uint32_t>(gpu::state::TEXTURE_FACTOR)] = 0x80ff0000;
    state.render_states[static_cast<uint32_t>(gpu::state::ALPHA_REF)] = 255;
    state.view = math::matrix_translation(0.0f, 0.0f, 5.0f);
    state.lights[0] = {};
    state.lights[0].type = gpu::light_type::POINT;
    state.lights[0].position = {1.0f, 2.0f, 3.0f};
    state.lights[0].theta = 0.0f;
    state.light_enabled[0] = true;

    gpu::vertex_layout layout;
    gpu::layout_from_fvf(gpu::fvf::XYZRHW | gpu::fvf::DIFFUSE | gpu::fvf::TEX1, layout);
    gpu::fixed_function_uniforms u = {};
    gpu::pack_uniforms(state, layout, {.flip_y = true, .half_pixel = true}, u);

    CHECK(near(u.viewport[0], 300.0f) && near(u.viewport[1], 200.0f));
    CHECK(near(u.viewport[2], -1.0f));
    CHECK_EQ(u.alpha[2], 10);
    CHECK_EQ(u.alpha[3], 20);
    // fog without a mode is linear vertex fog
    CHECK_EQ(u.fog_modes[1], int32_t(gpu::fog::LINEAR));
    CHECK(near(u.texture_factor[0], 1.0f) && near(u.texture_factor[3], 128.0f / 255.0f));
    CHECK(near(u.misc[0], 1.0f));
    CHECK(near(u.misc[1], 1.0f) && near(u.misc[2], 1.0f) && near(u.misc[3], 0.0f));
    // the light moves to view space; its type rides in w
    CHECK(near(u.lights[0].position[2], 8.0f));
    CHECK(near(u.lights[0].position[3], float(gpu::light_type::POINT)));
    CHECK(near(u.lights[1].position[3], 0.0f));
    CHECK_EQ(u.stage_a[0][0], int32_t(gpu::texture_op::MODULATE));
    CHECK_EQ(u.stage_c[0][2], 2);
}

TEST(dxt1_blocks_decode) {
    // one 4x4 block: colour 0 red, colour 1 blue, every pixel index 0 except the last (index 1)
    std::vector<uint8_t> file = dds_header(4, 4, gpu::four_cc('D', 'X', 'T', '1'));
    const uint16_t red = 0xf800;
    const uint16_t blue = 0x001f;
    const uint32_t indices = 1u << 30;
    file.resize(128 + 8);
    memcpy(file.data() + 128, &red, 2);
    memcpy(file.data() + 130, &blue, 2);
    memcpy(file.data() + 132, &indices, 4);

    dxtc_image image;
    CHECK(image.load_header_from_memory(file.data(), file.size()));
    CHECK(image.comp_format == PF_DXT1);
    std::vector<uint8_t> rgba(4 * 4 * 4);
    CHECK(image.decompress(0, rgba.data()));
    CHECK_EQ(rgba[0], uint8_t(255));
    CHECK_EQ(rgba[2], uint8_t(0));
    CHECK_EQ(rgba[3], uint8_t(255));
    CHECK_EQ(rgba[15 * 4 + 2], uint8_t(255));
    CHECK_EQ(rgba[15 * 4 + 0], uint8_t(0));
}

TEST(dxt5_alpha_decodes) {
    std::vector<uint8_t> file = dds_header(4, 4, gpu::four_cc('D', 'X', 'T', '5'));
    file.resize(128 + 16, 0);
    file[128] = 200; // alpha 0; all alpha indices 0
    file[129] = 100;
    dxtc_image image;
    CHECK(image.load_header_from_memory(file.data(), file.size()));
    std::vector<uint8_t> rgba(4 * 4 * 4);
    CHECK(image.decompress(0, rgba.data()));
    CHECK_EQ(rgba[3], uint8_t(200));
}

TEST(uncompressed_dds_decodes) {
    std::vector<uint8_t> file = dds_header(2, 1, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    file.resize(128 + 8);
    put32(file, 128, 0x80102030);
    put32(file, 132, 0xff405060);
    rgba_image image;
    CHECK(decode_image(file.data(), file.size(), image));
    CHECK_EQ(image.width, 2);
    CHECK(image.has_alpha);
    CHECK_EQ(image.pixels[0], uint8_t(0x10));
    CHECK_EQ(image.pixels[2], uint8_t(0x30));
    CHECK_EQ(image.pixels[3], uint8_t(0x80));
    CHECK_EQ(image.pixels[4], uint8_t(0x40));
}
