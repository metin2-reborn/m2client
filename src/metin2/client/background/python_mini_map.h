#pragma once

#include <metin2/graphics/grp_expanded_image_instance.h>
#include <metin2/graphics/grp_screen.h>
#include <metin2/script/python_header.h>
#include <metin2/world/terrain/map_outdoor.h>

namespace metin2::inline client::inline background {
    class python_mini_map : public screen {
      public:
        enum {
            EMPIRE_NUM = 4,

            MINI_WAYPOINT_IMAGE_COUNT = 12,
            WAYPOINT_IMAGE_COUNT = 15,
            TARGET_MARK_IMAGE_COUNT = 2,
        };

        enum {
            TYPE_OPC,
            TYPE_OPCPVP,
            TYPE_OPCPVPSELF,
            TYPE_NPC,
            TYPE_MONSTER,
            TYPE_WARP,
            TYPE_WAYPOINT,
            TYPE_PARTY,
            TYPE_EMPIRE,
            TYPE_EMPIRE_END = TYPE_EMPIRE + EMPIRE_NUM,
            TYPE_TARGET,
            TYPE_COUNT,
        };

      public:
        python_mini_map();

        ~python_mini_map() override;

        void destroy();
        auto create() -> bool;

        auto is_atlas() -> bool;
        auto can_show() -> bool;

        void set_mini_map_size(float width_value, float height);
        void set_scale(float scale);
        void scale_up();
        void scale_down();

        void set_center_position(float center_x, float center_y);

        void update(float center_x, float center_y);
        void render(float screen_x, float screen_y);

        void show();
        void hide();

        auto picked_instance_info(float screen_x, float screen_y, std::string &return_name, float *return_pos_x, float *return_pos_y, uint32_t *text_color) -> bool;

        auto load_atlas() -> bool;
        void update_atlas();
        void render_atlas(float screen_x, float screen_y);
        void show_atlas();
        void hide_atlas();

        auto atlas_info(float screen_x, float screen_y, std::string &return_string, float *return_pos_x, float *return_pos_y, uint32_t *text_color, uint32_t *guild_id) -> bool;
        auto atlas_size(float *size_x, float *size_y) -> bool;
        // The part of the atlas the window shows: a view of width x height screen pixels whose
        // top left is image pixel (offset_x, offset_y), at scale screen pixels per image pixel.
        void set_atlas_view(float width, float height, float scale, float offset_x, float offset_y);

        void add_way_point(uint8_t type, uint32_t id, float x, float y, std::string text, uint32_t chr_vid = 0);
        void remove_way_point(uint32_t id);

        void add_signal_point(float x, float y);
        void clear_all_signal_point();

        void register_atlas_window(PyObject *handler);
        void unregister_atlas_window();
        void open_atlas_window();
        void set_atlas_center_position(int x, int y);

        // NPC list
        void clear_atlas_mark_info();
        void register_atlas_mark(uint8_t type, const char *name, int32_t lx, int32_t ly);

        void clear_guild_area();
        void register_guild_area(uint32_t id, uint32_t guild_id, int32_t x, int32_t y, int32_t width_value, int32_t height);
        auto guild_area_id(uint32_t x, uint32_t y) -> uint32_t;

        void create_target(int id, const char *name);
        void create_target(int id, const char *name, uint32_t vid);
        void update_target(int id, int ix, int iy);
        void delete_target(int id);

      protected:
        void initialize_impl();
        void set_position_impl();
        void load_atlas_mark_info();

        void render_way_point_mark(int ix_center, int iy_center);
        void render_mini_way_point_mark(int ix_center, int iy_center);
        void render_target_mark(int ix_center, int iy_center);

        void global_position_to_atlas_position(int32_t lx, int32_t ly, float *pfx, float *pfy);

      protected:
        struct atlas_mark_info {
            uint8_t type_value;
            uint32_t id; // for waypoints
            float x;
            float y;
            float screen_x;
            float screen_y;
            float mini_map_x;
            float mini_map_y;
            uint32_t chr_vid;
            std::string text;
        };

