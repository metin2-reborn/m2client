#pragma once

#include <metin2/math/math.h>

#include <cstdint>

namespace metin2::inline world::inline terrain {
    class terrain_quadtree_node {
      public:
        terrain_quadtree_node();
        virtual ~terrain_quadtree_node();

      public:
        int32_t x0, y0, x1, y1;
        terrain_quadtree_node *nw_node;
        terrain_quadtree_node *ne_node;
        terrain_quadtree_node *sw_node;
        terrain_quadtree_node *se_node;
        int32_t size;
        int32_t patch_num;
        math::vector3 center;
        float radius;
        uint8_t lod_level;
    };
}
