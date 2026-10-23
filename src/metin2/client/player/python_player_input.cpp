#include <metin2/client/app/python_application.h>
#include <metin2/core/timer.h>
#include <metin2/graphics/camera.h>
#include <metin2/log/log.h>
#include <metin2/script/python_utils.h>
#include <metin2/world/actors/game_util.h>
#include "python_player.h"
#include "python_player_event_handler.h"

namespace metin2::inline client::inline player {

    const int c_iFastestSendingCount = 3;
    const int c_iSlowestSendingCount = 3;
    const float c_fFastestSendingDelay = 1.0f / float(c_iFastestSendingCount);
    const float c_fSlowestSendingDelay = 1.0f / float(c_iSlowestSendingCount);
    const float c_fRotatingStepTime = 0.5f;

    const float c_fComboDistance = 250.0f;
    const float c_fClickDistance = 300.0f;

    auto python_player::pickable_distance() -> uint32_t {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (inst_main)
            if (inst_main->is_mounting_horse())
                return 500;

        return 300;
    }

    void python_player::pick_close_item() {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return;

        pixel_position p_pos_main;
        inst_main->new_get_pixel_position(&p_pos_main);

        uint32_t item_id;
        python_item &item = get_python_item();
        if (!item.close_item(p_pos_main, &item_id, pickable_distance()))
            return;

        send_click_item_packet(item_id);
    }

    auto python_player::is_target() -> bool {
        return 0 != target_vid();
    }

    auto python_player::is_same_target_vid(uint32_t vid) -> bool {
        return vid == target_vid();
    }

    auto python_player::target_vid() -> uint32_t {
        return target_vid_;
    }

    auto python_player::get_target_vid() -> uint32_t {
        return target_vid();
    }

    auto python_player::target_actor_ptr() -> instance_base * {
        return new_find_actor_ptr(target_vid());
    }

    void python_player::set_target_vid(uint32_t vid) {
        target_vid_ = vid;
    }

    void python_player::set_target(uint32_t vid, bool force_change) {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return;

        if (!inst_main->can_change_target()) {
            return;
        }

        uint32_t current_time = get_timer().current_millisecond();

        if (is_same_target_vid(vid)) {
            if (vid == inst_main->virtual_id()) {
                set_target_vid(0);

                inst_main->on_untargeted();
                inst_main->clear_fly_target_instance();
                get_python_network_stream().send_target_packet(0);
                return;
            }
            target_end_time_ = current_time + 1000;
            return;
        }

        if (force_change) {
            target_end_time_ = current_time + 2000;
        } else {
            if (target_end_time_ > current_time) {
                return;
            }

            target_end_time_ = current_time + 1000;
        }

        if (is_target()) {
            instance_base *targeted_instance = target_actor_ptr();
            if (targeted_instance)
                targeted_instance->on_untargeted();
        }

        instance_base *inst_target = get_python_character_manager().instance_ptr(vid);
        if (inst_target) {
            if (inst_main->is_targetable_instance(*inst_target)) {
                set_target_vid(vid);

                inst_target->on_targeted();
                inst_main->set_fly_target_instance(*inst_target);
                inst_main->graphic_thing_instance_ref().set_fly_event_handler(get_python_player_event_handler().normal_bow_attack_fly_event_handler(inst_main, inst_target));
                get_python_network_stream().send_target_packet(vid);

                return;
            }
        }

        set_target_vid(0);

        inst_main->clear_fly_target_instance();
        get_python_network_stream().send_target_packet(0);
    }

    auto python_player::change_target_to_picked_instance() -> bool {
        uint32_t vid;
        if (!get_python_character_manager().old_get_picked_instance_vid(&vid))
            return false;

        set_target(vid);
        return true;
    }

    auto python_player::dead_target_instance_ptr() -> instance_base * {
        if (!is_target())
            return nullptr;

        instance_base *inst_target = target_actor_ptr();

        if (!inst_target)
            return nullptr;

        if (!inst_target->is_dead())
            return nullptr;

        return inst_target;
    }

    auto python_player::alive_target_instance_ptr() -> instance_base * {
        if (!is_target())
            return nullptr;

        instance_base *inst_target = target_actor_ptr();

        if (!inst_target)
            return nullptr;

        if (inst_target->is_dead())
            return nullptr;

        return inst_target;
    }

