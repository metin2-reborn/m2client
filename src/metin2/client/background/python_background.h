#pragma once

#include <metin2/world/maps/map_manager.h>
#include <metin2/world/maps/snow_environment.h>

namespace metin2::inline client::inline characters {
    class instance_base;
}

namespace metin2::inline client::inline background {

    class python_background : public map_manager {
      public:
        enum {
            SHADOW_NONE,
            SHADOW_GROUND,
            SHADOW_GROUND_AND_SOLO,
            SHADOW_ALL,
            SHADOW_ALL_HIGH,
            SHADOW_ALL_MAX,
        };

        enum {
            DISTANCE0,
            DISTANCE1,
            DISTANCE2,
            DISTANCE3,
            DISTANCE4,
            NUM_DISTANCE_SET
        };

        enum {
            DAY_MODE_LIGHT,
            DAY_MODE_DARK,
        };

        struct sviewdistanceset {
            float fog_start;
            float fog_end;
            float far_clip;
            math::vector3 sky_box_scale;
        };
        using tviewdistanceset = sviewdistanceset;

      public:
        python_background();
        ~python_background() override;

        void initialize();

        void destroy();
        void create();

        void global_position_to_local_position(int32_t &global_x, int32_t &global_y);
        void local_position_to_global_position(int32_t &local_x, int32_t &local_y);

        void enable_terrain_only_for_height();
        auto set_splat_limit(int splat_num) -> bool;
        auto set_visible_part(int part, bool is_visible) -> bool;
        auto set_shadow_level(int level) -> bool;
        void refresh_shadow_level();
        void select_view_distance_num(int num);
        void set_view_distance_set(int num, float far_clip);
        auto far_clip() -> float;

        auto render_shadow_time() -> uint32_t;
        void distance_set_info(int *pe_num, float *start, float *end, float *far_clip);

        auto picking_point(math::vector3 *intersect_pt) -> bool;
        auto picking_point_with_ray(const ray &ray, math::vector3 *intersect_pt) -> bool;
        auto picking_point_with_ray_only_terrain(const ray &ray, math::vector3 *intersect_pt) -> bool;

        void update(float center_x, float center_y, float center_z);

        void create_character_shadow_texture();
        void release_character_shadow_texture();
        void render();
        void render_snow();
        void render_pc_blocker();
        void render_collision();
        void render_character_shadow_to_texture();
        void render_sky();
        void render_cloud();
        void render_water();
        void render_effect();
        void render_before_lens_flare();
        void render_after_lens_flare();

        auto check_advancing(instance_base *instance) -> bool;

        void set_character_dir_light();
        void set_background_dir_light();

        void enable_snow_environment();
        void disable_snow_environment();
        void set_x_max_tree(int grade);

        void clear_guild_area();
        void register_guild_area(int isx, int isy, int iex, int iey);

        void create_target_effect(uint32_t id, uint32_t chr_vid);
        void create_target_effect(uint32_t id, int32_t lx, int32_t ly);
        void delete_target_effect(uint32_t id);

        void create_special_effect(uint32_t id, float fx, float fy, float fz, const char *file_name);
        void delete_special_effect(uint32_t id);

        void warp(uint32_t x, uint32_t y);

        void visible_guild_area();
        void disable_guild_area();

        void register_dungeon_map_name(const char *map_name);
        auto global_position_to_map_info(uint32_t global_x, uint32_t global_y) -> map_info *;
        auto warp_map_name() -> const char *;

      protected:
        void create_property();
        auto is_same(std::set<int> &rleft, std::set<int> &rright) -> bool;

      protected:
        std::string map_name_;

      private:
        snow_environment snow_environment_value_;

        int day_mode_;
        int x_mas_tree_grade_;

        int shadow_level_;
        int view_distance_num_;

        bool visible_guild_area_;

        uint32_t render_shadow_time_;

        uint32_t base_x_;
        uint32_t base_y_;

        tviewdistanceset view_distance_set_[NUM_DISTANCE_SET];

        std::set<int> set_showing_portal_id_;
        std::set<std::string> set_dungeon_map_name_;
        std::map<uint32_t, uint32_t> map_target_id_chr_id_;

        struct reserve_target_effect {
            int ilx;
            int ily;
        };
        std::map<uint32_t, reserve_target_effect> map_id_reserve_target_effect_;

        struct find_warp_map_name_fn {
            uint32_t x, y;
            find_warp_map_name_fn(uint32_t x_value, uint32_t y_value) {
                x = x_value;
                y = y_value;
            }
            auto operator()(map_info &map_info) -> bool {
                if (x < map_info.base_x || x >= map_info.end_x || y < map_info.base_y || y >= map_info.end_y)
                    return false;
                return true;
            }
        };
    };

    inline auto get_python_background() -> python_background & {
        static python_background instance;
        return instance;
    }
}
