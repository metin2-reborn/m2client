#include <metin2/core/random.h>
#include <metin2/log/log.h>
#include <metin2/world/flying/fly_handler.h>
#include "actor_instance.h"
#include "race_data.h"

namespace metin2::inline world::inline actors {

    auto actor_instance::motion_type() -> uint32_t {
        if (!cur_race_motion_data_)
            return race_motion_data::TYPE_NONE;

        return cur_race_motion_data_->get_type();
    }

    void actor_instance::motion_event_process_impl(bool is_pc) {
        if (is_attacking()) {
            auto next_frame = uint32_t(attacking_elapsed_time() * game_fps);
            for (; cur_mot_node_.dwcur_frame < next_frame; ++cur_mot_node_.dwcur_frame) {
                motion_event_process();
                sound_event_process(!is_pc);
            }
        } else {
            motion_event_process();
            sound_event_process(!is_pc);

            ++cur_mot_node_.dwcur_frame;
        }
    }

    void actor_instance::motion_process(bool is_pc) {
        motion_event_process_impl(is_pc);
        current_motion_process();
        reserving_motion_process();
    }

    void actor_instance::horse_motion_process(bool is_pc) {
        motion_event_process_impl(is_pc);

        if (MOTION_TYPE_LOOP == cur_mot_node_.motion_type_value)
            if (cur_mot_node_.dwcur_frame >= cur_mot_node_.frame_count)
                cur_mot_node_.dwcur_frame = 0;
    }

    void actor_instance::reserving_motion_process() {
        if (motion_deque_value_.empty())
            return;

        reserving_motion_node &reserving_motion_node_value = motion_deque_value_.front();

        float current_time = local_time();
        if (reserving_motion_node_value.start_time > current_time)
            return;

        uint32_t next_motion_index = GET_MOTION_INDEX(reserving_motion_node_value.motion_key);
        switch (next_motion_index) {
        case race_motion_data::NAME_STAND_UP:
        case race_motion_data::NAME_STAND_UP_BACK:
            if (is_faint()) {

                set_end_stop_motion();

                // Delay every following motion by one second.
                auto itor = motion_deque_value_.begin();
                for (; itor != motion_deque_value_.end(); ++itor) {
                    reserving_motion_node &node = *itor;
                    node.start_time += 1.0f;
                }
                return;
            }
            break;
        }

        current_motion_node prev_motion_node = cur_mot_node_;

        motion_push_type motion_type = reserving_motion_node_value.motion_type_value;
        float speed_ratio = reserving_motion_node_value.speed_ratio;
        float blend_time = reserving_motion_node_value.blend_time;

        uint32_t motion_key = reserving_motion_node_value.motion_key;
        float start_time = reserving_motion_node_value.start_time;

        motion_deque_value_.pop_front();

        uint32_t current_motion_index = GET_MOTION_INDEX(motion_key);
        switch (current_motion_index) {
        case race_motion_data::NAME_STAND_UP:
        case race_motion_data::NAME_STAND_UP_BACK:
            if (is_dead()) {
                cur_mot_node_ = prev_motion_node;
                clear_motion();

                // Keep the final state of the previous motion.
                set_end_stop_motion();
                return;
            }
            break;
        }

        int loop_count;
        if (MOTION_TYPE_ONCE == motion_type)
            loop_count = 1;
        else
            loop_count = 0;

        set_motion_data set_mot_data;
        set_mot_data.mot_key = motion_key;
        set_mot_data.blend_time = blend_time;
        set_mot_data.speed_ratio = speed_ratio;
        set_mot_data.loop_count = loop_count;

        uint32_t real_motion_key = set_motion_impl(set_mot_data);

        if (0 == real_motion_key)
            return;

        // set_motion_impl may pick another motion at random, so the duration must come from the motion that is actually playing.
        float duration_time = motion_duration(real_motion_key) / speed_ratio;
        float end_time = start_time + duration_time;

        cur_mot_node_.skill = 0;
        cur_mot_node_.motion_type_value = motion_type;
        cur_mot_node_.speed_ratio = speed_ratio;
        cur_mot_node_.start_time = start_time;
        cur_mot_node_.end_time = end_time;
        cur_mot_node_.motion_key = real_motion_key;
        cur_mot_node_.dwcur_frame = 0;
        cur_mot_node_.frame_count = duration_time / (1.0f / game_fps);
    }

