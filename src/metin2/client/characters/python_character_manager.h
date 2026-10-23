#pragma once

#include "instance_base.h"

#include <metin2/client/app/abstract_character_manager.h>
#include <metin2/client/app/game_type.h>
#include <metin2/world/actors/physics_object.h>

namespace metin2::inline client::inline characters {
    class python_character_manager : public abstract_character_manager, public object_manager {
      public:
        using character_instance_list = std::list<instance_base *>;
        using character_instance_map = std::map<uint32_t, instance_base *>;

        class character_iterator;

      public:
        python_character_manager();
        ~python_character_manager() override;

        void adjust_collision_with_other_objects(actor_instance *inst) override;

        auto is_registered_vid(uint32_t vid) -> bool;
        auto is_alive_vid(uint32_t vid) -> bool;
        auto is_dead_vid(uint32_t vid) -> bool;

        auto old_get_picked_instance_vid(uint32_t *picked_actor_id) -> bool;
        auto old_get_picked_instance_ptr() -> instance_base *;

        void insert_pvp_key(uint32_t vid_src, uint32_t vid_dst);
        void remove_pvp_key(uint32_t vid_src, uint32_t vid_dst);
        void change_gvg(uint32_t src_guild_id, uint32_t dst_guild_id);

        void get_info(std::string *info);

        void clear_main_instance();
        auto set_main_instance(uint32_t vid) -> bool;
        auto main_instance_ptr() -> instance_base *;

        void script_set_affect(uint32_t vid, uint32_t affect, bool is_visible);
        void set_emoticon(uint32_t vid, uint32_t emoticon);
        auto is_possible_emoticon(uint32_t vid) -> bool;
        void show_point_effect(uint32_t vid, uint32_t point);

        void destroy() override;

        void delete_all_instances();

        auto create_device_objects() -> bool;
        void destroy_device_objects();

        void update();
        void deform();
        void render();
        void render_shadow_main_instance();
        void render_shadow_all_instances();
        void render_collision();

        auto create_instance(const instance_base::create_data &create_data) -> instance_base *;
        auto register_instance(uint32_t virtual_id) -> instance_base *;

        void delete_instance(uint32_t virtual_id);
        void delete_instance_by_fade(uint32_t virtual_id);

        void destroy_alive_instance_map();
        void destroy_dead_instance_list();

        inline auto character_instance_begin() -> character_iterator { return character_iterator(alive_inst_map_.begin()); }
        inline auto character_instance_end() -> character_iterator { return character_iterator(alive_inst_map_.end()); }

        void select_instance(uint32_t virtual_id);
        auto selected_instance_ptr() -> instance_base *;

        auto instance_ptr(uint32_t virtual_id) -> instance_base * override;

        void refresh_all_pc_text_tail();
        void refresh_all_guild_mark();

      protected:
        void update_transform();
        void update_deleting();

      protected:
        void initialize_impl();

        void delete_blend_out_instance(instance_base *inst_del);

        void new_pick();

        void update_sort_picked_actor_list();
        void update_picked_actor_list();
        void sort_picked_actor_list();

        void render_sorted_alive_actor_list();
        void render_sorted_dead_actor_list();

      protected:
        instance_base *inst_main_;
        instance_base *inst_pick_;
        instance_base *inst_bind_;
        math::vector2 picked_inst_proj_pos_;

        character_instance_map alive_inst_map_;
        character_instance_list dead_inst_list_;

        std::vector<instance_base *> vct_inst_picked_;

        uint32_t point_effect_[POINT_MAX_NUM];

      public:
        class character_iterator {
          public:
            character_iterator() = default;
            character_iterator(const character_instance_map::iterator &it) : it_(it) {}

            inline auto operator*() -> instance_base * { return it_->second; }

            inline auto operator++() -> character_iterator & {
                ++it_;
                return *this;
            }

            inline auto operator++(int) -> character_iterator {
                character_iterator new_it = *this;
                ++(*this);
                return new_it;
            }

            inline auto operator==(const character_iterator &rhs) const -> bool {
                return it_ == rhs.it_;
            }

            inline auto operator!=(const character_iterator &rhs) const -> bool {
                return it_ != rhs.it_;
            }

          private:
            character_instance_map::iterator it_;
        };
    };

    inline auto get_python_character_manager() -> python_character_manager & {
        static python_character_manager instance;

        return instance;
    }
}
