#pragma once

// Handles a character's motion, combo, sound, weapon trace and attached effect data as a whole.

#include <list>
#include <metin2/world/flying/fly_target.h>
#include <metin2/world/maps/map_type.h>
#include "actor_instance_interface.h"
#include "interface.h"
#include "physics_object.h"
#include "race_data.h"
#include "race_motion_data.h"

namespace metin2::inline world::inline items {
    class item_data;
}
namespace metin2::inline world::inline flying {
    class fly_event_handler;
}
namespace metin2::inline trees {
    class speed_tree_wrapper;
}

namespace metin2::inline world::inline actors {

    class weapon_trace;

    class actor_instance : public actor_instance_interface, public fly_targetable_object {
      public:
        class event_handler {
          public:
            static auto empty_ptr() -> event_handler *;

          public:
            struct state {
                pixel_position p_pos_self;
                float adv_rot_self;
            };

          public:
            event_handler() = default;
            virtual ~event_handler() = default;

            virtual void on_syncing(const state &state_value) = 0;
            virtual void on_waiting(const state &state_value) = 0;
            virtual void on_moving(const state &state_value) = 0;
            virtual void on_move(const state &state_value) = 0;
            virtual void on_stop(const state &state_value) = 0;
            virtual void on_warp(const state &state_value) = 0;
            virtual void on_set_affect(uint32_t affect) = 0;
            virtual void on_reset_affect(uint32_t affect) = 0;
            virtual void on_clear_affects() = 0;

            virtual void on_attack(const state &state_value, uint16_t motion_index) = 0;
            virtual void on_use_skill(const state &state_value, uint32_t mot_skill, uint32_t mot_loop_count) = 0;

            virtual void on_hit(uint32_t skill, actor_instance &actor_victim, bool is_send_packet) = 0;

            virtual void on_change_shape() = 0;
        };

        // Fixes getting stuck in the map after using Gungsin Tanyoung.
      private:
        static auto get_background() -> background_interface &;

      public:
        static auto is_dir_line() -> bool;

      public:
        enum type {
            TYPE_ENEMY,
            TYPE_NPC,
            TYPE_STONE,
            TYPE_WARP,
            TYPE_DOOR,
            TYPE_BUILDING,
            TYPE_PC,
            TYPE_POLY,
            TYPE_HORSE,
            TYPE_GOTO,

            TYPE_OBJECT, // Client only.
        };

        enum render_mode {
            RENDER_MODE_NORMAL,
            RENDER_MODE_BLEND,
            RENDER_MODE_ADD,
            RENDER_MODE_MODULATE,
        };

        enum motion_push_type {
            MOTION_TYPE_NONE,
            MOTION_TYPE_ONCE,
            MOTION_TYPE_LOOP,
        };

        struct reserving_motion_node {
            motion_push_type motion_type_value;

            float start_time;
            float blend_time;
            float duration;
            float speed_ratio;

            uint32_t motion_key;
        };

        struct current_motion_node {
            motion_push_type motion_type_value;
            uint32_t motion_key;

            uint32_t dwcur_frame;
            uint32_t frame_count;

            float start_time;
            float end_time;
            float speed_ratio;

            int loop_count;
            uint32_t skill;
        };

        using motion_deque = std::deque<reserving_motion_node>;

        struct motion_event_instance {
            int type_value;
            int motion_event_index;
            float starting_time;

            const race_motion_data::motion_event_data *c_motion_data;
        };

        using motion_event_instance_list = std::list<motion_event_instance>;
        using motion_event_instance_list_iterator = motion_event_instance_list::iterator;

        struct collision_point_instance {
            const race_types::collision_data *c_collision_data;
            bool is_attached;
            uint32_t model_index;
            uint32_t bone_index;
            dynamic_sphere_instance_vector sphere_instance_vector;
        };
        using collision_point_instance_list = std::list<collision_point_instance>;
        using collision_point_instance_list_iterator = collision_point_instance_list::iterator;