    void actor_instance::current_motion_process() {
        if (MOTION_TYPE_LOOP == cur_mot_node_.motion_type_value)
            if (cur_mot_node_.dwcur_frame >= cur_mot_node_.frame_count)
                cur_mot_node_.dwcur_frame = 0;

        if (is_dead())
            return;

        if (!motion_deque_value_.empty())
            return;

        float current_time = local_time();

        uint32_t motion_index = GET_MOTION_INDEX(cur_mot_node_.motion_key);

        bool is_looping = false;

        if (cur_race_motion_data_ && cur_race_motion_data_->is_loop_motion()) {
            if (cur_mot_node_.loop_count > 1 || cur_mot_node_.loop_count == -1) {
                if (current_time - cur_mot_node_.start_time > cur_race_motion_data_->loop_end_time()) {
                    cur_mot_node_.dwcur_frame = uint32_t(cur_race_motion_data_->loop_start_time() * game_fps);
                    set_local_time_impl(cur_mot_node_.start_time + cur_race_motion_data_->loop_start_time());
                    if (-1 != cur_mot_node_.loop_count)
                        --cur_mot_node_.loop_count;

                    is_looping = true;
                }
            } else if (!que_fly_target_.empty()) {
                if (!backup_fly_target_.is_object()) {
                    backup_fly_target_ = fly_target_value_;
                }

                if (current_time - cur_mot_node_.start_time > cur_race_motion_data_->loop_end_time()) {
                    cur_mot_node_.dwcur_frame = uint32_t(cur_race_motion_data_->loop_start_time() * game_fps);
                    set_local_time_impl(cur_mot_node_.start_time + cur_race_motion_data_->loop_start_time());

                    set_fly_target(que_fly_target_.front());
                    que_fly_target_.pop_front();

                    is_looping = true;
                }
            }
        }

        if (!is_looping) {
            if (current_time > cur_mot_node_.end_time) {
                if (backup_fly_target_.is_valid_target()) {
                    fly_target_value_ = backup_fly_target_;
                    backup_fly_target_.clear();
                }

                if (MOTION_TYPE_ONCE == cur_mot_node_.motion_type_value) {
                    switch (motion_index) {
                    case race_motion_data::NAME_DAMAGE_FLYING:
                    case race_motion_data::NAME_DAMAGE_FLYING_BACK:
                    case race_motion_data::NAME_DEAD:
                    case race_motion_data::NAME_INTRO_SELECTED:
                    case race_motion_data::NAME_INTRO_NOT_SELECTED:
                        cur_mot_node_.end_time += 3.0f;
                        set_end_stop_motion();
                        break;
                    default:
                        intercept_loop_motion(race_motion_data::NAME_WAIT);
                        break;
                    }
                } else if (MOTION_TYPE_LOOP == cur_mot_node_.motion_type_value) {
                    if (race_motion_data::NAME_WAIT == motion_index) {
                        push_loop_motion(race_motion_data::NAME_WAIT, 0.5f);
                    }
                }
            }
        }
    }

    void actor_instance::set_motion_mode(int motion_mode) {
        if (is_poly())
            motion_mode = race_motion_data::MODE_GENERAL;

        wcur_motion_mode_ = motion_mode;
    }

    auto actor_instance::motion_mode() -> int {
        return wcur_motion_mode_;
    }

    void actor_instance::set_motion_loop_count(int count) {
        assert(count >= -1 && count < 100);
        cur_mot_node_.loop_count = count;
    }

