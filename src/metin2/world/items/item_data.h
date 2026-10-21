#pragma once

// Everything about one item: icon, ground model and game data.

#include <metin2/graphics/grp_sub_image.h>
#include <metin2/models/thing.h>
#include <metin2/world/actors/game_type.h>

namespace metin2::inline world::inline items {

    class item_data {
      public:
        enum {
            ITEM_NAME_MAX_LEN = 24,
            PROTO_NAME_MAX_LEN = 63, // UTF-8 names in item_proto/mob_proto v2
            ITEM_LIMIT_MAX_NUM = 2,
            ITEM_VALUES_MAX_NUM = 6,
            ITEM_SMALL_DESCR_MAX_LEN = 256,
            ITEM_APPLY_MAX_NUM = 3,
            ITEM_SOCKET_MAX_NUM = 3,
        };

        enum item_type {
            ITEM_TYPE_NONE,           // 0
            ITEM_TYPE_WEAPON,         // 1
            ITEM_TYPE_ARMOR,          // 2
            ITEM_TYPE_USE,            // 3
            ITEM_TYPE_AUTOUSE,        // 4
            ITEM_TYPE_MATERIAL,       // 5
            ITEM_TYPE_SPECIAL,        // 6
            ITEM_TYPE_TOOL,           // 7
            ITEM_TYPE_LOTTERY,        // 8
            ITEM_TYPE_ELK,            // 9
            ITEM_TYPE_METIN,          // 10
            ITEM_TYPE_CONTAINER,      // 11
            ITEM_TYPE_FISH,           // 12
            ITEM_TYPE_ROD,            // 13
            ITEM_TYPE_RESOURCE,       // 14
            ITEM_TYPE_CAMPFIRE,       // 15
            ITEM_TYPE_UNIQUE,         // 16
            ITEM_TYPE_SKILLBOOK,      // 17
            ITEM_TYPE_QUEST,          // 18
            ITEM_TYPE_POLYMORPH,      // 19
            ITEM_TYPE_TREASURE_BOX,   // 20
            ITEM_TYPE_TREASURE_KEY,   // 21
            ITEM_TYPE_SKILLFORGET,    // 22
            ITEM_TYPE_GIFTBOX,        // 23
            ITEM_TYPE_PICK,           // 24
            ITEM_TYPE_HAIR,           // 25
            ITEM_TYPE_TOTEM,          // 26
            ITEM_TYPE_BLEND,          // 27 potion that gets random attributes when created
            ITEM_TYPE_COSTUME,        // 28 costume item
            ITEM_TYPE_DS,             // 29 dragon soul stone
            ITEM_TYPE_SPECIAL_DS,     // 30 special dragon soul stone (worn in a dragon soul slot like a unique item)
            ITEM_TYPE_EXTRACT,        // 31 extraction tool
            ITEM_TYPE_SECONDARY_COIN, // 32 secondary coin
            ITEM_TYPE_RING,           // 33 ring (the plain ring slot, not the unique slot)
            ITEM_TYPE_BELT,           // 34

            ITEM_TYPE_MAX_NUM,
        };

        enum weapon_sub_types {
            WEAPON_SWORD,
            WEAPON_DAGGER, // dual blades
            WEAPON_BOW,
            WEAPON_TWO_HANDED,
            WEAPON_BELL,
            WEAPON_FAN,
            WEAPON_ARROW,
            WEAPON_NUM_TYPES,

            WEAPON_NONE = WEAPON_NUM_TYPES + 1,
        };

        enum material_sub_types {
            MATERIAL_LEATHER,
            MATERIAL_BLOOD,
            MATERIAL_ROOT,
            MATERIAL_NEEDLE,
            MATERIAL_JEWEL,
            MATERIAL_DS_REFINE_NORMAL,
            MATERIAL_DS_REFINE_BLESSED,
            MATERIAL_DS_REFINE_HOLLY,
        };

        enum armor_sub_types {
            ARMOR_BODY,
            ARMOR_HEAD,
            ARMOR_SHIELD,
            ARMOR_WRIST,
            ARMOR_FOOTS,
            ARMOR_NECK,
            ARMOR_EAR,
            ARMOR_NUM_TYPES
        };

