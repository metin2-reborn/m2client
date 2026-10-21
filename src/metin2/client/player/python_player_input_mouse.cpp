#include "python_player.h"

#include <metin2/client/app/python_application.h>
#include <metin2/client/background/python_background.h>
#include <metin2/graphics/camera.h>

namespace metin2::inline world::inline actors {
    extern auto is_huge_race(unsigned int vnum) -> bool;
}

namespace metin2::inline client::inline player {
    void python_player::new_set_mouse_move_state(int mbs) {
        if (MBS_PRESS == mbs) {
            new_move_to_mouse_screen_direction();

            is_dir_mov_ = true;
        } else if (MBS_CLICK == mbs) {
            new_stop();

            is_dir_mov_ = false;
        }
    }

    auto python_player::new_move_to_mouse_screen_direction() -> bool {
        widgets::window_manager &wnd_mgr = widgets::get_window_manager();

        int32_t l_mouse_x;
        int32_t l_mouse_y;
        wnd_mgr.mouse_position(l_mouse_x, l_mouse_y);

        int32_t scr_width = wnd_mgr.screen_width();
        int32_t scr_height = wnd_mgr.screen_height();
        float f_mouse_x = l_mouse_x / float(scr_width);
        float f_mouse_y = l_mouse_y / float(scr_height);

        float dir_rot;
        new_get_mouse_dir_rotation(f_mouse_x, f_mouse_y, &dir_rot);

        return new_move_to_direction(dir_rot);
    }

    void python_player::new_set_mouse_camera_state(int mbs) {
        python_application &app = get_python_application();
        python_background &bg = get_python_background();
        camera *cmr_cur = get_camera_manager().current_camera();

        if (cmr_cur) {
            if (MBS_PRESS == mbs) {
                widgets::window_manager &wnd_mgr = widgets::get_window_manager();

                int32_t mouse_x;
                int32_t mouse_y;
                wnd_mgr.mouse_position(mouse_x, mouse_y);

                cmr_cur->begin_drag(mouse_x, mouse_y);

                if (!bg.is_map_ready())
                    return;

                app.set_cursor_num(python_application::CAMERA_ROTATE);
                if (python_application::CURSOR_MODE_HARDWARE == app.get_cursor_mode())
                    app.set_cursor_visible(false, true);

            } else if (MBS_CLICK == mbs) {
                bool is_camera_drag = cmr_cur->end_drag();

                if (!bg.is_map_ready())
                    return;

                app.set_cursor_num(python_application::NORMAL);
                if (python_application::CURSOR_MODE_HARDWARE == app.get_cursor_mode())
                    app.set_cursor_visible(true);

                if (!is_camera_drag) {
                    change_target_to_picked_instance();

                    instance_base *inst_picked;
                    if (picked_actor_ptr(&inst_picked)) {
                        open_character_menu(inst_picked->virtual_id());
                    }
                }
            }
        }
    }

    void python_player::new_set_mouse_smart_state(int mbs, bool is_auto) {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main) return;

        if (is_open_private_shop()) {
            // Covers the moment before the shop packet arrives.
            is_smt_mov_ = false;
            return;
        }

        if (is_processing_emotion()) {
            return;
        }

        if (inst_main->is_sleep()) {
            return;
        }

