#pragma once

#include <metin2/client/player/affect_flag_container.h>
#include <metin2/world/actors/actor_instance.h>
#include <metin2/world/actors/race_data.h>

#include <list>
#include <memory>

namespace metin2::inline client::inline characters {
    class instance_base {
      public:
        struct create_data {
            uint8_t type_value;
            uint32_t state_flags;
            uint32_t empire_id;
            uint32_t guild_id;
            uint32_t level_value;
            uint32_t vid;
            uint32_t race;
            uint32_t mov_spd;
            uint32_t atk_spd;
            int32_t pos_x;
            int32_t pos_y;
            float rot;
            uint32_t armor;
            uint32_t weapon;
            uint32_t hair_value;
            uint32_t mount_vnum;

            short alignment;
            uint8_t pk_mode;
            affect_flag_container affect_flags;

            std::string name_value;

            bool is_main;
        };

      public:
        using type = uint32_t;

        enum direction {
            DIR_NORTH,
            DIR_NORTHEAST,
            DIR_EAST,
            DIR_SOUTHEAST,
            DIR_SOUTH,
            DIR_SOUTHWEST,
            DIR_WEST,
            DIR_NORTHWEST,
            DIR_MAX_NUM,
        };

        enum {
            FUNC_WAIT,
            FUNC_MOVE,
            FUNC_ATTACK,
            FUNC_COMBO,
            FUNC_MOB_SKILL,
            FUNC_EMOTION,
            FUNC_SKILL = 0x80,
        };

        enum {
            AFFECT_YMIR,
            AFFECT_INVISIBILITY,
            AFFECT_SPAWN,

            AFFECT_POISON,
            AFFECT_SLOW,
            AFFECT_STUN,

            AFFECT_DUNGEON_READY, // ready state in a dungeon
            AFFECT_SHOW_ALWAYS,   // Never culled on the client.

            AFFECT_BUILDING_CONSTRUCTION_SMALL,
            AFFECT_BUILDING_CONSTRUCTION_LARGE,
            AFFECT_BUILDING_UPGRADE,

            AFFECT_MOV_SPEED_POTION, // 11
            AFFECT_ATT_SPEED_POTION, // 12

            AFFECT_FISH_MIND, // 13

            AFFECT_JEONGWI,             // 14 Jeongwihon
            AFFECT_GEOMGYEONG,          // 15 Geomgyeong
            AFFECT_CHEONGEUN,           // 16 Cheongeunchu
            AFFECT_GYEONGGONG,          // 17 Gyeonggongsul
            AFFECT_EUNHYEONG,           // 18 Eunhyeongbeop
            AFFECT_GWIGEOM,             // 19 Gwigeom
            AFFECT_GONGPO,              // 20 Gongpo
            AFFECT_JUMAGAP,             // 21 Jumagap
            AFFECT_HOSIN,               // 22 Hosin
            AFFECT_BOHO,                // 23 Boho
            AFFECT_KWAESOK,             // 24 Kwaesok
            AFFECT_HEUKSIN,             // 25 Heuksin guardian
            AFFECT_MUYEONG,             // 26 Muyeongjin
            AFFECT_REVIVE_INVISIBILITY, // 27 invincible after revive
            AFFECT_FIRE,                // 28 burning
            AFFECT_GICHEON,             // 29 Gicheon daegong
            AFFECT_JEUNGRYEOK,          // 30 Jeungryeoksul
            AFFECT_DASH,                // 31 dash
            AFFECT_PABEOP,              // 32 Pabeopsul
            AFFECT_FALLEN_CHEONGEUN,    // 33 downgraded Cheongeunchu
            AFFECT_POLYMORPH,           // 34 polymorph
            AFFECT_WAR_FLAG1,           // 35
            AFFECT_WAR_FLAG2,           // 36
            AFFECT_WAR_FLAG3,           // 37
            AFFECT_CHINA_FIREWORK,      // 38
            AFFECT_PREMIUM_SILVER,
            AFFECT_PREMIUM_GOLD,
            AFFECT_RAMADAN_RING, // 41 wearing the crescent ring

            AFFECT_NUM = 64,

            AFFECT_HWAYEOM = AFFECT_GEOMGYEONG,
        };

        enum {
            NEW_AFFECT_MOV_SPEED = 200,
            NEW_AFFECT_ATT_SPEED,
            NEW_AFFECT_ATT_GRADE,
            NEW_AFFECT_INVISIBILITY,
            NEW_AFFECT_STR,
            NEW_AFFECT_DEX, // 205
            NEW_AFFECT_CON,
            NEW_AFFECT_INT,
            NEW_AFFECT_FISH_MIND_PILL,

            NEW_AFFECT_POISON,
            NEW_AFFECT_STUN, // 210
            NEW_AFFECT_SLOW,
            NEW_AFFECT_DUNGEON_READY,
            NEW_AFFECT_DUNGEON_UNIQUE,

