#include "instance_base.h"

#include "python_character_manager.h"

#include <cmath>
#include <metin2/client/background/python_background.h>
#include <metin2/core/random.h>
#include <metin2/world/terrain/terrain.h>

namespace metin2::inline client::inline characters {
    auto new_unsigned_degree_to_signed_degree(float ud) -> float {
        float sd;
        if (ud > 180.0f) {
            sd = -(360.0f - ud);
        } else if (ud < -180.0f) {
            sd = +(360.0f + ud);
        } else {
            sd = ud;
        }

        return sd;
    }

    auto new_get_signed_degree_from_dir_pixel_position(const pixel_position &p_pos_dir) -> float {
        math::vector3 vt_dir(p_pos_dir.x, -p_pos_dir.y, p_pos_dir.z);
        math::vector3 vt_dir_normal;
        vt_dir_normal = math::normalize(vt_dir);

        math::vector3 vt_dir_normal_stan(0, -1, 0);
        float dir_rot = math::to_degree(acosf(math::dot(vt_dir_normal, vt_dir_normal_stan)));

        if (vt_dir_normal.x < 0.0f) {
            dir_rot = -dir_rot;
        }

        return dir_rot;
    }

    auto instance_base::is_fly_target_object() -> bool {
        return graphic_thing_instance_value_.is_fly_target_object();
    }

    auto instance_base::fly_target_distance() -> float {
        return graphic_thing_instance_value_.fly_target_distance();
    }

    void instance_base::clear_fly_target_instance() {
        graphic_thing_instance_value_.clear_fly_target();
    }

    void instance_base::set_fly_target_instance(instance_base &inst_dst) {
        graphic_thing_instance_value_.set_fly_target(inst_dst.graphic_thing_instance_ptr());
    }

    void instance_base::add_fly_target_position(const pixel_position &p_pos_dst) {
        graphic_thing_instance_value_.add_fly_target(p_pos_dst);
    }

    void instance_base::add_fly_target_instance(instance_base &inst_dst) {
        graphic_thing_instance_value_.add_fly_target(inst_dst.graphic_thing_instance_ptr());
    }

    auto instance_base::new_get_distance_from_dest_instance(instance_base &inst_dst) -> float {
        pixel_position p_pos_dst;
        inst_dst.new_get_pixel_position(&p_pos_dst);

        return new_get_distance_from_dest_pixel_position(p_pos_dst);
    }

    auto instance_base::new_get_distance_from_dest_pixel_position(const pixel_position &p_pos_dst) -> float {
        pixel_position p_pos_cur;
        new_get_pixel_position(&p_pos_cur);

        pixel_position p_pos_dir;
        p_pos_dir = p_pos_dst - p_pos_cur;

        return new_get_distance_from_dir_pixel_position(p_pos_dir);
    }

    auto instance_base::new_get_distance_from_dir_pixel_position(const pixel_position &p_pos_dir) -> float {
        return sqrtf(p_pos_dir.x * p_pos_dir.x + p_pos_dir.y * p_pos_dir.y);
    }

    auto instance_base::new_get_rotation() -> float {
        float cur_rot = rotation();
        return new_unsigned_degree_to_signed_degree(cur_rot);
    }

    auto instance_base::new_get_rotation_from_dir_pixel_position(const pixel_position &p_pos_dir) -> float {
        return new_get_signed_degree_from_dir_pixel_position(p_pos_dir);
    }

    auto instance_base::new_get_rotation_from_dest_pixel_position(const pixel_position &p_pos_dst) -> float {
        pixel_position p_pos_cur;
        new_get_pixel_position(&p_pos_cur);

        pixel_position p_pos_dir;
        p_pos_dir = p_pos_dst - p_pos_cur;

        return new_get_rotation_from_dir_pixel_position(p_pos_dir);
    }

    auto instance_base::new_get_rotation_from_dest_instance(instance_base &inst_dst) -> float {
        pixel_position p_pos_dst;
        inst_dst.new_get_pixel_position(&p_pos_dst);

        return new_get_rotation_from_dest_pixel_position(p_pos_dst);
    }

