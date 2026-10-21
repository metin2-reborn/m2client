#include "terrain_quadtree.h"

namespace metin2::inline world::inline terrain {

    terrain_quadtree_node::terrain_quadtree_node() : nw_node(nullptr), ne_node(nullptr), sw_node(nullptr), se_node(nullptr), center(-1.0f, -1.0f, -1.0f) {
        x0 = y0 = x1 = y1 = 0;
        size = 0;
        patch_num = 0;
        radius = 0.0f;
        lod_level = 0;
    }

    terrain_quadtree_node::~terrain_quadtree_node() {
        if (nw_node) {
            delete nw_node;
            nw_node = nullptr;
        }
        if (ne_node) {
            delete ne_node;
            ne_node = nullptr;
        }
        if (sw_node) {
            delete sw_node;
            sw_node = nullptr;
        }
        if (se_node) {
            delete se_node;
            se_node = nullptr;
        }
    }
}