            NEW_AFFECT_BUILDING,
            NEW_AFFECT_REVIVE_INVISIBLE, // 215
            NEW_AFFECT_FIRE,
            NEW_AFFECT_CAST_SPEED,
            NEW_AFFECT_HP_RECOVER_CONTINUE,
            NEW_AFFECT_SP_RECOVER_CONTINUE,

            NEW_AFFECT_POLYMORPH, // 220
            NEW_AFFECT_MOUNT,

            NEW_AFFECT_WAR_FLAG, // 222

            NEW_AFFECT_BLOCK_CHAT, // 223
            NEW_AFFECT_CHINA_FIREWORK,

            NEW_AFFECT_BOW_DISTANCE, // 225

            NEW_AFFECT_EXP_BONUS = 500,     // ring of experience
            NEW_AFFECT_ITEM_BONUS = 501,    // thief's gloves
            NEW_AFFECT_SAFEBOX = 502,       // PREMIUM_SAFEBOX,
            NEW_AFFECT_AUTOLOOT = 503,      // PREMIUM_AUTOLOOT,
            NEW_AFFECT_FISH_MIND = 504,     // PREMIUM_FISH_MIND,
            NEW_AFFECT_MARRIAGE_FAST = 505, // mandarin duck feather (harmony),
            NEW_AFFECT_GOLD_BONUS = 506,

            NEW_AFFECT_MALL = 510,                // mall item effect
            NEW_AFFECT_NO_DEATH_PENALTY = 511,    // dragon god's protection (prevents one experience penalty)
            NEW_AFFECT_SKILL_BOOK_BONUS = 512,    // sage's lesson (book training success +50%)
            NEW_AFFECT_SKILL_BOOK_NO_DELAY = 513, // Juan's book (no book training delay)

            NEW_AFFECT_EXP_BONUS_EURO_FREE = 516, // ring of experience (European version, default effect up to level 14)
            NEW_AFFECT_EXP_BONUS_EURO_FREE_UNDER_15 = 517,

            NEW_AFFECT_AUTO_HP_RECOVERY = 534, // auto potion HP
            NEW_AFFECT_AUTO_SP_RECOVERY = 535, // auto potion SP

            NEW_AFFECT_DRAGON_SOUL_QUALIFIED = 540,
            NEW_AFFECT_DRAGON_SOUL_DECK1 = 541,
            NEW_AFFECT_DRAGON_SOUL_DECK2 = 542,

            NEW_AFFECT_RAMADAN_ABILITY = 300,
            NEW_AFFECT_RAMADAN_RING = 301, // whether the crescent ring of the Ramadan event is worn

            NEW_AFFECT_NOG_POCKET_ABILITY = 302,

            NEW_AFFECT_QUEST_START_IDX = 1000,
        };

        enum {
            STONE_SMOKE1 = 0, // 99%
            STONE_SMOKE2 = 1, // 85%
            STONE_SMOKE3 = 2, // 80%
            STONE_SMOKE4 = 3, // 60%
            STONE_SMOKE5 = 4, // 45%
            STONE_SMOKE6 = 5, // 40%
            STONE_SMOKE7 = 6, // 20%
            STONE_SMOKE8 = 7, // 10%
            STONE_SMOKE_NUM = 4,
        };

        enum building_affect {
            BUILDING_CONSTRUCTION_SMALL = 0,
            BUILDING_CONSTRUCTION_LARGE = 1,
            BUILDING_UPGRADE = 2,
        };

        enum {
            WEAPON_DUALHAND,
            WEAPON_ONEHAND,
            WEAPON_TWOHAND,
            WEAPON_NUM,
        };

        enum {
            EMPIRE_NONE,
            EMPIRE_A,
            EMPIRE_B,
            EMPIRE_C,
            EMPIRE_NUM,
        };

        enum {
            NAMECOLOR_MOB,
            NAMECOLOR_NPC,
            NAMECOLOR_PC,
            NAMECOLOR_PC_END = NAMECOLOR_PC + EMPIRE_NUM,
            NAMECOLOR_NORMAL_MOB,
            NAMECOLOR_NORMAL_NPC,
            NAMECOLOR_NORMAL_PC,
            NAMECOLOR_NORMAL_PC_END = NAMECOLOR_NORMAL_PC + EMPIRE_NUM,
            NAMECOLOR_EMPIRE_MOB,
            NAMECOLOR_EMPIRE_NPC,
            NAMECOLOR_EMPIRE_PC,
            NAMECOLOR_EMPIRE_PC_END = NAMECOLOR_EMPIRE_PC + EMPIRE_NUM,
            NAMECOLOR_FUNC,
            NAMECOLOR_PK,
            NAMECOLOR_PVP,
            NAMECOLOR_PARTY,
            NAMECOLOR_WARP,
            NAMECOLOR_WAYPOINT,
            NAMECOLOR_EXTRA = NAMECOLOR_FUNC + 10,
            NAMECOLOR_NUM = NAMECOLOR_EXTRA + 10,
        };

