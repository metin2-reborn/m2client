#include <metin2/core/random.h>
#include <metin2/effects/effect_manager.h>
#include <metin2/log/log.h>
#include <metin2/sound/sound_manager.h>

#include <metin2/core/timer.h>
#include "actor_instance.h"
#include "race_data.h"

namespace metin2::inline world::inline actors {

    namespace {
        constexpr uint32_t HIT_FLASH_DURATION_MS = 70;
    }

    void actor_instance::set_battle_hit_effect(uint32_t id) {
        battle_hit_effect_id_ = id;
    }

    auto actor_instance::can_act() -> bool {
        if (is_dead())
            return false;

        if (is_stun())
            return false;

        if (is_paralysis())
            return false;

        if (is_faint())
            return false;

        if (is_sleep())
            return false;

        return true;
    }

    auto actor_instance::can_use_skill() -> bool {
        if (!can_act())
            return false;

        uint32_t cur_motion_index = current_motion_index();

        switch (cur_motion_index) {
        case race_motion_data::NAME_FISHING_THROW:
        case race_motion_data::NAME_FISHING_WAIT:
        case race_motion_data::NAME_FISHING_STOP:
        case race_motion_data::NAME_FISHING_REACT:
        case race_motion_data::NAME_FISHING_CATCH:
        case race_motion_data::NAME_FISHING_FAIL:
            return true;
            break;
        }

        if (is_using_skill()) {
            if (cur_race_motion_data_->is_cancel_enable_skill())
                return true;

            return false;
        }

        return true;
    }

    auto actor_instance::can_move() -> bool {
        if (!can_act())
            return false;

        if (is_lock())
            return false;

        return true;
    }

    auto actor_instance::can_attack() -> bool {
        if (!can_act())
            return false;

        if (is_using_skill()) {
            if (!can_cancel_skill())
                return false;
        }

        return true;
    }

    auto actor_instance::can_fishing() -> bool {
        if (!can_act())
            return false;

        if (is_using_skill())
            return false;

        switch (current_motion_index()) {
        case race_motion_data::NAME_WAIT:
        case race_motion_data::NAME_WALK:
        case race_motion_data::NAME_RUN:
            break;
        default:
            return false;
            break;
        }

        return true;
    }

    auto actor_instance::is_clickable_distance_dest_instance(actor_instance &inst_dst, float distance) -> bool {
        pixel_position p_pos_src;
        get_pixel_position(&p_pos_src);

        math::vector3 source_vector(p_pos_src);

        collision_point_instance_list &lstk_def_pt_inst = inst_dst.defending_point_instance_list_;
        collision_point_instance_list::iterator i;

        for (i = lstk_def_pt_inst.begin(); i != lstk_def_pt_inst.end(); ++i) {
            dynamic_sphere_instance_vector &vctk_def_sphere = (*i).sphere_instance_vector;

            dynamic_sphere_instance_vector::iterator j;
            for (j = vctk_def_sphere.begin(); j != vctk_def_sphere.end(); ++j) {
                dynamic_sphere_instance &sphere = (*j);

                const auto vv = math::vector3(sphere.position_value - source_vector);
                float mov_distance = math::length(vv);
                float atk_distance = sphere.radius + distance;

                if (atk_distance > mov_distance)
                    return true;
            }
        }

        return false;
    }

    void actor_instance::input_normal_attack_command(float dir_rot) {
        if (!can_input_normal_attack_command())
            return;

        atk_dir_rot_ = dir_rot;
        normal_attack(atk_dir_rot_);
    }

