#include <metin2/log/log.h>
#include "python_item.h"

#include <metin2/client/app/abstract_application.h>
#include <metin2/client/app/game_type.h>
#include <metin2/client/characters/instance_base.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_module.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/items/item_manager.h>

namespace metin2::inline client::inline app {
    extern int two_handed_weapon_attack_speed_decrease;
}
namespace metin2::inline client::inline inventory {

    auto item_set_use_sound_file_name(PyObject *self, PyObject *args) -> PyObject * {
        int use_sound;
        if (!py_tuple_get_integer(args, 0, &use_sound))
            return py_bad_argument();

        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_bad_argument();

        python_item &item = get_python_item();
        item.set_use_sound_file_name(use_sound, file_name);
        return py_build_none();
    }

    auto item_set_drop_sound_file_name(PyObject *self, PyObject *args) -> PyObject * {
        int drop_sound;
        if (!py_tuple_get_integer(args, 0, &drop_sound))
            return py_bad_argument();

        char *file_name;
        if (!py_tuple_get_string(args, 1, &file_name))
            return py_bad_argument();

        python_item &item = get_python_item();
        item.set_drop_sound_file_name(drop_sound, file_name);
        return py_build_none();
    }

    auto item_select_item(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_bad_argument();

        if (!get_item_manager().select_item_data(index)) {
            metin2::log::error("Item {} not found; item 60001 is shown instead", index);
            get_item_manager().select_item_data(60001);
        }

        return py_build_none();
    }

    auto item_get_item_name(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("s", item_data->get_name());
    }

    auto item_get_item_description(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("s", item_data->description());
    }

    auto item_get_item_summary(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("s", item_data->summary());
    }

    auto item_get_icon_image(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("n", (Py_ssize_t)item_data->icon_image());
    }

    auto item_get_icon_image_file_name(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        graphic_sub_image *image = item_data->icon_image();
        if (!image)
            return Py_BuildValue("s", "Noname");

        return Py_BuildValue("s", image->file_name());
    }

    auto item_get_item_size(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("(ii)", 1, item_data->size());
    }

    auto item_get_item_type(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("i", item_data->get_type());
    }

    auto item_get_item_sub_type(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("i", item_data->sub_type());
    }

    auto item_get_i_sell_item_price(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();

        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("i", item_data->i_sell_item_price());
    }

