#pragma once

#include <metin2/math/math.h>

#include <cstdint>

// The fixed-function rendering model the game is written against: render states, two texture
// stages with combiner operations, transforms, lights and a material. Every backend implements it.
// Values follow the Direct3D 8 numbering the game's data and code grew up with.
namespace metin2::gpu {

    enum class state : uint32_t {
        Z_ENABLE = 7,
        FILL_MODE = 8,
        SHADE_MODE = 9,
        LINE_PATTERN = 10,
        Z_WRITE_ENABLE = 14,
        ALPHA_TEST_ENABLE = 15,
        LAST_PIXEL = 16,
        SRC_BLEND = 19,
        DEST_BLEND = 20,
        CULL_MODE = 22,
        Z_FUNC = 23,
        ALPHA_REF = 24,
        ALPHA_FUNC = 25,
        DITHER_ENABLE = 26,
        ALPHA_BLEND_ENABLE = 27,
        FOG_ENABLE = 28,
        SPECULAR_ENABLE = 29,
        Z_VISIBLE = 30,
        FOG_COLOR = 34,
        FOG_TABLE_MODE = 35,
        FOG_START = 36,
        FOG_END = 37,
        FOG_DENSITY = 38,
        EDGE_ANTIALIAS = 40,
        Z_BIAS = 47,
        RANGE_FOG_ENABLE = 48,
        STENCIL_ENABLE = 52,
        STENCIL_MASK = 58,
        STENCIL_WRITE_MASK = 59,
        TEXTURE_FACTOR = 60,
        WRAP0 = 128,
        WRAP1 = 129,
        WRAP2 = 130,
        WRAP3 = 131,
        WRAP4 = 132,
        WRAP5 = 133,
        WRAP6 = 134,
        WRAP7 = 135,
        CLIPPING = 136,
        LIGHTING = 137,
        AMBIENT = 139,
        FOG_VERTEX_MODE = 140,
        COLOR_VERTEX = 141,
        LOCAL_VIEWER = 142,
        NORMALIZE_NORMALS = 143,
        DIFFUSE_MATERIAL_SOURCE = 145,
        SPECULAR_MATERIAL_SOURCE = 146,
        AMBIENT_MATERIAL_SOURCE = 147,
        EMISSIVE_MATERIAL_SOURCE = 148,
        VERTEX_BLEND = 151,
        CLIP_PLANE_ENABLE = 152,
        SOFTWARE_VERTEX_PROCESSING = 153,
        POINT_SIZE = 154,
        MULTISAMPLE_ANTIALIAS = 161,
        MULTISAMPLE_MASK = 162,
        INDEXED_VERTEX_BLEND_ENABLE = 167,
        COLOR_WRITE_ENABLE = 168,
        BLEND_OP = 171,
        // Beyond Direct3D: shader effects the fixed-function model has no state for.
        EFFECT_MODE = 240,   // 0 none, 1 water (Fresnel towards the fog colour, sun glint, rippling texture),
                             // 2 terrain detail (relief from the tile texture), 3 grass (sway, distance fade)
        EFFECT_TIME = 241,   // float bits, seconds
        EFFECT_PARAM0 = 242, // float bits: terrain detail strength; grass wind strength
        EFFECT_PARAM1 = 243, // float bits: grass fade-out distance
    };

    // Per texture stage: the colour and alpha combiners, texture coordinates and sampling.
    enum class stage_state : uint32_t {
        COLOR_OP = 1,
        COLOR_ARG1 = 2,
        COLOR_ARG2 = 3,
        ALPHA_OP = 4,
        ALPHA_ARG1 = 5,
        ALPHA_ARG2 = 6,
        TEXCOORD_INDEX = 11,
        ADDRESS_U = 13,
        ADDRESS_V = 14,
        BORDER_COLOR = 15,
        MAG_FILTER = 16,
        MIN_FILTER = 17,
        MIP_FILTER = 18,
        MIPMAP_LOD_BIAS = 19,
        MAX_MIP_LEVEL = 20,
        MAX_ANISOTROPY = 21,
        TEXTURE_TRANSFORM_FLAGS = 24,
        ADDRESS_W = 25,
    };

    enum class transform : uint32_t {
        VIEW = 2,
        PROJECTION = 3,
        TEXTURE0 = 16,
        TEXTURE1 = 17,
        TEXTURE2 = 18,
        TEXTURE3 = 19,
        TEXTURE4 = 20,
        TEXTURE5 = 21,
        TEXTURE6 = 22,
        TEXTURE7 = 23,
        WORLD = 256,
        WORLD1 = 257,
    };

    constexpr auto texture_transform(uint32_t stage) -> transform {
        return static_cast<transform>(static_cast<uint32_t>(transform::TEXTURE0) + stage);
    }

    enum class primitive : uint32_t {
        POINT_LIST = 1,
        LINE_LIST = 2,
        LINE_STRIP = 3,
        TRIANGLE_LIST = 4,
        TRIANGLE_STRIP = 5,
        TRIANGLE_FAN = 6,
    };

    // State values. Each group is an unscoped enum inside a struct, so the names do not leak and the
    // values convert to the uint32_t the state calls take.
    struct blend {
        enum value : uint32_t {
            ZERO = 1,
            ONE = 2,
            SRC_COLOR = 3,
            INV_SRC_COLOR = 4,
            SRC_ALPHA = 5,
            INV_SRC_ALPHA = 6,
            DEST_ALPHA = 7,
            INV_DEST_ALPHA = 8,
            DEST_COLOR = 9,
            INV_DEST_COLOR = 10,
            SRC_ALPHA_SAT = 11,
            BOTH_SRC_ALPHA = 12,
            BOTH_INV_SRC_ALPHA = 13,
        };
    };

