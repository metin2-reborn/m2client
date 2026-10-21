#include <metin2/client/background/python_background.h>
#include <metin2/geometry/grp_math.h>
#include <metin2/world/actors/game_util.h>
#include "instance_base.h"

namespace metin2::inline client::inline characters {

    void instance_base::set_attack_speed(uint32_t atk_spd) {
        if (atk_spd > 1100)
            atk_spd = 0;

        graphic_thing_instance_value_.set_attack_speed(atk_spd / 100.0f);
        horse_.set_attack_speed(atk_spd);
    }

    void instance_base::set_move_speed(uint32_t mov_spd) {
        if (mov_spd > 1100)
            mov_spd = 0;

        graphic_thing_instance_value_.set_move_speed(mov_spd / 100.0f);
        horse_.set_move_speed(mov_spd);
    }

    void instance_base::set_rotation_speed(float rot_spd) {
        max_rot_spd_ = rot_spd;
    }

    void instance_base::new_stop() {
        if (is_syncing())
            return;

        if (is_lock())
            return;

        if (is_using_skill())
            return;

        if (!is_waiting())
            end_walking();

        graphic_thing_instance_value_.on_stop_impl();
    }

    void instance_base::new_sync_pixel_position(int32_t &p_pos_x, int32_t &p_pos_y) {
        graphic_thing_instance_value_.temp_push(p_pos_x, p_pos_y);
    }

    auto instance_base::new_can_move_to_dest_pixel_position(const pixel_position &p_pos_dst) -> bool {
        pixel_position p_pos_cur;
        new_get_pixel_position(&p_pos_cur);

        if (p_pos_cur.x == p_pos_dst.x && p_pos_cur.y == p_pos_dst.y)
            return false;

        return true;
    }

    auto c_instance_base_get_degree_from_position(float x, float y) -> float {
        math::vector3 vt_dir(floor(x), floor(y), 0.0f);
        vt_dir = math::normalize(vt_dir);

        math::vector3 vt_stan(0, -1, 0);
        float ret = math::to_degree(acosf(math::dot(vt_dir, vt_stan)));

        if (vt_dir.x < 0.0f)
            ret = 360.0f - ret;

        return ret;
    }

    auto instance_base::new_get_advancing_rotation_from_dir_pixel_position(const pixel_position &p_pos_dir) -> float {
        float dir_rot = c_instance_base_get_degree_from_position(p_pos_dir.x, -p_pos_dir.y);
        float clamp_dir_rot = clamp_degree(dir_rot);

        return clamp_dir_rot;
    }

    auto instance_base::new_get_advancing_rotation_from_pixel_position(const pixel_position &p_pos_src, const pixel_position &p_pos_dst) -> float {
        pixel_position p_pos_delta;
        p_pos_delta = p_pos_dst - p_pos_src;
        return new_get_advancing_rotation_from_dir_pixel_position(p_pos_delta);
    }

    void instance_base::new_set_advancing_rotation_from_dir_pixel_position(const pixel_position &p_pos_dir) {
        float clamp_dir_rot = new_get_advancing_rotation_from_dir_pixel_position(p_pos_dir);
        graphic_thing_instance_value_.set_advancing_rotation(clamp_dir_rot);

        float cur_rot = graphic_thing_instance_value_.rotation();
        float adv_rot = graphic_thing_instance_value_.advancing_rotation();

        rotating_direction_ = get_rotating_direction(cur_rot, adv_rot);
    }

    void instance_base::new_set_advancing_rotation_from_pixel_position(const pixel_position &p_pos_src, const pixel_position &p_pos_dst) {
        pixel_position p_pos_delta;
        p_pos_delta = p_pos_dst - p_pos_src;

        new_set_advancing_rotation_from_dir_pixel_position(p_pos_delta);
    }

    void instance_base::set_advancing_rotation(float rotation_value) {
        float frot_difference = get_degree_difference(rotation(), rotation_value);

        if (frot_difference > 45.0f)
            rot_spd_ = max_rot_spd_;
        else
            rot_spd_ = max_rot_spd_ * 5 / 12;

        graphic_thing_instance_value_.set_advancing_rotation(clamp_degree(rotation_value));
        rotating_direction_ = get_rotating_direction(graphic_thing_instance_value_.rotation(),
            graphic_thing_instance_value_.advancing_rotation());
    }

    void instance_base::start_walking() {
        graphic_thing_instance_value_.move();

        if (is_affect(AFFECT_GYEONGGONG)) {
            attached_affect_effects_[AFFECT_GYEONGGONG] = effect_container_attach_effect(EFFECT_AFFECT_GYEONGGONG);
        } else if (is_affect(AFFECT_KWAESOK)) {
            attached_affect_effects_[AFFECT_KWAESOK] = effect_container_attach_effect(EFFECT_AFFECT_KWAESOK);
        }
    }