    auto actor_instance::input_combo_attack_command(float dir_rot) -> bool {
        atk_dir_rot_ = dir_rot;

        if (is_pre_input_)
            return false;

        if (0 == dwcur_combo_index_) {
            run_next_combo();
            return true;
        } else if (cur_race_motion_data_->is_combo_input_time_data()) {
            float elapsed_time = attacking_elapsed_time();

            // The input window has already passed.
            if (elapsed_time > cur_race_motion_data_->combo_input_end_time()) {
                if (is_bow_mode())
                    is_next_pre_input_ = true;
                return false;
            }

            if (elapsed_time > cur_race_motion_data_->next_combo_time()) // After the combo trigger time.
            {
                run_next_combo();
                return true;
            } else if (elapsed_time > cur_race_motion_data_->combo_input_start_time()) // Inside the pre-input window.
            {
                is_pre_input_ = true;
                return false;
            }
        } else {
            float elapsed_time = attacking_elapsed_time();
            if (elapsed_time > cur_race_motion_data_->motion_duration() * 0.9f) // After the combo trigger time.
            {
                run_next_combo();
                return true;
            }
        }

        return false;
    }

    void actor_instance::combo_process() {
        if (0 != dwcur_combo_index_) {
            if (!cur_race_motion_data_) {
                metin2::log::debug("Combo {}: no attacking motion data, combo cleared", dwcur_combo_index_);
                clear_combo();
                return;
            }

            float elapsed_time = attacking_elapsed_time();

            if (is_pre_input_) {
                if (elapsed_time > cur_race_motion_data_->next_combo_time()) {
                    run_next_combo();
                    is_pre_input_ = false;

                    return;
                }
            }
        } else {
            is_pre_input_ = false;

            if (!is_using_skill())      // is_next_pre_input_ is only used in bow mode,
                if (is_next_pre_input_) // which is why skills are only cancelled with a bow.
                {
                    run_next_combo();
                    is_next_pre_input_ = false;
                }
        }
    }

    void actor_instance::run_next_combo() {
        ++dwcur_combo_index_;

        uint16_t combo_index = dwcur_combo_index_;
        uint16_t combo_type = current_combo_type();

        if (combo_index == 0) {
            metin2::log::error("Cannot run next combo (type {}, index {})", combo_type, combo_index);
            return;
        }

        uint32_t combo_array_index = combo_index - 1;

        race_data::combo_data *combo_data;

        if (!cur_race_data_->combo_data_pointer(wcur_motion_mode_, combo_type, &combo_data)) {
            metin2::log::error("Cannot run next combo (type {}, index {}): no combo data for motion mode {}", combo_type, combo_index, wcur_motion_mode_);
            return;
        }

        if (combo_array_index >= combo_data->combo_index_vector_value.size()) {
            metin2::log::error("Cannot run next combo (type {}, index {}): combo array index {} is past the {} combo entries", combo_type, combo_index, combo_array_index, combo_data->combo_index_vector_value.size());
            return;
        }

        uint16_t wcur_combo_motion_index = combo_data->combo_index_vector_value[combo_array_index];
        combo_attack(wcur_combo_motion_index, atk_dir_rot_, 0.1f);

        if (dwcur_combo_index_ == combo_data->combo_index_vector_value.size()) {
            on_end_combo();
        }
    }

    void actor_instance::on_end_combo() {
        if (is_mounting_horse_impl()) {
            dwcur_combo_index_ = 1;
        }

        // The combo must not be reset here. It is reset when the last combo ends and
        // the motion automatically returns to wait.
    }

    void actor_instance::clear_combo() {
        dwcur_combo_index_ = 0;
        is_pre_input_ = false;
        cur_race_motion_data_ = nullptr;
    }

    auto actor_instance::is_attacking() -> bool {
        if (is_normal_attacking())
            return true;

        if (is_combo_attacking())
            return true;

        if (is_splash_attacking())
            return true;

        return false;
    }

    auto actor_instance::is_valid_attacking() -> bool {
        if (!cur_race_motion_data_)
            return false;

        if (!cur_race_motion_data_->is_attacking_motion())
            return false;

        const race_types::motion_attack_data *data = cur_race_motion_data_->motion_attack_data_pointer();
        float elapsed_time = attacking_elapsed_time();
        auto itor = data->hit_data_container_value.begin();
        for (; itor != data->hit_data_container_value.end(); ++itor) {
            const race_types::hit_data &hit_data = *itor;
            if (elapsed_time > hit_data.attack_start_time &&
                elapsed_time < hit_data.attack_end_time)
                return true;
        }

        return true;
    }

