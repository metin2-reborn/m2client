#pragma once

#include <cmath>
#include <cstdint>

#include <metin2/gpu/device.h>

namespace metin2::inline world::inline terrain {

#define TERRAIN_PATCHSIZE  16
#define TERRAIN_SIZE       128
#define TERRAIN_PATCHCOUNT (TERRAIN_SIZE / TERRAIN_PATCHSIZE)
#define MAXTERRAINTEXTURES 256

    struct terain_splat {
        int32_t active;
        int32_t needs_update;
        gpu::texture *gpu_texture;
    };

    struct terrain_splat_patch {
        uint32_t tile_count[MAXTERRAINTEXTURES];
        uint32_t patch_tile_count[TERRAIN_PATCHCOUNT * TERRAIN_PATCHCOUNT][MAXTERRAINTEXTURES];
        terain_splat splats[MAXTERRAINTEXTURES];
        bool needs_update;
    };

    // Truncates a float towards negative infinity.
    inline auto pr_float_to_int(float value) -> int {
        return static_cast<int>(std::floor(value));
    }

#define PR_FLOAT_TO_INT(inreal, outint) ((outint) = pr_float_to_int(inreal))
}
