//	CSpeedTreeForest Class
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
#include <map>

#include "speed_tree_wrapper.h"

namespace metin2::inline trees {

#define Forest_RenderBranches  (1 << 0)
#define Forest_RenderLeaves    (1 << 1)
#define Forest_RenderFronds    (1 << 2)
#define Forest_RenderAll       ((1 << 3) - 1)
#define Forest_RenderToShadow  (1 << 5)
#define Forest_RenderToMiniMap (1 << 6)

    class speed_tree_forest {
      public:
        using tree_map = std::map<uint32_t, speed_tree_wrapper *>;

      public:
        speed_tree_forest();
        virtual ~speed_tree_forest();

        auto main_tree(uint32_t crc, speed_tree_wrapper **main_tree, const char *file_name) -> bool;

        auto create_instance(float x, float y, float z, uint32_t tree_crc, const char *tree_name) -> speed_tree_wrapper *;
        void delete_instance(speed_tree_wrapper *tree);

        void clear();

        void set_light(const float *direction, const float *ambient, const float *diffuse);
        void set_fog(float fog_near, float fog_far);

        virtual void render(uint32_t render_bit_vector) = 0;

      protected:
        tree_map main_tree_map_;

        float lighting_[12];
        float fog_[4];
    };
}
