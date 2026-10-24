#pragma once

#include <vector>

#include "terrain.h"
#include "terrain_patch.h"

namespace metin2::inline world::inline terrain {
    class map_outdoor;

    class terrain_area : public terrain_impl, public graphic_base {
      public:
        enum boundary_load_part {
            LOAD_INVALID,
            LOAD_NOBOUNDARY,
            LOAD_TOPLEFT,
            LOAD_TOP,
            LOAD_TOPRIGHT,
            LOAD_LEFT,
            LOAD_RIGHT,
            LOAD_BOTTOMLEFT,
            LOAD_BOTTOM,
            LOAD_BOTTOMRIGHT,
            LOAD_ALLBOUNDARY,
        };

        terrain_area();
        ~terrain_area() override;

        void clear() override;

        void set_map_out_door(map_outdoor *owner_outdoor_map);

        auto raw_load_tile_map(const char *file_name) -> bool;

        auto load_height_map(const char *file_name) -> bool;

        void calculate_terrain_patch();

        void copy_setting_from_global_setting();

        auto is_ready() -> bool { return ready_; }
        void set_ready(bool ready = true) { ready_ = ready; }

        auto height_map() -> uint16_t * { return raw_height_map_; }
        auto height(int x, int y) -> float;

        auto get_normal(int ix, int iy, math::vector3 *normal) -> bool;

        auto normal_map() -> char * { return normal_map_; }

        auto load_attr_map(const char *file_name) -> bool;
        auto attr(uint16_t coord_x, uint16_t coord_y) -> uint8_t;
        auto is_attr_on(uint16_t coord_x, uint16_t coord_y, uint8_t attr_flag) -> bool;

        auto water_map() -> uint8_t * { return water_map_; }
        void water_height(uint8_t water_num, int32_t *pl_water_height);
        auto water_height(uint16_t coord_x, uint16_t coord_y, int32_t *pl_water_height) -> bool;

        void load_shadow_texture(const char *file_name);

        auto load_shadow_map(const char *file_name) -> bool;

        void load_mini_map_texture(const char *file_name);
        inline auto mini_map_texture() -> gpu::texture * { return mini_map_texture_; }

        auto is_marked() -> bool { return marked_; }
        void allocate_marked_splats(uint8_t *alpha_map);
        void deallocate_marked_splats();
        auto marked_splat_patch() -> terrain_splat_patch & { return marked_splat_patch_; }

        void coordinate(uint16_t *us_coord_x, uint16_t *us_coord_y) {
            *us_coord_x = x_;
            *us_coord_y = y_;
        }

        void set_coordinate(uint16_t coord_x, uint16_t coord_y);

        auto get_name() -> std::string & { return name_value_; }
        void set_name(const std::string name) { name_value_ = name; }

        auto owner() -> map_outdoor * { return owner_outdoor_map_; }
        void raw_generate_splat();

      protected:
        auto initialize() -> bool;
        void raw_allocate_splats();
        void raw_deallocate_splats();
        virtual void raw_count_tiles();

        auto add_texture32(uint8_t image_num, uint8_t *image, int32_t texture_width, int32_t texture_height) -> gpu::texture *;
        void put_image32(uint8_t *src, uint8_t *dst, int32_t src_pitch, int32_t dst_pitch, int32_t texture_width, int32_t texture_height, bool resize = false);
        void put_image16(uint8_t *src, uint8_t *dst, int32_t src_pitch, int32_t dst_pitch, int32_t texture_width, int32_t texture_height, bool resize = false);

      protected:
        void calculate_normal(int32_t x, int32_t y);

      protected:
        std::string name_value_;
        uint16_t x_;
        uint16_t y_;

      protected:
        bool ready_;

        graphic_image_instance shadow_graphic_image_instance_;

        graphic_image_instance mini_map_graphic_image_instance_;
        gpu::texture *mini_map_texture_;

        map_outdoor *owner_outdoor_map_;

        math::vector3 pick_;

        uint32_t num_textures_show_;
        std::vector<uint32_t> vector_num_show_texture_;

        terrain_patch terrain_patch_list_[PATCH_XCOUNT * PATCH_YCOUNT];

        bool marked_;
        terrain_splat_patch marked_splat_patch_;
        std::shared_ptr<gpu::texture> marked_texture_;

      public:
        // The grass tufts of one patch (see map_outdoor_grass.cpp); density 1 to 3.
        void build_grass_patch(uint8_t patch_num_x, uint8_t patch_num_y, int density, std::vector<grass_vertex> &out);

        auto terrain_patch_ptr(uint8_t patch_num_x, uint8_t patch_num_y) -> terrain_patch *;

      protected:
        void calculate_terrain_patch_impl(uint8_t patch_num_x, uint8_t patch_num_y);

      public:
        static void destroy_system();

        static auto new_value() -> terrain_area *;
        static void delete_value(terrain_area *terrain);

        static dynamic_pool<terrain_area> pool_value;
    };
}