    void python_player::open_character_menu(uint32_t victim_actor_id) {
        instance_base *inst_main = get_python_player().new_get_main_actor_ptr();
        if (!inst_main)
            return;

        instance_base *inst_target = get_python_character_manager().instance_ptr(victim_actor_id);
        if (!inst_target)
            return;

        if (!inst_target->is_pc() && !inst_target->is_building())
            return;

        py_call_class_member_func(ppy_game_window_, "set_pc_target_board", Py_BuildValue("(is)", inst_target->virtual_id(), inst_target->name_string()));
    }

    void python_player::on_click_item(instance_base &inst_main, uint32_t item_id) {
    }

    void python_player::on_click_actor(instance_base &inst_main, uint32_t picked_actor_id, bool is_auto) {
        if (MODE_USE_SKILL == reserved_mode_) {
            if (target_vid() == picked_actor_id)
                return;

            // While charging with Tanhwangyeok, clicking another target redirects the charge.
            if (check_dash_affect(inst_main)) {
                vid_reserved_ = picked_actor_id;
                return;
            }
        }

        clear_reserved_action();

        instance_base *pk_inst_victim = new_find_actor_ptr(picked_actor_id);
        instance_base &rk_inst_victim = *pk_inst_victim;
        if (!pk_inst_victim)
            return;

        // A beginner horse cannot attack but must still be able to click NPCs.
        if (inst_main.is_attackable_instance(*pk_inst_victim))
            if (!can_attack_impl())
                return;

        if (!inst_main.new_is_clickable_distance_dest_instance(rk_inst_victim)) {
            reserve_click_actor(picked_actor_id);
            return;
        }

        if (rk_inst_victim.is_npc()) {
            send_click_actor_packet(rk_inst_victim);
        }

        inst_main.new_stop();
        return;
    }

    void python_player::on_press_actor(instance_base &inst_main, uint32_t picked_actor_id, bool is_auto) {
        if (MODE_USE_SKILL == reserved_mode_) {
            if (target_vid() == picked_actor_id)
                return;

            // While charging with Tanhwangyeok, clicking another target redirects the charge.
            if (check_dash_affect(inst_main)) {
                vid_reserved_ = picked_actor_id;
                return;
            }
        }

        change_target_to_picked_instance();
        clear_reserved_action();

        if (!can_attack_impl())
            return;

        instance_base *pk_inst_victim = new_find_actor_ptr(picked_actor_id);
        if (!pk_inst_victim)
            return;

        instance_base &rk_inst_victim = *pk_inst_victim;

        if (is_auto) {
            if (inst_main.is_attackable_instance(rk_inst_victim))
                set_auto_attack_target_actor_id(rk_inst_victim.virtual_id());
        }

        if (inst_main.is_bow_mode()) {
            if (inst_main.is_attackable_instance(rk_inst_victim))
                if (!can_shot(inst_main, rk_inst_victim))
                    return;
        }

        if (!inst_main.new_is_clickable_distance_dest_instance(rk_inst_victim)) {
            reserve_click_actor(picked_actor_id);
            return;
        }

        if (!inst_main.is_attackable_instance(rk_inst_victim)) {
            return;
        }

        python_player_event_handler &player_event_handler = get_python_player_event_handler();
        inst_main.new_attack_to_dest_instance_direction(rk_inst_victim, player_event_handler.normal_bow_attack_fly_event_handler(&inst_main, &rk_inst_victim));
    }

    void python_player::on_press_item(instance_base &inst_main, uint32_t picked_item_id) {
        static uint32_t last_pick_item_id = 0;

        if (last_pick_item_id == picked_item_id) {
            metin2::log::debug("An item is already picked up");
            return;
        }

        clear_reserved_action();
        clear_auto_attack_target_actor_id();

        python_item &item = get_python_item();

        pixel_position p_pos_picked_item;
        if (!item.ground_item_position(picked_item_id, &p_pos_picked_item))
            return;

        if (!inst_main.new_is_clickable_distance_dest_pixel_position(p_pos_picked_item)) {
            reserve_click_item(picked_item_id);
            return;
        }

        inst_main.new_stop();
        send_click_item_packet(picked_item_id);
    }

    void python_player::on_click_ground(instance_base &inst_main, const pixel_position &p_pos_picked_ground) {
        if (!is_movable_ground_distance(inst_main, p_pos_picked_ground))
            return;

        if (inst_main.new_move_to_dest_pixel_position_direction(p_pos_picked_ground))
            show_picked_effect(p_pos_picked_ground);
    }