        enum costume_sub_types {
            COSTUME_BODY, // 0
            COSTUME_HAIR, // 1
            COSTUME_NUM_TYPES,
        };

        enum use_sub_types {
            USE_POTION, // 0
            USE_TALISMAN,
            USE_TUNING,
            USE_MOVE,
            USE_TREASURE_BOX,
            USE_MONEYBAG,
            USE_BAIT,
            USE_ABILITY_UP,
            USE_AFFECT,
            USE_CREATE_STONE,
            USE_SPECIAL, // 10
            USE_POTION_NODELAY,
            USE_CLEAR,
            USE_INVISIBILITY,
            USE_DETACHMENT,
            USE_BUCKET,
            USE_POTION_CONTINUE,
            USE_CLEAN_SOCKET,
            USE_CHANGE_ATTRIBUTE,
            USE_ADD_ATTRIBUTE,
            USE_ADD_ACCESSORY_SOCKET, // 20
            USE_PUT_INTO_ACCESSORY_SOCKET,
            USE_ADD_ATTRIBUTE2,
            USE_RECIPE,
            USE_CHANGE_ATTRIBUTE2,
            USE_BIND,
            USE_UNBIND,
            USE_TIME_CHARGE_PER,
            USE_TIME_CHARGE_FIX,      // 28
            USE_PUT_INTO_BELT_SOCKET, // 29 item that can be used on a belt socket
            USE_PUT_INTO_RING_SOCKET, // 30 item that can be used on a ring socket (the new ring slot, not unique rings)
        };

        enum dragon_soul_sub_type {
            DS_SLOT1,
            DS_SLOT2,
            DS_SLOT3,
            DS_SLOT4,
            DS_SLOT5,
            DS_SLOT6,
            DS_SLOT_NUM_TYPES = 6,
        };

        enum metin_sub_types {
            METIN_NORMAL,
            METIN_GOLD,
        };

        enum limit_types {
            LIMIT_NONE,

            LIMIT_LEVEL,
            LIMIT_STR,
            LIMIT_DEX,
            LIMIT_INT,
            LIMIT_CON,
            LIMIT_PCBANG,

            // Time runs in real time whether worn or not; socket0 holds the expiry time as a unix timestamp.
            LIMIT_REAL_TIME,

            // The real-time timer starts the moment the item is first used or worn. Before that socket0 holds
            // the usable time in seconds (0 means the proto's limit value); on use socket1 gets the use count
            // and socket0 the expiry time as a unix timestamp.
            LIMIT_REAL_TIME_START_FIRST_USE,

            // Time only runs while the item is worn. socket0 holds the remaining seconds (on first use a 0 is
            // replaced by the proto's limit value).
            LIMIT_TIMER_BASED_ON_WEAR,

            LIMIT_MAX_NUM
        };

        enum item_anti_flag {
            ITEM_ANTIFLAG_FEMALE = (1 << 0),
            ITEM_ANTIFLAG_MALE = (1 << 1),
            ITEM_ANTIFLAG_WARRIOR = (1 << 2),
            ITEM_ANTIFLAG_ASSASSIN = (1 << 3),
            ITEM_ANTIFLAG_SURA = (1 << 4),
            ITEM_ANTIFLAG_SHAMAN = (1 << 5),
            ITEM_ANTIFLAG_GET = (1 << 6), // cannot be picked up
            ITEM_ANTIFLAG_DROP = (1 << 7),
            ITEM_ANTIFLAG_SELL = (1 << 8),
            ITEM_ANTIFLAG_EMPIRE_A = (1 << 9),
            ITEM_ANTIFLAG_EMPIRE_B = (1 << 10),
            ITEM_ANTIFLAG_EMPIRE_R = (1 << 11), // not usable in empire C (R is the red empire)
            ITEM_ANTIFLAG_SAVE = (1 << 12),
            ITEM_ANTIFLAG_GIVE = (1 << 13),   // cannot be traded
            ITEM_ANTIFLAG_PKDROP = (1 << 14), // not dropped on PK
            ITEM_ANTIFLAG_STACK = (1 << 15),
            ITEM_ANTIFLAG_MYSHOP = (1 << 16), // cannot be put in a private shop
        };