        enum {
            ALIGNMENT_TYPE_WHITE,
            ALIGNMENT_TYPE_NORMAL,
            ALIGNMENT_TYPE_DARK,
        };

        enum {
            EMOTICON_EXCLAMATION = 1,
            EMOTICON_FISH = 11,
            EMOTICON_NUM = 128,

            TITLE_NUM = 9,
            TITLE_NONE = 4,
        };

        enum // when these numbers change, registerEffect must change too.
        {
            EFFECT_REFINED_NONE,

            EFFECT_SWORD_REFINED7,
            EFFECT_SWORD_REFINED8,
            EFFECT_SWORD_REFINED9,

            EFFECT_BOW_REFINED7,
            EFFECT_BOW_REFINED8,
            EFFECT_BOW_REFINED9,

            EFFECT_FANBELL_REFINED7,
            EFFECT_FANBELL_REFINED8,
            EFFECT_FANBELL_REFINED9,

            EFFECT_SMALLSWORD_REFINED7,
            EFFECT_SMALLSWORD_REFINED8,
            EFFECT_SMALLSWORD_REFINED9,

            EFFECT_SMALLSWORD_REFINED7_LEFT,
            EFFECT_SMALLSWORD_REFINED8_LEFT,
            EFFECT_SMALLSWORD_REFINED9_LEFT,

            EFFECT_BODYARMOR_REFINED7,
            EFFECT_BODYARMOR_REFINED8,
            EFFECT_BODYARMOR_REFINED9,

            EFFECT_BODYARMOR_SPECIAL,  // armour 4-2-1
            EFFECT_BODYARMOR_SPECIAL2, // armour 4-2-2

            EFFECT_REFINED_NUM,
        };

        enum damage_flag {
            DAMAGE_NORMAL = (1 << 0),
            DAMAGE_POISON = (1 << 1),
            DAMAGE_DODGE = (1 << 2),
            DAMAGE_BLOCK = (1 << 3),
            DAMAGE_PENETRATE = (1 << 4),
            DAMAGE_CRITICAL = (1 << 5),
        };

        enum {
            EFFECT_DUST,
            EFFECT_STUN,
            EFFECT_HIT,
            EFFECT_FLAME_ATTACK,
            EFFECT_FLAME_HIT,
            EFFECT_FLAME_ATTACH,
            EFFECT_ELECTRIC_ATTACK,
            EFFECT_ELECTRIC_HIT,
            EFFECT_ELECTRIC_ATTACH,
            EFFECT_SPAWN_APPEAR,
            EFFECT_SPAWN_DISAPPEAR,
            EFFECT_LEVELUP,
            EFFECT_SKILLUP,
            EFFECT_HPUP_RED,
            EFFECT_SPUP_BLUE,
            EFFECT_SPEEDUP_GREEN,
            EFFECT_DXUP_PURPLE,
            EFFECT_CRITICAL,
            EFFECT_PENETRATE,
            EFFECT_BLOCK,
            EFFECT_DODGE,
            EFFECT_FIRECRACKER,
            EFFECT_SPIN_TOP,
            EFFECT_WEAPON,
            EFFECT_WEAPON_END = EFFECT_WEAPON + WEAPON_NUM,
            EFFECT_AFFECT,
            EFFECT_AFFECT_GYEONGGONG = EFFECT_AFFECT + AFFECT_GYEONGGONG,
            EFFECT_AFFECT_KWAESOK = EFFECT_AFFECT + AFFECT_KWAESOK,
            EFFECT_AFFECT_END = EFFECT_AFFECT + AFFECT_NUM,
            EFFECT_EMOTICON,
            EFFECT_EMOTICON_END = EFFECT_EMOTICON + EMOTICON_NUM,
            EFFECT_SELECT,
            EFFECT_TARGET,
            EFFECT_EMPIRE,
            EFFECT_EMPIRE_END = EFFECT_EMPIRE + EMPIRE_NUM,
            EFFECT_HORSE_DUST,
            EFFECT_REFINED,
            EFFECT_REFINED_END = EFFECT_REFINED + EFFECT_REFINED_NUM,
            EFFECT_DAMAGE_TARGET,
            EFFECT_DAMAGE_NOT_TARGET,
            EFFECT_DAMAGE_SELFDAMAGE,
            EFFECT_DAMAGE_SELFDAMAGE2,
            EFFECT_DAMAGE_POISON,
            EFFECT_DAMAGE_MISS,
            EFFECT_DAMAGE_TARGETMISS,
            EFFECT_DAMAGE_CRITICAL,
            EFFECT_SUCCESS,
            EFFECT_FAIL,
            EFFECT_FR_SUCCESS,
            EFFECT_LEVELUP_ON_14_FOR_GERMANY,    // level up at 14 (Germany only)
            EFFECT_LEVELUP_UNDER_15_FOR_GERMANY, // level up at 15 (Germany only)
            EFFECT_PERCENT_DAMAGE1,
            EFFECT_PERCENT_DAMAGE2,
            EFFECT_PERCENT_DAMAGE3,
            EFFECT_AUTO_HPUP,
            EFFECT_AUTO_SPUP,
            EFFECT_RAMADAN_RING_EQUIP,    // effect when the crescent ring is put on
            EFFECT_HALLOWEEN_CANDY_EQUIP, // effect when the Halloween candy is put on
            EFFECT_HAPPINESS_RING_EQUIP,  // effect when the ring of happiness is put on
            EFFECT_LOVE_PENDANT_EQUIP,    // effect when the love pendant is put on
            EFFECT_TEMP,
            EFFECT_NUM,
        };

