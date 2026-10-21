#pragma once

#include <list>
#include <metin2/graphics/grp_mark_instance.h>
#include <metin2/graphics/grp_object_instance.h>
#include <metin2/graphics/grp_text_instance.h>
#include <metin2/world/maps/map_type.h>

namespace metin2::inline client::inline social {

    // Text that follows characters: name, guild name, guild mark and so on.
    class python_text_tail {
      public:
        struct text_tail {
            graphic_text_instance *text_instance;
            graphic_text_instance *owner_text_instance;

            graphic_mark_instance *mark_instance;
            graphic_text_instance *guild_name_text_instance;

            graphic_text_instance *title_text_instance;
            graphic_text_instance *level_text_instance;

            // TODO: use a VID instead; this pointer may dangle when the character disappears meanwhile.
            graphic_object_instance *owner;

            uint32_t virtual_id;

            float x, y, z;
            float distance_from_player;
            math::color color_value;
            bool name_flag; // whether the name is shown as well

            float x_start, y_start;
            float x_end, y_end;

            uint32_t living_time;

            float height;

            text_tail() = default;
            virtual ~text_tail() = default;
        };

        using text_tail_map = std::map<uint32_t, text_tail *>;
        using text_tail_list = std::list<text_tail *>;
        using chat_tail_map = text_tail_map;

      public:
        python_text_tail(void);
        ~python_text_tail(void);

        void get_info(std::string *info);

        void initialize();
        void destroy();
        void clear();

        void update_all_text_tail();
        void update_showing_text_tail();
        void render();

        void arrange_text_tail();
        void hide_all_text_tail();
        void show_all_text_tail();
        void show_character_text_tail(uint32_t virtual_id);
        void show_item_text_tail(uint32_t virtual_id);

        void register_character_text_tail(uint32_t guild_id, uint32_t virtual_id, const math::color &color, float add_height = 10.0f);
        void register_item_text_tail(uint32_t virtual_id, const char *text, graphic_object_instance *owner);
        void register_chat_tail(uint32_t virtual_id, const char *chat);
        void register_info_tail(uint32_t virtual_id, const char *chat);
        void set_character_text_tail_color(uint32_t virtual_id, const math::color &color);
        void set_item_text_tail_owner(uint32_t vid, const char *name);
        void delete_character_text_tail(uint32_t virtual_id);
        void delete_item_text_tail(uint32_t virtual_id);

        auto pick(int ix_mouse, int iy_mouse) -> int;
        void select_item_name(uint32_t virtual_id);

        auto text_tail_position(uint32_t vid, float *px, float *py, float *pz) -> bool;
        auto is_chat_text_tail(uint32_t vid) -> bool;

        void enable_pk_title(bool flag);
        void attach_title(uint32_t vid, const char *name, const math::color &color);
        void detach_title(uint32_t vid);

        void attach_level(uint32_t vid, const char *text, const math::color &color);

      protected:
        auto register_text_tail(uint32_t virtual_id, const char *text, graphic_object_instance *owner, float height, const math::color &color) -> text_tail *;
        void delete_text_tail(text_tail *text_tail_value);

        void update_text_tail(text_tail *text_tail_value);
        void render_text_tail_box(text_tail *text_tail_value);
        void render_text_tail_name(text_tail *text_tail_value);
        void update_distance(const pixel_position &center_position, text_tail *text_tail_value);

        auto is_in(text_tail *source, text_tail *target) -> bool;

      protected:
        text_tail_map character_text_tail_map_;
        text_tail_map item_text_tail_map_;
        chat_tail_map chat_tail_map_value_;

        text_tail_list character_text_tail_list_;
        text_tail_list item_text_tail_list_;

      private:
        dynamic_pool<text_tail> text_tail_pool_;
    };

    inline auto get_python_text_tail() -> python_text_tail & {
        static python_text_tail instance;
        return instance;
    }
}