    void instance_base::end_walking(float blending_time) {
        assert(!is_waiting() && "CInstanceBase::EndWalking");

        is_going_ = false;

        if (is_walking() || !is_attacked()) {
            graphic_thing_instance_value_.stop(blending_time);
            if (is_affect(AFFECT_GYEONGGONG)) {
                effect_container_detach_effect(EFFECT_AFFECT_GYEONGGONG);
            } else if (is_affect(AFFECT_KWAESOK)) {
                effect_container_detach_effect(EFFECT_AFFECT_KWAESOK);
            }
        }
    }

    void instance_base::end_walking_without_blending() {
        end_walking(0.0f);
    }

    auto instance_base::is_waiting() -> bool {
        return graphic_thing_instance_value_.is_waiting();
    }

    auto instance_base::is_walking() -> bool {
        return graphic_thing_instance_value_.is_moving();
    }

    auto instance_base::is_pushing() -> bool {
        return graphic_thing_instance_value_.is_pushing();
    }

    auto instance_base::is_attacked() -> bool {
        return graphic_thing_instance_value_.is_attacked();
    }

    auto instance_base::is_knock_down() -> bool {
        if (!graphic_thing_instance_value_.is_knock_down())
            return false;

        return true;
    }

    auto instance_base::is_attacking() -> bool {
        return graphic_thing_instance_value_.is_attacking();
    }

    auto instance_base::is_acting_emotion() -> bool {
        return graphic_thing_instance_value_.is_act_emotion();
    }

    void instance_base::new_move_to_dest_instance_direction(instance_base &inst_dst) {
        pixel_position p_pos_dst;
        inst_dst.new_get_pixel_position(&p_pos_dst);

        new_move_to_dest_pixel_position_direction(p_pos_dst);
    }

    auto instance_base::new_move_to_dest_pixel_position_direction(const pixel_position &p_pos_dst) -> bool {
        pixel_position p_pos_cur;
        new_get_pixel_position(&p_pos_cur);
        float dst_rot = new_get_advancing_rotation_from_pixel_position(p_pos_cur, p_pos_dst);

        return new_goto(p_pos_dst, dst_rot);
    }

    auto instance_base::new_goto(const pixel_position &p_pos_dst, float dst_rot) -> bool {
        if (is_syncing()) {
            return false;
        }

        if (graphic_thing_instance_value_.is_using_moving_skill()) {
            set_advancing_rotation(dst_rot);
            return true;
        }

        if (is_lock()) {
            return false;
        }

        if (!new_can_move_to_dest_pixel_position(p_pos_dst)) {
            if (!is_waiting())
                end_walking();

            return true;
        }

        new_set_src_pixel_position(new_get_cur_pixel_position_ref());
        new_set_dst_pixel_position(p_pos_dst);
        new_set_dst_pixel_position_z(new_get_src_pixel_position_ref().z);
        dst_rot_ = dst_rot;
        is_going_ = true;

        if (!is_walking()) {
            start_walking();
        }

        new_set_advancing_rotation_from_pixel_position(new_get_src_pixel_position_ref(), new_get_dst_pixel_position_ref());

        return true;
    }

    void instance_base::new_move_to_direction(float dir_rot) {
        if (is_syncing())
            return;

        if (graphic_thing_instance_value_.is_using_moving_skill()) {
            set_advancing_rotation(dir_rot);
            return;
        }

        if (is_lock())
            return;

        is_going_ = false;

        set_advancing_rotation(dir_rot);

        if (!is_walking()) {
            start_walking();
        }

        pixel_position p_pos_cur;
        new_get_pixel_position(&p_pos_cur);

        math::vector3 current_position(p_pos_cur.x, -p_pos_cur.y, p_pos_cur.z);
        math::vector3 destination_position;

        math::vector3 advance_direction(0.0f, -1.0f, 0.0f);
        math::matrix advance_matrix;
        advance_matrix = math::matrix_rotation_z(math::to_radian(dir_rot));
        advance_direction = math::transform_coord(advance_direction, advance_matrix);
        advance_direction = advance_direction * 300.0f;
        destination_position = advance_direction + current_position;

        pixel_position p_pos_dst;
        p_pos_dst.x = +destination_position.x;
        p_pos_dst.y = -destination_position.y;
        p_pos_dst.z = +destination_position.z;

        new_set_src_pixel_position(p_pos_cur);
        new_set_dst_pixel_position(p_pos_dst);
    }

    void instance_base::end_going() {
        if (!is_waiting())
            end_walking();

        is_going_ = false;
    }

    void instance_base::set_run_mode() {
        graphic_thing_instance_value_.set_run_mode();
    }

    void instance_base::set_walk_mode() {
        graphic_thing_instance_value_.set_walk_mode();
    }
}
