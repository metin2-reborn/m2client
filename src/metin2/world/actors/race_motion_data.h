#pragma once

#include <metin2/sound/type.h>
#include <metin2/world/maps/map_type.h>
#include "race_motion_data_event.h"

namespace metin2::inline world::inline actors {

    class race_motion_data {
      public:
        enum type {
            TYPE_NONE,
            TYPE_WAIT,
            TYPE_MOVE,
            TYPE_ATTACK,
            TYPE_COMBO,
            TYPE_DAMAGE,
            TYPE_KNOCKDOWN,
            TYPE_DIE,
            TYPE_SKILL,
            TYPE_STANDUP,
            TYPE_EVENT,
            TYPE_FISHING,
            TYPE_NUM,
        };

        enum {
            SKILL_NUM = 255,
        };

        enum mode {
            MODE_RESERVED,
            MODE_GENERAL,

            MODE_ONEHAND_SWORD,
            MODE_TWOHAND_SWORD,
            MODE_DUALHAND_SWORD,
            MODE_BOW,
            MODE_FAN,
            MODE_BELL,
            MODE_FISHING,

            MODE_HORSE,
            MODE_HORSE_ONEHAND_SWORD,
            MODE_HORSE_TWOHAND_SWORD,
            MODE_HORSE_DUALHAND_SWORD,
            MODE_HORSE_BOW,
            MODE_HORSE_FAN,
            MODE_HORSE_BELL,

            MODE_WEDDING_DRESS,

            MODE_MAX_NUM,
        };

        enum name {
            NAME_NONE,               // 0 none
            NAME_WAIT,               // 1 wait          (00.msa)
            NAME_WALK,               // 2 walk          (02.msa)
            NAME_RUN,                // 3 run           (03.msa)
            NAME_CHANGE_WEAPON,      // 4 change weapon
            NAME_DAMAGE,             // 5 hit from the front      (30.msa)
            NAME_DAMAGE_FLYING,      // 6 knocked back from the front (32.msa)
            NAME_STAND_UP,           // 7 stand up, front   (33.msa)
            NAME_DAMAGE_BACK,        // 8 hit from behind      (34.msa)
            NAME_DAMAGE_FLYING_BACK, // 9 knocked back from behind  (35.msa)
            NAME_STAND_UP_BACK,      // 10 stand up, back (36.msa)
            NAME_DEAD,               // 11 die         (31.msa)
            NAME_DEAD_BACK,          // 12 die backwards     (37.msa)
            NAME_NORMAL_ATTACK,      // 13 attack         (20.msa)
            NAME_COMBO_ATTACK_1,     // 14 combo attack
            NAME_COMBO_ATTACK_2,     // 15 combo attack
            NAME_COMBO_ATTACK_3,     // 16 combo attack
            NAME_COMBO_ATTACK_4,     // 17 combo attack
            NAME_COMBO_ATTACK_5,     // 18 combo attack
            NAME_COMBO_ATTACK_6,     // 19 combo attack
            NAME_COMBO_ATTACK_7,     // 20 combo attack
            NAME_COMBO_ATTACK_8,     // 21 combo attack
            NAME_INTRO_WAIT,         // 22 selection screen wait
            NAME_INTRO_SELECTED,     // 23 selection screen selected
            NAME_INTRO_NOT_SELECTED, // 24 selection screen not selected
            NAME_SPAWN,              // 25 spawn
            NAME_FISHING_THROW,      // 26 fishing: throw
            NAME_FISHING_WAIT,       // 27 fishing: wait
            NAME_FISHING_STOP,       // 28 fishing: stop
            NAME_FISHING_REACT,      // 29 fishing: bite
            NAME_FISHING_CATCH,      // 30 fishing: catch
            NAME_FISHING_FAIL,       // 31 fishing: fail
            NAME_STOP,               // 32 horse stop
            NAME_SPECIAL_1,          // 33 monster/NPC special motion (24.msa)
            NAME_SPECIAL_2,          // 34 monster/NPC special motion (25.msa)
            NAME_SPECIAL_3,
            NAME_SPECIAL_4,
            NAME_SPECIAL_5,
            NAME_SPECIAL_6,
            NAME_SKILL = 50,
            NAME_SKILL_END = NAME_SKILL + SKILL_NUM,

            NAME_CLAP,

            NAME_CHEERS_1,
            NAME_CHEERS_2,

            NAME_KISS_START,
            NAME_KISS_WITH_WARRIOR = NAME_KISS_START + 0,
            NAME_KISS_WITH_ASSASSIN = NAME_KISS_START + 1,
            NAME_KISS_WITH_SURA = NAME_KISS_START + 2,
            NAME_KISS_WITH_SHAMAN = NAME_KISS_START + 3,

            NAME_FRENCH_KISS_START,
            NAME_FRENCH_KISS_WITH_WARRIOR = NAME_FRENCH_KISS_START + 0,
            NAME_FRENCH_KISS_WITH_ASSASSIN = NAME_FRENCH_KISS_START + 1,
            NAME_FRENCH_KISS_WITH_SURA = NAME_FRENCH_KISS_START + 2,
            NAME_FRENCH_KISS_WITH_SHAMAN = NAME_FRENCH_KISS_START + 3,

            NAME_SLAP_HIT_START,
            NAME_SLAP_HIT_WITH_WARRIOR = NAME_SLAP_HIT_START + 0,
            NAME_SLAP_HIT_WITH_ASSASSIN = NAME_SLAP_HIT_START + 1,
            NAME_SLAP_HIT_WITH_SURA = NAME_SLAP_HIT_START + 2,
            NAME_SLAP_HIT_WITH_SHAMAN = NAME_SLAP_HIT_START + 3,