        using hitted_instance_map = std::map<actor_instance *, float>;
        using hit_data_map = std::map<const race_types::hit_data *, hitted_instance_map>;
        struct splash_area {
            bool is_enable_hit_process;
            uint32_t skill;
            MOTION_KEY motion_key;
            float disappearing_time;
            const race_motion_data::motion_attacking_event_data *c_attacking_event;
            dynamic_sphere_instance_vector sphere_instance_vector;

            hitted_instance_map hitted_instance_map_value;
        };

        struct hitting_data {
            uint8_t attacking_type;
            uint32_t motion_key;
            uint8_t event_index;
        };

        enum attach_effect {
            EFFECT_LIFE_NORMAL,
            EFFECT_LIFE_INFINITE,
            EFFECT_LIFE_WITH_MOTION,
        };

        struct attaching_effect {
            uint32_t effect_index;
            int bone_index;
            uint32_t model_index;
            math::matrix translation;
            bool is_attaching;

            int life_type;
            uint32_t end_time;
        };

      public:
        static void show_direction_line(bool is_visible);
        static void destroy_system();

      public:
        actor_instance();
        ~actor_instance() override;

        void instancebase_transform();
        void instancebase_deform();

        void destroy();

        void move();
        void stop(float blending_time = 0.15f);

        void set_main_instance();

        void set_sleep(bool is_sleep);
        void set_resist_fallen(bool is_resist_fallen);

        void set_attack_speed(float atk_spd);
        void set_move_speed(float mov_spd);

        void set_material_color(uint32_t color);

        void set_event_handler(event_handler *event_handler_value);

        auto set_race(uint32_t race) -> bool;
        void set_hair(uint32_t hair);
        void set_virtual_id(uint32_t vid);

        void set_shape(uint32_t shape, float specular = 0.0f);
        void change_material(const char *file_name);

      public:
        void set_combo_type(uint16_t combo_type);

        auto race() -> uint32_t;
        auto virtual_id() -> uint32_t override;

        auto actor_type() const -> uint32_t;
        void set_actor_type(uint32_t type_value);

        auto can_act() -> bool;
        auto can_move() -> bool;
        auto can_attack() -> bool;
        auto can_use_skill() -> bool;

        auto is_pc() -> bool;
        auto is_npc() -> bool;
        auto is_enemy() -> bool;
        auto is_stone() -> bool;
        auto is_warp() -> bool;
        auto is_goto() -> bool;
        auto is_object() -> bool;
        auto is_door() -> bool;
        auto is_poly() -> bool;

        auto is_building() -> bool;

        auto is_hand_mode() -> bool;
        auto is_bow_mode() -> bool;
        auto is_two_hand_mode() -> bool;

        void attach_weapon(uint32_t item_index, uint32_t parent_part_index = race_data::PART_MAIN, uint32_t part_index = race_data::PART_WEAPON);
        void attach_weapon(uint32_t parent_part_index, uint32_t part_index, item_data *item_data);

        void refresh_actor_instance();
        auto part_item_id(uint32_t part_index) -> uint32_t;

        auto attaching_bone_name(uint32_t part_index, const char **bone_name) -> bool;
        void update_attaching_instances();
        void dettach_effect(uint32_t eid);
        auto attach_effect_by_name(uint32_t parent_part_index, const char *bone_name, const char *effect_file_name) -> uint32_t;
        auto attach_effect_by_id(uint32_t parent_part_index, const char *bone_name, uint32_t effect_id, const math::vector3 *position = nullptr) -> uint32_t;
        auto attach_smoke_effect(uint32_t smoke) -> uint32_t;

        void set_motion_mode(int motion_mode); // TODO: allow the motion mode change to be delayed.
        auto motion_mode() -> int;
        void set_loop_motion(uint32_t motion, float blend_time = 0.1f, float speed_ratio = 1.0f);
        auto intercept_once_motion(uint32_t motion, float blend_time = 0.1f, uint32_t skill = 0, float speed_ratio = 1.0f) -> bool;
        auto intercept_loop_motion(uint32_t motion, float blend_time = 0.1f) -> bool;
        auto push_once_motion(uint32_t motion, float blend_time = 0.1f, float speed_ratio = 1.0f) -> bool;
        auto push_loop_motion(uint32_t motion, float blend_time = 0.1f, float speed_ratio = 1.0f) -> bool;
        void set_motion_loop_count(int count);