    struct blend_op {
        enum value : uint32_t {
            ADD = 1,
            SUBTRACT = 2,
            REV_SUBTRACT = 3,
            MIN = 4,
            MAX = 5,
        };
    };

    struct compare {
        enum value : uint32_t {
            NEVER = 1,
            LESS = 2,
            EQUAL = 3,
            LESS_EQUAL = 4,
            GREATER = 5,
            NOT_EQUAL = 6,
            GREATER_EQUAL = 7,
            ALWAYS = 8,
        };
    };

    struct cull {
        enum value : uint32_t {
            NONE = 1,
            CW = 2,
            CCW = 3,
        };
    };

    struct fill {
        enum value : uint32_t {
            POINT = 1,
            WIREFRAME = 2,
            SOLID = 3,
        };
    };

    struct shade {
        enum value : uint32_t {
            FLAT = 1,
            GOURAUD = 2,
        };
    };

    struct fog {
        enum value : uint32_t {
            NONE = 0,
            EXP = 1,
            EXP2 = 2,
            LINEAR = 3,
        };
    };

    struct material_source {
        enum value : uint32_t {
            MATERIAL = 0,
            COLOR1 = 1,
            COLOR2 = 2,
        };
    };

    struct vertex_blend {
        enum value : uint32_t {
            DISABLE = 0,
        };
    };

    struct texture_op {
        enum value : uint32_t {
            DISABLE = 1,
            SELECT_ARG1 = 2,
            SELECT_ARG2 = 3,
            MODULATE = 4,
            MODULATE2X = 5,
            MODULATE4X = 6,
            ADD = 7,
            ADD_SIGNED = 8,
            ADD_SIGNED2X = 9,
            SUBTRACT = 10,
            ADD_SMOOTH = 11,
            BLEND_DIFFUSE_ALPHA = 12,
            BLEND_TEXTURE_ALPHA = 13,
            BLEND_FACTOR_ALPHA = 14,
            BLEND_TEXTURE_ALPHA_PM = 15,
            BLEND_CURRENT_ALPHA = 16,
            PREMODULATE = 17,
            MODULATE_ALPHA_ADD_COLOR = 18,
            MODULATE_COLOR_ADD_ALPHA = 19,
            MODULATE_INV_ALPHA_ADD_COLOR = 20,
            MODULATE_INV_COLOR_ADD_ALPHA = 21,
            DOT_PRODUCT3 = 24,
        };
    };

    struct texture_arg {
        enum value : uint32_t {
            DIFFUSE = 0,
            CURRENT = 1,
            TEXTURE = 2,
            TFACTOR = 3,
            SPECULAR = 4,
            // Modifiers, or'ed to an argument.
            COMPLEMENT = 0x10,
            ALPHA_REPLICATE = 0x20,
        };
    };

    // TEXCOORD_INDEX: a vertex texture coordinate set, or one generated from the camera-space vertex.
    struct texcoord {
        enum value : uint32_t {
            PASS_THROUGH = 0,
            CAMERA_SPACE_NORMAL = 0x10000,
            CAMERA_SPACE_POSITION = 0x20000,
            CAMERA_SPACE_REFLECTION_VECTOR = 0x30000,
        };
    };

    struct texture_transform_flags {
        enum value : uint32_t {
            DISABLE = 0,
            COUNT1 = 1,
            COUNT2 = 2,
            COUNT3 = 3,
            COUNT4 = 4,
            PROJECTED = 256,
        };
    };

    struct address {
        enum value : uint32_t {
            WRAP = 1,
            MIRROR = 2,
            CLAMP = 3,
            BORDER = 4,
        };
    };

    struct filter {
        enum value : uint32_t {
            NONE = 0,
            POINT = 1,
            LINEAR = 2,
            ANISOTROPIC = 3,
        };
    };

    // Vertex formats as bit sets of the fields, in their order in the vertex.
    struct fvf {
        enum value : uint32_t {
            XYZ = 0x002,
            XYZRHW = 0x004, // already transformed: screen x, y, z and 1/w
            POSITION_MASK = 0x00e,
            NORMAL = 0x010,
            PSIZE = 0x020,
            DIFFUSE = 0x040,
            SPECULAR = 0x080,
            TEX1 = 0x100,
            TEX2 = 0x200,
            TEX3 = 0x300,
            TEXCOUNT_MASK = 0xf00,
        };
    };

    constexpr uint32_t TEXCOUNT_SHIFT = 8;

    // Size in bytes of one vertex of the format.
    auto vertex_size(uint32_t format) -> uint32_t;

    enum class light_type : uint32_t {
        POINT = 1,
        SPOT = 2,
        DIRECTIONAL = 3,
    };

    struct light {
        light_type type = light_type::POINT;
        math::color diffuse;
        math::color specular;
        math::color ambient;
        math::vector3 position;
        math::vector3 direction;
        float range = 0.0f;
        float falloff = 0.0f;
        float attenuation0 = 0.0f;
        float attenuation1 = 0.0f;
        float attenuation2 = 0.0f;
        float theta = 0.0f; // inner cone angle of a spot light
        float phi = 0.0f;   // outer cone angle
    };

    struct material {
        math::color diffuse;
        math::color ambient;
        math::color specular;
        math::color emissive;
        float power = 0.0f;
    };
}