    auto actor_instance::can_check_attacking() -> bool {
        if (is_attacking())
            return true;

        return false;
    }

    auto actor_instance::is_in_splash_time() -> bool {
        if (splash_area_value_.disappearing_time > local_time())
            return true;

        return false;
    }

    auto actor_instance::is_normal_attacking() -> bool {
        if (!cur_race_motion_data_)
            return false;

        if (!cur_race_motion_data_->is_attacking_motion())
            return false;

        const race_types::motion_attack_data *data = cur_race_motion_data_->motion_attack_data_pointer();
        if (race_types::MOTION_TYPE_NORMAL != data->motion_type_value)
            return false;

        return true;
    }

    auto actor_instance::is_combo_attacking() -> bool {
        if (!cur_race_motion_data_)
            return false;

        if (!cur_race_motion_data_->is_attacking_motion())
            return false;

        const race_types::motion_attack_data *data = cur_race_motion_data_->motion_attack_data_pointer();
        if (race_types::MOTION_TYPE_COMBO != data->motion_type_value)
            return false;

        return true;
    }

    auto actor_instance::is_splash_attacking() -> bool {
        if (!cur_race_motion_data_)
            return false;

        if (cur_race_motion_data_->has_splash_motion_event())
            return true;

        return false;
    }

    auto actor_instance::is_moving_skill_impl(uint16_t skill_number) -> bool {
        enum {
            HORSE_DASH_SKILL_NUMBER = 137,
        };

        return HORSE_DASH_SKILL_NUMBER == skill_number;
    }

    auto actor_instance::is_act_emotion() -> bool {
        uint32_t cur_motion_index = current_motion_index();
        switch (cur_motion_index) {
        case race_motion_data::NAME_FRENCH_KISS_START + 0:
        case race_motion_data::NAME_FRENCH_KISS_START + 1:
        case race_motion_data::NAME_FRENCH_KISS_START + 2:
        case race_motion_data::NAME_FRENCH_KISS_START + 3:
        case race_motion_data::NAME_KISS_START + 0:
        case race_motion_data::NAME_KISS_START + 1:
        case race_motion_data::NAME_KISS_START + 2:
        case race_motion_data::NAME_KISS_START + 3:
            return true;
            break;
        }

        return false;
    }

    auto actor_instance::is_using_moving_skill() -> bool {
        return is_moving_skill_impl(cur_mot_node_.skill);
    }

    auto actor_instance::combo_index() -> uint32_t {
        return dwcur_combo_index_;
    }

    auto actor_instance::attacking_elapsed_time() -> float {
        return (local_time() - cur_mot_node_.start_time) * cur_mot_node_.speed_ratio;
    }

    auto actor_instance::can_input_normal_attack_command() -> bool {
        if (is_waiting())
            return true;

        if (is_normal_attacking()) {
            float elapsed_time = attacking_elapsed_time();

            if (elapsed_time > cur_race_motion_data_->motion_duration() * 0.9f)
                return true;
        }

        return false;
    }

    auto actor_instance::normal_attack(float dir_rot, float blend_time) -> bool {
        uint16_t motion_index;
        if (!cur_race_data_->normal_attack_index(wcur_motion_mode_, &motion_index))
            return false;

        blend_rotation(dir_rot, blend_time);
        set_advancing_rotation(dir_rot);
        intercept_once_motion(motion_index, 0.1f, 0, attack_speed());

        on_attack_impl(motion_index);

        new_set_atk_pixel_position(new_get_cur_pixel_position_ref());

        return true;
    }