    void actor_instance::push_motion(motion_push_type motion_type, uint32_t motion_key, float blend_time, float speed_ratio) {
        if (!check_motion_thing_index(motion_key)) {
            metin2::log::debug("Motion {} not found", GET_MOTION_INDEX(motion_key));
            return;
        }

        reserving_motion_node motion_node;

        motion_node.motion_type_value = motion_type;
        motion_node.motion_key = motion_key;
        motion_node.start_time = last_motion_time(blend_time);
        motion_node.blend_time = blend_time;
        motion_node.speed_ratio = speed_ratio;
        motion_node.duration = motion_duration(motion_key);

        motion_deque_value_.push_back(motion_node);
    }

    auto actor_instance::intercept_once_motion(uint32_t motion, float blend_time, uint32_t skill, float speed_ratio) -> bool {
        return intercept_motion(MOTION_TYPE_ONCE, motion, blend_time, skill, speed_ratio);
    }

    auto actor_instance::intercept_loop_motion(uint32_t motion, float blend_time) -> bool {
        return intercept_motion(MOTION_TYPE_LOOP, motion, blend_time);
    }

    void actor_instance::set_loop_motion(uint32_t motion, float blend_time, float speed_ratio) {
        if (!cur_race_data_) {
            metin2::log::debug("Set loop motion {} (blend time {:.6f}, speed ratio {:.6f})", motion, blend_time, speed_ratio);
            return;
        }

        MOTION_KEY motion_key;
        if (!cur_race_data_->motion_key(wcur_motion_mode_, motion, &motion_key)) {
            metin2::log::debug("Cannot set loop motion {} (blend time {:.6f}, speed ratio {:.6f}): no motion key for mode {} motion {}", motion, blend_time, speed_ratio, wcur_motion_mode_, motion);
            return;
        }

        clear_motion();

        set_motion_data set_mot_data;
        set_mot_data.mot_key = motion_key;
        set_mot_data.blend_time = blend_time;
        set_mot_data.speed_ratio = speed_ratio;

        uint32_t real_motion_key = set_motion_impl(set_mot_data);

        if (0 == real_motion_key)
            return;

        cur_mot_node_.motion_type_value = MOTION_TYPE_LOOP;
        cur_mot_node_.start_time = local_time();
        cur_mot_node_.motion_key = real_motion_key;
        cur_mot_node_.end_time = 0.0f;
        cur_mot_node_.speed_ratio = speed_ratio;
        cur_mot_node_.dwcur_frame = 0;
        cur_mot_node_.frame_count = motion_duration(real_motion_key) / (1.0f / game_fps);
        cur_mot_node_.skill = 0;
    }

    auto actor_instance::intercept_motion(motion_push_type motion_type, uint16_t motion, float blend_time, uint32_t skill, float speed_ratio) -> bool {
        if (!cur_race_data_) {
            metin2::log::debug("Cannot intercept motion (type {}, motion {}, blend time {:.6f}): no race data", motion_type, motion, blend_time);
            return false;
        }

        MOTION_KEY motion_key;
        if (!cur_race_data_->motion_key(wcur_motion_mode_, motion, &motion_key)) {
            metin2::log::debug("Cannot intercept motion (type {}, mode {}, motion {}, blend time {:.6f}): no motion key for mode {} motion {}", motion_type, wcur_motion_mode_, motion, blend_time, wcur_motion_mode_, motion);
            return false;
        }

        clear_motion();

        int loop_count;
        if (MOTION_TYPE_ONCE == motion_type)
            loop_count = 1;
        else
            loop_count = 0;

        set_motion_data set_mot_data;
        set_mot_data.mot_key = motion_key;
        set_mot_data.blend_time = blend_time;
        set_mot_data.loop_count = loop_count;
        set_mot_data.speed_ratio = speed_ratio;
        set_mot_data.skill = skill;

        uint32_t real_motion_key = set_motion_impl(set_mot_data);

        if (0 == real_motion_key)
            return false;

        if (fly_event_handler_value_) {
            if (is_need_fly_target_motion()) {
                fly_event_handler_value_->on_set_fly_target();
            }
        }

        assert(nullptr != cur_race_motion_data_);

        // set_motion_impl may pick another motion at random, so motion_key and the duration computed from it are not valid;
        // the duration must come from the motion that is actually playing.
        float duration = motion_duration(real_motion_key) / speed_ratio;

        cur_mot_node_.motion_type_value = motion_type;
        cur_mot_node_.start_time = local_time();
        cur_mot_node_.end_time = cur_mot_node_.start_time + duration;
        cur_mot_node_.motion_key = real_motion_key;
        cur_mot_node_.dwcur_frame = 0;
        cur_mot_node_.frame_count = duration / (1.0f / game_fps);
        cur_mot_node_.skill = skill;
        cur_mot_node_.speed_ratio = speed_ratio;

        return true;
    }