        enum {
            DUEL_NONE,
            DUEL_CANNOTATTACK,
            DUEL_START,
        };

      public:
        static void destroy_system();
        static void create_system(uint32_t capacity);
        static auto register_effect(uint32_t eft_type, const char *eft_attach_bone, const char *eft_name, bool is_cache) -> bool;
        static void register_title_name(int index, const char *title_name);
        static auto register_name_color(uint32_t index, uint32_t r, uint32_t g, uint32_t b) -> bool;
        static auto register_title_color(uint32_t index, uint32_t r, uint32_t g, uint32_t b) -> bool;

        static void set_dust_gap(float dust_gap);
        static void set_horse_dust_gap(float dust_gap);

        static void set_empire_name_mode(bool is_enable);
        static auto indexed_name_color(uint32_t name_color) -> const math::color &;

      public:
        void set_main_instance();

        void on_selected();
        void on_unselected();
        void on_targeted();
        void on_untargeted();

      protected:
        auto is_exist_main_instance() -> bool;
        auto is_main_instance() -> bool;
        auto main_can_see_hidden_thing() -> bool;
        auto bow_range() -> float;

      protected:
        auto attach_effect(uint32_t eft_type) -> uint32_t;
        auto attach_effect(char filename[128]) -> uint32_t;
        void detach_effect(uint32_t eid);

      public:
        void create_special_effect(uint32_t effect_index);
        void attach_special_effect(uint32_t effect);

      protected:
        static std::string affect_effect_attach_bone_[EFFECT_NUM];
        static uint32_t affect_effect_crcs_[EFFECT_NUM];
        static float dust_gap_;
        static float horse_dust_gap_;

      public:
        instance_base();
        virtual ~instance_base();

        auto less_render_order(instance_base *inst) -> bool;

        void mount_horse(uint32_t race);
        void dismount_horse();

        // TODO: script test function, remove.
        void script_set_affect(uint32_t affect, bool is_visible);

        auto calculate_distance_sq3d(const pixel_position &p_pos_dst) -> float;

        auto is_fly_target_object() -> bool;
        void clear_fly_target_instance();
        void set_fly_target_instance(instance_base &inst_dst);
        void add_fly_target_instance(instance_base &inst_dst);
        void add_fly_target_position(const pixel_position &p_pos_dst);

        auto fly_target_distance() -> float;

        void set_alpha(float alpha);

        void delete_blend_out();

        void attach_text_tail();
        void detach_text_tail();
        void update_text_tail_level(uint32_t level);

        void refresh_text_tail();
        void refresh_text_tail_title();

        auto create(const create_data &create_data_value) -> bool;

        auto create_device_objects() -> bool;
        void destroy_device_objects();

        void destroy();

        void update();
        auto update_deleting() -> bool;

        void transform();
        void deform();
        void render();
        void render_trace();
        void render_to_shadow_map();
        void render_collision();
        void register_bounding_sphere();

        void get_bound_box(math::vector3 *vt_min, math::vector3 *vt_max);

        void set_name_string(const char *name, int len);
        auto set_race(uint32_t race_index) -> bool;
        void set_virtual_id(uint32_t virtual_number);
        void set_virtual_number(uint32_t virtual_number);
        void set_instance_type(int instance_type);
        void set_alignment(short alignment);
        void set_pk_mode(uint8_t pk_mode);
        void set_killer(bool flag);
        void set_party_member_flag(bool flag);
        void set_state_flags(uint32_t state_flags);

        void set_armor(uint32_t armor);
        void set_shape(uint32_t shape, float specular = 0.0f);
        void set_hair(uint32_t hair);
        auto set_weapon(uint32_t weapon) -> bool;
        auto change_armor(uint32_t armor) -> bool;
        void change_weapon(uint32_t weapon);
        void change_hair(uint32_t hair);
        void change_guild(uint32_t guild_id);
        auto weapon_type() -> uint32_t;

