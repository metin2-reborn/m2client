#include "map_outdoor.h"

#include "terrain_quadtree.h"

#include <metin2/log/log.h>

namespace metin2::inline world::inline terrain {
    void map_outdoor::build_quad_tree() {
        free_quad_tree();

        // patch_count_ is also set in convert_patch_splat; checked here to be safe.
        if (0 == patch_count_) {
            metin2::log::error("Cannot build quadtree: patch count is zero (convert the patch splats first)");
            return;
        }

        root_node_ = alloc_quad_tree_node(0, 0, patch_count_ - 1, patch_count_ - 1);
        if (!root_node_) {
            metin2::log::error("Cannot build quadtree: no root node");
        }

        if (root_node_->size > 1) {
            sub_divide_node(root_node_);
        }
    }

    auto map_outdoor::alloc_quad_tree_node(int32_t x0, int32_t y0, int32_t x1, int32_t y1) -> terrain_quadtree_node * {
        terrain_quadtree_node *node;
        int32_t xsize, ysize;

        xsize = x1 - x0 + 1;
        ysize = y1 - y0 + 1;
        if ((xsize == 0) || (ysize == 0)) {
            return nullptr;
        }

        node = new terrain_quadtree_node;
        node->x0 = x0;
        node->y0 = y0;
        node->x1 = x1;
        node->y1 = y1;

        if (ysize > xsize) {
            node->size = ysize;
        } else {
            node->size = xsize;
        }

        node->patch_num = y0 * patch_count_ + x0;

        node->center.x = 0.0f;
        node->center.y = 0.0f;
        node->center.z = 0.0f;

        node->radius = 0.0f;

        return node;
    }

    void map_outdoor::sub_divide_node(terrain_quadtree_node *node) {
        int32_t nw_size;
        terrain_quadtree_node *tempnode;

        nw_size = node->size / 2;

        node->nw_node = alloc_quad_tree_node(node->x0, node->y0, node->x0 + nw_size - 1, node->y0 + nw_size - 1);
        node->ne_node = alloc_quad_tree_node(node->x0 + nw_size, node->y0, node->x1, node->y0 + nw_size - 1);
        node->sw_node = alloc_quad_tree_node(node->x0, node->y0 + nw_size, node->x0 + nw_size - 1, node->y1);
        node->se_node = alloc_quad_tree_node(node->x0 + nw_size, node->y0 + nw_size, node->x1, node->y1);

        tempnode = (terrain_quadtree_node *)node->nw_node;
        if ((tempnode != nullptr) && (tempnode->size > 1)) {
            sub_divide_node(tempnode);
        }

        tempnode = (terrain_quadtree_node *)node->ne_node;
        if ((tempnode != nullptr) && (tempnode->size > 1)) {
            sub_divide_node(tempnode);
        }

        tempnode = (terrain_quadtree_node *)node->sw_node;
        if ((tempnode != nullptr) && (tempnode->size > 1)) {
            sub_divide_node(tempnode);
        }

        tempnode = (terrain_quadtree_node *)node->se_node;
        if ((tempnode != nullptr) && (tempnode->size > 1)) {
            sub_divide_node(tempnode);
        }
    }

    void map_outdoor::free_quad_tree() {
        if (nullptr == root_node_) {
            return;
        }

        if (root_node_->nw_node) {
            delete root_node_->nw_node;
            root_node_->nw_node = nullptr;
        }

        if (root_node_->ne_node) {
            delete root_node_->ne_node;
            root_node_->ne_node = nullptr;
        }

        if (root_node_->sw_node) {
            delete root_node_->sw_node;
            root_node_->sw_node = nullptr;
        }

        if (root_node_->se_node) {
            delete root_node_->se_node;
            root_node_->se_node = nullptr;
        }

        delete root_node_;

        root_node_ = nullptr;
    }
}