        if (MBS_PRESS == mbs) {
            is_smt_mov_ = true;

            on_press_smart(*inst_main, is_auto);
        } else if (MBS_CLICK == mbs) {
            is_smt_mov_ = false;

            on_click_smart(*inst_main, is_auto);
        }
    }

    void python_player::on_press_smart(instance_base &inst_main, bool is_auto) {
        uint32_t picked_item_id_value;
        uint32_t picked_actor_id_value;
        pixel_position p_pos_picked_ground;

        bool is_picked_item_id = picked_item_id_impl(&picked_item_id_value);
        bool is_picked_actor_id = picked_actor_id(&picked_actor_id_value);
        bool is_picked_ground_pos = picked_ground_pos(&p_pos_picked_ground);

        if (is_picked_item_id) {
            on_press_item(inst_main, picked_item_id_value);
        } else if (is_picked_actor_id && picked_actor_id_value != main_character_index()) {
            on_press_actor(inst_main, picked_actor_id_value, is_auto);
        } else if (is_picked_ground_pos) {
            on_press_ground(inst_main, p_pos_picked_ground);
        } else {
            on_press_screen(inst_main);
        }
    }

    void python_player::on_click_smart(instance_base &inst_main, bool is_auto) {
        uint32_t picked_item_id_value;
        uint32_t picked_actor_id_value;
        pixel_position p_pos_picked_ground;
        if (picked_item_id_impl(&picked_item_id_value)) {
            on_click_item(inst_main, picked_item_id_value);
        } else if (picked_actor_id(&picked_actor_id_value)) {
            on_click_actor(inst_main, picked_actor_id_value, is_auto);
        } else if (picked_ground_pos(&p_pos_picked_ground)) {
            on_click_ground(inst_main, p_pos_picked_ground);
        } else {
            inst_main.new_stop();
        }
    }

    void python_player::show_picked_effect(const pixel_position &p_pos_picked_ground) {
        new_show_effect(EFFECT_PICK, p_pos_picked_ground);
    }

    auto python_player::new_set_mouse_func(int mbt, int mbf) -> bool {
        if (mbt >= MBT_NUM)
            return false;

        mbf_button_[mbt] = mbf;

        return true;
    }

    void python_player::set_quick_camera_mode(bool is_enable) {
        if (is_enable) {
        } else {
            new_set_mouse_camera_state(MBS_CLICK);
        }
    }

    auto python_player::new_set_mouse_state(int mbt, int mbs) -> bool {
        if (mbt >= MBT_NUM)
            return false;

        int mbf = mbf_button_[mbt];
        switch (mbf) {
        case MBF_MOVE:
            if (can_move_impl())
                new_set_mouse_move_state(mbs);
            break;
        case MBF_SMART:
            if (get_python_application().is_pressed(DIK_LCONTROL) || get_python_application().is_pressed(DIK_RCONTROL)) {
                new_attack();
            } else {
                new_set_mouse_smart_state(mbs, false);
            }
            break;
        case MBF_CAMERA:
            new_set_mouse_camera_state(mbs);
            break;
        case MBF_AUTO:
            new_set_mouse_smart_state(mbs, true);
            break;
        case MBF_ATTACK:
            new_attack();
            break;
        case MBF_SKILL:
            if (get_python_application().is_pressed(DIK_LCONTROL)) {
                new_set_mouse_camera_state(mbs);
            } else {
                if (MBS_PRESS == mbs) {
                    change_target_to_picked_instance();
                    use_current_skill();
                }
            }
            break;
        }

        return true;
    }

    void python_player::new_set_mouse_middle_button_state(int mb_state) {
        new_set_mouse_camera_state(mb_state);
    }

    void python_player::new_refresh_mouse_walking_direction() {
        instance_base *inst_main = new_get_main_actor_ptr();
        if (!inst_main) return;

        switch (reserved_mode_) {
        case MODE_CLICK_ITEM:
            {
                python_item &it = get_python_item();

                pixel_position p_pos_picked_item;
                if (it.ground_item_position(iid_reserved_, &p_pos_picked_item)) {
                    if (inst_main->new_get_distance_from_dest_pixel_position(p_pos_picked_item) < 20.0f) {
                        python_network_stream &net_stream = get_python_network_stream();

                        pixel_position p_pos_cur;
                        inst_main->new_get_pixel_position(&p_pos_cur);

                        float cur_rot = inst_main->rotation();
                        net_stream.send_character_state_packet(p_pos_cur, cur_rot, instance_base::FUNC_WAIT, 0);
                        send_click_item_packet(iid_reserved_);

                        inst_main->new_stop();

                        clear_reserved_action();
                    } else {
                        inst_main->new_move_to_dest_pixel_position_direction(p_pos_picked_item);
                    }
                } else {
                    clear_reserved_action();
                }

                break;
            }

        case MODE_CLICK_ACTOR:
            {
                reserve_process_click_actor();
                break;
            }

        case MODE_CLICK_POSITION:
            {
                if (!inst_main->is_lock())
                    if (new_is_empty_reserved_delay_time(get_python_application().global_elapsed_time())) {
                        inst_main->new_move_to_dest_pixel_position_direction(p_pos_reserved_);
                        clear_reserved_action();
                    }
                break;
            }

        case MODE_USE_SKILL:
            {
                instance_base *inst_reserved = new_find_actor_ptr(vid_reserved_);
                if (inst_reserved) {
                    float distance = inst_main->distance(inst_reserved);

                    // Huge races cannot be approached, so allow extra distance (#0000806: Tanhwangyeok failed on the water dragon).
                    if (is_huge_race(inst_reserved->race())) {
                        distance -= 200.0f; // TODO: hard-coded; this should use the bounding sphere.
                    }

                    if (distance < float(skill_range_reserved_)) {
                        // The target may have changed while approaching.
                        set_target(vid_reserved_);
                        if (use_skill(skill_slot_index_reserved_))
                            clear_reserved_action();
                    } else {
                        inst_main->new_move_to_dest_instance_direction(*inst_reserved);
                    }
                } else {
                    clear_reserved_action();
                }
                break;
            }
        case MODE_NONE:
            break;
        }

        if (is_smt_mov_)
            on_press_smart(*inst_main, false);

        if (is_dir_mov_)
            new_move_to_mouse_screen_direction();

        if (is_dir_key_)
            new_set_multi_dir_key_state(is_left_, is_right_, is_up_, is_down_);

        if (is_atk_key_)
            new_attack();

        combo_old_ = inst_main->combo_index();
    }

    auto python_player::is_right_button_skill_mode() -> bool {
        return MBF_SKILL == mbf_button_[MBT_RIGHT];
    }
}
