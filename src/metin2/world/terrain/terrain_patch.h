#pragma once

#include "terrain.h"

#include <metin2/gpu/types.h>
#include <metin2/graphics/grp_vertex_buffer.h>

namespace metin2::inline world::inline terrain {
#pragma pack(push)
#pragma pack(1)
    struct hardware_transform_patch_source_vertex {
        math::vector3 position_value;
        math::vector3 normal_value;
    };

    struct water_vertex {
        float x, y, z;
        uint32_t diffuse_value;
    };

    // Grass: two crossed quads per tuft. Diffuse alpha carries the tuft height / 128 for the
    // vertex shader's sway and fade; v is 0 at the tip and 1 at the root.
    struct grass_vertex {
        math::vector3 position_value;
        math::vector3 normal_value;
        uint32_t diffuse_value;
        float u, v;
    };
#pragma pack(pop)

    class terrain_patch {
      public:
        enum {
            PATCH_TYPE_PLAIN = 0,
            PATCH_TYPE_HILL,
            PATCH_TYPE_CLIFF,
        };

        enum {
            TERRAIN_VERTEX_COUNT = (terrain_impl::PATCH_XSIZE + 1) * (terrain_impl::PATCH_YSIZE + 1)
        };

      public:
        terrain_patch() { clear(); }

        ~terrain_patch() { clear(); }

        void clear();

        void set_min_x(float min_x) { min_x_ = min_x; }
        auto min_x() -> float { return min_x_; }

        void set_max_x(float max_x) { max_x_ = max_x; }
        auto max_x() -> float { return max_x_; }

        void set_min_y(float min_y) { min_y_ = min_y; }
        auto min_y() -> float { return min_y_; }

        void set_max_y(float max_y) { max_y_ = max_y; }
        auto max_y() -> float { return max_y_; }

        void set_min_z(float min_z) { min_z_ = min_z; }
        auto min_z() -> float { return min_z_; }

        void set_max_z(float max_z) { max_z_ = max_z; }
        auto max_z() -> float { return max_z_; }

        auto is_water_exist() -> bool { return water_exist_; }
        void set_water_exist(bool water_exist) { water_exist_ = water_exist; }

        void set_type(uint8_t type) { type_value_ = type; }
        auto get_type() -> uint8_t { return type_value_; }

        void need_update(bool need_update) { need_update_ = need_update; }
        auto need_update() -> bool { return need_update_; }

        auto water_face_count() -> uint32_t;

        void build_terrain_vertex_buffer(hardware_transform_patch_source_vertex *src_vertex);
        void build_water_vertex_buffer(water_vertex *src_vertex, uint32_t water_vertex_count);

      private:
        float min_x_;
        float max_x_;
        float min_y_;
        float max_y_;
        float min_z_;
        float max_z_;
        bool water_exist_;
        uint32_t water_pri_count_;

        graphic_vertex_buffer water_vertex_buffer_;
        uint8_t type_value_;

        bool need_update_;

      public:
        auto water_vertex_buffer_pointer() -> graphic_vertex_buffer * { return &water_vertex_buffer_; }

      public:
        auto hardware_transform_patch_get_vertex_buffer_ptr() -> graphic_vertex_buffer * { return &ht_.vb; }

        // Built on demand while the patch is near the camera and released when unused for a while.
        graphic_vertex_buffer grass_vb;
        uint32_t grass_quad_count = 0;
        uint32_t grass_generation = 0; // 0: not built
        uint32_t grass_last_used_frame = 0;
        void release_grass();

      protected:
        struct hardware_transform_patch {
            graphic_vertex_buffer vb;
        } ht_;
    };

    class terrain_patch_proxy {
      public:
        terrain_patch_proxy();
        virtual ~terrain_patch_proxy();

        void clear();

        void set_center_position(const math::vector3 &center);

        auto is_in(const math::vector3 &target, float radius) -> bool;

        auto is_used() -> bool { return used_; }
        void set_used(bool used) { used_ = used; }

        auto patch_num() -> short { return patch_num_; }
        void set_patch_num(short patch_num) { patch_num_ = patch_num; }

        auto terrain_num() -> uint8_t { return terrain_num_; }
        void set_terrain_num(uint8_t terrain_num) { terrain_num_ = terrain_num; }

        void set_terrain_patch(terrain_patch *terrain_patch) { terrain_patch_value_ = terrain_patch; }

        auto is_water_exists() -> bool;

        auto water_face_count() -> uint32_t;

        auto min_x() -> float;
        auto max_x() -> float;
        auto min_y() -> float;
        auto max_y() -> float;
        auto min_z() -> float;
        auto max_z() -> float;

        auto water_vertex_buffer_pointer() -> graphic_vertex_buffer *;
        auto hardware_transform_patch_get_vertex_buffer_ptr() -> graphic_vertex_buffer *;

      protected:
        bool used_;
        short patch_num_;

        uint8_t terrain_num_;

        terrain_patch *terrain_patch_value_;

        math::vector3 center_;
    };

    inline auto terrain_patch_proxy::is_water_exists() -> bool {
        return terrain_patch_value_->is_water_exist();
    }

    inline auto terrain_patch_proxy::min_x() -> float {
        return terrain_patch_value_->min_x();
    }

    inline auto terrain_patch_proxy::max_x() -> float {
        return terrain_patch_value_->max_x();
    }

    inline auto terrain_patch_proxy::min_y() -> float {
        return terrain_patch_value_->min_y();
    }

    inline auto terrain_patch_proxy::max_y() -> float {
        return terrain_patch_value_->max_y();
    }

    inline auto terrain_patch_proxy::min_z() -> float {
        return terrain_patch_value_->min_z();
    }

    inline auto terrain_patch_proxy::max_z() -> float {
        return terrain_patch_value_->max_z();
    }

    inline auto terrain_patch_proxy::water_vertex_buffer_pointer() -> graphic_vertex_buffer * {
        return terrain_patch_value_->water_vertex_buffer_pointer();
    }
}