        enum item_flag {
            ITEM_FLAG_REFINEABLE = (1 << 0),
            ITEM_FLAG_SAVE = (1 << 1),
            ITEM_FLAG_STACKABLE = (1 << 2),
            ITEM_FLAG_COUNT_PER_1GOLD = (1 << 3), // price is per unit
            ITEM_FLAG_SLOW_QUERY = (1 << 4),      // the server only writes it to SQL at logout
            ITEM_FLAG_RARE = (1 << 5),
            ITEM_FLAG_UNIQUE = (1 << 6),
            ITEM_FLAG_MAKECOUNT = (1 << 7),
            ITEM_FLAG_IRREMOVABLE = (1 << 8),
            ITEM_FLAG_CONFIRM_WHEN_USE = (1 << 9),
            ITEM_FLAG_QUEST_USE = (1 << 10),          // using it runs a quest script
            ITEM_FLAG_QUEST_USE_MULTIPLE = (1 << 11), // using it runs a quest script
            ITEM_FLAG_UNUSED03 = (1 << 12),
            ITEM_FLAG_LOG = (1 << 13), // logged when used
            ITEM_FLAG_APPLICABLE = (1 << 14),
        };

        enum wear_positions {
            WEAR_BODY,    // 0
            WEAR_HEAD,    // 1
            WEAR_FOOTS,   // 2
            WEAR_WRIST,   // 3
            WEAR_WEAPON,  // 4
            WEAR_NECK,    // 5
            WEAR_EAR,     // 6
            WEAR_UNIQUE1, // 7
            WEAR_UNIQUE2, // 8
            WEAR_ARROW,   // 9
            WEAR_SHIELD,  // 10
            WEAR_MAX_NUM,
        };

        enum item_wearable_flag {
            WEARABLE_BODY = (1 << 0),
            WEARABLE_HEAD = (1 << 1),
            WEARABLE_FOOTS = (1 << 2),
            WEARABLE_WRIST = (1 << 3),
            WEARABLE_WEAPON = (1 << 4),
            WEARABLE_NECK = (1 << 5),
            WEARABLE_EAR = (1 << 6),
            WEARABLE_UNIQUE = (1 << 7),
            WEARABLE_SHIELD = (1 << 8),
            WEARABLE_ARROW = (1 << 9),
        };

