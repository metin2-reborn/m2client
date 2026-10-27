#pragma once
#include <metin2/graphics/grp_text.h>
#include <metin2/world/items/item_data.h>

namespace metin2::inline client::inline app {
    enum {
        POINT_MAX_NUM = 255,
        CHARACTER_NAME_MAX_LEN = 24,
        PLAYER_NAME_MAX_LEN = 12,
    };

    struct affects {
        enum {
            AFFECT_MAX_NUM = 32,
        };

        affects() : affects_value(0) {}
        affects(const uint32_t &affects) {
            set_affects(affects);
        }

        auto operator=(const uint32_t &affects_value) -> affects & {
            set_affects(affects_value);
            return *this;
        }

        auto is_affect(uint8_t index) -> bool {
            return affects_value & (1 << index);
        }

        void set_affects(const uint32_t &affects) {
            affects_value = affects;
        }

        uint32_t affects_value;
    };

    extern std::string guild_symbol_path_name;

    const uint32_t c_Name_Max_Length = 64;
    const uint32_t c_FileName_Max_Length = 128;
    const uint32_t c_Short_Name_Max_Length = 32;

    const uint32_t c_Inventory_Page_Size = 5 * 9; // columns * rows
    const uint32_t c_Inventory_Page_Count = 2;
    const uint32_t c_ItemSlot_Count = c_Inventory_Page_Size * c_Inventory_Page_Count;
    const uint32_t c_Equipment_Count = 12;

    const uint32_t c_Equipment_Start = c_ItemSlot_Count;

    const uint32_t c_Equipment_Body = c_Equipment_Start + 0;
    const uint32_t c_Equipment_Head = c_Equipment_Start + 1;
    const uint32_t c_Equipment_Shoes = c_Equipment_Start + 2;
    const uint32_t c_Equipment_Wrist = c_Equipment_Start + 3;
    const uint32_t c_Equipment_Weapon = c_Equipment_Start + 4;
    const uint32_t c_Equipment_Neck = c_Equipment_Start + 5;
    const uint32_t c_Equipment_Ear = c_Equipment_Start + 6;
    const uint32_t c_Equipment_Unique1 = c_Equipment_Start + 7;
    const uint32_t c_Equipment_Unique2 = c_Equipment_Start + 8;
    const uint32_t c_Equipment_Arrow = c_Equipment_Start + 9;
    const uint32_t c_Equipment_Shield = c_Equipment_Start + 10;

    // New rings and the belt. The equipment positions are fragmented by the old equipment, a quest
    // reward buff implemented as an equipped item and the costume system. The old equipment uses
    // item positions 90-102 in the server database and the slots added in 2013 start at 111.
    // Equipment can use positions up to 121; 122 onwards belongs to the dragon soul slots.
    const uint32_t c_New_Equipment_Start = c_Equipment_Start + 21;
    const uint32_t c_New_Equipment_Count = 3;
    const uint32_t c_Equipment_Ring1 = c_New_Equipment_Start + 0;
    const uint32_t c_Equipment_Ring2 = c_New_Equipment_Start + 1;
    const uint32_t c_Equipment_Belt = c_New_Equipment_Start + 2;

    enum dragon_soul_deck_type {
        DS_DECK_1,
        DS_DECK_2,
        DS_DECK_MAX_NUM = 2,
    };

    enum dragon_soul_grade_types {
        DRAGON_SOUL_GRADE_NORMAL,
        DRAGON_SOUL_GRADE_BRILLIANT,
        DRAGON_SOUL_GRADE_RARE,
        DRAGON_SOUL_GRADE_ANCIENT,
        DRAGON_SOUL_GRADE_LEGENDARY,
        DRAGON_SOUL_GRADE_MAX,
    };

    enum dragon_soul_step_types {
        DRAGON_SOUL_STEP_LOWEST,
        DRAGON_SOUL_STEP_LOW,
        DRAGON_SOUL_STEP_MID,
        DRAGON_SOUL_STEP_HIGH,
        DRAGON_SOUL_STEP_HIGHEST,
        DRAGON_SOUL_STEP_MAX,
    };

