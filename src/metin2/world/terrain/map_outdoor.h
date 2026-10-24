#pragma once

#include "area_terrain.h"
#include "texture_set.h"

#include <metin2/core/rectangle.h>
#include <metin2/gpu/types.h>
#include <metin2/graphics/lens_flare.h>
#include <metin2/graphics/render_target.h>
#include <metin2/graphics/screen_filter.h>
#include <metin2/graphics/sky_box.h>
#include <metin2/trees/speed_tree_forest_direct_x8.h>
#include <metin2/world/maps/area.h>
#include <metin2/world/maps/map_base.h>

#include <list>
#include <memory>

namespace metin2::inline world::inline terrain {
#define LOAD_SIZE_WIDTH 1

#define AROUND_AREA_NUM  1 + (LOAD_SIZE_WIDTH * 2) * (LOAD_SIZE_WIDTH * 2) * 2
#define MAX_PREPARE_SIZE 9
#define MAX_MAPSIZE      256 // 0 to 255; with a cell size of 200 that is 64 km.

#define TERRAINPATCH_LODMAX 3

    struct outdoor_map_coordinate {
        short terrain_coord_x;
        short terrain_coord_y;
    };

    using outdoor_map_coordinate_map = std::map<const std::string, outdoor_map_coordinate>;

    class terrain_patch_proxy;
    class terrain_quadtree_node;

    class map_outdoor : public map_base {
      public:
        enum {
            VIEW_NONE = 0,
            VIEW_PART,
            VIEW_ALL,
        };

        enum part {
            PART_TERRAIN,
            PART_OBJECT,
            PART_CLOUD,
            PART_WATER,
            PART_TREE,
            PART_SKY,
            PART_NUM,
        };

        enum terrain_render_sort {
            DISTANCE_SORT,
            TEXTURE_SORT,
        };

      public:
        map_outdoor();
        ~map_outdoor() override;

        void on_begin_environment() override;

      protected:
        auto initialize() -> bool override;
        void initialize_fog();

        auto destroy() -> bool override;
        void on_set_environment_data_ptr() override;
        void on_reset_environment_data_ptr() override;

        void on_render() override;

      public:
        void set_inverse_view_and_dynamic_shaodw_matrices();
        // Where the shadow-casting light stands for the character shadow map: along the
        // environment's sun, at least 30 degrees up so the blobs do not stretch across the ground.
        auto shadow_light_eye(const math::vector3 &target) const -> math::vector3;
        auto load(float x, float y, float z) -> bool override;
        auto height(float x, float y) -> float override;

        auto update(float x, float y, float z) -> bool override;
        void update_around_ambience(float x, float y, float z) override;

      public:
        void clear() override;

        void set_visible_part(int part_value, bool is_visible);
        void set_splat_limit(int splat_num);
        auto rendered_splat_num(int *patch, int *splat, float *splat_ratio) -> std::vector<int> &;
        auto rendered_graphic_thing_instance_num(uint32_t *graphic_thing_instance_num, uint32_t *crc_num) -> area::crc_with_number_vector &;

        auto load_setting(const char *file_name) -> bool;

        void apply_light(uintptr_t version, const gpu::light &light) override;
        void set_environment_screen_filter();
        void set_environment_sky_box();
        void set_environment_lens_flare();

        void create_character_shadow_texture();
        void release_character_shadow_texture();
        void set_shadow_texture_size(uint16_t size);

        auto begin_render_character_shadow_to_texture() -> bool;
        void end_render_character_shadow_to_texture();
        void render_water();
        void render_marked_area();
        void recurse_render_attr(terrain_quadtree_node *node, bool cull_enable = true);
        void draw_patch_attr(int32_t patchnum);
        void clear_guild_area();
        void register_guild_area(int isx, int isy, int iex, int iey);

        void visible_marked_area();
        void disable_marked_area();

        void update_sky();
        void render_collision();
        void render_sky();
        void render_cloud();
        void render_before_lens_flare();
        void render_after_lens_flare();

        auto picking_point_with_ray(const ray &ray, math::vector3 *intersect_pt) -> bool;
        auto picking_point_with_ray_only_terrain(const ray &ray, math::vector3 *intersect_pt) -> bool;
        auto picking_point(math::vector3 *intersect_pt) -> bool;
        void terrain_count(short *terrain_count_x, short *terrain_count_y) {
            *terrain_count_x = terrain_count_x_;
            *terrain_count_y = terrain_count_y_;
        }

        auto set_terrain_count(short terrain_count_x, short terrain_count_y) -> bool;