        void set_combo_type(uint32_t combo_type);
        void set_attack_speed(uint32_t atk_spd);
        void set_move_speed(uint32_t mov_spd);
        void set_rotation_speed(float rot_spd);

        auto name_string() -> const char *;
        auto instance_type() -> int;
        auto get_part(race_data::parts part) -> uint32_t;
        auto get_shape() -> uint32_t;
        auto race() -> uint32_t;
        auto virtual_id() -> uint32_t;
        auto virtual_number() -> uint32_t;
        auto empire_id() -> uint32_t;
        auto guild_id() -> uint32_t;
        auto alignment() -> int;
        auto alignment_grade() -> uint32_t;
        auto alignment_type() -> int;
        auto pk_mode() -> uint8_t;
        auto is_killer() -> bool;
        auto is_party_member() -> bool;

        void act_dual_emotion(instance_base &dst_inst, uint16_t motion_number1, uint16_t motion_number2);
        void act_emotion(uint32_t motion_number);
        void level_up();
        void skill_up();
        void revive();
        void stun();
        void die();
        void hide();
        void show();

        auto can_act() -> bool;
        auto can_move() -> bool;
        auto can_attack() -> bool;
        auto can_use_skill() -> bool;
        auto can_fishing() -> bool;
        auto is_conflict_alignment_instance(instance_base &inst_victim) -> bool;
        auto is_attackable_instance(instance_base &inst_victim) -> bool;
        auto is_targetable_instance(instance_base &inst_victim) -> bool;
        auto is_pvp_instance(instance_base &inst_victim) -> bool;
        auto can_change_target() -> bool;
        auto can_pick_instance() -> bool;
        auto can_view_target_hp(instance_base &inst_victim) -> bool;

        auto new_goto(const pixel_position &p_pos_dst, float dst_rot) -> bool;
        void end_going();

        void set_run_mode();
        void set_walk_mode();

        auto is_affect(uint32_t affect) -> bool;
        auto is_invisibility() -> bool;
        auto is_paralysis() -> bool;
        auto is_game_master() -> bool;
        auto is_same_empire(instance_base &inst_dst) -> bool;
        auto is_bow_mode() -> bool;
        auto is_hand_mode() -> bool;
        auto is_fishing_mode() -> bool;
        auto is_fishing() -> bool;

        auto is_wearing_dress() -> bool;
        auto is_holding_pick_axe() -> bool;
        auto is_mounting_horse() -> bool;
        auto is_new_mount() -> bool;
        auto is_force_visible() -> bool;
        auto is_in_safe() -> bool;
        auto is_enemy() -> bool;
        auto is_stone() -> bool;
        auto is_resource() -> bool;
        auto is_npc() -> bool;
        auto is_pc() -> bool;
        auto is_poly() -> bool;
        auto is_warp() -> bool;
        auto is_goto() -> bool;
        auto is_object() -> bool;
        auto is_door() -> bool;
        auto is_building() -> bool;
        auto is_wooden_door() -> bool;
        auto is_stone_door() -> bool;
        auto is_flag() -> bool;
        auto is_guild_wall() -> bool;

        auto is_dead() -> bool;
        auto is_stun() -> bool;
        auto is_sleep() -> bool;
        auto is_syncing() -> bool;
        auto is_waiting() -> bool;
        auto is_walking() -> bool;
        auto is_pushing() -> bool;
        auto is_attacking() -> bool;
        auto is_acting_emotion() -> bool;
        auto is_attacked() -> bool;
        auto is_knock_down() -> bool;
        auto is_using_skill() -> bool;
        auto is_using_moving_skill() -> bool;
        auto can_cancel_skill() -> bool;
        auto can_attack_horse_level() -> bool;

        auto new_can_move_to_dest_pixel_position(const pixel_position &p_pos_dst) -> bool;

        void new_set_advancing_rotation_from_pixel_position(const pixel_position &p_pos_src, const pixel_position &p_pos_dst);
        void new_set_advancing_rotation_from_dir_pixel_position(const pixel_position &p_pos_dir);
        void set_advancing_rotation(float rotation);

        void end_walking(float blending_time = 0.15f);
        void end_walking_without_blending();

        void set_event_handler(actor_instance::event_handler *event_handler);

        void push_tcp_state(uint32_t cmd_time, const pixel_position &p_pos_dst, float dst_rot, uint32_t func, uint32_t arg);
        void push_tcp_state_expanded(uint32_t cmd_time, const pixel_position &p_pos_dst, float dst_rot, uint32_t func, uint32_t arg, uint32_t target_vid);

        void new_stop();