        enum apply_types {
            APPLY_NONE,                     // 0
            APPLY_MAX_HP,                   // 1
            APPLY_MAX_SP,                   // 2
            APPLY_CON,                      // 3
            APPLY_INT,                      // 4
            APPLY_STR,                      // 5
            APPLY_DEX,                      // 6
            APPLY_ATT_SPEED,                // 7
            APPLY_MOV_SPEED,                // 8
            APPLY_CAST_SPEED,               // 9
            APPLY_HP_REGEN,                 // 10
            APPLY_SP_REGEN,                 // 11
            APPLY_POISON_PCT,               // 12
            APPLY_STUN_PCT,                 // 13
            APPLY_SLOW_PCT,                 // 14
            APPLY_CRITICAL_PCT,             // 15
            APPLY_PENETRATE_PCT,            // 16
            APPLY_ATTBONUS_HUMAN,           // 17
            APPLY_ATTBONUS_ANIMAL,          // 18
            APPLY_ATTBONUS_ORC,             // 19
            APPLY_ATTBONUS_MILGYO,          // 20
            APPLY_ATTBONUS_UNDEAD,          // 21
            APPLY_ATTBONUS_DEVIL,           // 22
            APPLY_STEAL_HP,                 // 23
            APPLY_STEAL_SP,                 // 24
            APPLY_MANA_BURN_PCT,            // 25
            APPLY_DAMAGE_SP_RECOVER,        // 26
            APPLY_BLOCK,                    // 27
            APPLY_DODGE,                    // 28
            APPLY_RESIST_SWORD,             // 29
            APPLY_RESIST_TWOHAND,           // 30
            APPLY_RESIST_DAGGER,            // 31
            APPLY_RESIST_BELL,              // 32
            APPLY_RESIST_FAN,               // 33
            APPLY_RESIST_BOW,               // 34
            APPLY_RESIST_FIRE,              // 35
            APPLY_RESIST_ELEC,              // 36
            APPLY_RESIST_MAGIC,             // 37
            APPLY_RESIST_WIND,              // 38
            APPLY_REFLECT_MELEE,            // 39
            APPLY_REFLECT_CURSE,            // 40
            APPLY_POISON_REDUCE,            // 41
            APPLY_KILL_SP_RECOVER,          // 42
            APPLY_EXP_DOUBLE_BONUS,         // 43
            APPLY_GOLD_DOUBLE_BONUS,        // 44
            APPLY_ITEM_DROP_BONUS,          // 45
            APPLY_POTION_BONUS,             // 46
            APPLY_KILL_HP_RECOVER,          // 47
            APPLY_IMMUNE_STUN,              // 48
            APPLY_IMMUNE_SLOW,              // 49
            APPLY_IMMUNE_FALL,              // 50
            APPLY_SKILL,                    // 51
            APPLY_BOW_DISTANCE,             // 52
            APPLY_ATT_GRADE_BONUS,          // 53
            APPLY_DEF_GRADE_BONUS,          // 54
            APPLY_MAGIC_ATT_GRADE,          // 55
            APPLY_MAGIC_DEF_GRADE,          // 56
            APPLY_CURSE_PCT,                // 57
            APPLY_MAX_STAMINA,              // 58
            APPLY_ATT_BONUS_TO_WARRIOR,     // 59
            APPLY_ATT_BONUS_TO_ASSASSIN,    // 60
            APPLY_ATT_BONUS_TO_SURA,        // 61
            APPLY_ATT_BONUS_TO_SHAMAN,      // 62
            APPLY_ATT_BONUS_TO_MONSTER,     // 63
            APPLY_MALL_ATTBONUS,            // 64 attack +x%
            APPLY_MALL_DEFBONUS,            // 65 defence +x%
            APPLY_MALL_EXPBONUS,            // 66 experience +x%
            APPLY_MALL_ITEMBONUS,           // 67 item drop rate x/10
            APPLY_MALL_GOLDBONUS,           // 68 gold drop rate x/10
            APPLY_MAX_HP_PCT,               // 69 max HP +x%
            APPLY_MAX_SP_PCT,               // 70 max SP +x%
            APPLY_SKILL_DAMAGE_BONUS,       // 71 skill damage * (100+x)%
            APPLY_NORMAL_HIT_DAMAGE_BONUS,  // 72 normal hit damage * (100+x)%
            APPLY_SKILL_DEFEND_BONUS,       // 73 skill damage defence * (100-x)%
            APPLY_NORMAL_HIT_DEFEND_BONUS,  // 74 normal hit damage defence * (100-x)%
            APPLY_EXTRACT_HP_PCT,           // 75
            APPLY_PC_BANG_EXP_BONUS,        // 76
            APPLY_PC_BANG_DROP_BONUS,       // 77
            APPLY_RESIST_WARRIOR,           // 78
            APPLY_RESIST_ASSASSIN,          // 79
            APPLY_RESIST_SURA,              // 80
            APPLY_RESIST_SHAMAN,            // 81
            APPLY_ENERGY,                   // 82
            APPLY_DEF_GRADE,                // 83 defence (the client shows DEF_GRADE_BONUS doubled on purpose)
            APPLY_COSTUME_ATTR_BONUS,       // 84 attribute bonus on costume items
            APPLY_MAGIC_ATTBONUS_PER,       // 85 magic attack +x%
            APPLY_MELEE_MAGIC_ATTBONUS_PER, // 86 magic + melee attack +x%

            APPLY_RESIST_ICE,   // 87 ice resistance
            APPLY_RESIST_EARTH, // 88 earth resistance
            APPLY_RESIST_DARK,  // 89 darkness resistance

            APPLY_ANTI_CRITICAL_PCT,  // 90 critical resistance
            APPLY_ANTI_PENETRATE_PCT, // 91 piercing hit resistance

            MAX_APPLY_NUM,
        };

        enum immune_flags {
            IMMUNE_PARA = (1 << 0),
            IMMUNE_CURSE = (1 << 1),
            IMMUNE_STUN = (1 << 2),
            IMMUNE_SLEEP = (1 << 3),
            IMMUNE_SLOW = (1 << 4),
            IMMUNE_POISON = (1 << 5),
            IMMUNE_TERROR = (1 << 6),
        };

#pragma pack(push)
#pragma pack(1)
        struct item_limit {
            uint8_t type_value;
            int32_t value;
        };