        void set_draw_shadow(bool draw_shadow);
        void set_draw_character_shadow(bool draw_chr_shadow);
        // Relief shading from the tile textures ([graphics] terrain_detail); 0 is off.
        void set_terrain_detail(float strength) { terrain_detail_strength_ = strength; }

        auto shadow_map_color(float fx, float fy) -> uint32_t;

      protected:
        auto pick_terrain_height(float &pos, const math::vector3 &start, const math::vector3 &end, float step, float ray_range, float limit_range, math::vector3 *pick) -> bool;

        virtual void clear_garvage();
        virtual void update_garvage();

        virtual auto load_terrain(uint16_t terrain_coord_x, uint16_t terrain_coord_y, uint16_t cell_coord_x, uint16_t cell_coord_y) -> bool;
        virtual auto load_area(uint16_t area_coord_x, uint16_t area_coord_y, uint16_t cell_coord_x, uint16_t cell_coord_y) -> bool;
        virtual void update_area_list(int32_t center_x, int32_t center_y);
        auto is_terrain_loaded(uint16_t x, uint16_t y) -> bool;
        auto is_area_loaded(uint16_t x, uint16_t y) -> bool;

        void assign_terrain_ptr(); // Links the terrains and areas around the current coordinate (e.g. 3x3); called on update.

        void height_map(const uint8_t &ruc_terrain_num, uint16_t **height_map);
        void normal_map(const uint8_t &ruc_terrain_num, char **puc_normal_map);

        void water_map(const uint8_t &ruc_terrain_num, uint8_t **puc_water_map);
        void water_height(uint8_t terrain_num, uint8_t water_num, int32_t *pl_water_height);

      protected:
        terrain_area *terrain_[AROUND_AREA_NUM];
        std::unique_ptr<terrain_patch_proxy[]> terrain_patch_proxy_list_; // The patches that render a terrain, kept apart from it for the seamless map.

        int32_t view_radius_; // In cells.
        float height_scale_;  // At 1.0 the height map covers 0 to 655.35 m.

        short terrain_count_x_, terrain_count_y_; // Number of terrains in the seamless map.

        outdoor_map_coordinate cur_coordinate_;

        int32_t cur_coord_start_x_, cur_coord_start_y_;
        outdoor_map_coordinate prev_coordinate_;
        outdoor_map_coordinate_map entry_point_map_;

        uint16_t patch_count_;

        uint16_t *pwa_indices_[TERRAINPATCH_LODMAX];

        graphic_index_buffer index_buffer_[TERRAINPATCH_LODMAX];
        uint16_t num_indices_[TERRAINPATCH_LODMAX];
        virtual void destroy_terrain();

        void create_terrain_patch_proxy_list();
        void destroy_terrain_patch_proxy_list();

        void update_terrain(float x, float y);

        void convert_terrain_to_tn_l(int32_t lx, int32_t ly);

        void assign_patch(int32_t patch_num, int32_t lx0, int32_t ly0, int32_t lx1, int32_t ly1);