    auto actor_instance::push_once_motion(uint32_t motion, float blend_time, float speed_ratio) -> bool {
        assert(cur_race_data_);

        MOTION_KEY motion_key;
        if (!cur_race_data_->motion_key(wcur_motion_mode_, motion, &motion_key))
            return false;

        push_motion(MOTION_TYPE_ONCE, motion_key, blend_time, speed_ratio);
        return true;
    }

    auto actor_instance::push_loop_motion(uint32_t motion, float blend_time, float speed_ratio) -> bool {
        assert(cur_race_data_);

        MOTION_KEY motion_key;
        if (!cur_race_data_->motion_key(wcur_motion_mode_, motion, &motion_key))
            return false;

        push_motion(MOTION_TYPE_LOOP, motion_key, blend_time, speed_ratio);
        return true;
    }

    auto actor_instance::current_motion_index() -> uint16_t {
        return GET_MOTION_INDEX(cur_mot_node_.motion_key);
    }

    auto actor_instance::is_using_skill() -> bool {
        uint32_t cur_motion_index = current_motion_index();

        if (cur_motion_index >= race_motion_data::NAME_SKILL && cur_motion_index < race_motion_data::NAME_SKILL_END)
            return true;

        switch (cur_motion_index) {
        case race_motion_data::NAME_SPECIAL_1:
        case race_motion_data::NAME_SPECIAL_2:
        case race_motion_data::NAME_SPECIAL_3:
        case race_motion_data::NAME_SPECIAL_4:
        case race_motion_data::NAME_SPECIAL_5:
        case race_motion_data::NAME_SPECIAL_6:
            return true;
        }

        return false;
    }

    auto actor_instance::is_fishing() -> bool {
        if (!cur_race_motion_data_)
            return false;

        if (current_motion_index() == race_motion_data::NAME_FISHING_WAIT ||
            current_motion_index() == race_motion_data::NAME_FISHING_REACT)
            return true;

        return false;
    }

    auto actor_instance::can_cancel_skill() -> bool {
        assert(is_using_skill());
        return cur_race_motion_data_->is_cancel_enable_skill();
    }