        auto is_pushing() -> bool;

        auto is_lock() -> bool;
        auto is_using_skill() -> bool;
        auto can_check_attacking() -> bool;
        auto can_cancel_skill() -> bool;

        auto create_collision_instance_piece(uint32_t attaching_model_index, const race_types::attaching_data *attaching_data, collision_point_instance *point_instance) -> bool;

        void update_point_instance();
        void update_point_instance(collision_point_instance *point_instance);
        auto check_collision_detection(const dynamic_sphere_instance_vector *attacking_sphere_vector, math::vector3 *position) -> bool;

        auto test_collision_with_dynamic_sphere(const dynamic_sphere_instance &dsi) -> bool override;

        void update_advancing_point_instance();

        auto is_clickable_distance_dest_instance(actor_instance &inst_dst, float distance) -> bool;

        auto avoid_object(const graphic_object_instance &bg_obj) -> bool;
        auto is_block_object(const graphic_object_instance &bg_obj) -> bool;
        void block_movement();
        void add_movement(float fx, float fy, float fz);

      protected:
        auto test_object_collision(const graphic_object_instance *object_instance) -> bool;

      public:
        auto test_actor_collision(actor_instance &victim) -> bool;
        auto test_physics_blending_collision(actor_instance &victim) -> bool;

        auto attacking_process(actor_instance &victim) -> bool;

        // Only used by the player's character. With networking, other characters' combat is only displayed,
        // so no condition checks are needed.
        void input_normal_attack_command(float dir_rot);
        auto input_combo_attack_command(float dir_rot) -> bool;

        auto is_attacking() -> bool;
        auto is_normal_attacking() -> bool;
        auto is_combo_attacking() -> bool;
        auto is_splash_attacking() -> bool;
        auto is_using_moving_skill() -> bool;
        auto is_act_emotion() -> bool;
        auto combo_index() -> uint32_t;
        auto attacking_elapsed_time() -> float;
        void set_blending_position(const pixel_position &position, float blending_time = 1.0f);
        void reset_blending_position();
        void blending_position(pixel_position *position);

        auto normal_attack(float dir_rot, float blend_time = 0.1f) -> bool;
        auto combo_attack(uint32_t motion_index, float dir_rot, float blend_time = 0.1f) -> bool;

        void revive();

        auto is_sleep() -> bool;
        auto is_paralysis() -> bool;
        auto is_faint() -> bool;
        auto is_resist_fallen() -> bool;
        auto is_waiting() -> bool;
        auto is_moving() -> bool;
        auto is_dead() -> bool;
        auto is_stun() -> bool;
        auto is_attacked() -> bool;
        auto is_knock_down() -> bool;
        void set_walk_mode();
        void set_run_mode();
        void stun();
        void die();
        void die_end();

        void set_battle_hit_effect(uint32_t id);

        auto normal_attack_index() -> MOTION_KEY;

        auto movement_vector_ref() -> const math::vector3 &;

        void set_cur_pixel_position(const pixel_position &p_pos_cur);
        void new_set_atk_pixel_position(const pixel_position &p_pos_atk);
        void new_set_src_pixel_position(const pixel_position &p_pos_src);
        void new_set_dst_pixel_position(const pixel_position &p_pos_dst);
        void new_set_dst_pixel_position_z(float z);

        auto new_get_atk_pixel_position_ref() -> const pixel_position &;
        auto new_get_cur_pixel_position_ref() -> const pixel_position &;
        auto new_get_src_pixel_position_ref() -> const pixel_position &;
        auto new_get_dst_pixel_position_ref() -> const pixel_position &;

        auto new_get_last_pixel_position_ref() -> const pixel_position &;

        void get_pixel_position(pixel_position *pixel_position);
        void set_pixel_position(const pixel_position &pixel_pos);

        void look_at(float dir_rot);
        void look_at(float fx, float fy);
        void look_at(actor_instance *instance);

        void set_reach_scale(float scale);
        void set_owner(uint32_t owner_vid);

        auto rotation() -> float;
        auto target_rotation() -> float;

