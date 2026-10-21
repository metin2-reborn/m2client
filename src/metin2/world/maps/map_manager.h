#pragma once

class map_base;
#include <metin2/world/terrain/map_outdoor.h>
#include "property_manager.h"

#include <metin2/world/actors/physics_object.h>

namespace metin2::inline world::inline maps {

    class map_manager : public screen, public physics_world {
      public:
        map_manager();
        ~map_manager() override;

        auto is_map_outdoor() -> bool;
        auto map_outdoor_ref() -> map_outdoor &;

        auto is_software_tiling_enable() -> bool;
        void reserve_software_tiling_enable(bool is_enable);

        void initialize();
        void destroy();

        void create();

        virtual void clear();
        virtual auto alloc_map() -> map_base *;

        auto is_map_ready() -> bool;

        virtual auto load_map(const std::string &map_name, float x, float y, float z) -> bool;

        auto update_map(float fx, float fy, float fz) -> bool;
        void update_around_ambience(float fx, float fy, float fz);
        auto height(float fx, float fy) -> float;
        auto terrain_height(float fx, float fy) -> float;
        auto water_height(int x, int y, int32_t *pl_water_height) -> bool;

        auto get_normal(int ix, int iy, math::vector3 *normal) -> bool;

        // Gradual blending into the next environment is not implemented yet.
        void set_environment_data_ptr(const environment_data *environment_data);
        void reset_environment_data_ptr(const environment_data *environment_data);
        void set_environment_data(int env_data_index);

        void begin_environment();
        void end_environment();

        auto register_environment_data(uint32_t index, const char *file_name) -> bool;
        auto get_environment_data(uint32_t index, const environment_data **environment_data) -> bool;

        void refresh_portal();
        void clear_portal();
        void add_showing_portal_id(int id);

        auto shadow_map_color(float fx, float fy) -> uint32_t;

        auto is_physical_collision(const math::vector3 &check_position) -> bool override;

        auto is_attr_on(float x, float y, uint8_t attr) -> bool;
        auto attr(float x, float y, uint8_t *attr) -> bool;
        auto is_attr_on(int x, int y, uint8_t attr) -> bool;
        auto attr(int x, int y, uint8_t *attr) -> bool;

        auto rendered_splat_num(int *patch, int *splat, float *splat_ratio) -> std::vector<int> &;
        auto rendered_graphic_thing_instance_num(uint32_t *graphic_thing_instance_num, uint32_t *crc_num) -> area::crc_with_number_vector &;

      protected:
        auto alloc_environment_data() -> environment_data *;
        void delete_environment_data(environment_data *environment_data);
        auto load_environment_data(const char *file_name, environment_data *environment_data) -> bool;

      protected:
        environment_data_map environment_data_map_value_;
        const environment_data *mc_pcur_environment_data_;

        map_outdoor *map_;

      public:
        void set_terrain_render_sort(map_outdoor::terrain_render_sort terrain_render_sort);
        auto get_terrain_render_sort() -> map_outdoor::terrain_render_sort;

        void base_xy(uint32_t *base_x, uint32_t *base_y);

      public:
        void set_transparent_tree(bool transparen_tree);

      public:
        struct map_info {
            std::string name_value;
            uint32_t base_x;
            uint32_t base_y;
            uint32_t size_x;
            uint32_t size_y;
            uint32_t end_x;
            uint32_t end_y;
        };
        using map_info_vector = std::vector<map_info>;
        using map_info_vector_iterator = map_info_vector::iterator;

      protected:
        map_info_vector vct_map_info_;

        bool is_software_tiling_enable_reserved_;

      protected:
        void load_map_info_vector();

      public:
        void set_atlas_info_file_name(const char *filename) {
            atlas_info_file_name_ = filename;
        }

      private:
        std::string atlas_info_file_name_;
    };
}