    auto actor_instance::combo_attack(uint32_t motion_index, float dir_rot, float blend_time) -> bool {
        blend_rotation(dir_rot, blend_time);
        set_advancing_rotation(dir_rot);

        intercept_once_motion(motion_index, blend_time, 0, attack_speed());

        on_attack_impl(motion_index);

        new_set_atk_pixel_position(new_get_cur_pixel_position_ref());

        return true;
    }

    auto actor_instance::owner_vid() -> uint32_t {
        return owner_vid_;
    }

    auto actor_instance::owner_time() -> float {
        return local_time() - owner_base_time_;
    }

    auto is_huge_race(unsigned int vnum) -> bool {
        switch (vnum) {
        case 2493:
            return true;
        }
        return false;
    }

    auto actor_instance::can_push_dest_actor(actor_instance &actor_dst) -> bool {
        if (actor_dst.is_building())
            return false;

        if (actor_dst.is_door())
            return false;

        if (actor_dst.is_stone())
            return false;

        if (actor_dst.is_npc())
            return false;

        extern auto is_huge_race(unsigned int vnum) -> bool;
        if (is_huge_race(actor_dst.race()))
            return false;

        if (actor_dst.is_stun())
            return true;

        if (actor_dst.owner_vid() != virtual_id())
            return false;

        if (actor_dst.owner_time() > 3.0f)
            return false;

        return true;
    }

    auto is_party_hunting_race(unsigned int vnum) -> bool {
        return true;

        // Party hunting applies to all monsters.
    }

    void actor_instance::process_data_attack_success(const race_types::attack_data &attack_data, actor_instance &victim, const math::vector3 &position, uint32_t skill, bool is_send_packet) {
        if (race_types::HIT_TYPE_NONE == attack_data.hitting_type)
            return;

        insert_delay(attack_data.stiffen_time);

        if (can_push_dest_actor(victim) && attack_data.external_force > 0.0f) {
            push_circle(victim);

            const math::vector3 &victim_pos = victim.get_position();
            victim.physics_object_value_.increase_external_force(victim_pos, attack_data.external_force);
        }

        if (is_party_hunting_race(victim.race())) {
            if (skill) // Skills apply the invincibility time even to party hunting monsters.
                victim.invisible_time_ = get_timer().current_second() + attack_data.invisible_time;

            if (is_main_) // Polymorph balancing: only the player's own attacks set the invincibility time, not other players'.
                victim.invisible_time_ = get_timer().current_second() + attack_data.invisible_time;
        } else {
            victim.invisible_time_ = get_timer().current_second() + attack_data.invisible_time;
        }

        victim.insert_delay(attack_data.stiffen_time);
        victim.flash(HIT_FLASH_DURATION_MS);

        math::vector3 vec3_effect(victim.x_, victim.y_, victim.z_);

        extern auto is_huge_race(unsigned int vnum) -> bool;
        if (is_huge_race(victim.race())) {
            vec3_effect = position;
        }

        const math::vector3 &pos = get_position();

        float height = math::to_degree(atan2(-vec3_effect.x + pos.x, +vec3_effect.y - pos.y));

        if (victim.is_building() || victim.is_door()) {
            math::vector3 vec3_delta = vec3_effect - pos;
            vec3_delta = math::normalize(vec3_delta);
            vec3_delta *= 30.0f;

            effect_manager &eft_mgr = get_effect_manager();
            if (battle_hit_effect_id_)
                eft_mgr.create_effect(battle_hit_effect_id_, pos + vec3_delta, math::vector3(0.0f, 0.0f, 0.0f));
        } else {
            effect_manager &eft_mgr = get_effect_manager();
            if (battle_hit_effect_id_)
                eft_mgr.create_effect(battle_hit_effect_id_, vec3_effect, math::vector3(0.0f, 0.0f, height));
            if (battle_attach_effect_id_)
                victim.attach_effect_by_id(0, nullptr, battle_attach_effect_id_);
        }

        if (victim.is_building()) {
            // Buildings look wrong when they shake.
        } else if (victim.is_stone() || victim.is_door()) {
            hit_stone(victim);
        } else {
            if (race_types::HIT_TYPE_GOOD == attack_data.hitting_type || victim.is_resist_fallen()) {
                hit_good(victim);
            } else if (race_types::HIT_TYPE_GREAT == attack_data.hitting_type) {
                hit_greate(victim);
            } else {
                metin2::log::warn("Unknown hitting type {} in attacking data", attack_data.hitting_type);
            }
        }

        on_hit_impl(skill, victim, is_send_packet);
    }