        auto advancing_rotation() -> float;

        void set_rotation(float rot);
        void set_xy_rotation(float rot_x, float rot_y);
        void blend_rotation(float rot, float blend_time);
        void set_advancing_rotation(float rot);

        void motion_event_process();
        void motion_event_process(uint32_t dwcur_time, int index, const race_motion_data::motion_event_data *data);
        void sound_event_process(bool check_frequency);

        auto is_movement() -> bool;

        void restore_render_mode();

        void begin_diffuse_render();
        void end_diffuse_render();
        void begin_opacity_render();
        void end_opacity_render();

        void begin_blend_render();
        void end_blend_render();
        void set_blend_render_mode();
        void set_alpha_value(float alpha);
        auto alpha_value() -> float;
        void blend_alpha_value(float dst_alpha, float duration);
        void set_specular_info(bool enable, int part, float alpha);

        void begin_add_render();
        void end_add_render();
        void set_add_render_mode();
        void set_add_color(const math::color &color);

        void begin_modulate_render();
        void end_modulate_render();
        void set_modulate_render_mode();

        void set_render_mode(int render_mode_value);

        void render_trace();
        void render_collision_data();
        void render_to_shadow_map();

      protected:
        void adjust_collision_movement(const graphic_object_instance *graphic_object_instance);

      public:
        void adjust_dynamic_collision_movement(const actor_instance *actor_instance);

        void set_weapon_trace_texture(const char *texture_name);
        void use_texture_weapon_trace();
        void use_alpha_weapon_trace();

        void update_attribute();
        auto intersect_defending_sphere() -> bool;
        auto height() -> float;
        void show_all_attaching_effect();
        void hide_all_attaching_effect();
        void clear_attaching_effect();

        auto can_fishing() -> bool;
        auto is_fishing() -> bool;
        void set_fishing_position(math::vector3 &position);

        // As a fly target.
      public:
        auto on_get_fly_target_position() -> math::vector3 override;

        void on_shoot_damage() override;

        // As a shooter.
        // The target and the target position are mutually exclusive.
      public:
        void clear_fly_target();
        auto is_fly_target_object() -> bool;
        void add_fly_target(const fly_target &cr_fly_target);
        void set_fly_target(const fly_target &cr_fly_target);
        void look_at_fly_target();

        auto fly_target_distance() -> float;

        void set_fly_event_handler(fly_event_handler *handler);

        // Keeps the target from changing while a skill is in use.
        auto can_change_target() -> bool;

      protected:
        fly_event_handler *fly_event_handler_value_;

      public:
        void mount_horse(actor_instance *horse);
        void horse_motion_process(bool is_pc);
        void motion_process(bool is_pc);
        void rotation_process();
        void physics_process();
        void combo_process();
        void transform_process();
        void accumulation_movement();
        void shake_process();
        void trace_process();
        void motion_event_process_impl(bool is_pc);
        void accumulation_movement_impl(float rot);
        auto splash_attack_process(actor_instance &victim) -> bool;
        auto normal_attack_process(actor_instance &victim) -> bool;
        auto can_input_normal_attack_command() -> bool;

        // A short warm tint on being hit; off when damage numbers are off.
        void flash(uint32_t duration);
        static void set_hit_flash_enabled(bool enabled) { hit_flash_enabled_ = enabled; }

      private:
        void shake(uint32_t duration);

      protected:
        fly_target fly_target_value_;
        fly_target backup_fly_target_;
        std::deque<fly_target> que_fly_target_;

      protected:
        auto is_in_splash_time() -> bool;

        void on_update() override;
        void on_render() override;

        auto is_valid_attacking() -> bool;

        void reserving_motion_process();
        void current_motion_process();
        auto random_motion_key(MOTION_KEY motion_key) -> MOTION_KEY;

        auto last_motion_time(float blend_time) -> float; // Returns the time brought forward by blend_time.
        auto motion_duration(uint32_t motion_key) -> float;