        struct guild_area_info {
            uint32_t guild_id;
            int32_t lx, ly;
            int32_t lwidth, lheight;

            float fsx_render, fsy_render;
            float fex_render, fey_render;
        };

        struct mark_position {
            float x;
            float y;
            uint32_t name_color;
        };

        using instance_mark_position_vector = std::vector<mark_position>;
        using instance_position_vector_iterator = instance_mark_position_vector::iterator;

      protected:
        auto way_point(uint32_t id, atlas_mark_info **info) -> bool;
        void update_way_point(atlas_mark_info *info, int ix, int iy);

      protected:
        float width_;
        float height_;

        float scale_;

        float center_x_;
        float center_y_;

        float center_cell_x_;
        float center_cell_y_;

        float screen_x_;
        float screen_y_;

        float mini_map_radius_;

        gpu::texture *mini_map_texture_[AROUND_AREA_NUM];

        graphic_image_instance mini_map_filter_graphic_image_instance_;
        graphic_expanded_image_instance mini_map_cameraraphic_image_instance_;

        graphic_expanded_image_instance player_mark_;
        graphic_image_instance white_mark_;

        instance_mark_position_vector party_pc_position_vector_;
        instance_mark_position_vector other_pc_position_vector_;
        instance_mark_position_vector npc_position_vector_;
        instance_mark_position_vector monster_position_vector_;
        instance_mark_position_vector warp_position_vector_;

        bool atlas_;
        bool show_;

        graphic_vertex_buffer vertex_buffer_;
        graphic_index_buffer index_buffer_;

        math::matrix identity_;
        math::matrix world_;
        math::matrix mini_map_cover_;

        bool show_atlas_;
        graphic_expanded_image_instance atlas_image_instance_;
        math::matrix world_atlas_;
        graphic_expanded_image_instance atlas_player_mark_;
        float atlas_player_x_ = 0.0f; // image pixels
        float atlas_player_y_ = 0.0f;

        float atlas_screen_x_;
        float atlas_screen_y_;

        float atlas_view_width_ = 0.0f;
        float atlas_view_height_ = 0.0f;
        float atlas_scale_ = 1.0f;
        float atlas_offset_x_ = 0.0f;
        float atlas_offset_y_ = 0.0f;

        // Image pixels to screen pixels of the view; false outside it.
        auto atlas_to_screen(float image_x, float image_y, float *screen_x, float *screen_y) const -> bool;

        uint32_t atlas_base_x_;
        uint32_t atlas_base_y_;

        float atlas_max_x_;
        float atlas_max_y_;

        float atlas_image_size_x_;
        float atlas_image_size_y_;

        using atlas_mark_info_vector = std::vector<atlas_mark_info>;
        using atlas_mark_info_vector_iterator = atlas_mark_info_vector::iterator;
        using guild_area_info_vector = std::vector<guild_area_info>;
        using guild_area_info_vector_iterator = guild_area_info_vector::iterator;

        atlas_mark_info_vector_iterator atlas_mark_info_vector_iterator_value_;
        atlas_mark_info_vector atlas_npc_info_vector_;
        atlas_mark_info_vector atlas_warp_info_vector_;

        graphic_expanded_image_instance mini_way_point_graphic_image_instances_[MINI_WAYPOINT_IMAGE_COUNT];
        graphic_expanded_image_instance way_point_graphic_image_instances_[WAYPOINT_IMAGE_COUNT];
        graphic_expanded_image_instance target_mark_graphic_image_instances_[TARGET_MARK_IMAGE_COUNT];
        graphic_image_instance guild_area_flag_image_instance_;
        atlas_mark_info_vector atlas_way_point_info_vector_;
        guild_area_info_vector guild_area_info_vector_value_;

        struct signal_point {
            math::vector2 pos;
            unsigned int id;
        };

        std::vector<signal_point> signal_point_vector_;

        PyObject *handler_;
    };

    inline auto get_python_mini_map() -> python_mini_map & {
        static python_mini_map instance;

        return instance;
    }
}
