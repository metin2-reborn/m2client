#pragma once

#include <metin2/client/app/game_type.h>
#include <metin2/world/items/item_data.h>

#include <cstdint>
#include <list>

namespace metin2::inline client::inline characters {
    // Holds the NPC and monster prototypes loaded from mob_proto.
    class python_non_player {
      public:
        enum click_event {
            ON_CLICK_EVENT_NONE = 0,
            ON_CLICK_EVENT_BATTLE = 1,
            ON_CLICK_EVENT_SHOP = 2,
            ON_CLICK_EVENT_TALK = 3,
            ON_CLICK_EVENT_VEHICLE = 4,

            ON_CLICK_EVENT_MAX_NUM,
        };

        enum mob_enchants {
            MOB_ENCHANT_CURSE,
            MOB_ENCHANT_SLOW,
            MOB_ENCHANT_POISON,
            MOB_ENCHANT_STUN,
            MOB_ENCHANT_CRITICAL,
            MOB_ENCHANT_PENETRATE,
            MOB_ENCHANTS_MAX_NUM
        };
        enum mob_resists {
            MOB_RESIST_SWORD,
            MOB_RESIST_TWOHAND,
            MOB_RESIST_DAGGER,
            MOB_RESIST_BELL,
            MOB_RESIST_FAN,
            MOB_RESIST_BOW,
            MOB_RESIST_FIRE,
            MOB_RESIST_ELECT,
            MOB_RESIST_MAGIC,
            MOB_RESIST_WIND,
            MOB_RESIST_POISON,
            MOB_RESISTS_MAX_NUM
        };

#define MOB_ATTRIBUTE_MAX_NUM 12
#define MOB_SKILL_MAX_NUM     5

#pragma pack(push)
#pragma pack(1)
        struct mob_skill_level {
            uint32_t vnum;
            uint8_t level_value;
        };

        struct mob_table {
            uint32_t vnum;
            char name_value[item_data::PROTO_NAME_MAX_LEN + 1]; // UTF-8 (mob_proto v2)
            char locale_name[item_data::PROTO_NAME_MAX_LEN + 1];

            uint8_t type_value;  // Monster, NPC
            uint8_t rank;        // PAWN, KNIGHT, KING
            uint8_t battle_type; // MELEE, etc..
            uint8_t level_value;
            uint8_t size;

            uint32_t gold_min;
            uint32_t gold_max;
            uint32_t exp;
            uint32_t max_hp;
            uint8_t regen_cycle;
            uint8_t regen_percent;
            uint16_t def;

            uint32_t ai_flag;
            uint32_t race_flag;
            uint32_t immune_flag;

            uint8_t str, dex, con, int_value;
            uint32_t damage_range[2];

            short attack_speed;
            short moving_speed;
            uint8_t aggresive_hp_pct;
            uint16_t aggressive_sight;
            uint16_t attack_range;

            char enchants[MOB_ENCHANTS_MAX_NUM];
            char resists[MOB_RESISTS_MAX_NUM];

            uint32_t resurrection_vnum;
            uint32_t drop_item_vnum;

            uint8_t mount_capacity;
            uint8_t on_click_type;

            uint8_t empire;
            char folder[64 + 1];
            float dam_multiply;
            uint32_t summon_vnum;
            uint32_t drain_sp;
            uint32_t monster_color;
            uint32_t polymorph_item_vnum;

            mob_skill_level skills[MOB_SKILL_MAX_NUM];

            uint8_t berserk_point;
            uint8_t stone_skin_point;
            uint8_t god_speed_point;
            uint8_t death_blow_point;
            uint8_t revive_point;
        };
#pragma pack(pop)
        static_assert(sizeof(mob_table) == 333, "mob_proto v2 record layout");

        using mob_table_list = std::list<mob_table *>;
        using non_player_data_map = std::map<uint32_t, mob_table>;

      public:
        python_non_player(void);
        ~python_non_player(void);

        void clear();
        void destroy();

        auto load_non_player_data(const char *file_name) -> bool;

        auto table(uint32_t vnum) -> const mob_table *;
        auto get_name(uint32_t vnum, const char **name) -> bool;
        auto instance_type(uint32_t vnum, uint8_t *type) -> bool;
        auto get_event_type(uint32_t vnum) -> uint8_t;
        auto monster_color(uint32_t vnum) -> uint32_t;
        auto monster_name(uint32_t vnum) -> const char *;

      protected:
        non_player_data_map non_player_data_map_value_;
    };

    inline auto get_python_non_player() -> python_non_player & {
        static python_non_player instance;

        return instance;
    }
}