        auto intercept_motion(motion_push_type motion_type, uint16_t motion, float blend_time = 0.1f, uint32_t skill = 0, float speed_ratio = 1.0f) -> bool;
        void push_motion(motion_push_type motion_type, uint32_t motion_key, float blend_time, float speed_ratio = 1.0f);
        void process_motion_event_effect_event(const race_motion_data::motion_event_data *data);
        void process_motion_event_effect_to_target_event(const race_motion_data::motion_event_data *data);
        void process_motion_event_special_attacking(int motion_event_index, const race_motion_data::motion_event_data *data);
        void process_motion_event_sound(const race_motion_data::motion_event_data *data);
        void process_motion_event_fly(const race_motion_data::motion_event_data *data);
        void process_motion_event_warp(const race_motion_data::motion_event_data *data);

        auto is_left_hand_weapon(uint32_t type_value) -> bool;
        auto is_right_hand_weapon(uint32_t type_value) -> bool;
        auto is_weapon_trace(uint32_t weapon_type) -> bool;

      protected:
        void initialize_movement();

      protected:
        void initialize_impl();

        void clear_attaching_effect_impl();

        auto owner_time() -> float;
        auto owner_vid() -> uint32_t;
        auto can_push_dest_actor(actor_instance &actor_dst) -> bool;

      protected:
        void run_next_combo();
        void clear_combo();
        void on_end_combo();

        void process_data_attack_success(const race_types::attack_data &attack_data, actor_instance &victim, const math::vector3 &position, uint32_t skill = 0, bool is_send_packet = true);

        void hit_stone(actor_instance &victim);
        void hit_good(actor_instance &victim);
        void hit_greate(actor_instance &victim);

        void push_circle(actor_instance &victim);
        auto is_invisible() -> bool;
        void set_falling_direction(float fx, float fy);

      protected:
        struct set_motion_data {
            MOTION_KEY mot_key;
            float speed_ratio;
            float blend_time;
            int loop_count;
            uint32_t skill;

            set_motion_data() {
                loop_count = 0;
                mot_key = 0;
                speed_ratio = 1.0f;
                blend_time = 0.0f;
                skill = 0;
            }
        };

      protected:
        auto attack_speed() -> float;
        auto set_motion_impl(const set_motion_data &set_mot_data, uint32_t rand_mot_key = 0) -> uint32_t;
        void clear_motion();

        auto bind_motion_data(uint32_t motion_key) -> bool;

        auto motion_type() -> uint32_t;

        auto is_need_fly_target_motion() -> bool;
        auto is_wait_motion() -> bool;
        auto is_move_motion() -> bool;
        auto is_damage_motion() -> bool;
        auto is_knock_down_motion() -> bool;
        auto is_die_motion() -> bool;
        auto is_stand_up_motion() -> bool;
        auto is_mounting_horse_impl() -> bool;

        auto can_attack_impl() -> bool;
        auto can_next_combo_attack() -> bool;

        auto is_combo_attacking_impl() -> bool;

        auto current_motion_index() -> uint16_t;

        auto loop_count() -> int;
        auto current_combo_type() -> uint16_t;

        void show_event();
        void hide_event();
        auto is_hiding() -> bool;
        auto is_moving_skill_impl(uint16_t skill_number) -> bool;

        auto reach_scale() -> float;

        void create_attribute_instance(attribute_data *data);

        auto is_fly_target_pc() -> bool;
        auto is_same_fly_target(actor_instance *instance) -> bool;
        auto fly_target_position_impl() -> math::vector3;

      protected:
        void destroy_weapon_trace();
        void show_weapon_trace();
        void hide_weapon_trace();

      protected:
        void on_update_collision_data(const static_collision_data_vector *pscd_vector) override;
        void on_update_heigh_instance(attribute_instance *attribute_instance) override;
        auto on_get_object_height(float x, float y, float *height) -> bool override;

      protected:
        motion_deque motion_deque_value_;
        current_motion_node cur_mot_node_;
        float motion_tick_time_ = 0.0f; // seconds not yet spent on game_fps motion frames
        uint16_t wcur_motion_mode_;

        collision_point_instance_list body_point_instance_list_;
        collision_point_instance_list defending_point_instance_list_;
        splash_area splash_area_value_; // TODO: support more than one splash area.
        attribute_instance *attribute_instance_value_;