        struct item_apply {
            uint8_t type_value;
            int32_t value;
        };

        struct item_table {
            uint32_t vnum;
            uint32_t vnum_range;
            char name_value[PROTO_NAME_MAX_LEN + 1];
            char locale_name[PROTO_NAME_MAX_LEN + 1];
            uint8_t type_value;
            uint8_t sub_type;

            uint8_t weight;
            uint8_t size;

            uint32_t anti_flags;
            uint32_t flags_value;
            uint32_t wear_flags;
            uint32_t immune_flag;

            uint32_t i_buy_item_price;
            uint32_t i_sell_item_price;

            item_limit limits[ITEM_LIMIT_MAX_NUM];
            item_apply applies[ITEM_APPLY_MAX_NUM];
            int32_t values[ITEM_VALUES_MAX_NUM];
            int32_t sockets[ITEM_SOCKET_MAX_NUM];
            uint32_t refined_vnum;
            uint16_t refine_set;
            uint8_t alter_to_magic_item_pct;
            uint8_t specular_value;
            uint8_t gain_socket_pct;
        };

#pragma pack(pop)
        static_assert(sizeof(item_table) == 234, "item_proto v2 record layout");

      public:
        item_data();
        virtual ~item_data();

        void clear();
        void set_summary(const std::string &summ);
        void set_description(const std::string &desc);

        auto model_thing() -> graphic_thing *;
        auto sub_model_thing() -> graphic_thing *;
        auto drop_model_thing() -> graphic_thing *;
        auto icon_image() -> graphic_sub_image *;

        auto lod_model_thing_count() -> uint32_t;
        auto lod_model_thing_pointer(uint32_t index, graphic_thing **model_thing) -> bool;

        auto attaching_data_count() -> uint32_t;
        auto collision_data_pointer(uint32_t index, const race_types::attaching_data **attaching_data) -> bool;
        auto attaching_data_pointer(uint32_t index, const race_types::attaching_data **attaching_data) -> bool;

        auto table() const -> const item_table *;
        auto get_index() const -> uint32_t;
        auto get_name() const -> const char *;
        auto description() const -> const char *;
        auto summary() const -> const char *;
        auto get_type() const -> uint8_t;
        auto sub_type() const -> uint8_t;
        auto refine() const -> uint32_t;
        auto use_type_string() const -> const char *;
        auto weapon_type() const -> uint32_t;
        auto size() const -> uint8_t;
        auto is_anti_flag(uint32_t flag) const -> bool;
        auto is_flag(uint32_t flag) const -> bool;
        auto is_wearable_flag(uint32_t flag) const -> bool;
        auto has_next_grade() const -> bool;
        auto wear_flags() const -> uint32_t;
        auto i_sell_item_price() const -> uint32_t;
        auto limit(uint8_t index, item_limit *item_limit_value) const -> bool;
        auto apply(uint8_t index, item_apply *item_apply_value) const -> bool;
        auto value(uint8_t index) const -> int32_t;
        auto socket_count() const -> int;

        auto specular_poweru() const -> uint32_t;
        auto specular_powerf() const -> float;

        auto is_equipment() const -> bool;

        void set_default_item_data(const char *icon_file_name, const char *model_file_name = nullptr);
        void set_item_table_data(item_table *item_table_value);

      protected:
        void load_files();
        void set_icon_image(const char *file_name);

      protected:
        std::string model_file_name_;
        std::string sub_model_file_name_;
        std::string drop_model_file_name_;
        std::string icon_file_name_;
        std::string description_;
        std::string summary_;
        std::vector<std::string> lod_model_file_name_vector_;

        graphic_thing *model_thing_;
        graphic_thing *sub_model_thing_;
        graphic_thing *drop_model_thing_;
        graphic_sub_image *icon_image_;
        std::vector<graphic_thing *> lod_model_thing_vector_;

        race_types::attaching_data_vector attaching_data_vector_value_;
        uint32_t vnum_;
        item_table item_table_value_;

      public:
        static void destroy_system();

        static auto new_value() -> item_data *;
        static void delete_value(item_data *item_data);

        static dynamic_pool<item_data> pool_value;
    };
}
