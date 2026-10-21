#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace metin2::inline trees {

    // A tree generated from a SpeedTree 1.x parameter file (.spt). The file holds generation
    // parameters, not meshes: load() grows the branches, fronds and leaves from them, statically lit
    // and with texture coordinates flipped for the renderer. See PORTING.md, section 7.
    class spt_tree {
      public:
        // Branches and fronds share one vertex layout (the forest's branch vertex format).
        struct vertex {
            float position[3];
            uint32_t color; // ARGB, lighting included
            float tex_coords[2];
            float shadow_coords[2];
        };

        // A single triangle strip; degenerate triangles join the pieces.
        struct mesh {
            std::vector<vertex> vertices;
            std::vector<uint16_t> strip;
        };

        struct leaf {
            float center[3];
            uint32_t color;
            uint8_t map; // index into leaf_maps()
        };

        // Per leaf texture a plain and a mirrored entry (texture * 2 + mirrored). The corners are
        // offsets from the leaf center and face the camera given to face_leaves().
        struct leaf_map {
            float tex_coords[4][2];
            float corners[4][3]; // top right, top left, bottom left, bottom right
        };

        struct cylinder {
            float position[3];
            float radius;
            float height;
        };

        // Leaves and fronds are alpha tested against this reference value.
        static constexpr uint32_t alpha_test_value = 84;

        static auto load(std::span<const uint8_t> data) -> std::expected<spt_tree, std::string>;

        auto branches() const -> const mesh & { return branches_; }
        auto fronds() const -> const mesh & { return fronds_; }
        auto leaves() const -> const std::vector<leaf> & { return leaves_; }
        auto leaf_maps() const -> const std::vector<leaf_map> & { return leaf_maps_; }
        // min x, y, z, max x, y, z
        auto bounds() const -> const float (&)[6] { return bounds_; }
        // Only the trunk capsules: the crown spheres sit far above walking height and would push
        // the camera into the player under every tree.
        auto collision() const -> const std::vector<cylinder> & { return collision_; }

        // Texture file names without directory; empty when the tree has none.
        auto bark_texture() const -> const std::string & { return bark_texture_; }
        auto composite_texture() const -> const std::string & { return composite_texture_; }
        auto shadow_texture() const -> const std::string & { return shadow_texture_; }

        // Turns the leaf quads toward the camera looking along direction.
        void face_leaves(const float (&direction)[3]);

      private:
        friend class spt_generator;

        mesh branches_;
        mesh fronds_;
        std::vector<leaf> leaves_;
        std::vector<leaf_map> leaf_maps_;
        std::vector<float> leaf_sizes_;   // per leaf texture: width, height
        std::vector<float> leaf_origins_; // per leaf texture: x, y (fraction of the size)
        float bounds_[6] = {};
        std::vector<cylinder> collision_;
        std::string bark_texture_;
        std::string composite_texture_;
        std::string shadow_texture_;
        float facing_[3] = {};
        bool faced_ = false;
    };
}
