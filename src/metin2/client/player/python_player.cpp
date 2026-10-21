#include "python_player_event_handler.h"

#include <metin2/client/app/abstract_player.h>
#include <metin2/client/app/locale.h>
#include <metin2/client/app/python_application.h>
#include <metin2/client/background/python_background.h>
#include <metin2/client/inventory/python_item.h>
#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/actors/game_util.h>

namespace metin2::inline client::inline player {

    enum {
        MAIN_RACE_WARRIOR_M,
        MAIN_RACE_ASSASSIN_W,
        MAIN_RACE_SURA_M,
        MAIN_RACE_SHAMAN_W,
        MAIN_RACE_WARRIOR_W,
        MAIN_RACE_ASSASSIN_M,
        MAIN_RACE_SURA_W,
        MAIN_RACE_SHAMAN_M,
        MAIN_RACE_MAX_NUM,
    };

    const uint32_t POINT_MAGIC_NUMBER = 0xe73ac1da;

    void python_player::player_status::set_point(uint32_t e_point, int32_t l_point) {
        point[e_point] = l_point ^ POINT_MAGIC_NUMBER;
    }

    auto python_player::player_status::get_point(uint32_t point_value) -> int32_t {
        return point[point_value] ^ POINT_MAGIC_NUMBER;
    }

    auto python_player::affect_index_to_skill_index(uint32_t affect_index, uint32_t *skill_index) -> bool {
        if (map_affect_index_to_skill_index_.end() == map_affect_index_to_skill_index_.find(affect_index))
            return false;

        *skill_index = map_affect_index_to_skill_index_[affect_index];
        return true;
    }

    auto python_player::picked_actor_ptr(instance_base **ppk_inst_picked) -> bool {
        python_character_manager &chr_mgr = get_python_character_manager();
        instance_base *pk_inst_picked = chr_mgr.old_get_picked_instance_ptr();
        if (!pk_inst_picked)
            return false;

        *ppk_inst_picked = pk_inst_picked;
        return true;
    }

    auto python_player::picked_actor_id(uint32_t *actor_id) -> bool {
        python_character_manager &chr_mgr = get_python_character_manager();
        return chr_mgr.old_get_picked_instance_vid(actor_id);
    }

    auto python_player::picked_item_id_impl(uint32_t *item_id) -> bool {
        python_item &item_mgr = get_python_item();
        return item_mgr.picked_item_id(item_id);
    }

    auto python_player::picked_ground_pos(pixel_position *pk_p_pos_picked) -> bool {
        python_background &bg = get_python_background();

        if (bg.picking_point(pk_p_pos_picked)) {
            pk_p_pos_picked->y = -pk_p_pos_picked->y;
            return true;
        }

        return false;
    }

    void python_player::new_get_main_actor_position(pixel_position *p_pos_actor) {
        abstract_player &player = get_abstract_player();
        instance_base *instance = player.new_get_main_actor_ptr();
        if (instance) {
            instance->new_get_pixel_position(p_pos_actor);
        } else {
            get_python_application().center_position(p_pos_actor);
        }
    }

    auto python_player::register_effect(uint32_t eid, const char *file_name, bool is_cache) -> bool {
        if (eid >= EFFECT_NUM)
            return false;

        effect_manager &eft_mgr = get_effect_manager();
        eft_mgr.register_effect2(file_name, &effect_value_[eid], is_cache);
        return true;
    }

    void python_player::new_show_effect(int eid, pixel_position p_pos_dst) {
        if (eid >= EFFECT_NUM)
            return;

        math::vector3 position_vector(p_pos_dst.x, -p_pos_dst.y, p_pos_dst.z);
        math::vector3 direction_vector(0.0f, 0.0f, 1.0f);

        effect_manager &eft_mgr = get_effect_manager();
        eft_mgr.create_effect(effect_value_[eid], position_vector, direction_vector);
    }

    auto python_player::new_find_actor_ptr(uint32_t vid) -> instance_base * {
        python_character_manager &chr_mgr = get_python_character_manager();
        return chr_mgr.instance_ptr(vid);
    }

    auto python_player::new_get_main_actor_ptr() -> instance_base * {
        return new_find_actor_ptr(main_character_index_);
    }

    void python_player::update() {
        new_refresh_mouse_walking_direction();

        python_player_event_handler &player_event_handler = get_python_player_event_handler();
        player_event_handler.flush_victim_list();

        if (is_dest_position_) {
            instance_base *instance = new_get_main_actor_ptr();
            if (instance) {
                pixel_position pixel_position;
                instance->new_get_pixel_position(&pixel_position);

                if (abs(int(pixel_position.x) - ix_dest_pos_) + abs(int(pixel_position.y) - iy_dest_pos_) < 10000) {
                    is_dest_position_ = false;
                } else {
                    if (get_timer().current_millisecond() - last_alarm_time_ > 20000) {
                        alarm_have_to_go();
                    }
                }
            }
        }

        if (is_consuming_stamina_) {
            float elapsed_time = get_timer().elapsed_second();
            current_stamina_ -= (elapsed_time * consume_stamina_per_sec_);

            set_status(POINT_STAMINA, uint32_t(current_stamina_));

            py_call_class_member_func(ppy_game_window_, "RefreshStamina", Py_BuildValue("()"));
        }

        update_auto_attack();
        update_notify_guild_area_event();
    }