    auto actor_instance::is_lock() -> bool {
        uint32_t cur_motion_index = current_motion_index();

        switch (cur_motion_index) {
        case race_motion_data::NAME_NORMAL_ATTACK:
        case race_motion_data::NAME_COMBO_ATTACK_1:
        case race_motion_data::NAME_COMBO_ATTACK_2:
        case race_motion_data::NAME_COMBO_ATTACK_3:
        case race_motion_data::NAME_COMBO_ATTACK_4:
        case race_motion_data::NAME_COMBO_ATTACK_5:
        case race_motion_data::NAME_COMBO_ATTACK_6:
        case race_motion_data::NAME_COMBO_ATTACK_7:
        case race_motion_data::NAME_COMBO_ATTACK_8:
        case race_motion_data::NAME_SPECIAL_1:
        case race_motion_data::NAME_SPECIAL_2:
        case race_motion_data::NAME_SPECIAL_3:
        case race_motion_data::NAME_SPECIAL_4:
        case race_motion_data::NAME_SPECIAL_5:
        case race_motion_data::NAME_SPECIAL_6:
        case race_motion_data::NAME_FISHING_THROW:
        case race_motion_data::NAME_FISHING_WAIT:
        case race_motion_data::NAME_FISHING_STOP:
        case race_motion_data::NAME_FISHING_REACT:
        case race_motion_data::NAME_FISHING_CATCH:
        case race_motion_data::NAME_FISHING_FAIL:
        case race_motion_data::NAME_CLAP:
        case race_motion_data::NAME_DANCE_1:
        case race_motion_data::NAME_DANCE_2:
        case race_motion_data::NAME_DANCE_3:
        case race_motion_data::NAME_DANCE_4:
        case race_motion_data::NAME_DANCE_5:
        case race_motion_data::NAME_DANCE_6:
        case race_motion_data::NAME_CONGRATULATION:
        case race_motion_data::NAME_FORGIVE:
        case race_motion_data::NAME_ANGRY:
        case race_motion_data::NAME_ATTRACTIVE:
        case race_motion_data::NAME_SAD:
        case race_motion_data::NAME_SHY:
        case race_motion_data::NAME_CHEERUP:
        case race_motion_data::NAME_BANTER:
        case race_motion_data::NAME_JOY:
        case race_motion_data::NAME_CHEERS_1:
        case race_motion_data::NAME_CHEERS_2:
        case race_motion_data::NAME_KISS_WITH_WARRIOR:
        case race_motion_data::NAME_KISS_WITH_ASSASSIN:
        case race_motion_data::NAME_KISS_WITH_SURA:
        case race_motion_data::NAME_KISS_WITH_SHAMAN:
        case race_motion_data::NAME_FRENCH_KISS_WITH_WARRIOR:
        case race_motion_data::NAME_FRENCH_KISS_WITH_ASSASSIN:
        case race_motion_data::NAME_FRENCH_KISS_WITH_SURA:
        case race_motion_data::NAME_FRENCH_KISS_WITH_SHAMAN:
        case race_motion_data::NAME_SLAP_HIT_WITH_WARRIOR:
        case race_motion_data::NAME_SLAP_HIT_WITH_ASSASSIN:
        case race_motion_data::NAME_SLAP_HIT_WITH_SURA:
        case race_motion_data::NAME_SLAP_HIT_WITH_SHAMAN:
        case race_motion_data::NAME_SLAP_HURT_WITH_WARRIOR:
        case race_motion_data::NAME_SLAP_HURT_WITH_ASSASSIN:
        case race_motion_data::NAME_SLAP_HURT_WITH_SURA:
        case race_motion_data::NAME_SLAP_HURT_WITH_SHAMAN:
            return true;
            break;
        }

        if (is_using_skill()) {
            if (cur_race_motion_data_->is_cancel_enable_skill())
                return false;

            return true;
        }

        return false;
    }

    auto actor_instance::last_motion_time(float blend_time) -> float {
        if (motion_deque_value_.empty()) {
            if (MOTION_TYPE_ONCE == cur_mot_node_.motion_type_value)
                return (cur_mot_node_.end_time - blend_time);

            return local_time();
        }

        reserving_motion_node &motion_node = motion_deque_value_[motion_deque_value_.size() - 1];

        return motion_node.start_time + motion_node.duration - blend_time;
    }

    auto actor_instance::motion_duration(uint32_t motion_key_value) -> float {
        graphic_thing *motion_value;

        if (!motion_thing_pointer(motion_key_value, &motion_value)) {
            metin2::log::debug("Cannot get motion duration: no motion for mode {} index {}", GET_MOTION_MODE(motion_key_value), GET_MOTION_INDEX(motion_key_value));
            return 0.0f;
        }

        if (0 == motion_value->motion_count()) {
            return 0.0f;
        }

        motion *granny_motion = motion_value->motion_pointer(0);
        return granny_motion->get_duration();
    }

    auto actor_instance::random_motion_key(MOTION_KEY motion_key) -> MOTION_KEY {
        // FIXME: the case where no motion was selected at first is not handled.
        uint16_t mode = GET_MOTION_MODE(motion_key);
        uint16_t index = GET_MOTION_INDEX(motion_key);

        const race_data::motion_vector *motion_vector;
        if (cur_race_data_->motion_vector_pointer(mode, index, &motion_vector))
            if (motion_vector->size() > 1) {
                int percentage = metin2::random::number(0, 99);
                for (uint32_t i = 0; i < motion_vector->size(); ++i) {
                    const race_data::motion &motion = motion_vector->at(i);
                    percentage -= motion.percentage;

                    if (percentage < 0) {
                        motion_key = MAKE_RANDOM_MOTION_KEY(mode, index, i);

                        return motion_key;
                    }
                }
            }

        return motion_key;
    }

