#include <metin2/client/characters/python_character_manager.h>
#include <metin2/client/inventory/python_item.h>
#include <metin2/client/trade/python_exchange.h>
#include <metin2/client/trade/python_safe_box.h>
#include <metin2/client/trade/python_shop.h>
#include <metin2/log/log.h>
#include "python_network_stream.h"

#include <metin2/client/app/abstract_player.h>
#include <metin2/script/python_utils.h>

namespace metin2::inline client::inline protocol {

    auto python_network_stream::send_safe_box_money_packet(uint8_t state, uint32_t money) -> bool {
        assert(!"CPythonNetworkStream::SendSafeBoxMoneyPacket - unused function");
        return false;
    }

    auto python_network_stream::send_safe_box_checkin_packet(item_pos inventory_pos, uint8_t safe_box_pos) -> bool {
        play_inventory_item_drop_sound(inventory_pos);

        packet_cg_safebox_checkin safebox_checkin;
        safebox_checkin.header_value = HEADER_CG_SAFEBOX_CHECKIN;
        safebox_checkin.item_pos_value = inventory_pos;
        safebox_checkin.safe_pos = safe_box_pos;
        if (!send(sizeof(safebox_checkin), &safebox_checkin))
            return false;

        return send_sequence();
    }

    auto python_network_stream::send_safe_box_checkout_packet(uint8_t safe_box_pos, item_pos inventory_pos) -> bool {
        play_safe_box_item_drop_sound(safe_box_pos);

        packet_cg_safebox_checkout safebox_checkout;
        safebox_checkout.header_value = HEADER_CG_SAFEBOX_CHECKOUT;
        safebox_checkout.safe_pos = safe_box_pos;
        safebox_checkout.item_pos_value = inventory_pos;
        if (!send(sizeof(safebox_checkout), &safebox_checkout))
            return false;

        return send_sequence();
    }

    auto python_network_stream::send_safe_box_item_move_packet(uint8_t source_pos, uint8_t target_pos, uint8_t count) -> bool {
        play_safe_box_item_drop_sound(source_pos);

        packet_cg_item_move item_move;
        item_move.header_value = HEADER_CG_SAFEBOX_ITEM_MOVE;
        item_move.pos = item_pos(INVENTORY, source_pos);
        item_move.num = count;
        item_move.change_pos = item_pos(INVENTORY, target_pos);
        if (!send(sizeof(item_move), &item_move))
            return false;

        return send_sequence();
    }

    auto python_network_stream::recv_safe_box_set_packet() -> bool {
        packet_gc_item_set2 item_set;
        if (!recv(sizeof(item_set), &item_set))
            return false;

        packet_item item_data;
        item_data.vnum = item_set.vnum;
        item_data.count = item_set.count;
        item_data.flags_value = item_set.flags_value;
        item_data.anti_flags = item_set.anti_flags;
        for (int isocket = 0; isocket < ITEM_SOCKET_SLOT_MAX_NUM; ++isocket)
            item_data.sockets[isocket] = item_set.sockets[isocket];
        for (int iattr = 0; iattr < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++iattr)
            item_data.attr[iattr] = item_set.attr[iattr];

        get_python_safe_box().set_item_data(item_set.cell.cell, item_data);

        refresh_safebox_window();

        return true;
    }

    auto python_network_stream::recv_safe_box_del_packet() -> bool {
        packet_gc_item_del item_del;
        if (!recv(sizeof(item_del), &item_del))
            return false;

        get_python_safe_box().del_item_data(item_del.pos);

        refresh_safebox_window();

        return true;
    }

    auto python_network_stream::recv_safe_box_wrong_password_packet() -> bool {
        packet_gc_safebox_wrong_password safebox_wrong_password;

        if (!recv(sizeof(safebox_wrong_password), &safebox_wrong_password))
            return false;

        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "OnSafeBoxError", Py_BuildValue("()"));