        auto new_use_skill(uint32_t skill, uint32_t mot, uint32_t mot_loop_count, bool is_moving_skill) -> bool;
        void new_attack();
        void new_attack(float dir_rot);
        void new_attack_to_dest_pixel_position_direction(const pixel_position &p_pos_dst);
        auto new_attack_to_dest_instance_direction(instance_base &inst_dst, fly_event_handler *fly_handler) -> bool;
        auto new_attack_to_dest_instance_direction(instance_base &inst_dst) -> bool;

        auto new_move_to_dest_pixel_position_direction(const pixel_position &p_pos_dst) -> bool;
        void new_move_to_dest_instance_direction(instance_base &inst_dst);
        void new_move_to_direction(float dir_rot);

        auto new_get_distance_from_dir_pixel_position(const pixel_position &p_pos_dir) -> float;
        auto new_get_distance_from_dest_pixel_position(const pixel_position &p_pos_dst) -> float;
        auto new_get_distance_from_dest_instance(instance_base &inst_dst) -> float;

        auto new_get_rotation() -> float;
        auto new_get_rotation_from_dest_pixel_position(const pixel_position &p_pos_dst) -> float;
        auto new_get_rotation_from_dir_pixel_position(const pixel_position &p_pos_dir) -> float;
        auto new_get_rotation_from_dest_instance(instance_base &inst_dst) -> float;

        auto new_get_advancing_rotation_from_dir_pixel_position(const pixel_position &p_pos_dir) -> float;
        auto new_get_advancing_rotation_from_pixel_position(const pixel_position &p_pos_src, const pixel_position &p_pos_dst) -> float;

        auto new_is_clickable_distance_dest_pixel_position(const pixel_position &p_pos_dst) -> bool;
        auto new_is_clickable_distance_dest_instance(instance_base &inst_dst) -> bool;

        auto new_get_front_instance(instance_base **ppout_target_instance, float distance) -> bool;
        void new_get_random_position_in_fan_range(instance_base &inst_target, pixel_position *p_pos_dst);
        auto new_get_instance_vector_in_fan_range(float skill_distance, instance_base &inst_target, std::vector<instance_base *> *vct_inst) -> bool;
        auto new_get_instance_vector_in_circle_range(float skill_distance, std::vector<instance_base *> *vct_inst) -> bool;

        void new_set_owner(uint32_t owner_vid);
        void new_sync_pixel_position(int32_t &p_pos_x, int32_t &p_pos_y);

        void new_set_pixel_position(const pixel_position &p_pos_dst);

        auto new_get_last_pixel_position_ref() -> const pixel_position &;

        auto is_normal_attacking() -> bool;
        auto is_combo_attacking() -> bool;
        auto normal_attack_index() -> MOTION_KEY;
        auto combo_index() -> uint32_t;
        auto attacking_elapsed_time() -> float;
        void input_normal_attack(float atk_dir_rot);
        void input_combo_attack(float atk_dir_rot);

        void run_normal_attack(float atk_dir_rot);
        void run_combo_attack(float atk_dir_rot, uint32_t motion_index);

        auto check_advancing() -> bool;

        auto avoid_object(const graphic_object_instance &bg_obj) -> bool;
        auto is_block_object(const graphic_object_instance &bg_obj) -> bool;
        void block_movement();

      public:
        auto check_attacking(instance_base &inst_victim) -> bool;
        void process_hitting(uint32_t motion_key, instance_base *victim_instance);
        void process_hitting(uint32_t motion_key, uint8_t event_index, instance_base *victim_instance);
        void blending_position(pixel_position *pixel_position);
        void set_blending_position(const pixel_position &pixel_position);

        void start_fishing(float frot);
        void stop_fishing();
        void react_fishing();
        void catch_success();
        void catch_fail();
        auto fishing_rot(int *pirot) -> bool;

        void restore_render_mode();
        void set_add_render_mode();
        void set_modulate_render_mode();
        void set_render_mode(int render_mode);
        void set_add_color(const math::color &color);

        void script_set_pixel_position(float fx, float fy);
        void new_get_pixel_position(pixel_position *pixel_position);

        void new_look_at_fly_target();
        void new_look_at_dest_instance(instance_base &inst_dst);
        void new_look_at_dest_pixel_position(const pixel_position &p_pos_dst);

        auto rotation() -> float;
        auto advancing_rotation() -> float;
        void set_rotation(float rotation);
        void blend_rotation(float rotation, float blend_time = 0.1f);

        void set_direction(int dir);
        auto degree_from_direction(int dir) -> float;

        auto is_lock() -> bool;

        void set_motion_mode(int motion_mode);
        auto motion_mode(uint32_t motion_index) -> int;

        void reset_local_time();
        void set_loop_motion(uint16_t motion, float blend_time = 0.1f, float speed_ratio = 1.0f);
        void push_once_motion(uint16_t motion, float blend_time = 0.1f, float speed_ratio = 1.0f);
        void push_loop_motion(uint16_t motion, float blend_time = 0.1f, float speed_ratio = 1.0f);
        void set_end_stop_motion();