        std::vector<weapon_trace *> weapon_trace_vector_;
        physics_object physics_object_value_;

        uint32_t dwcur_combo_index_;

        uint32_t actor_type_;

        uint32_t race_;
        uint32_t shape_value_;
        uint32_t hair_value_;
        bool is_pre_input_;
        bool is_next_pre_input_;
        uint32_t dwcur_combo_back_motion_index_;

        uint16_t wcur_combo_type_;

        float atk_dir_rot_;

        race_data *cur_race_data_;
        race_motion_data *cur_race_motion_data_;

        float invisible_time_;
        bool is_hiding_;

        // TODO: consider merging these flags into the state.
        bool is_resist_fallen_;
        bool is_sleep_;
        bool is_faint_;
        bool is_paralysis_;
        bool is_stun_;
        bool is_real_dead_;
        bool is_walking_;
        bool is_main_;

        uint32_t battle_hit_effect_id_;
        uint32_t battle_attach_effect_id_;

        math::vector3 fishing_position_;
        int fishing_effect_id_;

        float x_;
        float y_;
        float z_;
        math::vector3 pos_;
        math::vector3 movement_;
        bool need_update_collision_;

        uint32_t shake_time_;
        uint32_t flash_time_;
        static bool hit_flash_enabled_;

        float reach_scale_;
        float mov_spd_;
        float atk_spd_;

        float fcur_rotation_;
        float rot_begin_;
        float rot_end_;
        float rot_end_time_;
        float rot_begin_time_;
        float rot_blend_time_;
        float advancing_rotation_;
        float rot_x_;
        float rot_y_;

        float owner_base_time_;

        int render_mode_value_;
        math::color add_color_;
        float alpha_value_;

        uint32_t part_item_id_[race_data::PART_MAX_NUM];

        std::list<attaching_effect> attaching_effect_list_;
        bool effect_initialized_;

        uint32_t mtrl_color_;
        uint32_t mtrl_alpha_;

        pixel_position p_pos_cur_;
        pixel_position p_pos_src_;
        pixel_position p_pos_dst_;
        pixel_position p_pos_atk_;

        pixel_position p_pos_last_;

        hit_data_map hit_data_map_value_;

        actor_instance *horse_;
        speed_tree_wrapper *tree_;

      protected:
        uint32_t self_vid_;
        uint32_t owner_vid_;

      protected:
        void initialize_state_data();
        void initialize_motion_data();
        void initialize_rotation_data();
        void initialize_position_data();

      public: // Public until merged with instance_base.
        auto event_handler_ref_impl() -> event_handler &;

        void on_syncing_impl();
        void on_waiting_impl();
        void on_moving_impl();
        void on_move_impl();
        void on_stop_impl();
        void on_warp_impl();
        void on_clear_affects_impl();
        void on_set_affect_impl(uint32_t affect);
        void on_reset_affect_impl(uint32_t affect);
        void on_attack_impl(uint16_t motion_index);
        void on_use_skill_impl(uint32_t mot_skill, uint32_t loop_count, bool is_moving);

      protected:
        void on_hit_impl(uint32_t skill, actor_instance &inst_victm, bool is_send_packet);

      public:
        void enable_skip_collision();
        void disable_skip_collision();
        auto can_skip_collision() -> bool;

      protected:
        void initialize_collision_data();

        bool can_skip_collision_;

      protected:
        struct blend_alpha {
            float base_time;
            float base_alpha;
            float duration;
            float dst_alpha;

            uint32_t old_render_mode;
            bool is_blending;
        } blend_alpha_value_;

        void blend_alpha_initialize();
        void blend_alpha_apply(float dst_alpha, float duration);
        void blend_alpha_update();
        void blend_alpha_update_complete();
        auto blend_alpha_get_elapsed_time() -> float;

        void push_impl(int x, int y);

      public:
        void temp_push(int x, int y);
        auto is_syncing() -> bool;

        void create_tree(const char *file_name);
        void destroy_tree();
        void set_tree_position(float fx, float fy, float fz);

      protected:
        event_handler *event_handler_value_;

      protected:
        static bool is_dir_line_;
    };
}