            NAME_SLAP_HURT_START,
            NAME_SLAP_HURT_WITH_WARRIOR = NAME_SLAP_HURT_START + 0,
            NAME_SLAP_HURT_WITH_ASSASSIN = NAME_SLAP_HURT_START + 1,
            NAME_SLAP_HURT_WITH_SURA = NAME_SLAP_HURT_START + 2,
            NAME_SLAP_HURT_WITH_SHAMAN = NAME_SLAP_HURT_START + 3,

            NAME_DIG,

            NAME_DANCE_1,
            NAME_DANCE_2,
            NAME_DANCE_3,
            NAME_DANCE_4,
            NAME_DANCE_5,
            NAME_DANCE_6, // Gangnam Style

            NAME_DANCE_END = NAME_DANCE_1 + 16,

            NAME_CONGRATULATION,
            NAME_FORGIVE,
            NAME_ANGRY,
            NAME_ATTRACTIVE,
            NAME_SAD,
            NAME_SHY,
            NAME_CHEERUP,
            NAME_BANTER,
            NAME_JOY,

            NAME_MAX_NUM,
        };

        enum motion_event_type {
            MOTION_EVENT_TYPE_NONE,

            MOTION_EVENT_TYPE_EFFECT,
            MOTION_EVENT_TYPE_SCREEN_WAVING,
            MOTION_EVENT_TYPE_SCREEN_FLASHING,
            MOTION_EVENT_TYPE_SPECIAL_ATTACKING,
            MOTION_EVENT_TYPE_SOUND,
            MOTION_EVENT_TYPE_FLY,
            MOTION_EVENT_TYPE_CHARACTER_SHOW,
            MOTION_EVENT_TYPE_CHARACTER_HIDE,
            MOTION_EVENT_TYPE_WARP,
            MOTION_EVENT_TYPE_EFFECT_TO_TARGET,

            MOTION_EVENT_TYPE_MAX_NUM,
        };

        struct combo_input_data {
            float input_start_time;
            float next_combo_time;
            float input_end_time;
        };

        using motion_event_data = motion_events::motion_event_data;
        using screen_waving_event_data = motion_events::motion_event_data_screen_waving;
        using screen_flashing_event_data = motion_events::motion_event_data_screen_flashing;
        using motion_effect_event_data = motion_events::motion_event_data_effect;
        using motion_fly_event_data = motion_events::motion_event_data_fly;
        using motion_attacking_event_data = motion_events::motion_event_data_attack;
        using motion_sound_event_data = motion_events::motion_event_data_sound;
        using motion_character_show_event_data = motion_events::motion_event_data_character_show;
        using motion_character_hide_event_data = motion_events::motion_event_data_character_hide;
        using motion_warp_event_data = motion_events::motion_event_data_warp;
        using motion_effect_to_target_event_data = motion_events::motion_event_data_effect_to_target;
        using motion_event_data_vector = std::vector<motion_event_data *>;

      public:
        static auto new_value() -> race_motion_data *;
        static void delete_value(race_motion_data *data);

        static void create_system(uint32_t capacity);
        static void destroy_system();

      public:
        race_motion_data();
        virtual ~race_motion_data();

        void initialize();
        void destroy();

        void set_name(uint32_t name_value);

        auto get_type() const -> uint32_t;
        auto is_lock() const -> bool; // Locked motions are not cancelled by other motions.

        auto get_loop_count() const -> int;

        auto motion_file_name() const -> const char *;

        auto motion_duration() -> float;

        void set_accumulation_position(const pixel_position &pos);

        auto is_combo_input_time_data() const -> bool;

        auto combo_input_start_time() const -> float;
        auto next_combo_time() const -> float;
        auto combo_input_end_time() const -> float;

        auto is_attacking_motion() const -> bool;
        auto motion_attack_data_pointer() const -> const race_types::motion_attack_data *;
        auto has_splash_motion_event() const -> bool;

        auto is_cancel_enable_skill() const -> bool;

        auto is_loop_motion() const -> bool;
        auto loop_start_time() const -> float;
        auto loop_end_time() const -> float;

        auto motion_event_data_count() const -> uint32_t;
        auto motion_event_data_pointer(uint8_t index, const race_motion_data::motion_event_data **data) const -> bool;
        auto get_event_type(uint32_t index) const -> int;

        auto sound_instance_vector_pointer() const -> const sound_types::sound_instance_vector *;

        auto load_motion_data(const char *file_name) -> bool;
        auto load_sound_script_data(const char *file_name) -> bool;

      protected:
        void set_type(uint32_t type_value);

      protected:
        uint32_t type_value_;
        uint32_t name_value_;
        bool is_lock_;
        int loop_count_;

        std::string motion_file_name_;
        std::string sound_script_data_file_name_;
        float motion_duration_;

        bool is_accumulation_motion_;
        pixel_position accumulation_position_;

        bool is_combo_motion_;
        combo_input_data combo_input_data_value_;

        bool is_loop_motion_;
        float loop_start_time_;
        float loop_end_time_;

        bool is_attacking_motion_;
        race_types::motion_attack_data motion_attack_data_value_;

        bool cancel_enable_skill_;

        motion_event_data_vector motion_event_data_vector_value_;
        sound_types::sound_instance_vector sound_instance_vector_value_;

      private:
        bool has_splash_event_;

      protected:
        static dynamic_pool<race_motion_data> pool_value_;
    };
}