    void instance_base::new_get_random_position_in_fan_range(instance_base &inst_target, pixel_position *p_pos_dst) {
        float dst_dir_rot = new_get_rotation_from_dest_instance(inst_target);

        float rot = metin2::random::fnumber(dst_dir_rot - 10.0f, dst_dir_rot + 10.0f);

        math::matrix mat_rot;
        mat_rot = math::matrix_rotation_z(math::to_radian(-rot));

        math::vector3 src(0.0f, 8000.0f, 0.0f);
        math::vector3 pos;
        pos = math::transform_coord(src, mat_rot);

        const pixel_position &p_pos_cur = new_get_cur_pixel_position_ref();

        p_pos_dst->x = p_pos_cur.x + pos.x;
        p_pos_dst->y = p_pos_cur.y + pos.y;
        p_pos_dst->z = background_height(p_pos_cur.x, p_pos_cur.y);
    }

    auto instance_base::new_get_front_instance(instance_base **ppout_target_instance, float distance) -> bool {
        const float half_fan_rot_min = 10.0f;
        const float half_fan_rot_max = 50.0f;
        const float half_fan_rot_min_distance = 1000.0f;
        const float rpm = (half_fan_rot_max - half_fan_rot_min) / half_fan_rot_min_distance;

        float dst_rot = new_get_rotation();

        std::multimap<float, instance_base *> map_inst_near;
        {
            python_character_manager &chr_mgr = get_python_character_manager();
            python_character_manager::character_iterator i;
            for (i = chr_mgr.character_instance_begin(); i != chr_mgr.character_instance_end(); ++i) {
                instance_base *inst_each = *i;
                if (inst_each == this) {
                    continue;
                }

                if (!is_attackable_instance(*inst_each)) {
                    continue;
                }

                if (new_get_distance_from_dest_instance(*inst_each) > distance) {
                    continue;
                }

                float each_inst_distance = std::min(new_get_distance_from_dest_instance(*inst_each), half_fan_rot_min_distance);
                float each_inst_dir_rot = new_get_rotation_from_dest_instance(*inst_each);

                float half_fan_rot = (half_fan_rot_max - half_fan_rot_min) - rpm * each_inst_distance + half_fan_rot_min;

                // The shortest angle between the two headings, so the fan also works across 0/360°.
                const float delta_rot = std::fabs(std::fmod(each_inst_dir_rot - dst_rot + 540.0f, 360.0f) - 180.0f);

                if (delta_rot <= half_fan_rot)
                    map_inst_near.insert(std::multimap<float, instance_base *>::value_type(each_inst_distance, inst_each));
            }
        }

        if (map_inst_near.empty()) {
            return false;
        }

        *ppout_target_instance = map_inst_near.begin()->second;

        return true;
    }

    auto instance_base::new_get_instance_vector_in_fan_range(float skill_distance, instance_base &inst_target, std::vector<instance_base *> *vct_inst) -> bool {
        const float half_fan_rot_min = 20.0f;
        const float half_fan_rot_max = 40.0f;
        const float half_fan_rot_min_distance = 1000.0f;
        const float rpm = (half_fan_rot_max - half_fan_rot_min) / half_fan_rot_min_distance;

        float dst_dir_rot = new_get_rotation_from_dest_instance(inst_target);

        std::multimap<float, instance_base *> map_inst_near;
        {
            python_character_manager &chr_mgr = get_python_character_manager();
            python_character_manager::character_iterator i;
            for (i = chr_mgr.character_instance_begin(); i != chr_mgr.character_instance_end(); ++i) {
                instance_base *inst_each = *i;
                if (inst_each == this) {
                    continue;
                }

                if (!is_attackable_instance(*inst_each)) {
                    continue;
                }

                if (graphic_thing_instance_value_.is_clickable_distance_dest_instance(inst_each->graphic_thing_instance_value_, skill_distance)) {
                    float each_inst_distance = std::min(new_get_distance_from_dest_instance(*inst_each), half_fan_rot_min_distance);
                    float each_inst_dir_rot = new_get_rotation_from_dest_instance(*inst_each);

                    float half_fan_rot = (half_fan_rot_max - half_fan_rot_min) - rpm * each_inst_distance + half_fan_rot_min;

                    float min_dst_dir_rot = dst_dir_rot - half_fan_rot;
                    float max_dst_dir_rot = dst_dir_rot + half_fan_rot;

                    if (each_inst_dir_rot >= min_dst_dir_rot && each_inst_dir_rot <= max_dst_dir_rot)
                        map_inst_near.insert(std::multimap<float, instance_base *>::value_type(each_inst_distance, inst_each));
                }
            }
        }

        {
            auto i = map_inst_near.begin();
            for (i = map_inst_near.begin(); i != map_inst_near.end(); ++i)
                vct_inst->push_back(i->second);
        }

        if (vct_inst->empty())
            return false;

        return true;
    }

