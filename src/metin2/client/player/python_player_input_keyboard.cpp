#include <metin2/log/log.h>
#include "python_player.h"

#include <metin2/client/characters/instance_base.h>
#include <metin2/input/keys.h>

namespace metin2::inline client::inline player {

    void python_player::set_attack_key_state(bool is_press) {
        if (is_press) {
            instance_base *inst_main = new_get_main_actor_ptr();
            if (inst_main)
                if (inst_main->is_fishing_mode()) {
                    new_fishing();
                    return;
                }
        }

        is_atk_key_ = is_press;
    }

    void python_player::new_set_single_dik_key_state(int dik_key, bool is_press) {
        if (new_cancel_fishing())
            return;

        switch (dik_key) {
        case DIK_UP:
            new_set_single_dir_key_state(DIR_UP, is_press);
            break;
        case DIK_DOWN:
            new_set_single_dir_key_state(DIR_DOWN, is_press);
            break;
        case DIK_LEFT:
            new_set_single_dir_key_state(DIR_LEFT, is_press);
            break;
        case DIK_RIGHT:
            new_set_single_dir_key_state(DIR_RIGHT, is_press);
            break;
        }
    }

    void python_player::new_set_single_dir_key_state(int dir_key, bool is_press) {
        switch (dir_key) {
        case DIR_UP:
            is_up_ = is_press;
            break;
        case DIR_DOWN:
            is_down_ = is_press;
            break;
        case DIR_LEFT:
            is_left_ = is_press;
            break;
        case DIR_RIGHT:
            is_right_ = is_press;
            break;
        }

        is_dir_key_ = (is_up_ || is_down_ || is_left_ || is_right_);

        new_set_multi_dir_key_state(is_left_, is_right_, is_up_, is_down_);
    }

    void python_player::new_set_multi_dir_key_state(bool is_left, bool is_right, bool is_up, bool is_down) {
        if (!can_move_impl())
            return;

        bool is_any = (is_left || is_right || is_up || is_down);

        if (is_any) {
            float dir_rot = 0.0f;
            new_get_multi_key_dir_rotation(is_left, is_right, is_up, is_down, &dir_rot);

            if (!new_move_to_direction(dir_rot)) {
                metin2::log::debug("Cannot move in the pressed direction");
                return;
            }
        } else {
            new_stop();
        }
    }

    auto python_player::degree_from_direction(int ud, int lr) -> float {
        switch (ud) {
        case KEYBOARD_UD_UP:
            if (KEYBOARD_LR_LEFT == lr) {
                return +45.0f;
            } else if (KEYBOARD_LR_RIGHT == lr) {
                return -45.0f;
            }

            return 0.0f;
            break;

        case KEYBOARD_UD_DOWN:
            if (KEYBOARD_LR_LEFT == lr) {
                return +135.0f;
            } else if (KEYBOARD_LR_RIGHT == lr) {
                return -135.0f;
            }

            return +180.0f;
            break;

        case KEYBOARD_UD_NONE:
            if (KEYBOARD_LR_LEFT == lr) {
                return +90.0f;
            } else if (KEYBOARD_LR_RIGHT == lr) {
                return -90.0f;
            }
            break;
        }

        return 0.0f;
    }
}