        return true;
    }

    auto python_network_stream::recv_safe_box_size_packet() -> bool {
        packet_gc_safebox_size safe_box_size;
        if (!recv(sizeof(safe_box_size), &safe_box_size))
            return false;

        get_python_safe_box().open_safe_box(safe_box_size.size);
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "OpenSafeboxWindow", Py_BuildValue("(i)", safe_box_size.size));

        return true;
    }

    auto python_network_stream::send_mall_checkout_packet(uint8_t mall_pos, item_pos inventory_pos) -> bool {
        play_mall_item_drop_sound(mall_pos);

        packet_cg_mall_checkout mall_checkout_packet;
        mall_checkout_packet.header_value = HEADER_CG_MALL_CHECKOUT;
        mall_checkout_packet.mall_pos = mall_pos;
        mall_checkout_packet.item_pos_value = inventory_pos;
        if (!send(sizeof(mall_checkout_packet), &mall_checkout_packet))
            return false;

        return send_sequence();
    }

    auto python_network_stream::recv_mall_open_packet() -> bool {
        packet_gc_mall_open mall_open;
        if (!recv(sizeof(mall_open), &mall_open))
            return false;

        get_python_safe_box().open_mall(mall_open.size);
        py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "OpenMallWindow", Py_BuildValue("(i)", mall_open.size));

        return true;
    }
    auto python_network_stream::recv_mall_item_set_packet() -> bool {
        packet_gc_item_set2 item_set;
        if (!recv(sizeof(item_set), &item_set))
            return false;

        packet_item item_data;
        item_data.vnum = item_set.vnum;
        item_data.count = item_set.count;
        item_data.flags_value = item_set.flags_value;
        item_data.anti_flags = item_set.anti_flags;
        for (int isocket = 0; isocket < ITEM_SOCKET_SLOT_MAX_NUM; ++isocket)
            item_data.sockets[isocket] = item_set.sockets[isocket];
        for (int iattr = 0; iattr < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++iattr)
            item_data.attr[iattr] = item_set.attr[iattr];

        get_python_safe_box().set_mall_item_data(item_set.cell.cell, item_data);

        refresh_mall_window();

        return true;
    }
    auto python_network_stream::recv_mall_item_del_packet() -> bool {
        packet_gc_item_del item_del;
        if (!recv(sizeof(item_del), &item_del))
            return false;

        get_python_safe_box().del_mall_item_data(item_del.pos);

        refresh_mall_window();
        metin2::log::debug("Mall item removed");

        return true;
    }

    auto python_network_stream::recv_item_set_packet() -> bool {
        packet_gc_item_set packet_item_set;

        if (!recv(sizeof(packet_gc_item_set), &packet_item_set))
            return false;

        packet_item item_data;
        item_data.vnum = packet_item_set.vnum;
        item_data.count = packet_item_set.count;
        item_data.flags_value = 0;
        for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
            item_data.sockets[i] = packet_item_set.sockets[i];
        for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j)
            item_data.attr[j] = packet_item_set.attr[j];

        abstract_player &player = get_abstract_player();

        player.set_item_data(packet_item_set.cell, item_data);

        refresh_inventory_window();
        return true;
    }

    auto python_network_stream::recv_item_set_packet2() -> bool {
        packet_gc_item_set2 packet_item_set;

        if (!recv(sizeof(packet_gc_item_set2), &packet_item_set))
            return false;

        packet_item item_data;
        item_data.vnum = packet_item_set.vnum;
        item_data.count = packet_item_set.count;
        item_data.flags_value = packet_item_set.flags_value;
        item_data.anti_flags = packet_item_set.anti_flags;

        for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
            item_data.sockets[i] = packet_item_set.sockets[i];
        for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j)
            item_data.attr[j] = packet_item_set.attr[j];

        abstract_player &player = get_abstract_player();
        player.set_item_data(packet_item_set.cell, item_data);

        if (packet_item_set.highlight)
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "BINARY_Highlight_Item", Py_BuildValue("(ii)", packet_item_set.cell.window_type, packet_item_set.cell.cell));

        refresh_inventory_window();
        return true;
    }

    auto python_network_stream::recv_item_update_packet() -> bool {
        packet_gc_item_update packet_item_update;

        if (!recv(sizeof(packet_gc_item_update), &packet_item_update))
            return false;

        abstract_player &player = get_abstract_player();
        player.set_item_count(packet_item_update.cell, packet_item_update.count);
        for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
            player.set_item_metin_socket(packet_item_update.cell, i, packet_item_update.sockets[i]);
        for (int j = 0; j < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++j)
            player.set_item_attribute(packet_item_update.cell, j, packet_item_update.attr[j].type_value, packet_item_update.attr[j].value);

        refresh_inventory_window();
        return true;
    }

    auto python_network_stream::recv_item_ground_add_packet() -> bool {
        packet_gc_item_ground_add packet_item_ground_add;

        if (!recv(sizeof(packet_gc_item_ground_add), &packet_item_ground_add))
            return false;

        global_position_to_local_position_impl(packet_item_ground_add.x, packet_item_ground_add.y);

        get_python_item().create_item(packet_item_ground_add.vid,
            packet_item_ground_add.vnum,
            packet_item_ground_add.x,
            packet_item_ground_add.y,
            packet_item_ground_add.z);
        return true;
    }

    auto python_network_stream::recv_item_ownership() -> bool {
        packet_gc_item_ownership p;

        if (!recv(sizeof(packet_gc_item_ownership), &p))
            return false;

        get_python_item().set_ownership(p.vid, packet_string(p.name_value).c_str());
        return true;
    }

    auto python_network_stream::recv_item_ground_del_packet() -> bool {
        packet_gc_item_ground_del packet_item_ground_del;

        if (!recv(sizeof(packet_gc_item_ground_del), &packet_item_ground_del))
            return false;

        get_python_item().delete_item(packet_item_ground_del.vid);
        return true;
    }

    auto python_network_stream::recv_quick_slot_add_packet() -> bool {
        packet_gc_quick_slot_add packet_quick_slot_add;

        if (!recv(sizeof(packet_gc_quick_slot_add), &packet_quick_slot_add))
            return false;

        abstract_player &player = get_abstract_player();
        player.add_quick_slot(packet_quick_slot_add.pos, packet_quick_slot_add.slot_value.type_value, packet_quick_slot_add.slot_value.position_value);

        refresh_inventory_window();

        return true;
    }

    auto python_network_stream::recv_quick_slot_del_packet() -> bool {
        packet_gc_quick_slot_del packet_quick_slot_del;

        if (!recv(sizeof(packet_gc_quick_slot_del), &packet_quick_slot_del))
            return false;

        abstract_player &player = get_abstract_player();
        player.delete_quick_slot(packet_quick_slot_del.pos);

        refresh_inventory_window();

        return true;
    }

    auto python_network_stream::recv_quick_slot_move_packet() -> bool {
        packet_gc_quick_slot_swap packet_quick_slot_swap;

        if (!recv(sizeof(packet_gc_quick_slot_swap), &packet_quick_slot_swap))
            return false;

        abstract_player &player = get_abstract_player();
        player.move_quick_slot(packet_quick_slot_swap.pos, packet_quick_slot_swap.change_pos);

        refresh_inventory_window();

        return true;
    }

    auto python_network_stream::send_shop_end_packet() -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_shop packet_shop;
        packet_shop.header_value = HEADER_CG_SHOP;
        packet_shop.subheader = SHOP_SUBHEADER_CG_END;

        if (!send(sizeof(packet_shop), &packet_shop)) {
            metin2::log::debug("Cannot send the shop end packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_shop_buy_packet(uint8_t pos) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_shop packet_shop;
        packet_shop.header_value = HEADER_CG_SHOP;
        packet_shop.subheader = SHOP_SUBHEADER_CG_BUY;

        if (!send(sizeof(packet_cg_shop), &packet_shop)) {
            metin2::log::debug("Cannot send the shop buy packet");
            return false;
        }

        uint8_t count = 1;
        if (!send(sizeof(uint8_t), &count)) {
            metin2::log::debug("Cannot send the shop buy packet");
            return false;
        }

        if (!send(sizeof(uint8_t), &pos)) {
            metin2::log::debug("Cannot send the shop buy packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_shop_sell_packet(uint8_t slot) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_shop packet_shop;
        packet_shop.header_value = HEADER_CG_SHOP;
        packet_shop.subheader = SHOP_SUBHEADER_CG_SELL;

        if (!send(sizeof(packet_cg_shop), &packet_shop)) {
            metin2::log::debug("Cannot send the shop sell packet");
            return false;
        }
        if (!send(sizeof(uint8_t), &slot)) {
            metin2::log::debug("Cannot send the shop sell slot packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_shop_sell_packet_new(uint8_t slot, uint8_t count) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_shop packet_shop;
        packet_shop.header_value = HEADER_CG_SHOP;
        packet_shop.subheader = SHOP_SUBHEADER_CG_SELL2;

        if (!send(sizeof(packet_cg_shop), &packet_shop)) {
            metin2::log::debug("Cannot send the shop sell packet");
            return false;
        }
        if (!send(sizeof(uint8_t), &slot)) {
            metin2::log::debug("Cannot send the shop sell slot packet");
            return false;
        }
        if (!send(sizeof(uint8_t), &count)) {
            metin2::log::debug("Cannot send the shop sell count packet");
            return false;
        }

        metin2::log::debug("Sent shop sell request (slot {}, count {})", slot, count);

        return send_sequence();
    }

    auto python_network_stream::send_item_use_packet(item_pos pos) -> bool {
        if (!can_act_main_instance())
            return true;

        if (is_equip_item_in_slot_impl(pos)) {
            if (get_python_exchange().is_trading()) {
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", Py_BuildValue("(s)", "CANNOT_EQUIP_EXCHANGE"));
                return true;
            }

            if (get_python_shop().is_open()) {
                py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", Py_BuildValue("(s)", "CANNOT_EQUIP_SHOP"));
                return true;
            }

            if (is_player_attacking())
                return true;
        }

        play_inventory_item_use_sound(pos);

        packet_cg_item_use item_use_packet;
        item_use_packet.header_value = HEADER_CG_ITEM_USE;
        item_use_packet.pos = pos;

        if (!send(sizeof(packet_cg_item_use), &item_use_packet)) {
            metin2::log::debug("Cannot send the item use packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_item_use_to_item_packet(item_pos source_pos, item_pos target_pos) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_item_use_to_item item_use_to_item_packet;
        item_use_to_item_packet.header_value = HEADER_CG_ITEM_USE_TO_ITEM;
        item_use_to_item_packet.source_pos = source_pos;
        item_use_to_item_packet.target_pos = target_pos;

        if (!send(sizeof(packet_cg_item_use_to_item), &item_use_to_item_packet)) {
            metin2::log::debug("Cannot send the item use on item packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_item_drop_packet(item_pos pos, uint32_t elk) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_item_drop item_drop_packet;
        item_drop_packet.header_value = HEADER_CG_ITEM_DROP;
        item_drop_packet.pos = pos;
        item_drop_packet.elk = elk;

        if (!send(sizeof(packet_cg_item_drop), &item_drop_packet)) {
            metin2::log::debug("Cannot send the item drop packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_item_drop_packet_new(item_pos pos, uint32_t elk, uint32_t count) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_item_drop2 item_drop_packet;
        item_drop_packet.header_value = HEADER_CG_ITEM_DROP2;
        item_drop_packet.pos = pos;
        item_drop_packet.gold = elk;
        item_drop_packet.count = count;

        if (!send(sizeof(item_drop_packet), &item_drop_packet)) {
            metin2::log::debug("Cannot send the item drop packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::is_equip_item_in_slot_impl(item_pos slot_pos) -> bool {
        abstract_player &player = get_abstract_player();
        return player.is_equip_item_in_slot(slot_pos);
    }

    void python_network_stream::play_inventory_item_use_sound(item_pos slot_pos) {
        abstract_player &player = get_abstract_player();
        uint32_t item_id = player.item_index(slot_pos);

        python_item &item = get_python_item();
        item.play_use_sound(item_id);
    }

    void python_network_stream::play_inventory_item_drop_sound(item_pos slot_pos) {
        abstract_player &player = get_abstract_player();
        uint32_t item_id = player.item_index(slot_pos);

        python_item &item = get_python_item();
        item.play_drop_sound(item_id);
    }

    void python_network_stream::play_safe_box_item_drop_sound(uint32_t slot_pos) {
        uint32_t item_id;
        python_safe_box &safe_box = get_python_safe_box();
        if (!safe_box.slot_item_id(slot_pos, &item_id))
            return;

        python_item &item = get_python_item();
        item.play_drop_sound(item_id);
    }

    void python_network_stream::play_mall_item_drop_sound(uint32_t slot_pos) {
        uint32_t item_id;
        python_safe_box &safe_box = get_python_safe_box();
        if (!safe_box.slot_mall_item_id(slot_pos, &item_id))
            return;

        python_item &item = get_python_item();
        item.play_drop_sound(item_id);
    }

    auto python_network_stream::send_item_move_packet(item_pos pos, item_pos change_pos, uint8_t num) -> bool {
        if (!can_act_main_instance())
            return true;

        if (is_equip_item_in_slot_impl(pos)) {
            if (get_python_exchange().is_trading()) {
                if (pos.is_equip_cell() || change_pos.is_equip_cell()) {
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", Py_BuildValue("(s)", "CANNOT_EQUIP_EXCHANGE"));
                    return true;
                }
            }

            if (get_python_shop().is_open()) {
                if (pos.is_equip_cell() || change_pos.is_equip_cell()) {
                    py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", Py_BuildValue("(s)", "CANNOT_EQUIP_SHOP"));
                    return true;
                }
            }

            if (is_player_attacking())
                return true;
        }

        play_inventory_item_drop_sound(pos);

        packet_cg_item_move item_move_packet;
        item_move_packet.header_value = HEADER_CG_ITEM_MOVE;
        item_move_packet.pos = pos;
        item_move_packet.change_pos = change_pos;
        item_move_packet.num = num;

        if (!send(sizeof(packet_cg_item_move), &item_move_packet)) {
            metin2::log::debug("Cannot send the item move packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_item_pick_up_packet(uint32_t vid) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_item_pick_up item_pick_up_packet;
        item_pick_up_packet.header_value = HEADER_CG_ITEM_PICKUP;
        item_pick_up_packet.vid = vid;

        if (!send(sizeof(packet_cg_item_pick_up), &item_pick_up_packet)) {
            metin2::log::debug("Cannot send the item pick up packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_quick_slot_add_packet(uint8_t wpos, uint8_t type, uint8_t pos) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_quick_slot_add quick_slot_add_packet;

        quick_slot_add_packet.header_value = HEADER_CG_QUICKSLOT_ADD;
        quick_slot_add_packet.pos = wpos;
        quick_slot_add_packet.slot_value.type_value = type;
        quick_slot_add_packet.slot_value.position_value = pos;

        if (!send(sizeof(packet_cg_quick_slot_add), &quick_slot_add_packet)) {
            metin2::log::debug("Cannot send the quick slot add packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_quick_slot_del_packet(uint8_t pos) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_quick_slot_del quick_slot_del_packet;

        quick_slot_del_packet.header_value = HEADER_CG_QUICKSLOT_DEL;
        quick_slot_del_packet.pos = pos;

        if (!send(sizeof(packet_cg_quick_slot_del), &quick_slot_del_packet)) {
            metin2::log::debug("Cannot send the quick slot remove packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::send_quick_slot_move_packet(uint8_t pos, uint8_t change_pos) -> bool {
        if (!can_act_main_instance())
            return true;

        packet_cg_quick_slot_swap quick_slot_swap_packet;

        quick_slot_swap_packet.header_value = HEADER_CG_QUICKSLOT_SWAP;
        quick_slot_swap_packet.pos = pos;
        quick_slot_swap_packet.change_pos = change_pos;

        if (!send(sizeof(packet_cg_quick_slot_swap), &quick_slot_swap_packet)) {
            metin2::log::debug("Cannot send the quick slot swap packet");
            return false;
        }

        return send_sequence();
    }

    auto python_network_stream::recv_special_effect() -> bool {
        packet_gc_special_effect special_effect;
        if (!recv(sizeof(special_effect), &special_effect))
            return false;

        uint32_t effect = -1;
        bool play_potion_sound = false;
        bool attach_effect = true; // attach the effect to the character instead of playing it in place
        switch (special_effect.type_value) {
        case SE_HPUP_RED:
            effect = instance_base::EFFECT_HPUP_RED;
            play_potion_sound = true;
            break;
        case SE_SPUP_BLUE:
            effect = instance_base::EFFECT_SPUP_BLUE;
            play_potion_sound = true;
            break;
        case SE_SPEEDUP_GREEN:
            effect = instance_base::EFFECT_SPEEDUP_GREEN;
            play_potion_sound = true;
            break;
        case SE_DXUP_PURPLE:
            effect = instance_base::EFFECT_DXUP_PURPLE;
            play_potion_sound = true;
            break;
        case SE_CRITICAL:
            effect = instance_base::EFFECT_CRITICAL;
            break;
        case SE_PENETRATE:
            effect = instance_base::EFFECT_PENETRATE;
            break;
        case SE_BLOCK:
            effect = instance_base::EFFECT_BLOCK;
            break;
        case SE_DODGE:
            effect = instance_base::EFFECT_DODGE;
            break;
        case SE_CHINA_FIREWORK:
            effect = instance_base::EFFECT_FIRECRACKER;
            attach_effect = false;
            break;
        case SE_SPIN_TOP:
            effect = instance_base::EFFECT_SPIN_TOP;
            attach_effect = false;
            break;
        case SE_SUCCESS:
            effect = instance_base::EFFECT_SUCCESS;
            attach_effect = false;
            break;
        case SE_FAIL:
            effect = instance_base::EFFECT_FAIL;
            break;
        case SE_FR_SUCCESS:
            effect = instance_base::EFFECT_FR_SUCCESS;
            attach_effect = false;
            break;
        case SE_LEVELUP_ON_14_FOR_GERMANY: // level up at 14 (Germany only)
            effect = instance_base::EFFECT_LEVELUP_ON_14_FOR_GERMANY;
            attach_effect = false;
            break;
        case SE_LEVELUP_UNDER_15_FOR_GERMANY: // level up at 15 (Germany only)
            effect = instance_base::EFFECT_LEVELUP_UNDER_15_FOR_GERMANY;
            attach_effect = false;
            break;
        case SE_PERCENT_DAMAGE1:
            effect = instance_base::EFFECT_PERCENT_DAMAGE1;
            break;
        case SE_PERCENT_DAMAGE2:
            effect = instance_base::EFFECT_PERCENT_DAMAGE2;
            break;
        case SE_PERCENT_DAMAGE3:
            effect = instance_base::EFFECT_PERCENT_DAMAGE3;
            break;
        case SE_AUTO_HPUP:
            effect = instance_base::EFFECT_AUTO_HPUP;
            break;
        case SE_AUTO_SPUP:
            effect = instance_base::EFFECT_AUTO_SPUP;
            break;
        case SE_EQUIP_RAMADAN_RING:
            effect = instance_base::EFFECT_RAMADAN_RING_EQUIP;
            break;
        case SE_EQUIP_HALLOWEEN_CANDY:
            effect = instance_base::EFFECT_HALLOWEEN_CANDY_EQUIP;
            break;
        case SE_EQUIP_HAPPINESS_RING:
            effect = instance_base::EFFECT_HAPPINESS_RING_EQUIP;
            break;
        case SE_EQUIP_LOVE_PENDANT:
            effect = instance_base::EFFECT_LOVE_PENDANT_EQUIP;
            break;

        default:
            metin2::log::warn("Unknown special effect {} is skipped", special_effect.type_value);
            break;
        }

        if (play_potion_sound) {
            abstract_player &player = get_abstract_player();
            if (player.is_main_character_index(special_effect.vid)) {
                python_item &item = get_python_item();
                item.play_use_potion_sound();
            }
        }

        if (uint32_t(-1) != effect) {
            instance_base *instance = get_python_character_manager().instance_ptr(special_effect.vid);
            if (instance) {
                if (attach_effect)
                    instance->attach_special_effect(effect);
                else
                    instance->create_special_effect(effect);
            }
        }

        return true;
    }

    auto python_network_stream::recv_specific_effect() -> bool {
        packet_gc_specific_effect specific_effect;
        if (!recv(sizeof(specific_effect), &specific_effect))
            return false;

        instance_base *instance = get_python_character_manager().instance_ptr(specific_effect.vid);
        if (instance) {
            instance_base::register_effect(instance_base::EFFECT_TEMP, "", packet_string(specific_effect.effect_file).c_str(), false);
            instance->attach_special_effect(instance_base::EFFECT_TEMP);
        }

        return true;
    }

    auto python_network_stream::recv_dragon_soul_refine() -> bool {
        packet_gc_dragon_soul_refine dragon_soul;

        if (!recv(sizeof(dragon_soul), &dragon_soul))
            return false;

        switch (dragon_soul.sub_type) {
        case DS_SUB_HEADER_OPEN:
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "BINARY_DragonSoulRefineWindow_Open", Py_BuildValue("()"));
            break;
        case DS_SUB_HEADER_REFINE_FAIL:
        case DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE:
        case DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL:
        case DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY:
        case DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL:
        case DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL:
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "BINARY_DragonSoulRefineWindow_RefineFail", Py_BuildValue("(iii)", dragon_soul.sub_type, dragon_soul.pos.window_type, dragon_soul.pos.cell));
            break;
        case DS_SUB_HEADER_REFINE_SUCCEED:
            py_call_class_member_func(apo_phase_wnd_[PHASE_WINDOW_GAME], "BINARY_DragonSoulRefineWindow_RefineSucceed",
                Py_BuildValue("(ii)", dragon_soul.pos.window_type, dragon_soul.pos.cell));
            break;
        }

        return true;
    }
}