    auto instance_base::new_get_instance_vector_in_circle_range(float skill_distance, std::vector<instance_base *> *vct_inst) -> bool {
        std::multimap<float, instance_base *> map_inst_near;

        {
            python_character_manager &chr_mgr = get_python_character_manager();
            python_character_manager::character_iterator i;
            for (i = chr_mgr.character_instance_begin(); i != chr_mgr.character_instance_end(); ++i) {
                instance_base *inst_each = *i;

                if (inst_each == this)
                    continue;

                if (!is_attackable_instance(*inst_each))
                    continue;

                if (graphic_thing_instance_value_.is_clickable_distance_dest_instance(inst_each->graphic_thing_instance_value_, skill_distance)) {
                    float each_inst_distance = new_get_distance_from_dest_instance(*inst_each);
                    map_inst_near.insert(std::make_pair(each_inst_distance, inst_each));
                }
            }
        }

        {
            auto i = map_inst_near.begin();
            for (i = map_inst_near.begin(); i != map_inst_near.end(); ++i)
                vct_inst->push_back(i->second);
        }

        if (vct_inst->empty())
            return false;

        return true;
    }

    auto instance_base::new_is_clickable_distance_dest_pixel_position(const pixel_position &p_pos_dst) -> bool {
        float distance = new_get_distance_from_dest_pixel_position(p_pos_dst);

        if (distance > 150.0f)
            return false;

        return true;
    }

    auto instance_base::new_is_clickable_distance_dest_instance(instance_base &inst_dst) -> bool {
        float distance = 150.0f;

        if (is_bow_mode())
            distance = bow_range();

        if (inst_dst.is_npc())
            distance = 500.0f;

        if (inst_dst.is_resource())
            distance = 100.0f;

        return graphic_thing_instance_value_.is_clickable_distance_dest_instance(inst_dst.graphic_thing_instance_value_, distance);
    }

    auto instance_base::new_use_skill(uint32_t skill, uint32_t mot, uint32_t mot_loop_count, bool is_moving_skill) -> bool {
        if (is_dead())
            return false;

        if (is_stun())
            return false;

        if (is_knock_down())
            return false;

        if (is_moving_skill) {
            if (!is_walking())
                start_walking();

            is_going_ = true;
        } else {
            if (is_walking())
                end_walking();

            is_going_ = false;
        }

        float cur_rot = graphic_thing_instance_value_.target_rotation();
        set_advancing_rotation(cur_rot);

        graphic_thing_instance_value_.intercept_once_motion(race_motion_data::NAME_SKILL + mot, 0.1f, skill, 1.0f);

        graphic_thing_instance_value_.on_use_skill_impl(mot, mot_loop_count, is_moving_skill);

        if (mot_loop_count > 0)
            graphic_thing_instance_value_.set_motion_loop_count(mot_loop_count);

        return true;
    }

    void instance_base::new_attack() {
        float dir_rot = rotation();
        new_attack(dir_rot);
    }

    void instance_base::new_attack(float dir_rot) {
        if (is_dead())
            return;

        if (is_stun())
            return;

        if (is_knock_down())
            return;

        if (is_using_skill())
            return;

        if (is_walking())
            end_walking();

        is_going_ = false;

        if (is_poly()) {
            input_normal_attack(dir_rot);
        } else {
            if (horse_.is_mounting()) {
                input_combo_attack(dir_rot);
            } else {
                input_combo_attack(dir_rot);
            }
        }
    }