    void actor_instance::clear_motion() {
        hide_weapon_trace();

        motion_deque_value_.clear();
        cur_mot_node_.dwcur_frame = 0;
        cur_mot_node_.frame_count = 0;
        cur_mot_node_.skill = 0;
        cur_mot_node_.loop_count = 0;
        cur_mot_node_.motion_type_value = MOTION_TYPE_NONE;
    }

    auto actor_instance::set_motion_impl(const set_motion_data &set_mot_data, uint32_t rand_mot_key) -> uint32_t {
        uint32_t mot_key = rand_mot_key;

        if (mot_key == 0)
            mot_key = random_motion_key(set_mot_data.mot_key);

        uint32_t next_mot = GET_MOTION_INDEX(set_mot_data.mot_key);

        if (is_dead()) {
            if (next_mot != race_motion_data::NAME_DAMAGE_FLYING && next_mot != race_motion_data::NAME_DAMAGE_FLYING_BACK && next_mot != race_motion_data::NAME_DEAD && next_mot != race_motion_data::NAME_DEAD_BACK)
                return 0;
        }
        if (is_using_skill()) {
            on_stop_impl();
        }

        if (is_stand_up_motion()) {
            on_stop_impl();
        }

        if (is_move_motion()) {
            if (next_mot == race_motion_data::NAME_DAMAGE || next_mot == race_motion_data::NAME_DAMAGE_BACK || next_mot == race_motion_data::NAME_DAMAGE_FLYING || next_mot == race_motion_data::NAME_DAMAGE_FLYING_BACK) {
                if (!is_main_) {
                    metin2::log::debug("Not the main actor and moving, damage motion skipped");
                    return false;
                }
            }
            // The player runs on through ordinary hits: the flinch stopped the run while the held
            // direction key never restarted it, so a pack hitting in turn froze the player in place.
            if (is_main_ && (next_mot == race_motion_data::NAME_DAMAGE || next_mot == race_motion_data::NAME_DAMAGE_BACK)) {
                return false;
            }

            if (next_mot != race_motion_data::NAME_RUN &&
                next_mot != race_motion_data::NAME_WALK &&
                !is_moving_skill_impl(set_mot_data.skill)) {
                on_stop_impl();
            }
        } else {
            if (next_mot == race_motion_data::NAME_RUN || is_moving_skill_impl(set_mot_data.skill)) {
                on_move_impl();
            }
        }

        // Safety net for actors that disappear while using a skill.
        if (is_hiding()) {
            show_event();
        }

        if (-1 != fishing_effect_id_) {
            effect_manager &eft_mgr = get_effect_manager();
            eft_mgr.deactive_effect_instance(fishing_effect_id_);

            fishing_effect_id_ = -1;
        }

        if (horse_) {
            uint16_t motion_index = GET_MOTION_INDEX(mot_key);
            uint16_t motion_sub_index = GET_MOTION_SUB_INDEX(mot_key);
            uint32_t child_mot_key = MAKE_RANDOM_MOTION_KEY(horse_->wcur_motion_mode_, motion_index, motion_sub_index);

            if (race_motion_data::NAME_DEAD == motion_index)
                graphic_thing_instance::change_motion(mot_key, set_mot_data.loop_count, set_mot_data.speed_ratio);
            else
                graphic_thing_instance::set_motion(mot_key, set_mot_data.blend_time, set_mot_data.loop_count, set_mot_data.speed_ratio);

            horse_->set_motion(child_mot_key, set_mot_data.blend_time, set_mot_data.loop_count, set_mot_data.speed_ratio);
            horse_->bind_motion_data(child_mot_key);

            if (set_mot_data.loop_count)
                horse_->cur_mot_node_.motion_type_value = MOTION_TYPE_ONCE;
            else
                horse_->cur_mot_node_.motion_type_value = MOTION_TYPE_LOOP;

            horse_->cur_mot_node_.frame_count = horse_->motion_duration(child_mot_key) / (1.0f / game_fps);
            horse_->cur_mot_node_.dwcur_frame = 0;
            horse_->cur_mot_node_.motion_key = child_mot_key;
        } else {
            graphic_thing_instance::set_motion(mot_key, set_mot_data.blend_time, set_mot_data.loop_count, set_mot_data.speed_ratio);
        }

        hide_weapon_trace();

        if (bind_motion_data(mot_key)) {
            int loop_count_value = loop_count();
            set_motion_loop_count(loop_count_value);

            if (can_attack_impl()) {
                show_weapon_trace();

                hit_data_map_value_.clear();
            }

            if (is_combo_attacking_impl()) {
                if (!can_next_combo_attack()) {
                    dwcur_combo_index_ = 0;

                    // clear_combo() must not be called here: when a skill follows a combo it would also reset
                    // cur_race_motion_data_.
                }
            }
        }

        return mot_key;
    }