        auto intersect_defending_sphere() -> bool;
        auto intersect_bounding_box() -> bool;

        void refresh(uint32_t mot_index, bool is_loop);

        auto distance(instance_base *target_inst) -> float;
        auto distance(const pixel_position &pixel_position) -> float;

        auto graphic_thing_instance_ref() -> actor_instance &;
        auto graphic_thing_instance_ptr() -> actor_instance *;

        auto background_is_water_pixel_position(const pixel_position &p_pos) -> bool;
        auto background_get_water_height(const pixel_position &p_pos, float *height) -> bool;

        void clear_affect_flag_container();
        void clear_affects_impl();

        void set_affect_impl(uint32_t affect, bool is_visible);

        void set_affect_flag_container(const affect_flag_container &affect_flag_container);

        void set_normal_affect_flag_container(const affect_flag_container &affect_flag_container);
        void set_stone_smoke_flag_container(const affect_flag_container &affect_flag_container);

        void set_emoticon(uint32_t emoticon);
        void set_fish_emoticon();
        auto is_possible_emoticon() -> bool;

      protected:
        auto less_render_order_get_lod_level() -> uint32_t;
        void initialize_impl();
        void initialize_rotation_speed();

        void create_set_name(const create_data &create_data_value);
        void create_set_warp_name(const create_data &create_data_value);

        auto main_instance_ptr_impl() -> instance_base *;

        auto find_race_type(uint32_t race, uint8_t *type_value) -> bool;

        auto is_shape_animal_wear() -> bool;
        auto is_changable_weapon(int weapon_id) -> bool;

        void enable_skip_collision_impl();
        void disable_skip_collision_impl();

        void clear_main_instance_impl();

        void warrior_set_geomgyeong_affect(bool is_visible);
        void assassin_set_eunhyeong_affect(bool is_visible);

        auto can_process_network_state_packet() -> bool;

        auto is_in_dust_range() -> bool;

        void process_function_emotion(uint32_t motion_number, uint32_t target_vid, const pixel_position &pos_dst);
        void enable_changing_tcp_state();
        void disable_changing_tcp_state();
        auto is_enable_tcp_process(uint32_t cur_func) -> bool;

        auto can_render() -> bool;
        auto is_in_view_frustum() -> bool;

        void attach_horse_saddle();
        void detach_horse_saddle();

        struct shorse {
            bool mounting;
            std::unique_ptr<actor_instance> actor;

            shorse();
            ~shorse();

            void destroy();
            void create(const pixel_position &p_pos, uint32_t race, uint32_t hit_effect);

            void set_attack_speed(uint32_t atk_spd);
            void set_move_speed(uint32_t mov_spd);
            void deform();
            void render();
            auto actor_ref() -> actor_instance &;
            auto actor_ptr() -> actor_instance *;

            auto is_mounting() -> bool;
            auto can_attack() -> bool;
            auto can_use_skill() -> bool;

            auto get_level() -> uint32_t;
            auto is_new_mount() -> bool;

            void initialize_impl();
        } horse_;

      protected:
        void set_blend_rendering_mode();
        void set_alpha_value_impl(float alpha);
        auto alpha_value_impl() -> float;

        void combo_process_impl();
        void movement_process();
        void state_process();
        void attack_process();

        void start_walking();
        auto local_time() -> float;

        void refresh_state(uint32_t mot_index, bool is_loop);
        void refresh_actor_instance();

      protected:
        void on_syncing();
        void on_waiting();
        void on_moving();

        void new_set_src_pixel_position(const pixel_position &p_pos_dst);
        void new_set_dst_pixel_position(const pixel_position &p_pos_dst);
        void new_set_dst_pixel_position_z(float z);

        auto new_get_cur_pixel_position_ref() -> const pixel_position &;
        auto new_get_src_pixel_position_ref() -> const pixel_position &;

      public:
        auto new_get_dst_pixel_position_ref() -> const pixel_position &;

      protected:
        bool is_text_tail_;

        std::string name_value_;

        uint32_t part_value_[race_data::PART_MAX_NUM];

        uint32_t level_value_;
        uint32_t empire_id_;
        uint32_t guild_id_;

      protected:
        affect_flag_container affect_flag_container_value_;
        uint32_t attached_affect_effects_[AFFECT_NUM];

        auto refined_effect(item_data *item) -> uint32_t;
        void clear_weapon_refine_effect();
        void clear_armor_refine_effect();

      protected:
        void attach_select_effect();
        void detach_select_effect();

        void attach_target_effect();
        void detach_target_effect();

        void attach_empire_effect(uint32_t empire);

      protected:
        struct effect_container {
            using dict = std::map<uint32_t, uint32_t>;
            dict dct_eft_id;
        } effect_container_value_;

        void effect_container_initialize();
        void effect_container_destroy();