    void instance_base::new_attack_to_dest_pixel_position_direction(const pixel_position &p_pos_dst) {
        float dir_rot = new_get_rotation_from_dest_pixel_position(p_pos_dst);

        new_attack(dir_rot);
    }

    auto instance_base::new_attack_to_dest_instance_direction(instance_base &inst_dst, fly_event_handler *fly_handler) -> bool {
        return new_attack_to_dest_instance_direction(inst_dst);
    }

    auto instance_base::new_attack_to_dest_instance_direction(instance_base &inst_dst) -> bool {
        pixel_position p_pos_dst;
        inst_dst.new_get_pixel_position(&p_pos_dst);
        new_attack_to_dest_pixel_position_direction(p_pos_dst);

        return true;
    }

    void instance_base::attack_process() {
        if (!graphic_thing_instance_value_.can_check_attacking())
            return;

        instance_base *inst_last = nullptr;
        python_character_manager &chr_mgr = get_python_character_manager();
        python_character_manager::character_iterator i = chr_mgr.character_instance_begin();
        while (chr_mgr.character_instance_end() != i) {
            instance_base *inst_each = *i;
            ++i;

            if (!is_attackable_instance(*inst_each))
                continue;

            if (inst_each != this) {
                if (check_attacking(*inst_each)) {
                    inst_last = inst_each;
                }
            }
        }

        if (inst_last) {
            last_dmg_actor_vid_ = inst_last->virtual_id();
        }
    }

    void instance_base::input_normal_attack(float atk_dir_rot) {
        graphic_thing_instance_value_.input_normal_attack_command(atk_dir_rot);
    }

    void instance_base::input_combo_attack(float atk_dir_rot) {
        graphic_thing_instance_value_.input_combo_attack_command(atk_dir_rot);
        combo_process_impl();
    }

    void instance_base::run_normal_attack(float atk_dir_rot) {
        end_going();
        graphic_thing_instance_value_.normal_attack(atk_dir_rot);
    }

    void instance_base::run_combo_attack(float atk_dir_rot, uint32_t motion_index) {
        end_going();
        graphic_thing_instance_value_.combo_attack(motion_index, atk_dir_rot);
    }

    // Returns true when the movement is blocked.
    auto instance_base::check_advancing() -> bool {
        if (!is_main_instance() && !is_attacking()) {
            if (is_pc() && is_walking()) {
                python_character_manager &chr_mgr = get_python_character_manager();
                for (python_character_manager::character_iterator i = chr_mgr.character_instance_begin(); i != chr_mgr.character_instance_end(); ++i) {
                    instance_base *inst_each = *i;
                    if (inst_each == this)
                        continue;
                    if (!inst_each->is_door())
                        continue;

                    if (graphic_thing_instance_value_.test_actor_collision(inst_each->graphic_thing_instance_ref())) {
                        block_movement();
                        return true;
                    }
                }
            }
            return false;
        }

        if (graphic_thing_instance_value_.can_skip_collision()) {
            return false;
        }

        bool using_skill = graphic_thing_instance_value_.is_using_skill();

        adv_actor_vid_ = 0;
        uint32_t collision_count = 0;

        python_character_manager &chr_mgr = get_python_character_manager();
        for (python_character_manager::character_iterator i = chr_mgr.character_instance_begin(); i != chr_mgr.character_instance_end(); ++i) {
            instance_base *inst_each = *i;
            if (inst_each == this)
                continue;

            actor_instance &actor_self = graphic_thing_instance_value_;
            actor_instance &actor_each = inst_each->graphic_thing_instance_ref();

            // Doors still block movement while a skill is in use.
            if (using_skill && !actor_each.is_door())
                continue;

            if (actor_self.test_actor_collision(actor_each)) {
                collision_count++;
                if (collision_count == 2) {
                    actor_self.block_movement();
                    return true;
                }
                actor_self.adjust_dynamic_collision_movement(&actor_each);

                if (actor_self.test_actor_collision(actor_each)) {
                    actor_self.block_movement();
                    return true;
                } else {
                    new_move_to_dest_pixel_position_direction(new_get_dst_pixel_position_ref());
                }
            }
        }

        python_background &bg = get_python_background();
        const math::vector3 &position = graphic_thing_instance_value_.get_position();
        const math::vector3 &move_direction = graphic_thing_instance_value_.movement_vector_ref();

        // Long moves are checked in 10-unit steps so a thin blocking attribute is not skipped.
        int step = int(math::length(move_direction) / 10.0f);
        math::vector3 check_step = move_direction / float(step);
        math::vector3 check_position = position;
        for (int j = 0; j < step; ++j) {
            check_position += check_step;

            if (bg.is_attr_on(check_position.x, -check_position.y, terrain_impl::ATTRIBUTE_BLOCK)) {
                block_movement();
            }
        }

        math::vector3 next_position = position + move_direction;
        if (bg.is_attr_on(next_position.x, -next_position.y, terrain_impl::ATTRIBUTE_BLOCK)) {
            block_movement();
            return true;
        }

        return false;
    }