    auto python_player::is_using_charge_skill() -> bool {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return false;

        if (check_dash_affect(*inst_main))
            return true;

        if (MODE_USE_SKILL != reserved_mode_)
            return false;

        if (skill_slot_index_reserved_ >= SKILL_MAX_NUM)
            return false;

        skill_instance &skill_inst = player_status_value_.skill[skill_slot_index_reserved_];

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_inst.index_value, &skill_data))
            return false;

        return skill_data->is_charge_skill() ? true : false;
    }

    void python_player::update_auto_attack() {
        if (0 == auto_attack_target_vid_)
            return;

        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return;

        // Skipped while charging with Tanhwangyeok.
        if (is_using_charge_skill())
            return;

        instance_base *inst_victim = new_find_actor_ptr(auto_attack_target_vid_);
        if (!inst_victim) {
            clear_auto_attack_target_actor_id();
        } else {
            if (inst_victim->is_dead()) {
                clear_auto_attack_target_actor_id();
            } else if (inst_main->is_mounting_horse() && !inst_main->can_attack_horse_level()) {
                clear_auto_attack_target_actor_id();
            } else if (inst_main->is_attackable_instance(*inst_victim)) {
                if (inst_main->is_sleep()) {
                } else {
                    reserve_click_actor(auto_attack_target_vid_);
                }
            }
        }
    }

    void python_player::update_notify_guild_area_event() {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (inst_main) {
            pixel_position pixel_position;
            inst_main->new_get_pixel_position(&pixel_position);

            uint32_t area_id = get_python_mini_map().guild_area_id(
                static_cast<uint32_t>(pixel_position.x), static_cast<uint32_t>(pixel_position.y));

            if (area_id != in_guild_area_id_) {
                if (0xffffffff != area_id) {
                    py_call_class_member_func(ppy_game_window_, "BINARY_Guild_EnterGuildArea", Py_BuildValue("(i)", area_id));
                } else {
                    py_call_class_member_func(ppy_game_window_, "BINARY_Guild_ExitGuildArea", Py_BuildValue("(i)", area_id));
                }

                in_guild_area_id_ = area_id;
            }
        }
    }

    void python_player::set_main_character_index(int index) {
        main_character_index_ = index;

        instance_base *inst_main = new_get_main_actor_ptr();
        if (inst_main) {
            python_player_event_handler &player_event_handler = get_python_player_event_handler();
            inst_main->set_event_handler(&player_event_handler);
        }
    }

    auto python_player::main_character_index() -> uint32_t {
        return main_character_index_;
    }

    auto python_player::is_main_character_index(uint32_t index) -> bool {
        return (main_character_index_ == index);
    }

    auto python_player::guild_id() -> uint32_t {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return 0xffffffff;

        return inst_main->guild_id();
    }

    void python_player::set_weapon_power(uint32_t min_power, uint32_t max_power, uint32_t min_magic_power, uint32_t max_magic_power, uint32_t add_power) {
        weapon_min_power_ = min_power;
        weapon_max_power_ = max_power;
        weapon_min_magic_power_ = min_magic_power;
        weapon_max_magic_power_ = max_magic_power;
        weapon_add_power_ = add_power;

        update_battle_status();
    }

    void python_player::set_race(uint32_t race) {
        race_ = race;
    }

    auto python_player::race() -> uint32_t {
        return race_;
    }

    auto python_player::race_stat() -> uint32_t {
        switch (race()) {
        case MAIN_RACE_WARRIOR_M:
        case MAIN_RACE_WARRIOR_W:
            return status(POINT_ST);
            break;
        case MAIN_RACE_ASSASSIN_M:
        case MAIN_RACE_ASSASSIN_W:
            return status(POINT_DX);
            break;
        case MAIN_RACE_SURA_M:
        case MAIN_RACE_SURA_W:
            return status(POINT_ST);
            break;
        case MAIN_RACE_SHAMAN_M:
        case MAIN_RACE_SHAMAN_W:
            return status(POINT_IQ);
            break;
        }
        return status(POINT_ST);
    }

    auto python_player::level_atk() -> uint32_t {
        return 2 * status(POINT_LEVEL);
    }

    auto python_player::stat_atk() -> uint32_t {
        return (4 * status(POINT_ST) + 2 * race_stat()) / 3;
    }

    auto python_player::weapon_atk(uint32_t weapon_power) -> uint32_t {
        return 2 * weapon_power;
    }

    auto python_player::total_atk(uint32_t weapon_power, uint32_t refine_bonus) -> uint32_t {
        uint32_t lv_atk = level_atk();
        uint32_t st_atk = stat_atk();

        uint32_t wep_atk;
        uint32_t total_atk;

        int hr = hit_rate();
        wep_atk = weapon_atk(weapon_power + refine_bonus);
        total_atk = lv_atk + (st_atk + wep_atk) * hr / 100;

        return total_atk;
    }

    auto python_player::hit_rate() -> uint32_t {
        int src = 0;

        src = (status(POINT_DX) * 4 + status(POINT_LEVEL) * 2) / 6;

        return 100 * (std::min(90, src) + 210) / 300;
    }

    auto python_player::evade_rate() -> uint32_t {
        return 30 * (2 * status(POINT_DX) + 5) / (status(POINT_DX) + 95);
    }

    void python_player::update_battle_status() {
        player_status_value_.set_point(POINT_NONE, 0);
        player_status_value_.set_point(POINT_EVADE_RATE, evade_rate());
        player_status_value_.set_point(POINT_HIT_RATE, hit_rate());
        player_status_value_.set_point(POINT_MIN_WEP, weapon_min_power_ + weapon_add_power_);
        player_status_value_.set_point(POINT_MAX_WEP, weapon_max_power_ + weapon_add_power_);
        player_status_value_.set_point(POINT_MIN_MAGIC_WEP, weapon_min_magic_power_ + weapon_add_power_);
        player_status_value_.set_point(POINT_MAX_MAGIC_WEP, weapon_max_magic_power_ + weapon_add_power_);
        player_status_value_.set_point(POINT_MIN_ATK, total_atk(weapon_min_power_, weapon_add_power_));
        player_status_value_.set_point(POINT_MAX_ATK, total_atk(weapon_max_power_, weapon_add_power_));
    }

    void python_player::set_status(uint32_t type, int32_t value) {
        if (type >= POINT_MAX_NUM) {
            assert(!" CPythonPlayer::SetStatus - Strange Status Type!");
            metin2::log::debug("Cannot set status: unknown status type");
            return;
        }

        if (type == POINT_LEVEL) {
            instance_base *player = new_get_main_actor_ptr();

            if (player) {
                player->set_level(value);
                player->update_text_tail_level(value);
            }
        }

        switch (type) {
        case POINT_MIN_WEP:
        case POINT_MAX_WEP:
        case POINT_MIN_ATK:
        case POINT_MAX_ATK:
        case POINT_HIT_RATE:
        case POINT_EVADE_RATE:
        case POINT_LEVEL:
        case POINT_ST:
        case POINT_DX:
        case POINT_IQ:
            player_status_value_.set_point(type, value);
            update_battle_status();
            break;
        default:
            player_status_value_.set_point(type, value);
            break;
        }
    }

    auto python_player::status(uint32_t type) -> int {
        if (type >= POINT_MAX_NUM) {
            assert(!" CPythonPlayer::GetStatus - Strange Status Type!");
            metin2::log::debug("Cannot get status: unknown status type");
            return 0;
        }

        return player_status_value_.get_point(type);
    }

    auto python_player::get_name() -> const char * {
        return name_value_.c_str();
    }

    void python_player::set_name(const char *name) {
        name_value_ = name;
    }

    void python_player::notify_deleting_character_instance(uint32_t vid) {
        if (main_character_index_ == vid)
            main_character_index_ = 0;
    }

    void python_player::notify_character_dead(uint32_t vid) {
        if (is_same_target_vid(vid)) {
            set_target(0);
        }
    }

    void python_player::notify_character_update(uint32_t vid) {
        if (is_same_target_vid(vid)) {
            instance_base *main_instance = new_get_main_actor_ptr();
            instance_base *target_instance = get_python_character_manager().instance_ptr(vid);
            if (main_instance && target_instance) {
                if (!main_instance->is_targetable_instance(*target_instance)) {
                    set_target(0);
                    py_call_class_member_func(ppy_game_window_, "CloseTargetBoard", Py_BuildValue("()"));
                } else {
                    py_call_class_member_func(ppy_game_window_, "RefreshTargetBoardByVID", Py_BuildValue("(i)", vid));
                }
            }
        }
    }

    void python_player::notify_dead_main_character() {
        clear_auto_attack_target_actor_id();
    }

    void python_player::notify_change_pk_mode() {
        py_call_class_member_func(ppy_game_window_, "OnChangePKMode", Py_BuildValue("()"));
    }

    auto python_player::get_item_data(item_pos cell) const -> const packet_item * {
        if (!cell.is_valid_cell())
            return nullptr;

        switch (cell.window_type) {
        case INVENTORY:
        case EQUIPMENT:
            return &player_status_value_.item_value[cell.cell];
        case DRAGON_SOUL_INVENTORY:
            return &player_status_value_.ds_item[cell.cell];
        default:
            return nullptr;
        }
    }

    void python_player::set_item_data(item_pos cell, const packet_item &item_inst) {
        if (!cell.is_valid_cell())
            return;

        if (item_inst.vnum != 0) {
            item_data *item_data;
            if (!get_item_manager().item_data_pointer(item_inst.vnum, &item_data)) {
                metin2::log::error("Window {} slot {}: no item data for item {}", cell.window_type, cell.cell, item_inst.vnum);
                return;
            }
        }

        switch (cell.window_type) {
        case INVENTORY:
        case EQUIPMENT:
            player_status_value_.item_value[cell.cell] = item_inst;
            break;
        case DRAGON_SOUL_INVENTORY:
            player_status_value_.ds_item[cell.cell] = item_inst;
            break;
        }
    }

    auto python_player::item_index(item_pos cell) -> uint32_t {
        if (!cell.is_valid_cell())
            return 0;

        return get_item_data(cell)->vnum;
    }

    auto python_player::item_flags(item_pos cell) -> uint32_t {
        if (!cell.is_valid_cell())
            return 0;
        const packet_item *item = get_item_data(cell);
        assert(item != nullptr);
        return item->flags_value;
    }

    auto python_player::item_count(item_pos cell) -> uint32_t {
        if (!cell.is_valid_cell())
            return 0;
        const packet_item *item = get_item_data(cell);
        if (item == nullptr)
            return 0;
        else
            return item->count;
    }

    auto python_player::item_count_by_vnum(uint32_t vnum) -> uint32_t {
        uint32_t count = 0;

        for (uint32_t i = 0; i < c_Inventory_Count; ++i) {
            const packet_item &item_data = player_status_value_.item_value[i];
            if (item_data.vnum == vnum) {
                count += item_data.count;
            }
        }

        return count;
    }

    auto python_player::item_metin_socket(item_pos cell, uint32_t metin_socket_index) -> uint32_t {
        if (!cell.is_valid_cell())
            return 0;

        if (metin_socket_index >= ITEM_SOCKET_SLOT_MAX_NUM)
            return 0;

        return get_item_data(cell)->sockets[metin_socket_index];
    }

    void python_player::item_attribute(item_pos cell, uint32_t attr_slot_index, uint8_t *type, short *value) {
        *type = 0;
        *value = 0;

        if (!cell.is_valid_cell())
            return;

        if (attr_slot_index >= ITEM_ATTRIBUTE_SLOT_MAX_NUM)
            return;

        *type = get_item_data(cell)->attr[attr_slot_index].type_value;
        *value = get_item_data(cell)->attr[attr_slot_index].value;
    }

    void python_player::set_item_count(item_pos cell, uint8_t count) {
        if (!cell.is_valid_cell())
            return;

        (const_cast<packet_item *>(get_item_data(cell)))->count = count;
        py_call_class_member_func(ppy_game_window_, "RefreshInventory", Py_BuildValue("()"));
    }

    void python_player::set_item_metin_socket(item_pos cell, uint32_t metin_socket_index, uint32_t metin_number) {
        if (!cell.is_valid_cell())
            return;
        if (metin_socket_index >= ITEM_SOCKET_SLOT_MAX_NUM)
            return;

        (const_cast<packet_item *>(get_item_data(cell)))->sockets[metin_socket_index] = metin_number;
    }

    void python_player::set_item_attribute(item_pos cell, uint32_t attr_index, uint8_t type, short value) {
        if (!cell.is_valid_cell())
            return;
        if (attr_index >= ITEM_ATTRIBUTE_SLOT_MAX_NUM)
            return;

        (const_cast<packet_item *>(get_item_data(cell)))->attr[attr_index].type_value = type;
        (const_cast<packet_item *>(get_item_data(cell)))->attr[attr_index].value = value;
    }

    auto python_player::quick_page() -> int {
        return player_status_value_.quick_page_index;
    }

    void python_player::set_quick_page(int quick_page_index) {
        if (quick_page_index < 0)
            player_status_value_.quick_page_index = QUICKSLOT_MAX_LINE + quick_page_index;
        else if (quick_page_index >= QUICKSLOT_MAX_LINE)
            player_status_value_.quick_page_index = quick_page_index % QUICKSLOT_MAX_LINE;
        else
            player_status_value_.quick_page_index = quick_page_index;

        py_call_class_member_func(ppy_game_window_, "RefreshInventory", Py_BuildValue("()"));
    }

    auto python_player::local_quick_slot_index_to_global_quick_slot_index(uint32_t local_slot_index) -> uint32_t {
        return player_status_value_.quick_page_index * QUICKSLOT_MAX_COUNT_PER_LINE + local_slot_index;
    }

    void python_player::global_quick_slot_data(uint32_t global_slot_index, uint32_t *wnd_type, uint32_t *wnd_item_pos) {
        quick_slot &quick_slot = ref_global_quick_slot(global_slot_index);
        *wnd_type = quick_slot.type_value;
        *wnd_item_pos = quick_slot.position_value;
    }

    void python_player::local_quick_slot_data(uint32_t slot_pos, uint32_t *wnd_type, uint32_t *wnd_item_pos) {
        quick_slot &quick_slot = ref_local_quick_slot(slot_pos);
        *wnd_type = quick_slot.type_value;
        *wnd_item_pos = quick_slot.position_value;
    }

    auto python_player::ref_local_quick_slot(int slot_index) -> quick_slot & {
        return ref_global_quick_slot(local_quick_slot_index_to_global_quick_slot_index(slot_index));
    }

    auto python_player::ref_global_quick_slot(int slot_index) -> quick_slot & {
        if (slot_index < 0 || slot_index >= QUICKSLOT_MAX_NUM) {
            static quick_slot quick_slot;
            quick_slot.type_value = 0;
            quick_slot.position_value = 0;
            return quick_slot;
        }

        return player_status_value_.quick_slot_value[slot_index];
    }

    void python_player::request_move_global_quick_slot_to_local_quick_slot(uint32_t global_src_slot_index, uint32_t local_dst_slot_index) {
        uint32_t global_dst_slot_index = local_quick_slot_index_to_global_quick_slot_index(local_dst_slot_index);

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_quick_slot_move_packet((uint8_t)global_src_slot_index, (uint8_t)global_dst_slot_index);
    }

    void python_player::request_add_local_quick_slot(uint32_t local_slot_index, uint32_t wnd_type, uint32_t wnd_item_pos) {
        if (local_slot_index >= QUICKSLOT_MAX_COUNT_PER_LINE)
            return;

        uint32_t global_slot_index = local_quick_slot_index_to_global_quick_slot_index(local_slot_index);

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_quick_slot_add_packet((uint8_t)global_slot_index, (uint8_t)wnd_type, (uint8_t)wnd_item_pos);
    }

    void python_player::request_add_to_empty_local_quick_slot(uint32_t wnd_type, uint32_t wnd_item_pos) {
        for (int i = 0; i < QUICKSLOT_MAX_COUNT_PER_LINE; ++i) {
            quick_slot &quick_slot = ref_local_quick_slot(i);

            if (0 == quick_slot.type_value) {
                uint32_t global_quick_slot_index = local_quick_slot_index_to_global_quick_slot_index(i);
                python_network_stream &net_stream = get_python_network_stream();
                net_stream.send_quick_slot_add_packet((uint8_t)global_quick_slot_index, (uint8_t)wnd_type, (uint8_t)wnd_item_pos);
                return;
            }
        }
    }

    void python_player::request_delete_global_quick_slot(uint32_t global_slot_index) {
        if (global_slot_index >= QUICKSLOT_MAX_COUNT)
            return;

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_quick_slot_del_packet((uint8_t)global_slot_index);
    }

    void python_player::request_use_local_quick_slot(uint32_t local_slot_index) {
        if (local_slot_index >= QUICKSLOT_MAX_COUNT_PER_LINE)
            return;

        uint32_t registered_type;
        uint32_t registered_item_pos;
        local_quick_slot_data(local_slot_index, &registered_type, &registered_item_pos);

        switch (registered_type) {
        case SLOT_TYPE_INVENTORY:
            {
                python_network_stream &net_stream = get_python_network_stream();
                net_stream.send_item_use_packet(item_pos(INVENTORY, (uint16_t)registered_item_pos));
                break;
            }
        case SLOT_TYPE_SKILL:
            {
                click_skill_slot(registered_item_pos);
                break;
            }
        case SLOT_TYPE_EMOTION:
            {
                py_call_class_member_func(ppy_game_window_, "BINARY_ActEmotion", Py_BuildValue("(i)", registered_item_pos));
                break;
            }
        }
    }

    void python_player::add_quick_slot(int quick_slot_index, char icon_type, char icon_position) {
        if (quick_slot_index < 0 || quick_slot_index >= QUICKSLOT_MAX_NUM)
            return;

        player_status_value_.quick_slot_value[quick_slot_index].type_value = icon_type;
        player_status_value_.quick_slot_value[quick_slot_index].position_value = icon_position;
    }

    void python_player::delete_quick_slot(int quick_slot_index) {
        if (quick_slot_index < 0 || quick_slot_index >= QUICKSLOT_MAX_NUM)
            return;

        player_status_value_.quick_slot_value[quick_slot_index].type_value = 0;
        player_status_value_.quick_slot_value[quick_slot_index].position_value = 0;
    }

    void python_player::move_quick_slot(int source, int target) {
        if (source < 0 || source >= QUICKSLOT_MAX_NUM)
            return;

        if (target < 0 || target >= QUICKSLOT_MAX_NUM)
            return;

        quick_slot &src_slot = ref_global_quick_slot(source);
        quick_slot &dst_slot = ref_global_quick_slot(target);

        std::swap(src_slot, dst_slot);
    }

    auto python_player::is_belt_inventory_slot(item_pos cell) -> bool {
        return cell.is_belt_inventory_cell();
    }

    auto python_player::is_equipment_slot(item_pos cell) -> bool {
        return cell.is_equip_cell();
    }

    auto python_player::is_equip_item_in_slot(item_pos cell) -> bool {
        if (!cell.is_equip_cell()) {
            return false;
        }

        const packet_item *data = get_item_data(cell);

        if (nullptr == data) {
            return false;
        }

        uint32_t item_index = data->vnum;

        get_item_manager().select_item_data(item_index);
        item_data *item_data = get_item_manager().selected_item_data_pointer();
        if (!item_data) {
            metin2::log::error("Window {} slot {}: no item data", cell.window_type, cell.cell);
            return false;
        }

        return item_data->is_equipment() ? true : false;
    }

    void python_player::set_skill(uint32_t slot_index, uint32_t skill_index) {
        if (slot_index >= SKILL_MAX_NUM)
            return;

        player_status_value_.skill[slot_index].index_value = skill_index;
        skill_slot_dict_[skill_index] = slot_index;
    }

    auto python_player::skill_index(uint32_t slot_index) -> int {
        if (slot_index >= SKILL_MAX_NUM)
            return 0;

        return player_status_value_.skill[slot_index].index_value;
    }

    auto python_player::skill_slot_index(uint32_t skill_index, uint32_t *slot_index) -> bool {
        auto f = skill_slot_dict_.find(skill_index);
        if (skill_slot_dict_.end() == f) {
            return false;
        }

        *slot_index = f->second;

        return true;
    }

    auto python_player::skill_grade(uint32_t slot_index) -> int {
        if (slot_index >= SKILL_MAX_NUM)
            return 0;

        return player_status_value_.skill[slot_index].grade;
    }

    auto python_player::skill_level(uint32_t slot_index) -> int {
        if (slot_index >= SKILL_MAX_NUM)
            return 0;

        return player_status_value_.skill[slot_index].level_value;
    }

    auto python_player::skill_current_efficient_percentage(uint32_t slot_index) -> float {
        if (slot_index >= SKILL_MAX_NUM)
            return 0;

        return player_status_value_.skill[slot_index].fcur_efficient_percentage;
    }

    auto python_player::skill_next_efficient_percentage(uint32_t slot_index) -> float {
        if (slot_index >= SKILL_MAX_NUM)
            return 0;

        return player_status_value_.skill[slot_index].fnext_efficient_percentage;
    }

    void python_player::set_skill_level(uint32_t slot_index, uint32_t skill_level) {
        assert(!"CPythonPlayer::SetSkillLevel - unused function");
        if (slot_index >= SKILL_MAX_NUM)
            return;

        player_status_value_.skill[slot_index].grade = -1;
        player_status_value_.skill[slot_index].level_value = skill_level;
    }

    void python_player::set_skill_level_impl(uint32_t skill_index, uint32_t skill_grade, uint32_t skill_level) {
        uint32_t slot_index;
        if (!skill_slot_index(skill_index, &slot_index))
            return;

        if (slot_index >= SKILL_MAX_NUM)
            return;

        switch (skill_grade) {
        case 0:
            player_status_value_.skill[slot_index].grade = skill_grade;
            player_status_value_.skill[slot_index].level_value = skill_level;
            break;
        case 1:
            player_status_value_.skill[slot_index].grade = skill_grade;
            player_status_value_.skill[slot_index].level_value = skill_level - 20 + 1;
            break;
        case 2:
            player_status_value_.skill[slot_index].grade = skill_grade;
            player_status_value_.skill[slot_index].level_value = skill_level - 30 + 1;
            break;
        case 3:
            player_status_value_.skill[slot_index].grade = skill_grade;
            player_status_value_.skill[slot_index].level_value = skill_level - 40 + 1;
            break;
        }

        const uint32_t skill_max_level = 40;

        if (skill_level > skill_max_level) {
            player_status_value_.skill[slot_index].fcur_efficient_percentage = 0.0f;
            player_status_value_.skill[slot_index].fnext_efficient_percentage = 0.0f;

            metin2::log::error("Slot {}: skill level {} exceeds the maximum", slot_index, skill_level);
            return;
        }

        player_status_value_.skill[slot_index].fcur_efficient_percentage = locale_service_get_skill_power(skill_level) / 100.0f;
        player_status_value_.skill[slot_index].fnext_efficient_percentage = locale_service_get_skill_power(skill_level + 1) / 100.0f;
    }

    void python_player::set_skill_cool_time(uint32_t skill_index) {
        uint32_t slot_index;
        if (!skill_slot_index(skill_index, &slot_index)) {
            metin2::log::debug("Skill {}: no slot for the cooldown", skill_index);
            return;
        }

        if (slot_index >= SKILL_MAX_NUM) {
            metin2::log::debug("Skill {}: slot {} out of range (limit {})", skill_index, slot_index, SKILL_MAX_NUM);
            return;
        }

        player_status_value_.skill[slot_index].is_cool_time = true;
    }

    void python_player::end_skill_cool_time(uint32_t skill_index) {
        uint32_t slot_index;
        if (!skill_slot_index(skill_index, &slot_index)) {
            metin2::log::debug("Skill {}: no slot to end the cooldown", skill_index);
            return;
        }

        if (slot_index >= SKILL_MAX_NUM) {
            metin2::log::debug("Skill {}: slot {} out of range (limit {})", skill_index, slot_index, SKILL_MAX_NUM);
            return;
        }

        player_status_value_.skill[slot_index].is_cool_time = false;
    }

    auto python_player::skill_cool_time(uint32_t slot_index) -> float {
        if (slot_index >= SKILL_MAX_NUM)
            return 0.0f;

        return player_status_value_.skill[slot_index].cool_time;
    }

    auto python_player::skill_elapsed_cool_time(uint32_t slot_index) -> float {
        if (slot_index >= SKILL_MAX_NUM)
            return 0.0f;

        return get_timer().current_second() - player_status_value_.skill[slot_index].last_used_time;
    }

    void python_player::activate_skill_slot(uint32_t slot_index) {
        if (slot_index >= SKILL_MAX_NUM) {
            metin2::log::debug("Cannot activate skill: slot {} out of range (limit {})", slot_index, SKILL_MAX_NUM);
            return;
        }

        player_status_value_.skill[slot_index].active = true;
        py_call_class_member_func(ppy_game_window_, "ActivateSkillSlot", Py_BuildValue("(i)", slot_index));
    }

    void python_player::deactivate_skill_slot(uint32_t slot_index) {
        if (slot_index >= SKILL_MAX_NUM) {
            metin2::log::debug("Cannot deactivate skill: slot {} out of range (limit {})", slot_index, SKILL_MAX_NUM);
            return;
        }

        player_status_value_.skill[slot_index].active = false;
        py_call_class_member_func(ppy_game_window_, "DeactivateSkillSlot", Py_BuildValue("(i)", slot_index));
    }

    auto python_player::is_skill_cool_time(uint32_t slot_index) -> bool {
        if (!check_rest_skill_cool_time(slot_index))
            return false;

        return true;
    }

    auto python_player::is_skill_active(uint32_t slot_index) -> bool {
        if (slot_index >= SKILL_MAX_NUM)
            return false;

        return player_status_value_.skill[slot_index].active;
    }

    auto python_player::is_toggle_skill(uint32_t slot_index) -> bool {
        if (slot_index >= SKILL_MAX_NUM)
            return false;

        uint32_t skill_index = player_status_value_.skill[slot_index].index_value;

        python_skill::skill_data *skill_data;
        if (!get_python_skill().get_skill_data(skill_index, &skill_data))
            return false;

        return skill_data->is_toggle_skill();
    }

    void python_player::set_play_time(uint32_t play_time) {
        play_time_ = play_time;
    }

    auto python_player::play_time() -> uint32_t {
        return play_time_;
    }

    void python_player::send_click_item_packet(uint32_t iid) {
        if (is_observer_mode())
            return;

        static uint32_t next_tcp_time = 0;

        uint32_t cur_time = el_timer_get_m_sec();

        if (cur_time >= next_tcp_time) {
            next_tcp_time = cur_time + 500;

            const char *owner_name;
            if (!get_python_item().ownership(iid, &owner_name))
                return;

            if (strlen(owner_name) > 0)
                if (0 != strcmp(owner_name, get_name())) {
                    item_data *item_data;
                    if (!get_item_manager().item_data_pointer(get_python_item().virtual_number_of_ground_item(iid), &item_data)) {
                        metin2::log::debug("Cannot click item {}: it does not exist", iid);
                        return;
                    }
                    if (!is_party_member_by_name(owner_name) || item_data->is_anti_flag(item_data::ITEM_ANTIFLAG_DROP | item_data::ITEM_ANTIFLAG_GIVE)) {
                        py_call_class_member_func(ppy_game_window_, "OnCannotPickItem", Py_BuildValue("()"));
                        return;
                    }
                }

            python_network_stream &net_stream = get_python_network_stream();
            net_stream.send_item_pick_up_packet(iid);
        }
    }

    void python_player::send_click_actor_packet(instance_base &inst_victim) {
        // Mining while riding is an exception.
        instance_base *inst_main = new_get_main_actor_ptr();
        if (inst_main)
            if (inst_main->is_holding_pick_axe())
                if (inst_main->is_mounting_horse())
                    if (inst_victim.is_resource()) {
                        py_call_class_member_func(ppy_game_window_, "OnCannotMining", Py_BuildValue("()"));
                        return;
                    }

        static uint32_t next_tcp_time = 0;

        uint32_t cur_time = el_timer_get_m_sec();

        if (cur_time >= next_tcp_time) {
            next_tcp_time = cur_time + 1000;

            python_network_stream &net_stream = get_python_network_stream();

            uint32_t victim_vid = inst_victim.virtual_id();
            net_stream.send_on_click_packet(victim_vid);
        }
    }

    void python_player::act_emotion(uint32_t emotion_id) {
        instance_base *inst_target = alive_target_instance_ptr();
        if (!inst_target) {
            py_call_class_member_func(ppy_game_window_, "OnCannotShotError", Py_BuildValue("(is)", main_character_index(), "NEED_TARGET"));
            return;
        }

        get_python_network_stream().send_chat_packet(format_string("/kiss %s", inst_target->name_string()));
    }

    void python_player::start_emotion_process() {
        clear_reserved_action();
        clear_auto_attack_target_actor_id();

        bis_processing_emotion_ = true;
    }

    void python_player::end_emotion_process() {
        bis_processing_emotion_ = false;
    }

    auto python_player::is_processing_emotion() -> bool {
        return bis_processing_emotion_;
    }

    void python_player::set_dungeon_destination_position(int ix, int iy) {
        is_dest_position_ = true;
        ix_dest_pos_ = ix;
        iy_dest_pos_ = iy;

        alarm_have_to_go();
    }

    void python_player::alarm_have_to_go() {
        last_alarm_time_ = get_timer().current_millisecond();

        instance_base *instance = new_get_main_actor_ptr();
        if (!instance)
            return;

        pixel_position pixel_position;
        instance->new_get_pixel_position(&pixel_position);

        float angle = get_degree_from_position2(pixel_position.x, pixel_position.y, float(ix_dest_pos_), float(iy_dest_pos_));
        angle = fmod(540.0f - angle, 360.0f);
        math::vector3 rotation(0.0f, 0.0f, angle);

        pixel_position.y *= -1.0f;

        get_effect_manager().register_effect("d:/ymir work/effect/etc/compass/appear_middle.mse");
        get_effect_manager().create_effect("d:/ymir work/effect/etc/compass/appear_middle.mse", pixel_position, rotation);
    }

    void python_player::exit_party() {
        party_member_map_.clear();

        get_python_character_manager().refresh_all_pc_text_tail();
    }

    void python_player::append_party_member(uint32_t pid, const char *name) {
        party_member_map_.insert(std::make_pair(pid, party_member_info(pid, name)));
    }

    void python_player::link_party_member(uint32_t pid, uint32_t vid) {
        party_member_info *party_member_info_value;
        if (!party_member_ptr(pid, &party_member_info_value)) {
            metin2::log::error("Party member {} (vid {}) not found", pid, vid);
            return;
        }

        party_member_info_value->vid = vid;

        instance_base *instance = new_find_actor_ptr(vid);
        if (instance)
            instance->refresh_text_tail();
    }

    void python_player::unlink_party_member(uint32_t pid) {
        party_member_info *party_member_info_value;
        if (!party_member_ptr(pid, &party_member_info_value)) {
            metin2::log::error("Party member {} not found", pid);
            return;
        }

        party_member_info_value->vid = 0;
    }

    void python_player::update_party_member_info(uint32_t pid, uint8_t state, uint8_t hp_percentage) {
        party_member_info *party_member_info_value;
        if (!party_member_ptr(pid, &party_member_info_value)) {
            metin2::log::error("Party member {} not found (state {}, hp {}%)", pid, state, hp_percentage);
            return;
        }

        party_member_info_value->state_value = state;
        party_member_info_value->hp_percentage = hp_percentage;
    }

    void python_player::update_party_member_affect(uint32_t pid, uint8_t affect_slot_index, short affect_number) {
        if (affect_slot_index >= PARTY_AFFECT_SLOT_MAX_NUM) {
            metin2::log::error("Party member {}: affect slot {} out of range (affect {})", pid, affect_slot_index, affect_number);
            return;
        }

        party_member_info *party_member_info_value;
        if (!party_member_ptr(pid, &party_member_info_value)) {
            metin2::log::error("Party member {} not found (affect slot {}, affect {})", pid, affect_slot_index, affect_number);
            return;
        }

        party_member_info_value->affects_value[affect_slot_index] = affect_number;
    }

    void python_player::remove_party_member(uint32_t pid) {
        uint32_t vid = 0;
        party_member_info *party_member_info_value;
        if (party_member_ptr(pid, &party_member_info_value)) {
            vid = party_member_info_value->vid;
        }

        party_member_map_.erase(pid);

        if (vid > 0) {
            instance_base *instance = new_find_actor_ptr(vid);
            if (instance)
                instance->refresh_text_tail();
        }
    }

    auto python_player::is_party_member_by_vid(uint32_t vid) -> bool {
        auto itor = party_member_map_.begin();
        for (; itor != party_member_map_.end(); ++itor) {
            party_member_info &party_member_info_value = itor->second;
            if (vid == party_member_info_value.vid)
                return true;
        }

        return false;
    }

    auto python_player::is_party_member_by_name(const char *name) -> bool {
        auto itor = party_member_map_.begin();
        for (; itor != party_member_map_.end(); ++itor) {
            party_member_info &party_member_info_value = itor->second;
            if (0 == party_member_info_value.name_value.compare(name))
                return true;
        }

        return false;
    }

    auto python_player::party_member_ptr(uint32_t pid, party_member_info **party_member_info_value) -> bool {
        auto itor = party_member_map_.find(pid);

        if (party_member_map_.end() == itor)
            return false;

        *party_member_info_value = &(itor->second);

        return true;
    }

    auto python_player::party_member_pid_to_vid(uint32_t pid, uint32_t *vid) -> bool {
        auto itor = party_member_map_.find(pid);

        if (party_member_map_.end() == itor)
            return false;

        const party_member_info &party_member_info_value = itor->second;
        *vid = party_member_info_value.vid;

        return true;
    }

    auto python_player::party_member_vid_to_pid(uint32_t vid, uint32_t *pid) -> bool {
        auto itor = party_member_map_.begin();
        for (; itor != party_member_map_.end(); ++itor) {
            party_member_info &party_member_info_value = itor->second;
            if (vid == party_member_info_value.vid) {
                *pid = party_member_info_value.pid;
                return true;
            }
        }

        return false;
    }

    auto python_player::is_same_party_member(uint32_t vid1, uint32_t vid2) -> bool {
        return (is_party_member_by_vid(vid1) && is_party_member_by_vid(vid2));
    }

    void python_player::remember_challenge_instance(uint32_t vid) {
        revenge_instance_set_.erase(vid);
        challenge_instance_set_.insert(vid);
    }
    void python_player::remember_revenge_instance(uint32_t vid) {
        challenge_instance_set_.erase(vid);
        revenge_instance_set_.insert(vid);
    }
    void python_player::remember_cant_fight_instance(uint32_t vid) {
        cant_fight_instance_set_.insert(vid);
    }
    void python_player::forget_instance(uint32_t vid) {
        challenge_instance_set_.erase(vid);
        revenge_instance_set_.erase(vid);
        cant_fight_instance_set_.erase(vid);
    }

    auto python_player::is_challenge_instance(uint32_t vid) -> bool {
        return challenge_instance_set_.end() != challenge_instance_set_.find(vid);
    }
    auto python_player::is_revenge_instance(uint32_t vid) -> bool {
        return revenge_instance_set_.end() != revenge_instance_set_.find(vid);
    }
    auto python_player::is_cant_fight_instance(uint32_t vid) -> bool {
        return cant_fight_instance_set_.end() != cant_fight_instance_set_.find(vid);
    }

    void python_player::open_private_shop() {
        is_open_private_shop_ = true;
    }
    void python_player::close_private_shop() {
        is_open_private_shop_ = false;
    }

    auto python_player::is_open_private_shop() -> bool {
        return is_open_private_shop_;
    }

    void python_player::set_observer_mode(bool is_enable) {
        is_observer_mode_ = is_enable;
    }

    auto python_player::is_observer_mode() -> bool {
        return is_observer_mode_;
    }

    auto python_player::toggle_cool_time() -> bool {
        sys_is_cool_time_ = 1 - sys_is_cool_time_;
        return sys_is_cool_time_;
    }

    auto python_player::toggle_level_limit() -> bool {
        sys_is_level_limit_ = 1 - sys_is_level_limit_;
        return sys_is_level_limit_;
    }

    void python_player::start_stamina_consume(uint32_t consume_per_sec, uint32_t current_stamina) {
        is_consuming_stamina_ = true;
        consume_stamina_per_sec_ = float(consume_per_sec);
        current_stamina_ = float(current_stamina);

        set_status(POINT_STAMINA, current_stamina);
    }

    void python_player::stop_stamina_consume(uint32_t current_stamina) {
        is_consuming_stamina_ = false;
        consume_stamina_per_sec_ = 0.0f;
        current_stamina_ = float(current_stamina);

        set_status(POINT_STAMINA, current_stamina);
    }

    auto python_player::pk_mode() -> uint32_t {
        instance_base *instance = new_get_main_actor_ptr();
        if (!instance)
            return 0;

        return instance->pk_mode();
    }

    void python_player::set_mobile_flag(bool flag) {
        mobile_flag_ = flag;
        py_call_class_member_func(ppy_game_window_, "RefreshMobile", Py_BuildValue("()"));
    }

    auto python_player::has_mobile_phone_number() -> bool {
        return mobile_flag_;
    }

    void python_player::set_game_window(PyObject *ppy_object) {
        ppy_game_window_ = ppy_object;
    }

    void python_player::new_clear_skill_data(bool all) {
        std::map<uint32_t, uint32_t>::iterator it;

        for (it = skill_slot_dict_.begin(); it != skill_slot_dict_.end();) {
            if (all || skill_type(it->first) == python_skill::SKILL_TYPE_ACTIVE)
                it = skill_slot_dict_.erase(it);
            else
                ++it;
        }

        for (int i = 0; i < SKILL_MAX_NUM; ++i) {
            memset(&player_status_value_.skill[i], 0, sizeof(skill_instance));
        }

        for (int j = 0; j < SKILL_MAX_NUM; ++j) {
            // Reset the grade too, or the skill point [+] button does not show after a skill update.
            player_status_value_.skill[j].grade = 0;
            player_status_value_.skill[j].fcur_efficient_percentage = 0.0f;
            player_status_value_.skill[j].fnext_efficient_percentage = 0.05f;
        }

        if (ppy_game_window_)
            py_call_class_member_func(ppy_game_window_, "BINARY_CheckGameButton", py_build_none());
    }

    void python_player::clear_skill_dict() {
        skill_slot_dict_.clear();

        is_open_private_shop_ = false;
        is_observer_mode_ = false;

        is_consuming_stamina_ = false;
        consume_stamina_per_sec_ = 0.0f;
        current_stamina_ = 0.0f;

        mobile_flag_ = false;

        clear_auto_attack_target_actor_id();
    }

    void python_player::clear() {
        memset(&player_status_value_, 0, sizeof(player_status_value_));
        new_clear_skill_data(true);

        bis_processing_emotion_ = false;

        sending_target_vid_ = 0;
        target_update_time_ = 0.0f;

        name_value_ = "";
        main_character_index_ = 0;
        race_ = 0;
        weapon_min_power_ = 0;
        weapon_max_power_ = 0;
        weapon_min_magic_power_ = 0;
        weapon_max_magic_power_ = 0;
        weapon_add_power_ = 0;

        moving_cursor_position_ = pixel_position(0, 0, 0);
        moving_cursor_setting_time_ = 0.0f;

        reserved_mode_ = MODE_NONE;
        reserved_delay_time_ = 0.0f;
        p_pos_reserved_ = pixel_position(0, 0, 0);
        vid_reserved_ = 0;
        iid_reserved_ = 0;
        skill_slot_index_reserved_ = 0;
        skill_range_reserved_ = 0;

        is_up_ = false;
        is_down_ = false;
        is_left_ = false;
        is_right_ = false;
        is_smt_mov_ = false;
        is_dir_mov_ = false;
        is_dir_key_ = false;
        is_atk_key_ = false;

        is_cmr_rot_ = true;
        cmr_rot_spd_ = 20.0f;

        combo_old_ = 0;

        vid_picked_ = 0;
        iid_picked_ = 0;

        dwcur_skill_slot_index_ = uint32_t(-1);

        target_vid_ = 0;
        target_end_time_ = 0;

        party_member_map_.clear();

        challenge_instance_set_.clear();
        revenge_instance_set_.clear();

        is_open_private_shop_ = false;
        is_observer_mode_ = false;

        is_consuming_stamina_ = false;
        consume_stamina_per_sec_ = 0.0f;
        current_stamina_ = 0.0f;

        in_guild_area_id_ = 0xffffffff;

        mobile_flag_ = false;

        clear_auto_attack_target_actor_id();
    }

    python_player::python_player(void) {
        set_movable_ground_distance(40.0f);

        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_JEONGWI), 3));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_GEOMGYEONG), 4));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_CHEONGEUN), 19));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_GYEONGGONG), 49));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_EUNHYEONG), 34));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_GONGPO), 64));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_JUMAGAP), 65));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_HOSIN), 94));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_BOHO), 95));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_KWAESOK), 110));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_GICHEON), 96));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_JEUNGRYEOK), 111));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_PABEOP), 66));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_FALLEN_CHEONGEUN), 19));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_GWIGEOM), 63));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_MUYEONG), 78));
        map_affect_index_to_skill_index_.insert(std::make_pair(int(instance_base::AFFECT_HEUKSIN), 79));

        ppy_game_window_ = nullptr;

        sys_is_cool_time_ = true;
        sys_is_level_limit_ = true;
        play_time_ = 0;

        mbf_button_[MBT_LEFT] = python_player::MBF_SMART;
        mbf_button_[MBT_RIGHT] = python_player::MBF_CAMERA;
        mbf_button_[MBT_MIDDLE] = python_player::MBF_CAMERA;

        memset(effect_value_, 0, sizeof(effect_value_));

        is_dest_position_ = false;
        ix_dest_pos_ = 0;
        iy_dest_pos_ = 0;
        last_alarm_time_ = 0;

        clear();
    }

    python_player::~python_player(void) = default;
}

namespace metin2::inline client::inline app {

    auto get_abstract_player() -> abstract_player & {
        return get_python_player();
    }
}
