//	CSpeedTreeForestDirectX8 Class
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

#include <metin2/graphics/grp_base.h>
#include "speed_tree_forest.h"

namespace metin2::inline trees {

    class speed_tree_forest_direct_x8 : public speed_tree_forest, public graphic_base {
      public:
        speed_tree_forest_direct_x8();
        ~speed_tree_forest_direct_x8() override;

        void update_compund_matrix(const math::vector3 &eye_vec, const math::matrix &view, const math::matrix &proj);

        void render(uint32_t render_bit_vector = Forest_RenderAll) override;

        // The direction the leaf quads face (set by update_compund_matrix).
        auto camera_direction() const -> const float (&)[3] { return camera_direction_; }

      private:
        float camera_direction_[3] = {0.0f, 1.0f, 0.0f};
    };

    inline auto get_speed_tree_forest_direct_x8() -> speed_tree_forest_direct_x8 & {
        static speed_tree_forest_direct_x8 instance;
        return instance;
    }
}