    auto instance_base::check_attacking(instance_base &inst_victim) -> bool {
        if (is_in_safe())
            return false;

        if (inst_victim.is_in_safe())
            return false;

        if (!graphic_thing_instance_value_.attacking_process(inst_victim.graphic_thing_instance_value_))
            return false;

        return true;
    }

    auto instance_base::is_normal_attacking() -> bool {
        return graphic_thing_instance_value_.is_normal_attacking();
    }

    auto instance_base::is_combo_attacking() -> bool {
        return graphic_thing_instance_value_.is_combo_attacking();
    }

    auto instance_base::is_using_skill() -> bool {
        return graphic_thing_instance_value_.is_using_skill();
    }

    auto instance_base::is_using_moving_skill() -> bool {
        return graphic_thing_instance_value_.is_using_moving_skill();
    }

    auto instance_base::can_cancel_skill() -> bool {
        return graphic_thing_instance_value_.can_cancel_skill();
    }

    auto instance_base::can_attack_horse_level() -> bool {
        if (!is_mounting_horse())
            return false;

        return horse_.can_attack();
    }

    auto instance_base::is_affect(uint32_t affect) -> bool {
        return affect_flag_container_value_.is_set(affect);
    }

    auto instance_base::normal_attack_index() -> MOTION_KEY {
        return graphic_thing_instance_value_.normal_attack_index();
    }

    auto instance_base::combo_index() -> uint32_t {
        return graphic_thing_instance_value_.combo_index();
    }

    auto instance_base::attacking_elapsed_time() -> float {
        return graphic_thing_instance_value_.attacking_elapsed_time();
    }

    void instance_base::process_hitting(uint32_t motion_key, instance_base *victim_instance) {
        assert(!"-_-" && "CInstanceBase::ProcessHitting");
    }

    void instance_base::process_hitting(uint32_t motion_key, uint8_t event_index, instance_base *victim_instance) {
        assert(!"-_-" && "CInstanceBase::ProcessHitting");
    }

    void instance_base::blending_position(pixel_position *pixel_position) {
        graphic_thing_instance_value_.blending_position(pixel_position);
    }

    void instance_base::set_blending_position(const pixel_position &pixel_position) {
        graphic_thing_instance_value_.set_blending_position(pixel_position);
    }

    void instance_base::revive() {
        is_going_ = false;
        graphic_thing_instance_value_.revive();

        attach_horse_saddle();
    }

    void instance_base::stun() {
        new_stop();
        graphic_thing_instance_value_.stun();

        attach_effect(EFFECT_STUN);
    }

    void instance_base::die() {
        detach_horse_saddle();

        if (is_affect(AFFECT_SPAWN))
            attach_effect(EFFECT_SPAWN_DISAPPEAR);

        clear_affects_impl();

        on_unselected();
        on_untargeted();

        graphic_thing_instance_value_.die();
    }

    void instance_base::hide() {
        graphic_thing_instance_value_.set_alpha_value(0.0f);
        graphic_thing_instance_value_.blend_alpha_value(0.0f, 0.1f);
    }

    void instance_base::show() {
        graphic_thing_instance_value_.set_alpha_value(1.0f);
        graphic_thing_instance_value_.blend_alpha_value(1.0f, 0.1f);
    }
}