    auto actor_instance::bind_motion_data(uint32_t motion_key) -> bool {
        if (!cur_race_data_->motion_data_pointer(motion_key, &cur_race_motion_data_)) {
            metin2::log::debug("Cannot bind motion");
            cur_race_motion_data_ = nullptr;
            dwcur_combo_index_ = 0;
            return false;
        }

        return true;
    }

    auto actor_instance::loop_count() -> int {
        if (!cur_race_motion_data_) {
            metin2::log::error("Cannot get loop count: no current motion data");
            return -1;
        }

        return cur_race_motion_data_->get_loop_count();
    }

    auto actor_instance::can_attack_impl() -> bool {
        if (!cur_race_motion_data_) {
            metin2::log::error("Cannot check attack: no current motion data");
            return false;
        }

        if (!cur_race_motion_data_->is_attacking_motion())
            return false;

        return true;
    }

    auto actor_instance::can_next_combo_attack() -> bool {
        if (!cur_race_motion_data_) {
            metin2::log::error("Cannot check next combo attack: no current motion data");
            return false;
        }

        if (!cur_race_motion_data_->is_combo_input_time_data())
            return false;

        return true;
    }

    auto actor_instance::is_combo_attacking_impl() -> bool {
        if (0 == dwcur_combo_index_)
            return false;

        return true;
    }

    auto actor_instance::is_need_fly_target_motion() -> bool {
        if (!cur_race_motion_data_)
            return true;

        for (uint32_t i = 0; i < cur_race_motion_data_->motion_event_data_count(); ++i) {
            const race_motion_data::motion_event_data *data;
            if (!cur_race_motion_data_->motion_event_data_pointer(i, &data))
                continue;

            if (data->type_value == race_motion_data::MOTION_EVENT_TYPE_WARP)
                return true;

            if (data->type_value == race_motion_data::MOTION_EVENT_TYPE_FLY)
                return true;

            if (data->type_value == race_motion_data::MOTION_EVENT_TYPE_EFFECT_TO_TARGET)
                return true;
        }

        return false;
    }

    auto actor_instance::is_wait_motion() -> bool {
        return (motion_type() == race_motion_data::TYPE_WAIT);
    }

    auto actor_instance::is_move_motion() -> bool {
        return (motion_type() == race_motion_data::TYPE_MOVE);
    }

    auto actor_instance::is_damage_motion() -> bool {
        return (motion_type() == race_motion_data::TYPE_DAMAGE);
    }

    auto actor_instance::is_knock_down_motion() -> bool {
        return (motion_type() == race_motion_data::TYPE_KNOCKDOWN);
    }

    auto actor_instance::is_die_motion() -> bool {
        if (is_knock_down_motion())
            return true;

        return (motion_type() == race_motion_data::TYPE_DIE);
    }

    auto actor_instance::is_stand_up_motion() -> bool {
        return (motion_type() == race_motion_data::TYPE_STANDUP);
    }
}
