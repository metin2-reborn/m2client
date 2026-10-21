#include <metin2/client/app/abstract_player.h>
#include <metin2/core/timer.h>
#include <metin2/core/utils.h>
#include <metin2/world/actors/actor_instance.h>
#include "instance_base.h"

namespace metin2::inline client::inline characters {

    const float c_fFishingDistance = 600.0f;

    void instance_base::set_motion_mode(int motion_mode) {
        graphic_thing_instance_value_.set_motion_mode(motion_mode);
    }

    auto instance_base::motion_mode(uint32_t motion_index) -> int {
        return graphic_thing_instance_value_.motion_mode();
    }

    void instance_base::set_loop_motion(uint16_t motion, float blend_time /* =0.1f */, float speed_ratio) {
        graphic_thing_instance_value_.set_loop_motion(motion, blend_time, speed_ratio);
    }

    void instance_base::push_once_motion(uint16_t motion, float blend_time, float speed_ratio) {
        graphic_thing_instance_value_.push_once_motion(motion, blend_time, speed_ratio);
    }

    void instance_base::push_loop_motion(uint16_t motion, float blend_time, float speed_ratio) {
        graphic_thing_instance_value_.push_loop_motion(motion, blend_time, speed_ratio);
    }

    void instance_base::reset_local_time() {
        graphic_thing_instance_value_.reset_local_time();
    }

    void instance_base::set_end_stop_motion() {
        graphic_thing_instance_value_.set_end_stop_motion();
    }

    auto instance_base::is_lock() -> bool {
        return graphic_thing_instance_value_.is_lock();
    }

    void instance_base::start_fishing(float frot) {
        blend_rotation(frot);

        const pixel_position &p_pos_cur = graphic_thing_instance_value_.new_get_cur_pixel_position_ref();
        float rot = graphic_thing_instance_value_.target_rotation();

        pixel_position p_pos_fishing;
        el_plain_coord_get_rotated_pixel_position(p_pos_cur.x, p_pos_cur.y, c_fFishingDistance, rot, &p_pos_fishing.x, &p_pos_fishing.y);
        if (!background_get_water_height(p_pos_fishing, &p_pos_fishing.z))
            p_pos_fishing.z = p_pos_cur.z;

        math::vector3 fishing;
        pixel_position_to_vector3(p_pos_fishing, &fishing);
        graphic_thing_instance_value_.set_fishing_position(fishing);

        push_once_motion(race_motion_data::NAME_FISHING_THROW);
        push_loop_motion(race_motion_data::NAME_FISHING_WAIT);
    }
    void instance_base::stop_fishing() {
        graphic_thing_instance_value_.intercept_once_motion(race_motion_data::NAME_FISHING_STOP);
        push_loop_motion(race_motion_data::NAME_WAIT);
    }
    void instance_base::react_fishing() {
        push_once_motion(race_motion_data::NAME_FISHING_REACT);
        push_loop_motion(race_motion_data::NAME_FISHING_WAIT);
    }
    void instance_base::catch_success() {
        graphic_thing_instance_value_.intercept_once_motion(race_motion_data::NAME_FISHING_CATCH);
        push_loop_motion(race_motion_data::NAME_WAIT);
    }
    void instance_base::catch_fail() {
        graphic_thing_instance_value_.intercept_once_motion(race_motion_data::NAME_FISHING_FAIL);
        push_loop_motion(race_motion_data::NAME_WAIT);
    }

    auto instance_base::fishing_rot(int *pirot) -> bool {
        const pixel_position &p_pos_cur = graphic_thing_instance_value_.new_get_cur_pixel_position_ref();
        float character_rot = graphic_thing_instance_value_.rotation();

        for (float rot = 0.0f; rot <= 180.0f; rot += 10.0f) {
            pixel_position p_pos_fishing_right;
            el_plain_coord_get_rotated_pixel_position(p_pos_cur.x, p_pos_cur.y, c_fFishingDistance, character_rot + rot, &p_pos_fishing_right.x, &p_pos_fishing_right.y);
            if (background_is_water_pixel_position(p_pos_fishing_right)) {
                *pirot = character_rot + rot;
                return true;
            }

            pixel_position p_pos_fishing_left;
            el_plain_coord_get_rotated_pixel_position(p_pos_cur.x, p_pos_cur.y, c_fFishingDistance, character_rot - rot, &p_pos_fishing_left.x, &p_pos_fishing_left.y);
            if (background_is_water_pixel_position(p_pos_fishing_left)) {
                *pirot = character_rot - rot;
                return true;
            }
        }

        return false;
    }

    void instance_base::enable_changing_tcp_state() {
        enable_tcp_state_ = true;
    }

    void instance_base::disable_changing_tcp_state() {
        enable_tcp_state_ = false;
    }

    void instance_base::act_dual_emotion(instance_base &dst_inst, uint16_t motion_number1, uint16_t motion_number2) {
        if (!is_waiting()) {
            graphic_thing_instance_value_.set_loop_motion(race_motion_data::NAME_WAIT, 0.05f);
        }
        if (!dst_inst.is_waiting()) {
            dst_inst.graphic_thing_instance_value_.set_loop_motion(race_motion_data::NAME_WAIT, 0.05f);
        }

        const float emotion_distance = 100.0f;
        const pixel_position &main_position = new_get_cur_pixel_position_ref();
        const pixel_position &target_position = dst_inst.new_get_cur_pixel_position_ref();
        pixel_position direction_value = main_position - target_position;
        float distance = sqrtf((direction_value.x * direction_value.x) + (direction_value.y * direction_value.y));
        pixel_position dst_position;
        dst_position.x = target_position.x + (direction_value.x / distance) * emotion_distance;
        dst_position.y = target_position.y + (direction_value.y / distance) * emotion_distance;

        uint32_t cur_time = el_timer_get_server_m_sec() + 500;
        push_tcp_state_expanded(cur_time, dst_position, 0.0f, FUNC_EMOTION, (static_cast<uint32_t>(motion_number2) << 16) | static_cast<uint16_t>(motion_number1), dst_inst.virtual_id());

        disable_changing_tcp_state();
        dst_inst.disable_changing_tcp_state();

        if (is_main_instance() || dst_inst.is_main_instance()) {
            abstract_player &player = get_abstract_player();
            player.start_emotion_process();
        }
    }

    void instance_base::act_emotion(uint32_t motion_number) {
        push_once_motion(motion_number);
    }
}
