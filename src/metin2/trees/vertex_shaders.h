//	SpeedTreeRT DirectX Example: vertex formats
//	(c) 2003 IDV, Inc.
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//		IDV, Inc.
//		1233 Washington St. Suite 610
//		Columbia, SC 29201
//		Voice: (803) 799-1699
//		Fax:   (803) 931-0320
//		Web:   http://www.idvinc.com

#pragma once

#include <cstdint>

#include <metin2/gpu/types.h>
#include <metin2/math/math.h>

namespace metin2::inline trees {

    // Branches and fronds: position, precomputed colour, texture and shadow coordinates.
    const uint32_t SPEEDTREE_BRANCH_VERTEX_FORMAT = gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX2 | 0 | 0;

    struct fvf_branch_vertex {
        math::vector3 v_position;
        uint32_t diffuse_color;
        float tex_coords[2];
        float shadow_coords[2];
    };

    static_assert(sizeof(fvf_branch_vertex) == 32, "branch vertex layout");

    // Leaves: position, precomputed colour, texture coordinates.
    const uint32_t SPEEDTREE_LEAF_VERTEX_FORMAT = gpu::fvf::XYZ | gpu::fvf::DIFFUSE | gpu::fvf::TEX1 | 0;

    struct fvf_leaf_vertex {
        math::vector3 v_position;
        uint32_t diffuse_color;
        float tex_coords[2];
    };

    static_assert(sizeof(fvf_leaf_vertex) == 24, "leaf vertex layout");
}