    void actor_instance::on_shoot_damage() {
        if (is_stun()) {
            die();
        } else {
            shake(100);

            if (!is_lock() && !is_knock_down_motion() && !is_stand_up_motion()) {
                if (intercept_once_motion(race_motion_data::NAME_DAMAGE))
                    push_loop_motion(race_motion_data::NAME_WAIT);
            }
        }
    }

    void actor_instance::shake(uint32_t duration) {
        uint32_t cur_time = el_timer_get_m_sec();
        shake_time_ = cur_time + duration;
    }

    bool actor_instance::hit_flash_enabled_ = true;

    void actor_instance::flash(uint32_t duration) {
        if (hit_flash_enabled_)
            flash_time_ = el_timer_get_m_sec() + duration;
    }

    void actor_instance::shake_process() {
        if (shake_time_) {
            math::vector3 pos(0.0f, 0.0f, 0.0f);

            uint32_t cur_time = el_timer_get_m_sec();

            if (shake_time_ < cur_time) {
                shake_time_ = 0;
            } else {
                int shake_size = 10;

                switch (metin2::random::number(0, static_cast<int>(2) - 1)) {
                case 0: pos.x += metin2::random::number(0, static_cast<int>(shake_size) - 1); break;
                case 1: pos.x -= metin2::random::number(0, static_cast<int>(shake_size) - 1); break;
                }

                switch (metin2::random::number(0, static_cast<int>(2) - 1)) {
                case 0: pos.y += metin2::random::number(0, static_cast<int>(shake_size) - 1); break;
                case 1: pos.y -= metin2::random::number(0, static_cast<int>(shake_size) - 1); break;
                }

                switch (metin2::random::number(0, static_cast<int>(2) - 1)) {
                case 0: pos.z += metin2::random::number(0, static_cast<int>(shake_size) - 1); break;
                case 1: pos.z -= metin2::random::number(0, static_cast<int>(shake_size) - 1); break;
                }
            }

            world_matrix_._41 += pos.x;
            world_matrix_._42 += pos.y;
            world_matrix_._43 += pos.z;
        }
    }

    void actor_instance::hit_stone(actor_instance &victim) {
        if (victim.is_stun()) {
            victim.die();
        } else {
            victim.shake(100);
        }
    }

    void actor_instance::hit_good(actor_instance &victim) {
        if (victim.is_knock_down())
            return;

        if (victim.is_stun()) {
            victim.die();
        } else {
            victim.shake(100);

            if (!victim.is_lock()) {
                float rot_rad = math::to_radian(rotation());
                float victim_rot_rad = math::to_radian(victim.rotation());

                math::vector2 normal(sin(rot_rad), cos(rot_rad));
                math::vector2 victim_normal(sin(victim_rot_rad), cos(victim_rot_rad));

                normal = math::normalize(normal);
                victim_normal = math::normalize(victim_normal);

                float scalar = math::dot(normal, victim_normal);

                if (scalar < 0.0f) {
                    if (victim.intercept_once_motion(race_motion_data::NAME_DAMAGE))
                        victim.push_loop_motion(race_motion_data::NAME_WAIT);
                } else {
                    if (victim.intercept_once_motion(race_motion_data::NAME_DAMAGE_BACK))
                        victim.push_loop_motion(race_motion_data::NAME_WAIT);
                    else if (victim.intercept_once_motion(race_motion_data::NAME_DAMAGE))
                        victim.push_loop_motion(race_motion_data::NAME_WAIT);
                }
            }
        }
    }