    void python_player::set_movable_ground_distance(float distance) {
        movable_ground_distance_ = distance;
    }

    auto python_player::is_movable_ground_distance(instance_base &inst_main, const pixel_position &p_pos_picked_ground) -> bool {
        float distance = inst_main.new_get_distance_from_dest_pixel_position(p_pos_picked_ground);

        if (distance < movable_ground_distance_)
            return false;

        return true;
    }

    void python_player::on_press_ground(instance_base &inst_main, const pixel_position &p_pos_picked_ground) {
        clear_reserved_action();
        clear_auto_attack_target_actor_id();

        if (new_cancel_fishing())
            return;

        if (!is_movable_ground_distance(inst_main, p_pos_picked_ground))
            return;

        if (!inst_main.new_move_to_dest_pixel_position_direction(p_pos_picked_ground)) {
            reserve_click_ground(p_pos_picked_ground);
            return;
        }
    }

    void python_player::on_press_screen(instance_base &inst_main) {
        clear_reserved_action();

        new_move_to_mouse_screen_direction();
    }

    auto python_player::new_move_to_direction(float dir_rot) -> bool {
        if (is_open_private_shop())
            return true;

        clear_reserved_action();

        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return false;

        if (inst_main->is_lock())
            if (!inst_main->is_using_moving_skill())
                return true;

        camera *cmr_cur = get_camera_manager().current_camera();
        if (cmr_cur) {
            float cmr_cur_rot = camera_rotation_to_character_rotation(cmr_cur->get_roll());

            if (is_cmr_rot_) {
                float sig_dir_rot = dir_rot;
                if (sig_dir_rot > 180.0f)
                    sig_dir_rot = sig_dir_rot - 360.0f;

                float rot_rat = sig_dir_rot;
                if (rot_rat > 90.0f)
                    rot_rat = (180.0f - rot_rat);
                else if (rot_rat < -90.0f)
                    rot_rat = (-180.0f - rot_rat);

                float elapsed_time = get_python_application().global_elapsed_time();

                float rot_deg = -cmr_rot_spd_ * elapsed_time * rot_rat / 90.0f;
                cmr_cur->roll(rot_deg);
            }

            dir_rot = fmod(360.0f + cmr_cur_rot + dir_rot, 360.0f);
        }

        inst_main->new_move_to_direction(dir_rot);

        return true;
    }

    void python_player::new_stop() {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return;

        inst_main->new_stop();
        is_left_ = false;
        is_right_ = false;
        is_up_ = false;
        is_down_ = false;
    }

    auto python_player::new_cancel_fishing() -> bool {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return false;

        if (inst_main->is_fishing()) {
            static uint32_t last_cancel_time = 0;
            if (get_timer().current_millisecond() < last_cancel_time + 500)
                return false;

            get_python_network_stream().send_fishing_packet(0);
            last_cancel_time = get_timer().current_millisecond();
            return true;
        }

        return false;
    }

    void python_player::new_fishing() {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return;

        if (inst_main->is_fishing()) {
            get_python_network_stream().send_fishing_packet(0);
        } else {
            if (inst_main->can_fishing()) {
                int irot;
                if (inst_main->fishing_rot(&irot))
                    get_python_network_stream().send_fishing_packet(irot);
                else {
                    py_call_class_member_func(ppy_game_window_, "on_fishing_wrong_place", Py_BuildValue("()"));
                }
            }
        }
    }

    void python_player::new_attack() {
        if (is_open_private_shop())
            return;

        if (!can_attack_impl())
            return;

        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return;

        if (inst_main->is_bow_mode()) {

            instance_base *inst_target = alive_target_instance_ptr();
            if (!inst_target) {
                change_target_to_picked_instance();
                inst_target = alive_target_instance_ptr();
            }

            if (inst_target) {
                if (!can_shot(*inst_main, *inst_target))
                    return;

                if (!inst_main->new_is_clickable_distance_dest_instance(*inst_target)) {
                    reserve_click_actor(inst_target->virtual_id());
                    return;
                }

                if (inst_main->is_attackable_instance(*inst_target)) {
                    inst_main->new_look_at_dest_instance(*inst_target);
                    inst_main->new_attack_to_dest_instance_direction(*inst_target);
                }
            } else {
                py_call_class_member_func(ppy_game_window_, "on_cannot_use_skill", Py_BuildValue("(is)", main_character_index(), "NEED_TARGET"));
                return;
            }
        } else if (is_dir_key_) {
            float dir_rot = 0.0f;
            new_get_multi_key_dir_rotation(is_left_, is_right_, is_up_, is_down_, &dir_rot);

            camera *cmr_cur = get_camera_manager().current_camera();
            if (cmr_cur) {
                float cmr_cur_rot = camera_rotation_to_character_rotation(cmr_cur->get_roll());

                dir_rot = fmod(360.0f + cmr_cur_rot + dir_rot, 360.0f);
            }

            inst_main->new_attack(dir_rot);
        } else {
            // Unarmed attacks are not supported on horseback.
            if (inst_main->is_mounting_horse()) {
                if (inst_main->is_hand_mode())
                    return;
            }

            inst_main->new_attack();
        }
    }