    const uint32_t c_Costume_Slot_Start = c_Equipment_Start + 19; // hard-coded: the server's costume slots start at wear position 19
    const uint32_t c_Costume_Slot_Body = c_Costume_Slot_Start + 0;
    const uint32_t c_Costume_Slot_Hair = c_Costume_Slot_Start + 1;
    const uint32_t c_Costume_Slot_Count = 2;
    const uint32_t c_Costume_Slot_End = c_Costume_Slot_Start + c_Costume_Slot_Count;

    // Hard-coded 32: the server's dragon soul slots start at wear position 32. The server's wear
    // positions are expected to stay below 32, so everything after that is used for dragon soul slots.
    const uint32_t c_Wear_Max = 32;
    const uint32_t c_DragonSoul_Equip_Start = c_ItemSlot_Count + c_Wear_Max;
    const uint32_t c_DragonSoul_Equip_Slot_Max = 6;
    const uint32_t c_DragonSoul_Equip_End = c_DragonSoul_Equip_Start + c_DragonSoul_Equip_Slot_Max * DS_DECK_MAX_NUM;

    // There are 2 dragon soul decks, with room reserved for 3. The belt inventory follows.
    const uint32_t c_DragonSoul_Equip_Reserved_Count = c_DragonSoul_Equip_Slot_Max * 3;

    const uint32_t c_Belt_Inventory_Slot_Start = c_DragonSoul_Equip_End + c_DragonSoul_Equip_Reserved_Count;
    const uint32_t c_Belt_Inventory_Width = 4;
    const uint32_t c_Belt_Inventory_Height = 4;
    const uint32_t c_Belt_Inventory_Slot_Count = c_Belt_Inventory_Width * c_Belt_Inventory_Height;
    const uint32_t c_Belt_Inventory_Slot_End = c_Belt_Inventory_Slot_Start + c_Belt_Inventory_Slot_Count;

    const uint32_t c_Inventory_Count = c_Belt_Inventory_Slot_End;

    const uint32_t c_DragonSoul_Inventory_Start = 0;
    const uint32_t c_DragonSoul_Inventory_Box_Size = 32;
    const uint32_t c_DragonSoul_Inventory_Count = static_cast<uint32_t>(item_data::DS_SLOT_NUM_TYPES) * DRAGON_SOUL_GRADE_MAX * c_DragonSoul_Inventory_Box_Size;
    const uint32_t c_DragonSoul_Inventory_End = c_DragonSoul_Inventory_Start + c_DragonSoul_Inventory_Count;

    enum slot_type {
        SLOT_TYPE_NONE,
        SLOT_TYPE_INVENTORY,
        SLOT_TYPE_SKILL,
        SLOT_TYPE_EMOTION,
        SLOT_TYPE_SHOP,
        SLOT_TYPE_EXCHANGE_OWNER,
        SLOT_TYPE_EXCHANGE_TARGET,
        SLOT_TYPE_QUICK_SLOT,
        SLOT_TYPE_SAFEBOX,
        SLOT_TYPE_PRIVATE_SHOP,
        SLOT_TYPE_MALL,
        SLOT_TYPE_DRAGON_SOUL_INVENTORY,
        SLOT_TYPE_MAX,
    };

    enum windows {
        RESERVED_WINDOW,
        INVENTORY, // 2 pages of 45 = 90 cells
        EQUIPMENT,
        SAFEBOX,
        MALL,
        DRAGON_SOUL_INVENTORY,
        GROUND, // unused
        BELT_INVENTORY,

        WINDOW_TYPE_MAX,
    };

    enum ds_inventory_max_num {
        DS_INVENTORY_MAX_NUM = c_DragonSoul_Inventory_Count,
        DS_REFINE_WINDOW_MAX_NUM = 15,
    };

#pragma pack(push, 1)
    struct item_pos {
        uint8_t window_type;
        uint16_t cell;

        item_pos() {
            window_type = INVENTORY;
            cell = 0xffff;
        }

        item_pos(uint8_t window_type_value, uint16_t cell_value) {
            window_type = window_type_value;
            cell = cell_value;
        }