        auto effect_container_attach_effect(uint32_t effect) -> uint32_t;
        void effect_container_detach_effect(uint32_t effect);

        auto effect_container_get_dict() -> effect_container::dict &;

      protected:
        struct stone_smoke {
            uint32_t eft_id;
        } stone_smoke_value_;

        void stone_smoke_inialize();
        void stone_smoke_destroy();
        void stone_smoke_create(uint32_t smoke);

      protected:
        uint8_t type_value_;
        uint8_t race_type_;
        uint32_t shape_value_;
        uint32_t race_;
        uint32_t virtual_number_;
        short alignment_;
        uint8_t pk_mode_;
        bool is_killer_;
        bool is_party_member_;

        int rotating_direction_;

        uint32_t adv_actor_vid_;
        uint32_t last_dmg_actor_vid_;

        int32_t average_network_gap_;
        uint32_t next_update_height_time_;

        bool is_going_;

        pixel_position p_pos_dust_;

        uint32_t last_combo_index_;

        uint32_t sword_refine_effect_right_;
        uint32_t sword_refine_effect_left_;
        uint32_t armor_refine_effect_;

        struct move_after_func {
            uint32_t func;
            uint32_t arg_value;

            // Used by emotions.
            uint32_t arg_expanded;
            pixel_position pos_dst;
        };

        move_after_func mov_after_func_;

        float dst_rot_;
        float atk_pos_time_;
        float rot_spd_;
        float max_rot_spd_;

        bool enable_tcp_state_;

        actor_instance graphic_thing_instance_value_;

      protected:
        struct command {
            uint32_t chk_time;
            uint32_t cmd_time;
            float dst_rot;
            uint32_t func;
            uint32_t arg_value;
            uint32_t target_vid;
            pixel_position p_pos_dst;
        };

        using command_queue = std::list<command>;

        uint32_t base_chk_time_;
        uint32_t base_cmd_time_;

        uint32_t skip_time_;

        command_queue que_cmd_new_;

        bool damage_effect_type_;

        struct effect_damage {
            uint32_t damage;
            uint8_t flag;
            bool self;
        };

        using command_damage_queue = std::list<effect_damage>;
        command_damage_queue damage_queue_;
        // Damage numbers leave the queue at most one per 1/60 s, as they did at 60 fps.
        float damage_pop_time_ = 0.0f;

        void process_damage();

      public:
        void add_damage_effect(uint32_t damage, uint8_t flag, bool self);

      protected:
        struct warrior {
            uint32_t geomgyeong_effect;
        };

        warrior warrior_value_;

        void warrior_initialize();

      public:
        static void clear_pvp_key_system();

        static void insert_pvp_key(uint32_t src_vid, uint32_t dst_vid);
        static void remove_pvp_key(uint32_t src_vid, uint32_t dst_vid);

        static void insert_gvg_key(uint32_t src_guild_vid, uint32_t dst_guild_vid);
        static void remove_gvg_key(uint32_t src_guild_vid, uint32_t dst_guild_vid);

        static void insert_duel_key(uint32_t src_vid, uint32_t dst_vid);

        auto name_color_index() -> uint32_t;

        auto name_color() -> const math::color &;
        auto title_color() -> const math::color &;

      protected:
        static auto pvp_key(uint32_t src_vid, uint32_t dst_vid) -> uint32_t;
        static auto find_pvp_key(uint32_t src_vid, uint32_t dst_vid) -> bool;
        static auto find_gvg_key(uint32_t src_guild_id, uint32_t dst_guild_id) -> bool;
        static auto find_duel_key(uint32_t src_guild_id, uint32_t dst_guild_id) -> bool;

      protected:
        auto event_handler_ref() -> actor_instance::event_handler &;

      protected:
        static auto background_height(float x, float y) -> float;
        static auto shadow_map_color_impl(float x, float y) -> uint32_t;

      public:
        static void reset_performance_counter();
        static void get_info(std::string *info);

      public:
        static auto new_value() -> instance_base *;
        static void delete_value(instance_base *inst);

        static dynamic_pool<instance_base> pool_value;

      protected:
        static uint32_t update_counter_;
        static uint32_t render_counter_;
        static uint32_t deform_counter_;

      public:
        auto duel_mode() -> uint32_t;
        void set_duel_mode(uint32_t type_value);

      protected:
        uint32_t duel_mode_;
        uint32_t emoticon_time_;

      public:
        auto get_level() const -> uint32_t { return level_value_; }
        void set_level(uint32_t lvl);
    };

    inline auto race_to_job(int race) -> int {
        const int job_num = 4;
        return race % job_num;
    }

    inline auto race_to_sex(int race) -> int {
        switch (race) {
        case 0:
        case 2:
        case 5:
        case 7:
            return 1;

        case 1:
        case 3:
        case 4:
        case 6:
            return 0;
        }

        return 0;
    }
}