    auto python_player::new_is_empty_reserved_delay_time(float elapsed_time) -> bool {
        reserved_delay_time_ -= elapsed_time;

        if (reserved_delay_time_ <= 0.0f) {
            reserved_delay_time_ = 0.0f;
            return true;
        }

        return false;
    }

    void python_player::new_set_auto_camera_rotation_speed(float rot_spd) {
        cmr_rot_spd_ = rot_spd;
    }

    void python_player::new_reset_camera_rotation() {
        camera *cmr_cur = get_camera_manager().current_camera();
        python_application &app = get_python_application();

        cmr_cur->end_drag();

        app.set_cursor_num(python_application::NORMAL);
        if (python_application::CURSOR_MODE_HARDWARE == app.get_cursor_mode())
            app.set_cursor_visible(true);
    }

    auto python_player::can_shot(instance_base &inst_main, instance_base &inst_target) -> bool {
        if (!has_enough_arrow()) {
            py_call_class_member_func(ppy_game_window_, "on_cannot_shot", Py_BuildValue("(is)", main_character_index(), "EMPTY_ARROW"));
            return false;
        }

        if (inst_main.is_in_safe()) {
            py_call_class_member_func(ppy_game_window_, "on_cannot_shot", Py_BuildValue("(is)", main_character_index(), "IN_SAFE"));
            return false;
        }

        if (inst_target.is_in_safe()) {
            py_call_class_member_func(ppy_game_window_, "on_cannot_shot", Py_BuildValue("(is)", main_character_index(), "DEST_IN_SAFE"));
            return false;
        }

        return true;
    }

    auto python_player::can_move_impl() -> bool {
        if (is_processing_emotion()) {
            return false;
        }

        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return false;

        if (!inst_main->can_move()) {
            if (!inst_main->is_using_moving_skill())
                return false;
        }

        return true;
    }

    auto python_player::can_attack_impl() -> bool {
        if (is_processing_emotion()) {
            return false;
        }

        if (is_open_private_shop())
            return false;

        if (is_observer_mode())
            return false;

        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main)
            return false;

        // TODO: the server should do this check. New mounts cannot attack until horse riding (skill 109)
        // reaches grade 1 or level 11; mounts in the unique slot are exempt, so only new mounts are checked.
        if (inst_main->is_mounting_horse() && inst_main->is_new_mount() && (skill_grade(109) < 1 && skill_level(109) < 11)) {
            return false;
        }