        auto is_valid_cell() -> bool {
            switch (window_type) {
            case INVENTORY:             return cell < c_Inventory_Count;
            case EQUIPMENT:             return cell < c_DragonSoul_Equip_End;
            case DRAGON_SOUL_INVENTORY: return cell < (DS_INVENTORY_MAX_NUM);
            default:                    return false;
            }
        }
        auto is_equip_cell() -> bool {
            switch (window_type) {
            case INVENTORY:
            case EQUIPMENT:
                return (c_Equipment_Start + c_Wear_Max > cell) && (c_Equipment_Start <= cell);

            case BELT_INVENTORY:
            case DRAGON_SOUL_INVENTORY:
                return false;

            default: return false;
            }
        }

        auto is_belt_inventory_cell() -> bool {
            return c_Belt_Inventory_Slot_Start <= cell && c_Belt_Inventory_Slot_End > cell;
        }

        auto operator==(const struct item_pos &rhs) const -> bool {
            return (window_type == rhs.window_type) && (cell == rhs.cell);
        }

        auto operator<(const struct item_pos &rhs) const -> bool {
            return (window_type < rhs.window_type) || ((window_type == rhs.window_type) && (cell < rhs.cell));
        }
    };
#pragma pack(pop)

    const uint32_t c_QuickBar_Line_Count = 3;
    const uint32_t c_QuickBar_Slot_Count = 12;

    const float c_Idle_WaitTime = 5.0f;

    const int c_Monster_Race_Start_Number = 6;
    const int c_Monster_Model_Start_Number = 20001;

    const float c_fAttack_Delay_Time = 0.2f;
    const float c_fHit_Delay_Time = 0.1f;
    const float c_fCrash_Wave_Time = 0.2f;
    const float c_fCrash_Wave_Distance = 3.0f;

    const float c_fHeight_Step_Distance = 50.0f;

    enum {
        DISTANCE_TYPE_FOUR_WAY,
        DISTANCE_TYPE_EIGHT_WAY,
        DISTANCE_TYPE_ONE_WAY,
        DISTANCE_TYPE_MAX_NUM,
    };

    const float c_fMagic_Script_Version = 1.0f;
    const float c_fSkill_Script_Version = 1.0f;
    const float c_fMagicSoundInformation_Version = 1.0f;
    const float c_fBattleCommand_Script_Version = 1.0f;
    const float c_fEmotionCommand_Script_Version = 1.0f;
    const float c_fActive_Script_Version = 1.0f;
    const float c_fPassive_Script_Version = 1.0f;

    // Used by PushMove
    const float c_fWalkDistance = 175.0f;
    const float c_fRunDistance = 310.0f;

#define FILE_MAX_LEN 128

    enum {
        ITEM_SOCKET_SLOT_MAX_NUM = 3,
        ITEM_ATTRIBUTE_SLOT_MAX_NUM = 7,
    };

#pragma pack(push)
#pragma pack(1)

    struct quick_slot {
        uint8_t type_value;
        uint8_t position_value;
    };

    struct player_item_attribute {
        uint8_t type_value;
        short value;
    };

    struct packet_item {
        uint32_t vnum;
        uint8_t count;
        uint32_t flags_value;
        uint32_t anti_flags;
        int32_t sockets[ITEM_SOCKET_SLOT_MAX_NUM];
        player_item_attribute attr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
    };

    struct packet_shop_item {
        uint32_t vnum;
        uint32_t price;
        uint8_t count;
        uint8_t display_pos;
        int32_t sockets[ITEM_SOCKET_SLOT_MAX_NUM];
        player_item_attribute attr[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
    };
    using shop_item_data = packet_shop_item;

#pragma pack(pop)

    void default_font_startup();
    void default_font_cleanup();
    // The font of chat lines, name tags and quest text, set by the game scripts from the stylesheet;
    // the italic one (GM whispers) is the same spec in italic. Null before it is set.
    void default_font_set(const font_spec &font);
    auto default_font_spec() -> const font_spec &;
    auto default_font_get_resource() -> resource *;
    auto default_italic_font_get_resource() -> resource *;

    void set_guild_symbol_path(const char *path_name);
    auto get_guild_symbol_file_name(uint32_t guild_id) -> const char *;
    auto slot_type_to_inven_type(uint8_t slot_type_value) -> uint8_t;
}