    auto item_is_anti_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag))
            return py_bad_argument();

        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("i", item_data->is_anti_flag(flag));
    }

    auto item_is_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag))
            return py_bad_argument();

        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("i", item_data->is_flag(flag));
    }

    auto item_is_wearable_flag(PyObject *self, PyObject *args) -> PyObject * {
        int flag;
        if (!py_tuple_get_integer(args, 0, &flag))
            return py_bad_argument();

        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("i", item_data->is_wearable_flag(flag));
    }

    auto item_is1_gold_item(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("no selected item data");

        return Py_BuildValue("i", item_data->is_flag(item_data::ITEM_FLAG_COUNT_PER_1GOLD));
    }

    auto item_get_limit(PyObject *self, PyObject *args) -> PyObject * {
        int value_index;
        if (!py_tuple_get_integer(args, 0, &value_index))
            return py_bad_argument();

        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Not yet select item data");

        item_data::item_limit item_limit;
        if (!item_data->limit(value_index, &item_limit))
            return py_build_exception();

        return Py_BuildValue("ii", item_limit.type_value, item_limit.value);
    }

    auto item_get_affect(PyObject *self, PyObject *args) -> PyObject * {
        int value_index;
        if (!py_tuple_get_integer(args, 0, &value_index))
            return py_bad_argument();

        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Not yet select item data");

        item_data::item_apply item_apply;
        if (!item_data->apply(value_index, &item_apply))
            return py_build_exception();

        if ((item_data::APPLY_ATT_SPEED == item_apply.type_value) && (item_data::ITEM_TYPE_WEAPON == item_data->get_type()) && (item_data::WEAPON_TWO_HANDED == item_data->sub_type())) {
            item_apply.value -= two_handed_weapon_attack_speed_decrease;
        }

        return Py_BuildValue("ii", item_apply.type_value, item_apply.value);
    }

    auto item_get_value(PyObject *self, PyObject *args) -> PyObject * {
        int value_index;
        if (!py_tuple_get_integer(args, 0, &value_index))
            return py_bad_argument();

        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Not yet select item data");

        return Py_BuildValue("i", item_data->value(value_index));
    }

    auto item_get_icon_instance(PyObject *self, PyObject *args) -> PyObject * {
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Not yet select item data");

        graphic_sub_image *image = item_data->icon_image();
        if (!image)
            return py_build_exception("Cannot get icon image by %d", item_data->get_index());

        graphic_image_instance *image_instance = graphic_image_instance::new_value();
        image_instance->set_image_pointer(image);

        return Py_BuildValue("n", (Py_ssize_t)image_instance);
    }

    auto item_delete_icon_instance(PyObject *self, PyObject *args) -> PyObject * {
        intptr_t handle;
        if (!py_tuple_get_integer(args, 0, &handle))
            return py_bad_argument();

        graphic_image_instance::delete_value((graphic_image_instance *)handle);

        return py_build_none();
    }

    auto item_is_equipment_vid(PyObject *self, PyObject *args) -> PyObject * {
        int item_vid;
        if (!py_tuple_get_integer(args, 0, &item_vid))
            return py_bad_argument();

        get_item_manager().select_item_data(item_vid);
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Not yet select item data");

        return Py_BuildValue("i", item_data->is_equipment());
    }

    auto item_get_use_type(PyObject *self, PyObject *args) -> PyObject * {
        int item_vid;
        if (!py_tuple_get_integer(args, 0, &item_vid))
            return py_bad_argument();

        get_item_manager().select_item_data(item_vid);
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Can't find select item data");

        return Py_BuildValue("s", item_data->use_type_string());
    }

    auto item_is_refine_scroll(PyObject *self, PyObject *args) -> PyObject * {
        int item_index;
        if (!py_tuple_get_integer(args, 0, &item_index))
            return py_bad_argument();

        get_item_manager().select_item_data(item_index);
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Can't find select item data");

        if (item_data->get_type() != item_data::ITEM_TYPE_USE)
            return Py_BuildValue("i", false);

        switch (item_data->sub_type()) {
        case item_data::USE_TUNING:
            return Py_BuildValue("i", true);
            break;
        }

        return Py_BuildValue("i", false);
    }

    auto item_is_detach_scroll(PyObject *self, PyObject *args) -> PyObject * {
        int item_index;
        if (!py_tuple_get_integer(args, 0, &item_index))
            return py_bad_argument();

        get_item_manager().select_item_data(item_index);
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Can't find select item data");

        int type = item_data->get_type();
        int sub_type = item_data->sub_type();
        if (type == item_data::ITEM_TYPE_USE)
            if (sub_type == item_data::USE_DETACHMENT) {
                return Py_BuildValue("i", true);
            }

        return Py_BuildValue("i", false);
    }

    auto item_can_add_to_quick_slot_item(PyObject *self, PyObject *args) -> PyObject * {
        int item_index;
        if (!py_tuple_get_integer(args, 0, &item_index))
            return py_bad_argument();

        get_item_manager().select_item_data(item_index);
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Can't find select item data");

        if (item_data::ITEM_TYPE_USE == item_data->get_type() || item_data::ITEM_TYPE_QUEST == item_data->get_type()) {
            return Py_BuildValue("i", true);
        }

        return Py_BuildValue("i", false);
    }

    auto item_is_key(PyObject *self, PyObject *args) -> PyObject * {
        int item_index;
        if (!py_tuple_get_integer(args, 0, &item_index))
            return py_bad_argument();

        get_item_manager().select_item_data(item_index);
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Can't find select item data");

        if (item_data::ITEM_TYPE_TREASURE_KEY == item_data->get_type()) {
            return Py_BuildValue("i", true);
        }

        return Py_BuildValue("i", false);
    }

    auto item_is_metin(PyObject *self, PyObject *args) -> PyObject * {
        int item_index;
        if (!py_tuple_get_integer(args, 0, &item_index))
            return py_bad_argument();

        get_item_manager().select_item_data(item_index);
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data)
            return py_build_exception("Can't find select item data");

        if (item_data::ITEM_TYPE_METIN == item_data->get_type()) {
            return Py_BuildValue("i", true);
        }

        return Py_BuildValue("i", false);
    }

    auto item_pick(PyObject *self, PyObject *args) -> PyObject * {
        uint32_t item_id;
        if (get_python_item().picked_item_id(&item_id))
            return Py_BuildValue("i", item_id);
        else
            return Py_BuildValue("i", -1);
    }

    void init_items_module() {
        static PyMethodDef s_methods[] =
            {
                {"SetUseSoundFileName", item_set_use_sound_file_name, METH_VARARGS},
                {"SetDropSoundFileName", item_set_drop_sound_file_name, METH_VARARGS},
                {"SelectItem", item_select_item, METH_VARARGS},

                {"GetItemName", item_get_item_name, METH_VARARGS},
                {"GetItemDescription", item_get_item_description, METH_VARARGS},
                {"GetItemSummary", item_get_item_summary, METH_VARARGS},
                {"GetIconImage", item_get_icon_image, METH_VARARGS},
                {"GetIconImageFileName", item_get_icon_image_file_name, METH_VARARGS},
                {"GetItemSize", item_get_item_size, METH_VARARGS},
                {"GetItemType", item_get_item_type, METH_VARARGS},
                {"GetItemSubType", item_get_item_sub_type, METH_VARARGS},
                {"GetISellItemPrice", item_get_i_sell_item_price, METH_VARARGS},
                {"IsAntiFlag", item_is_anti_flag, METH_VARARGS},
                {"IsFlag", item_is_flag, METH_VARARGS},
                {"IsWearableFlag", item_is_wearable_flag, METH_VARARGS},
                {"Is1GoldItem", item_is1_gold_item, METH_VARARGS},
                {"GetLimit", item_get_limit, METH_VARARGS},
                {"GetAffect", item_get_affect, METH_VARARGS},
                {"GetValue", item_get_value, METH_VARARGS},
                {"GetIconInstance", item_get_icon_instance, METH_VARARGS},
                {"GetUseType", item_get_use_type, METH_VARARGS},
                {"DeleteIconInstance", item_delete_icon_instance, METH_VARARGS},
                {"IsEquipmentVID", item_is_equipment_vid, METH_VARARGS},
                {"IsRefineScroll", item_is_refine_scroll, METH_VARARGS},
                {"IsDetachScroll", item_is_detach_scroll, METH_VARARGS},
                {"IsKey", item_is_key, METH_VARARGS},
                {"IsMetin", item_is_metin, METH_VARARGS},
                {"CanAddToQuickSlotItem", item_can_add_to_quick_slot_item, METH_VARARGS},

                {"Pick", item_pick, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = init_python_module("metin2.items", s_methods);

        PyModule_AddIntConstant(module_value, "USESOUND_ACCESSORY", python_item::USESOUND_ACCESSORY);
        PyModule_AddIntConstant(module_value, "USESOUND_ARMOR", python_item::USESOUND_ARMOR);
        PyModule_AddIntConstant(module_value, "USESOUND_BOW", python_item::USESOUND_BOW);
        PyModule_AddIntConstant(module_value, "USESOUND_DEFAULT", python_item::USESOUND_DEFAULT);
        PyModule_AddIntConstant(module_value, "USESOUND_WEAPON", python_item::USESOUND_WEAPON);
        PyModule_AddIntConstant(module_value, "USESOUND_POTION", python_item::USESOUND_POTION);
        PyModule_AddIntConstant(module_value, "USESOUND_PORTAL", python_item::USESOUND_PORTAL);

        PyModule_AddIntConstant(module_value, "DROPSOUND_ACCESSORY", python_item::DROPSOUND_ACCESSORY);
        PyModule_AddIntConstant(module_value, "DROPSOUND_ARMOR", python_item::DROPSOUND_ARMOR);
        PyModule_AddIntConstant(module_value, "DROPSOUND_BOW", python_item::DROPSOUND_BOW);
        PyModule_AddIntConstant(module_value, "DROPSOUND_DEFAULT", python_item::DROPSOUND_DEFAULT);
        PyModule_AddIntConstant(module_value, "DROPSOUND_WEAPON", python_item::DROPSOUND_WEAPON);

        PyModule_AddIntConstant(module_value, "EQUIPMENT_COUNT", c_Equipment_Count);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_HEAD", c_Equipment_Head);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_BODY", c_Equipment_Body);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_WEAPON", c_Equipment_Weapon);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_WRIST", c_Equipment_Wrist);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_SHOES", c_Equipment_Shoes);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_NECK", c_Equipment_Neck);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_EAR", c_Equipment_Ear);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_UNIQUE1", c_Equipment_Unique1);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_UNIQUE2", c_Equipment_Unique2);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_ARROW", c_Equipment_Arrow);

        PyModule_AddIntConstant(module_value, "EQUIPMENT_RING1", c_Equipment_Ring1);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_RING2", c_Equipment_Ring2);
        PyModule_AddIntConstant(module_value, "EQUIPMENT_BELT", c_Equipment_Belt);

        PyModule_AddIntConstant(module_value, "ITEM_TYPE_NONE", item_data::ITEM_TYPE_NONE);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_WEAPON", item_data::ITEM_TYPE_WEAPON);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_ARMOR", item_data::ITEM_TYPE_ARMOR);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_USE", item_data::ITEM_TYPE_USE);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_AUTOUSE", item_data::ITEM_TYPE_AUTOUSE);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_MATERIAL", item_data::ITEM_TYPE_MATERIAL);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_SPECIAL", item_data::ITEM_TYPE_SPECIAL);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_TOOL", item_data::ITEM_TYPE_TOOL);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_LOTTERY", item_data::ITEM_TYPE_LOTTERY);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_ELK", item_data::ITEM_TYPE_ELK);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_METIN", item_data::ITEM_TYPE_METIN);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_CONTAINER", item_data::ITEM_TYPE_CONTAINER);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_FISH", item_data::ITEM_TYPE_FISH);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_ROD", item_data::ITEM_TYPE_ROD);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_RESOURCE", item_data::ITEM_TYPE_RESOURCE);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_CAMPFIRE", item_data::ITEM_TYPE_CAMPFIRE);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_UNIQUE", item_data::ITEM_TYPE_UNIQUE);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_SKILLBOOK", item_data::ITEM_TYPE_SKILLBOOK);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_QUEST", item_data::ITEM_TYPE_QUEST);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_POLYMORPH", item_data::ITEM_TYPE_POLYMORPH);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_TREASURE_BOX", item_data::ITEM_TYPE_TREASURE_BOX);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_TREASURE_KEY", item_data::ITEM_TYPE_TREASURE_KEY);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_PICK", item_data::ITEM_TYPE_PICK);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_BLEND", item_data::ITEM_TYPE_BLEND);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_DS", item_data::ITEM_TYPE_DS);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_SPECIAL_DS", item_data::ITEM_TYPE_SPECIAL_DS);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_RING", item_data::ITEM_TYPE_RING);
        PyModule_AddIntConstant(module_value, "ITEM_TYPE_BELT", item_data::ITEM_TYPE_BELT);

        PyModule_AddIntConstant(module_value, "ITEM_TYPE_COSTUME", item_data::ITEM_TYPE_COSTUME);

        PyModule_AddIntConstant(module_value, "COSTUME_TYPE_BODY", item_data::COSTUME_BODY);
        PyModule_AddIntConstant(module_value, "COSTUME_TYPE_HAIR", item_data::COSTUME_HAIR);

        // Slot numbers in the inventory and the equipment window.
        PyModule_AddIntConstant(module_value, "COSTUME_SLOT_START", c_Costume_Slot_Start);
        PyModule_AddIntConstant(module_value, "COSTUME_SLOT_COUNT", c_Costume_Slot_Count);
        PyModule_AddIntConstant(module_value, "COSTUME_SLOT_BODY", c_Costume_Slot_Body);
        PyModule_AddIntConstant(module_value, "COSTUME_SLOT_HAIR", c_Costume_Slot_Hair);
        PyModule_AddIntConstant(module_value, "COSTUME_SLOT_END", c_Costume_Slot_End);

        PyModule_AddIntConstant(module_value, "BELT_INVENTORY_SLOT_START", c_Belt_Inventory_Slot_Start);
        PyModule_AddIntConstant(module_value, "BELT_INVENTORY_SLOT_COUNT", c_Belt_Inventory_Slot_Count);
        PyModule_AddIntConstant(module_value, "BELT_INVENTORY_SLOT_END", c_Belt_Inventory_Slot_End);

        PyModule_AddIntConstant(module_value, "WEAPON_SWORD", item_data::WEAPON_SWORD);
        PyModule_AddIntConstant(module_value, "WEAPON_DAGGER", item_data::WEAPON_DAGGER);
        PyModule_AddIntConstant(module_value, "WEAPON_BOW", item_data::WEAPON_BOW);
        PyModule_AddIntConstant(module_value, "WEAPON_TWO_HANDED", item_data::WEAPON_TWO_HANDED);
        PyModule_AddIntConstant(module_value, "WEAPON_BELL", item_data::WEAPON_BELL);
        PyModule_AddIntConstant(module_value, "WEAPON_FAN", item_data::WEAPON_FAN);
        PyModule_AddIntConstant(module_value, "WEAPON_ARROW", item_data::WEAPON_ARROW);
        PyModule_AddIntConstant(module_value, "WEAPON_NUM_TYPES", item_data::WEAPON_NUM_TYPES);

        PyModule_AddIntConstant(module_value, "USE_POTION", item_data::USE_POTION);
        PyModule_AddIntConstant(module_value, "USE_TALISMAN", item_data::USE_TALISMAN);
        PyModule_AddIntConstant(module_value, "USE_TUNING", item_data::USE_TUNING);
        PyModule_AddIntConstant(module_value, "USE_MOVE", item_data::USE_MOVE);
        PyModule_AddIntConstant(module_value, "USE_TREASURE_BOX", item_data::USE_TREASURE_BOX);
        PyModule_AddIntConstant(module_value, "USE_MONEYBAG", item_data::USE_MONEYBAG);
        PyModule_AddIntConstant(module_value, "USE_BAIT", item_data::USE_BAIT);
        PyModule_AddIntConstant(module_value, "USE_ABILITY_UP", item_data::USE_ABILITY_UP);
        PyModule_AddIntConstant(module_value, "USE_AFFECT", item_data::USE_AFFECT);
        PyModule_AddIntConstant(module_value, "USE_CREATE_STONE", item_data::USE_CREATE_STONE);
        PyModule_AddIntConstant(module_value, "USE_SPECIAL", item_data::USE_SPECIAL);
        PyModule_AddIntConstant(module_value, "USE_POTION_NODELAY", item_data::USE_POTION_NODELAY);
        PyModule_AddIntConstant(module_value, "USE_CLEAR", item_data::USE_CLEAR);
        PyModule_AddIntConstant(module_value, "USE_INVISIBILITY", item_data::USE_INVISIBILITY);
        PyModule_AddIntConstant(module_value, "USE_DETACHMENT", item_data::USE_DETACHMENT);
        PyModule_AddIntConstant(module_value, "USE_TIME_CHARGE_PER", item_data::USE_TIME_CHARGE_PER);
        PyModule_AddIntConstant(module_value, "USE_TIME_CHARGE_FIX", item_data::USE_TIME_CHARGE_FIX);

        PyModule_AddIntConstant(module_value, "MATERIAL_DS_REFINE_NORMAL", item_data::MATERIAL_DS_REFINE_NORMAL);
        PyModule_AddIntConstant(module_value, "MATERIAL_DS_REFINE_BLESSED", item_data::MATERIAL_DS_REFINE_BLESSED);
        PyModule_AddIntConstant(module_value, "MATERIAL_DS_REFINE_HOLLY", item_data::MATERIAL_DS_REFINE_HOLLY);

        PyModule_AddIntConstant(module_value, "METIN_NORMAL", item_data::METIN_NORMAL);
        PyModule_AddIntConstant(module_value, "METIN_GOLD", item_data::METIN_GOLD);

        PyModule_AddIntConstant(module_value, "LIMIT_NONE", item_data::LIMIT_NONE);
        PyModule_AddIntConstant(module_value, "LIMIT_LEVEL", item_data::LIMIT_LEVEL);
        PyModule_AddIntConstant(module_value, "LIMIT_STR", item_data::LIMIT_STR);
        PyModule_AddIntConstant(module_value, "LIMIT_DEX", item_data::LIMIT_DEX);
        PyModule_AddIntConstant(module_value, "LIMIT_INT", item_data::LIMIT_INT);
        PyModule_AddIntConstant(module_value, "LIMIT_CON", item_data::LIMIT_CON);
        PyModule_AddIntConstant(module_value, "LIMIT_PCBANG", item_data::LIMIT_PCBANG);
        PyModule_AddIntConstant(module_value, "LIMIT_REAL_TIME", item_data::LIMIT_REAL_TIME);
        PyModule_AddIntConstant(module_value, "LIMIT_REAL_TIME_START_FIRST_USE", item_data::LIMIT_REAL_TIME_START_FIRST_USE);
        PyModule_AddIntConstant(module_value, "LIMIT_TIMER_BASED_ON_WEAR", item_data::LIMIT_TIMER_BASED_ON_WEAR);
        PyModule_AddIntConstant(module_value, "LIMIT_TYPE_MAX_NUM", item_data::LIMIT_MAX_NUM);
        PyModule_AddIntConstant(module_value, "LIMIT_MAX_NUM", item_data::ITEM_LIMIT_MAX_NUM);

        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_FEMALE", item_data::ITEM_ANTIFLAG_FEMALE);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_MALE", item_data::ITEM_ANTIFLAG_MALE);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_WARRIOR", item_data::ITEM_ANTIFLAG_WARRIOR);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_ASSASSIN", item_data::ITEM_ANTIFLAG_ASSASSIN);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_SURA", item_data::ITEM_ANTIFLAG_SURA);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_SHAMAN", item_data::ITEM_ANTIFLAG_SHAMAN);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_GET", item_data::ITEM_ANTIFLAG_GET);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_DROP", item_data::ITEM_ANTIFLAG_DROP);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_SELL", item_data::ITEM_ANTIFLAG_SELL);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_EMPIRE_A", item_data::ITEM_ANTIFLAG_EMPIRE_A);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_EMPIRE_B", item_data::ITEM_ANTIFLAG_EMPIRE_B);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_EMPIRE_R", item_data::ITEM_ANTIFLAG_EMPIRE_R);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_SAVE", item_data::ITEM_ANTIFLAG_SAVE);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_GIVE", item_data::ITEM_ANTIFLAG_GIVE);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_PKDROP", item_data::ITEM_ANTIFLAG_PKDROP);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_STACK", item_data::ITEM_ANTIFLAG_STACK);
        PyModule_AddIntConstant(module_value, "ITEM_ANTIFLAG_MYSHOP", item_data::ITEM_ANTIFLAG_MYSHOP);

        PyModule_AddIntConstant(module_value, "ITEM_FLAG_RARE", item_data::ITEM_FLAG_RARE);
        PyModule_AddIntConstant(module_value, "ITEM_FLAG_UNIQUE", item_data::ITEM_FLAG_UNIQUE);
        PyModule_AddIntConstant(module_value, "ITEM_FLAG_CONFIRM_WHEN_USE", item_data::ITEM_FLAG_CONFIRM_WHEN_USE);

        PyModule_AddIntConstant(module_value, "ANTIFLAG_FEMALE", item_data::ITEM_ANTIFLAG_FEMALE);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_MALE", item_data::ITEM_ANTIFLAG_MALE);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_WARRIOR", item_data::ITEM_ANTIFLAG_WARRIOR);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_ASSASSIN", item_data::ITEM_ANTIFLAG_ASSASSIN);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_SURA", item_data::ITEM_ANTIFLAG_SURA);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_SHAMAN", item_data::ITEM_ANTIFLAG_SHAMAN);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_GET", item_data::ITEM_ANTIFLAG_GET);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_DROP", item_data::ITEM_ANTIFLAG_DROP);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_SELL", item_data::ITEM_ANTIFLAG_SELL);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_EMPIRE_A", item_data::ITEM_ANTIFLAG_EMPIRE_A);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_EMPIRE_B", item_data::ITEM_ANTIFLAG_EMPIRE_B);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_EMPIRE_R", item_data::ITEM_ANTIFLAG_EMPIRE_R);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_SAVE", item_data::ITEM_ANTIFLAG_SAVE);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_GIVE", item_data::ITEM_ANTIFLAG_GIVE);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_PKDROP", item_data::ITEM_ANTIFLAG_PKDROP);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_STACK", item_data::ITEM_ANTIFLAG_STACK);
        PyModule_AddIntConstant(module_value, "ANTIFLAG_MYSHOP", item_data::ITEM_ANTIFLAG_MYSHOP);

        PyModule_AddIntConstant(module_value, "WEARABLE_BODY", item_data::WEARABLE_BODY);
        PyModule_AddIntConstant(module_value, "WEARABLE_HEAD", item_data::WEARABLE_HEAD);
        PyModule_AddIntConstant(module_value, "WEARABLE_FOOTS", item_data::WEARABLE_FOOTS);
        PyModule_AddIntConstant(module_value, "WEARABLE_WRIST", item_data::WEARABLE_WRIST);
        PyModule_AddIntConstant(module_value, "WEARABLE_WEAPON", item_data::WEARABLE_WEAPON);
        PyModule_AddIntConstant(module_value, "WEARABLE_NECK", item_data::WEARABLE_NECK);
        PyModule_AddIntConstant(module_value, "WEARABLE_EAR", item_data::WEARABLE_EAR);
        PyModule_AddIntConstant(module_value, "WEARABLE_UNIQUE", item_data::WEARABLE_UNIQUE);
        PyModule_AddIntConstant(module_value, "WEARABLE_SHIELD", item_data::WEARABLE_SHIELD);
        PyModule_AddIntConstant(module_value, "WEARABLE_ARROW", item_data::WEARABLE_ARROW);

        PyModule_AddIntConstant(module_value, "ARMOR_BODY", item_data::ARMOR_BODY);
        PyModule_AddIntConstant(module_value, "ARMOR_HEAD", item_data::ARMOR_HEAD);
        PyModule_AddIntConstant(module_value, "ARMOR_SHIELD", item_data::ARMOR_SHIELD);
        PyModule_AddIntConstant(module_value, "ARMOR_WRIST", item_data::ARMOR_WRIST);
        PyModule_AddIntConstant(module_value, "ARMOR_FOOTS", item_data::ARMOR_FOOTS);
        PyModule_AddIntConstant(module_value, "ARMOR_NECK", item_data::ARMOR_NECK);
        PyModule_AddIntConstant(module_value, "ARMOR_EAR", item_data::ARMOR_EAR);

        PyModule_AddIntConstant(module_value, "ITEM_APPLY_MAX_NUM", item_data::ITEM_APPLY_MAX_NUM);
        PyModule_AddIntConstant(module_value, "ITEM_SOCKET_MAX_NUM", item_data::ITEM_SOCKET_MAX_NUM);

        PyModule_AddIntConstant(module_value, "APPLY_NONE", item_data::APPLY_NONE);
        PyModule_AddIntConstant(module_value, "APPLY_STR", item_data::APPLY_STR);
        PyModule_AddIntConstant(module_value, "APPLY_DEX", item_data::APPLY_DEX);
        PyModule_AddIntConstant(module_value, "APPLY_CON", item_data::APPLY_CON);
        PyModule_AddIntConstant(module_value, "APPLY_INT", item_data::APPLY_INT);
        PyModule_AddIntConstant(module_value, "APPLY_MAX_HP", item_data::APPLY_MAX_HP);
        PyModule_AddIntConstant(module_value, "APPLY_MAX_SP", item_data::APPLY_MAX_SP);
        PyModule_AddIntConstant(module_value, "APPLY_HP_REGEN", item_data::APPLY_HP_REGEN);
        PyModule_AddIntConstant(module_value, "APPLY_SP_REGEN", item_data::APPLY_SP_REGEN);
        PyModule_AddIntConstant(module_value, "APPLY_DEF_GRADE_BONUS", item_data::APPLY_DEF_GRADE_BONUS);
        PyModule_AddIntConstant(module_value, "APPLY_ATT_GRADE_BONUS", item_data::APPLY_ATT_GRADE_BONUS);
        PyModule_AddIntConstant(module_value, "APPLY_ATT_SPEED", item_data::APPLY_ATT_SPEED);
        PyModule_AddIntConstant(module_value, "APPLY_MOV_SPEED", item_data::APPLY_MOV_SPEED);
        PyModule_AddIntConstant(module_value, "APPLY_CAST_SPEED", item_data::APPLY_CAST_SPEED);
        PyModule_AddIntConstant(module_value, "APPLY_MAGIC_ATT_GRADE", item_data::APPLY_MAGIC_ATT_GRADE);
        PyModule_AddIntConstant(module_value, "APPLY_MAGIC_DEF_GRADE", item_data::APPLY_MAGIC_DEF_GRADE);
        PyModule_AddIntConstant(module_value, "APPLY_SKILL", item_data::APPLY_SKILL);
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_ANIMAL", item_data::APPLY_ATTBONUS_ANIMAL);
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_UNDEAD", item_data::APPLY_ATTBONUS_UNDEAD);
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_DEVIL", item_data::APPLY_ATTBONUS_DEVIL);
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_HUMAN", item_data::APPLY_ATTBONUS_HUMAN);
        PyModule_AddIntConstant(module_value, "APPLY_BOW_DISTANCE", item_data::APPLY_BOW_DISTANCE);
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_BOW", item_data::APPLY_RESIST_BOW);
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_FIRE", item_data::APPLY_RESIST_FIRE);
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_ELEC", item_data::APPLY_RESIST_ELEC);
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_MAGIC", item_data::APPLY_RESIST_MAGIC);
        PyModule_AddIntConstant(module_value, "APPLY_POISON_PCT", item_data::APPLY_POISON_PCT);
        PyModule_AddIntConstant(module_value, "APPLY_SLOW_PCT", item_data::APPLY_SLOW_PCT);
        PyModule_AddIntConstant(module_value, "APPLY_STUN_PCT", item_data::APPLY_STUN_PCT);
        PyModule_AddIntConstant(module_value, "APPLY_CRITICAL_PCT", item_data::APPLY_CRITICAL_PCT);           // n% chance of a double hit
        PyModule_AddIntConstant(module_value, "APPLY_PENETRATE_PCT", item_data::APPLY_PENETRATE_PCT);         // n% chance to ignore the enemy's defence
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_ORC", item_data::APPLY_ATTBONUS_ORC);           // n% more damage against orcs
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_MILGYO", item_data::APPLY_ATTBONUS_MILGYO);     // n% more damage against the esoteric
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_UNDEAD", item_data::APPLY_ATTBONUS_UNDEAD);     // n% more damage against undead
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_DEVIL", item_data::APPLY_ATTBONUS_DEVIL);       // n% more damage against devils
        PyModule_AddIntConstant(module_value, "APPLY_STEAL_HP", item_data::APPLY_STEAL_HP);                   // n% chance to absorb 10% of the damage as HP
        PyModule_AddIntConstant(module_value, "APPLY_STEAL_SP", item_data::APPLY_STEAL_SP);                   // n% chance to absorb 10% of the damage as SP
        PyModule_AddIntConstant(module_value, "APPLY_MANA_BURN_PCT", item_data::APPLY_MANA_BURN_PCT);         // n% chance to burn the enemy's mana
        PyModule_AddIntConstant(module_value, "APPLY_DAMAGE_SP_RECOVER", item_data::APPLY_DAMAGE_SP_RECOVER); // n% chance to recover 2 SP
        PyModule_AddIntConstant(module_value, "APPLY_BLOCK", item_data::APPLY_BLOCK);                         // n% chance to block a physical attack completely
        PyModule_AddIntConstant(module_value, "APPLY_DODGE", item_data::APPLY_DODGE);                         // n% chance to dodge a physical attack completely
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_SWORD", item_data::APPLY_RESIST_SWORD);           // n% less damage from one-handed swords
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_TWOHAND", item_data::APPLY_RESIST_TWOHAND);       // n% less damage from two-handed swords
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_DAGGER", item_data::APPLY_RESIST_DAGGER);         // n% less damage from daggers
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_BELL", item_data::APPLY_RESIST_BELL);             // n% less damage from bells
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_FAN", item_data::APPLY_RESIST_FAN);               // n% less damage from fans
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_WIND", item_data::APPLY_RESIST_WIND);             // n% less damage from wind
        PyModule_AddIntConstant(module_value, "APPLY_REFLECT_MELEE", item_data::APPLY_REFLECT_MELEE);         // reflects n% of melee damage to the enemy
        PyModule_AddIntConstant(module_value, "APPLY_REFLECT_CURSE", item_data::APPLY_REFLECT_CURSE);         // n% chance to reflect a curse back at its caster
        PyModule_AddIntConstant(module_value, "APPLY_POISON_REDUCE", item_data::APPLY_POISON_REDUCE);         // less poison damage
        PyModule_AddIntConstant(module_value, "APPLY_KILL_SP_RECOVER", item_data::APPLY_KILL_SP_RECOVER);     // n% chance to recover 10 SP when killing an enemy
        PyModule_AddIntConstant(module_value, "APPLY_EXP_DOUBLE_BONUS", item_data::APPLY_EXP_DOUBLE_BONUS);   // n% chance of double experience
        PyModule_AddIntConstant(module_value, "APPLY_GOLD_DOUBLE_BONUS", item_data::APPLY_GOLD_DOUBLE_BONUS); // n% chance of double gold
        PyModule_AddIntConstant(module_value, "APPLY_ITEM_DROP_BONUS", item_data::APPLY_ITEM_DROP_BONUS);     // n% chance of double item drops
        PyModule_AddIntConstant(module_value, "APPLY_POTION_BONUS", item_data::APPLY_POTION_BONUS);           // potions are n% stronger
        PyModule_AddIntConstant(module_value, "APPLY_KILL_HP_RECOVER", item_data::APPLY_KILL_HP_RECOVER);     // recovers HP on every kill
        PyModule_AddIntConstant(module_value, "APPLY_IMMUNE_STUN", item_data::APPLY_IMMUNE_STUN);             // cannot be stunned
        PyModule_AddIntConstant(module_value, "APPLY_IMMUNE_SLOW", item_data::APPLY_IMMUNE_SLOW);             // cannot be slowed
        PyModule_AddIntConstant(module_value, "APPLY_IMMUNE_FALL", item_data::APPLY_IMMUNE_FALL);             // cannot be knocked down
        PyModule_AddIntConstant(module_value, "APPLY_MAX_STAMINA", item_data::APPLY_MAX_STAMINA);             // more maximum stamina
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_WARRIOR", item_data::APPLY_ATT_BONUS_TO_WARRIOR);
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_ASSASSIN", item_data::APPLY_ATT_BONUS_TO_ASSASSIN);
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_SURA", item_data::APPLY_ATT_BONUS_TO_SURA);
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_SHAMAN", item_data::APPLY_ATT_BONUS_TO_SHAMAN);
        PyModule_AddIntConstant(module_value, "APPLY_ATTBONUS_MONSTER", item_data::APPLY_ATT_BONUS_TO_MONSTER);
        PyModule_AddIntConstant(module_value, "APPLY_MALL_ATTBONUS", item_data::APPLY_MALL_ATTBONUS);
        PyModule_AddIntConstant(module_value, "APPLY_MALL_DEFBONUS", item_data::APPLY_MALL_DEFBONUS);
        PyModule_AddIntConstant(module_value, "APPLY_MALL_EXPBONUS", item_data::APPLY_MALL_EXPBONUS);
        PyModule_AddIntConstant(module_value, "APPLY_MALL_ITEMBONUS", item_data::APPLY_MALL_ITEMBONUS);
        PyModule_AddIntConstant(module_value, "APPLY_MALL_GOLDBONUS", item_data::APPLY_MALL_GOLDBONUS);
        PyModule_AddIntConstant(module_value, "APPLY_MAX_HP_PCT", item_data::APPLY_MAX_HP_PCT);
        PyModule_AddIntConstant(module_value, "APPLY_MAX_SP_PCT", item_data::APPLY_MAX_SP_PCT);
        PyModule_AddIntConstant(module_value, "APPLY_SKILL_DAMAGE_BONUS", item_data::APPLY_SKILL_DAMAGE_BONUS);
        PyModule_AddIntConstant(module_value, "APPLY_NORMAL_HIT_DAMAGE_BONUS", item_data::APPLY_NORMAL_HIT_DAMAGE_BONUS);
        PyModule_AddIntConstant(module_value, "APPLY_SKILL_DEFEND_BONUS", item_data::APPLY_SKILL_DEFEND_BONUS);
        PyModule_AddIntConstant(module_value, "APPLY_NORMAL_HIT_DEFEND_BONUS", item_data::APPLY_NORMAL_HIT_DEFEND_BONUS);

        PyModule_AddIntConstant(module_value, "APPLY_PC_BANG_EXP_BONUS", item_data::APPLY_PC_BANG_EXP_BONUS);
        PyModule_AddIntConstant(module_value, "APPLY_PC_BANG_DROP_BONUS", item_data::APPLY_PC_BANG_DROP_BONUS);

        PyModule_AddIntConstant(module_value, "APPLY_RESIST_WARRIOR", item_data::APPLY_RESIST_WARRIOR);
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_ASSASSIN", item_data::APPLY_RESIST_ASSASSIN);
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_SURA", item_data::APPLY_RESIST_SURA);
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_SHAMAN", item_data::APPLY_RESIST_SHAMAN);
        PyModule_AddIntConstant(module_value, "APPLY_ENERGY", item_data::APPLY_ENERGY);
        PyModule_AddIntConstant(module_value, "APPLY_COSTUME_ATTR_BONUS", item_data::APPLY_COSTUME_ATTR_BONUS);

        PyModule_AddIntConstant(module_value, "APPLY_MAGIC_ATTBONUS_PER", item_data::APPLY_MAGIC_ATTBONUS_PER);
        PyModule_AddIntConstant(module_value, "APPLY_MELEE_MAGIC_ATTBONUS_PER", item_data::APPLY_MELEE_MAGIC_ATTBONUS_PER);
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_ICE", item_data::APPLY_RESIST_ICE);
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_EARTH", item_data::APPLY_RESIST_EARTH);
        PyModule_AddIntConstant(module_value, "APPLY_RESIST_DARK", item_data::APPLY_RESIST_DARK);
        PyModule_AddIntConstant(module_value, "APPLY_ANTI_CRITICAL_PCT", item_data::APPLY_ANTI_CRITICAL_PCT);
        PyModule_AddIntConstant(module_value, "APPLY_ANTI_PENETRATE_PCT", item_data::APPLY_ANTI_PENETRATE_PCT);
    }
}