        return inst_main->can_attack();
    }

    void python_player::new_get_multi_key_dir_rotation(bool is_left, bool is_right, bool is_up, bool is_down, float *dir_rot) {
        float scr_x = 0.0f;
        float scr_y = 0.0f;

        if (is_left)
            scr_x = 0.0f;
        else if (is_right)
            scr_x = 1.0f;
        else
            scr_x = 0.5f;

        if (is_up)
            scr_y = 0.0f;
        else if (is_down)
            scr_y = 1.0f;
        else
            scr_y = 0.5f;

        new_get_mouse_dir_rotation(scr_x, scr_y, dir_rot);
    }

    void python_player::new_get_mouse_dir_rotation(float scr_x, float scr_y, float *dir_rot) {
        int32_t width_value = widgets::get_window_manager().screen_width();
        int32_t height = widgets::get_window_manager().screen_height();
        int scr_pos_x = width_value * scr_x;
        int scr_pos_y = height * scr_y;
        int scr_width = width_value;
        int scr_height = height;
        int scr_center_x = scr_width / 2;
        int scr_center_y = scr_height / 2;

        float finput_rotation = degree_from_position(scr_pos_x, scr_pos_y, scr_center_x, scr_center_y);
        *dir_rot = finput_rotation;
    }

    auto python_player::degree_from_position(int ix, int iy, int half_width, int half_height) -> float {
        math::vector3 vt_dir(float(ix - half_width), float(iy - half_height), 0.0f);
        vt_dir = math::normalize(vt_dir);

        math::vector3 vt_stan(0, -1, 0);
        float ret = math::to_degree(acosf(math::dot(vt_dir, vt_stan)));

        if (vt_dir.x < 0.0f)
            ret = 360.0f - ret;

        return 360.0f - ret;
    }

    void python_player::clear_reserved_action() {
        reserved_mode_ = MODE_NONE;
    }

    void python_player::reserve_click_item(uint32_t item_id) {
        reserved_mode_ = MODE_CLICK_ITEM;
        iid_reserved_ = item_id;
    }

    void python_player::reserve_click_actor(uint32_t actor_id) {
        reserved_mode_ = MODE_CLICK_ACTOR;
        vid_reserved_ = actor_id;
    }

    void python_player::reserve_click_ground(const pixel_position &p_pos_picked_ground) {
        reserved_mode_ = MODE_CLICK_POSITION;
        p_pos_reserved_ = p_pos_picked_ground;
        reserved_delay_time_ = 0.1f;
    }

    auto python_player::is_reserved_use_skill(uint32_t skill_slot_index) -> bool {
        if (MODE_USE_SKILL != reserved_mode_)
            return false;

        if (skill_slot_index_reserved_ != skill_slot_index)
            return false;

        return true;
    }

    void python_player::reserve_use_skill(uint32_t actor_id, uint32_t skill_slot_index, uint32_t range) {
        reserved_mode_ = MODE_USE_SKILL;
        vid_reserved_ = actor_id;
        skill_slot_index_reserved_ = skill_slot_index;
        skill_range_reserved_ = range;

        // Shrink the range slightly; the distance was borderline.
        if (skill_range_reserved_ > 100)
            skill_range_reserved_ -= 10;
    }

    void python_player::clear_auto_attack_target_actor_id() {
        set_auto_attack_target_actor_id(0);
    }

    void python_player::set_auto_attack_target_actor_id(uint32_t vid) {
        auto_attack_target_vid_ = vid;
    }

    void python_player::reserve_process_click_actor() {
        instance_base *inst_main = new_get_main_actor_ptr();
        instance_base *inst_reserved = new_find_actor_ptr(vid_reserved_);
        if (!inst_main || !inst_reserved) {
            clear_reserved_action();
            return;
        }

        if (!inst_main->new_is_clickable_distance_dest_instance(*inst_reserved)) {
            inst_main->new_move_to_dest_instance_direction(*inst_reserved);
            return;
        }

        if (!inst_main->is_attackable_instance(*inst_reserved)) {
            inst_main->new_stop();
            send_click_actor_packet(*inst_reserved);
            clear_reserved_action();
            return;
        }

        if (inst_reserved->is_dead()) {
            clear_reserved_action();
            return;
        }

        if (inst_main->is_in_safe()) {
            py_call_class_member_func(ppy_game_window_, "on_cannot_attack", Py_BuildValue("(is)", main_character_index(), "IN_SAFE"));
            inst_main->new_stop();
            clear_reserved_action();
            return;
        }

        if (inst_reserved->is_in_safe()) {
            py_call_class_member_func(ppy_game_window_, "on_cannot_attack", Py_BuildValue("(is)", main_character_index(), "DEST_IN_SAFE"));
            inst_main->new_stop();
            clear_reserved_action();
            return;
        }

        // No attack while charging with Tanhwangyeok.
        if (check_dash_affect(*inst_main)) {
            return;
        }

        if (inst_main->is_bow_mode()) {
            if (!has_enough_arrow()) {
                py_call_class_member_func(ppy_game_window_, "on_cannot_shot", Py_BuildValue("(is)", main_character_index(), "EMPTY_ARROW"));
                inst_main->new_stop();
                clear_reserved_action();
                return;
            }
        }

        if (inst_reserved->virtual_id() != get_target_vid()) {
            set_target(inst_reserved->virtual_id());
        }

        inst_main->new_attack_to_dest_instance_direction(*inst_reserved);
        clear_reserved_action();
    }
}