    void actor_instance::hit_greate(actor_instance &victim) {
        if (victim.is_knock_down())
            return;
        if (victim.is_stand_up_motion())
            return;

        float rot_rad = math::to_radian(rotation());
        float victim_rot_rad = math::to_radian(victim.rotation());

        math::vector2 normal(sin(rot_rad), cos(rot_rad));
        math::vector2 victim_normal(sin(victim_rot_rad), cos(victim_rot_rad));

        normal = math::normalize(normal);
        victim_normal = math::normalize(victim_normal);

        float scalar = math::dot(normal, victim_normal);

        victim.shake(100);

        if (victim.is_using_skill())
            return;

        if (victim.is_stun()) {
            if (scalar < 0.0f)
                victim.intercept_once_motion(race_motion_data::NAME_DAMAGE_FLYING);
            else {
                if (!victim.intercept_once_motion(race_motion_data::NAME_DAMAGE_FLYING_BACK))
                    victim.intercept_once_motion(race_motion_data::NAME_DAMAGE_FLYING);
            }

            victim.is_real_dead_ = true;
        } else {
            if (scalar < 0.0f) {
                if (victim.intercept_once_motion(race_motion_data::NAME_DAMAGE_FLYING)) {
                    victim.push_once_motion(race_motion_data::NAME_STAND_UP);
                    victim.push_loop_motion(race_motion_data::NAME_WAIT);
                }
            } else {
                if (!victim.intercept_once_motion(race_motion_data::NAME_DAMAGE_FLYING_BACK)) {
                    if (victim.intercept_once_motion(race_motion_data::NAME_DAMAGE_FLYING)) {
                        victim.push_once_motion(race_motion_data::NAME_STAND_UP);
                        victim.push_loop_motion(race_motion_data::NAME_WAIT);
                    }
                } else {
                    victim.push_once_motion(race_motion_data::NAME_STAND_UP_BACK);
                    victim.push_loop_motion(race_motion_data::NAME_WAIT);
                }
            }
        }
    }

    void actor_instance::set_blending_position(const pixel_position &c_r_position, float blending_time) {
        pixel_position position;

        position.x = c_r_position.x - x_;
        position.y = c_r_position.y - y_;
        position.z = 0;

        physics_object_value_.set_last_position(position, blending_time);
    }

    void actor_instance::reset_blending_position() {
        physics_object_value_.initialize();
    }

    void actor_instance::blending_position(pixel_position *position) {
        if (physics_object_value_.is_blending()) {
            physics_object_value_.last_position(position);
            position->x += x_;
            position->y += y_;
            position->z += z_;
        } else {
            position->x = x_;
            position->y = y_;
            position->z = z_;
        }
    }

    void actor_instance::push_circle(actor_instance &victim) {
        const pixel_position &p_pos_atk = new_get_atk_pixel_position_ref();

        math::vector3 v3_src_pos(p_pos_atk.x, -p_pos_atk.y, p_pos_atk.z);

        const math::vector3 &c_rv3_src_pos = v3_src_pos;
        const math::vector3 &dst_pos = victim.get_position();

        math::vector3 direction;
        direction.x = dst_pos.x - c_rv3_src_pos.x;
        direction.y = dst_pos.y - c_rv3_src_pos.y;
        direction.z = 0.0f;
        direction = math::normalize(direction);

        victim.set_falling_direction(direction.x, direction.y);
    }

    auto actor_instance::is_invisible() -> bool {
        if (is_dead())
            return true;

        if (get_timer().current_second() >= invisible_time_)
            return false;

        return true;
    }

    void actor_instance::set_falling_direction(float fx, float fy) {
        physics_object_value_.set_direction(math::vector3(fx, fy, 0.0f));
    }
}