        void add_lvl1tl(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl1t(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl1tr(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl1l(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl1r(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl1bl(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl1b(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl1br(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl1m(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl2tl(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl2t(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl2tr(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl2l(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl2r(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl2bl(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl2b(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl2br(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);
        void add_lvl2m(uint16_t *indices, uint16_t &rw_count, const uint16_t &rw_cur_count, const uint8_t &ruc_num_line_warp);

      public:
        auto terrain_pointer(uint8_t terrain_num, terrain_area **terrain) -> bool;
        auto terrain_height(float fx, float fy) -> float;
        auto water_height(int x, int y, int32_t *pl_water_height) -> bool;
        auto get_normal(int ix, int iy, math::vector3 *normal) -> bool;

        void render_terrain();

        auto view_radius() -> int32_t { return view_radius_; }
        auto height_scale() -> float { return height_scale_; }

        auto cur_coordinate() -> const outdoor_map_coordinate & { return cur_coordinate_; }

      protected:
        area *area_value_[AROUND_AREA_NUM];

        virtual void destroy_area();

        void update_area(math::vector3 &player);
        void game_update_area(math::vector3 &player);

        void collect_shadow_receiver(math::vector3 &target, math::vector3 &light);
        void collect_collision_pc_blocker(math::vector3 &eye, math::vector3 &target, float distance);
        void collect_collision_shadow_receiver(math::vector3 &target, math::vector3 &light);
        void update_around_area_list();
        auto is_in_shadow_receiver_list(graphic_object_instance *obj_inst_test) -> bool;
        auto is_in_pc_blocker_list(graphic_object_instance *obj_inst_test) -> bool;

        void convert_to_map_coords(float fx, float fy, int *cell_x, int *cell_y, uint8_t *puc_sub_cell_x, uint8_t *puc_sub_cell_y, uint16_t *terrain_num_x, uint16_t *terrain_num_y);

      public:
        auto area_pointer(const uint8_t area_num, area **area) -> bool;
        void render_area(bool render_ambience = true);
        void render_blend_area();
        void render_dungeon();
        void render_effect();
        void render_pc_blocker();
        void render_tree();

      public:
        auto height(float *pos) -> float;
        auto is_attr_on(float x, float y, uint8_t attr) -> bool;
        auto attr(float x, float y, uint8_t *attr) -> bool;
        auto is_attr_on(int x, int y, uint8_t attr) -> bool;
        auto attr(int x, int y, uint8_t *attr) -> bool;

        auto terrain_num(float fx, float fy, uint8_t *terrain_num) -> bool;
        auto terrain_num_from_coord(uint16_t coord_x, uint16_t coord_y, uint8_t *terrain_num) -> bool;

      protected:
        int32_t center_x_, center_y_; // Cell coordinate within the terrain.
        int32_t old_read_x_, old_read_y_;

        terrain_quadtree_node *root_node_;

        void build_quad_tree();
        auto alloc_quad_tree_node(int32_t x0, int32_t y0, int32_t x1, int32_t y1) -> terrain_quadtree_node *;
        void sub_divide_node(terrain_quadtree_node *node);
        void update_quad_tree_heights(terrain_quadtree_node *node);

        void free_quad_tree();

      protected:
        std::vector<std::pair<float, int32_t>> patch_vector_;

        void draw_water(int32_t patchnum);

        bool draw_shadow_;
        bool draw_chr_shadow_;
        float terrain_detail_strength_ = 1.5f;

        math::matrix bump_;
        void load_water_texture();
        void unload_water_texture();

        graphic_image_instance alpha_fog_image_instance_;
        math::matrix alpha_fog_texture_;

        // The characters are drawn into the raw target; a 2x2 tent blur of it in
        // character_shadow_target_ is what the ground and the receivers project.
        render_target character_shadow_raw_target_;
        render_target character_shadow_target_;
        gpu::viewport shadow_map_viewport_;
        uint16_t shadow_map_size_;
        void blur_character_shadow_texture();

        math::plane plane_[6];

        void build_view_frustum(math::matrix &mat);

        texture_set texture_set_value_;

      protected:
        sky_box sky_box_value_;
        lens_flare lens_flare_value_;
        screen_filter screen_filter_value_;

      protected:
        void set_index_buffer();
        void select_index_buffer(uint8_t lod_level, uint16_t *primitive_count, gpu::primitive *pe_primitive_type);

        math::matrix world_for_common_use_;
        math::matrix view_inverse_;

        math::matrix splat_alpha_;
        math::matrix static_shadow_;
        math::matrix dynamic_shadow_;
        math::matrix dynamic_shadow_scale_;
        math::matrix light_view_;

        float terrain_tex_coord_base_;
        float water_tex_coord_base_;

        float xfor_distance_caculation_, yfor_distance_caculation_;

      protected:
        using terrain_ptr_vector = std::vector<terrain_area *>;
        using terrain_ptr_vector_iterator = terrain_ptr_vector::iterator;
        using area_ptr_vector = std::vector<area *>;
        using area_ptr_vector_iterator = area_ptr_vector::iterator;

        terrain_ptr_vector terrain_vector_;
        terrain_ptr_vector terrain_delete_vector_;
        terrain_ptr_vector terrain_load_request_vector_;
        terrain_ptr_vector terrain_load_wait_vector_;
        terrain_ptr_vector_iterator terrain_ptr_vector_iterator_value_;

        area_ptr_vector area_vector_;
        area_ptr_vector area_delete_vector_;
        area_ptr_vector area_load_request_vector_;
        area_ptr_vector area_load_wait_vector_;
        area_ptr_vector_iterator area_ptr_vector_iterator_value_;

        struct push_to_delete_vector_fn {
            enum delete_dir {
                DELETE_LEFT,
                DELETE_RIGHT,
                DELETE_TOP,
                DELETE_BOTTOM,
            };

            delete_dir lr_delete_dir;
            delete_dir tb_delete_dir;
            outdoor_map_coordinate cur_coordinate;

            push_to_delete_vector_fn(delete_dir lr_delete_dir_value, delete_dir tb_delete_dir_value, outdoor_map_coordinate cur_coord) {
                lr_delete_dir = lr_delete_dir_value;
                tb_delete_dir = tb_delete_dir_value;
                cur_coordinate = cur_coord;
            }
        };

        struct push_terrain_to_delete_vector_fn : public push_to_delete_vector_fn {
            terrain_ptr_vector return_terrain_vector;

            push_terrain_to_delete_vector_fn(delete_dir lr_delete_dir, delete_dir tb_delete_dir, outdoor_map_coordinate cur_coord)
                : push_to_delete_vector_fn(lr_delete_dir, tb_delete_dir, cur_coord) {
                return_terrain_vector.clear();
            }

            void operator()(terrain_area *terrain);
        };

        struct push_area_to_delete_vector_fn : public push_to_delete_vector_fn {
            area_ptr_vector return_area_vector;

            push_area_to_delete_vector_fn(delete_dir lr_delete_dir, delete_dir tb_delete_dir, outdoor_map_coordinate cur_coord)
                : push_to_delete_vector_fn(lr_delete_dir, tb_delete_dir, cur_coord) {
                return_area_vector.clear();
            }

            void operator()(area *area);
        };

      protected:
        void initialize_visible_parts();
        auto is_visible_part(int part_value) -> bool;

        auto no_fog_distance() -> float;
        auto fog_distance() -> float;

      protected:
        uint32_t visible_part_flags_;

        int rendered_splat_num_sq_sum_;
        int rendered_splat_num_;
        int rendered_patch_num_;
        std::vector<int> rendered_texture_num_vector_;
        int splat_limit_;

      protected:
        int patch_terrain_vertex_count_;
        int patch_water_vertex_count_;

        int patch_terrain_vertex_size_;
        int patch_water_vertex_size_;

        uint32_t rendered_crc_num_;
        uint32_t rendered_graphic_thing_instance_num_;

        std::list<rectangle> list_guild_area_;

      protected:
        void render_terrain_recurse_render_quad_tree(terrain_quadtree_node *node, bool cull_check_need = true);
        auto render_terrain_recurse_render_quad_tree_check_bounding_circle(const math::vector3 &center, const float &radius) -> int;

        void render_terrain_append_patch(const math::vector3 &center, float distance, int32_t patch_num);

        void render_terrain_render_hardware_transform_patch();

      protected:
        void hardware_transform_patch_render_patch_splat(int32_t patchnum, uint16_t primitive_count, gpu::primitive primitive_type);
        void hardware_transform_patch_render_patch_none(int32_t patchnum, uint16_t primitive_count, gpu::primitive primitive_type);

      protected:
        std::vector<graphic_object_instance *> shadow_receiver_vector_;
        std::vector<graphic_object_instance *> pc_blocker_vector_;

      protected:
        float opaque_water_depth_;
        graphic_image_instance water_instances_[30];

      public:
        auto opaque_water_depth() -> float { return opaque_water_depth_; }
        void set_terrain_render_sort(terrain_render_sort terrain_render_sort_value) { terrain_render_sort_value_ = terrain_render_sort_value; }
        auto get_terrain_render_sort() -> terrain_render_sort { return terrain_render_sort_value_; }

      protected:
        terrain_render_sort terrain_render_sort_value_;

      protected:
        graphic_image_instance attr_image_instance_;
        graphic_image_instance building_transparent_image_instance_;
        math::matrix building_transparent_;

      protected:
      public:
      public:
        void base_xy(uint32_t *base_x, uint32_t *base_y);
        void set_base_xy(uint32_t base_x, uint32_t base_y);

        void set_transparent_tree(bool transparent_tree) { transparent_tree_ = transparent_tree; }
        void enable_terrain_only_for_height(bool flag) { enable_terrain_only_for_height_ = flag; }
        void enable_portal(bool flag);
        auto is_enable_portal() -> bool { return enable_portal_; }

      protected:
        uint32_t base_x_;
        uint32_t base_y_;

        math::vector3 player_;

        bool show_entire_patch_texture_count_;
        bool transparent_tree_;
        bool enable_terrain_only_for_height_;
        bool enable_portal_;

      private:
        struct x_mas_tree {
            speed_tree_wrapper *tree;
            int effect_id;
        } x_mas_;

        void x_mas_tree_initialize();
        void x_mas_tree_create(float x, float y, float z, const char *tree_name, const char *eff_name);

      public:
        void x_mas_tree_destroy();
        void x_mas_tree_set(float x, float y, float z, const char *tree_name, const char *eff_name);

      private:
        using special_effect_map = std::map<uint32_t, int>;
        special_effect_map map_id_effect_id_;

      public:
        void special_effect_create(uint32_t id, float x, float y, float z, const char *eff_name);
        void special_effect_delete(uint32_t id);
        void special_effect_destroy();

      private:
      public:
        auto environment_data_name() -> std::string &;

      protected:
        std::string settings_env_data_name_;
        std::string env_data_name_;

      private:
        bool setting_terrain_visible_;
    };
}
