#include "python_player_event_handler.h"
#include <metin2/client/characters/python_character_manager.h>
#include <metin2/client/protocol/python_network_stream.h>
#include <metin2/core/timer.h>
#include <metin2/log/log.h>
#include "python_player.h"

namespace metin2::inline world::inline actors {
    extern auto is_huge_race(unsigned int vnum) -> bool;
}
namespace metin2::inline client::inline player {

    auto get_python_player_event_handler() -> python_player_event_handler & {
        static python_player_event_handler player_event_handler;
        return player_event_handler;
    }

    python_player_event_handler::~python_player_event_handler() = default;

    void python_player_event_handler::on_clear_affects() {
        get_python_player().clear_affects();
    }

    void python_player_event_handler::on_set_affect(uint32_t affect) {
        get_python_player().set_affect(affect);
    }

    void python_player_event_handler::on_reset_affect(uint32_t affect) {
        get_python_player().reset_affect(affect);
    }

    void python_player_event_handler::on_syncing(const state &state) {
        const pixel_position &p_pos_cur_syncing = state.p_pos_self;
        p_pos_prev_waiting_ = p_pos_cur_syncing;
    }

    void python_player_event_handler::on_waiting(const state &state) {
        uint32_t cur_time = el_timer_get_m_sec();
        if (next_waiting_notify_time_ > cur_time)
            return;

        next_waiting_notify_time_ = cur_time + 100;

        const pixel_position &p_pos_cur_waiting = state.p_pos_self;
        float dx = p_pos_prev_waiting_.x - p_pos_cur_waiting.x;
        float dy = p_pos_prev_waiting_.y - p_pos_cur_waiting.y;
        float len = sqrt(dx * dx + dy * dy);

        if (len < 1.0f)
            return;

        p_pos_prev_waiting_ = p_pos_cur_waiting;

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_character_state_packet(state.p_pos_self, state.adv_rot_self, instance_base::FUNC_WAIT, 0);
    }

    void python_player_event_handler::on_moving(const state &state) {
        uint32_t cur_time = el_timer_get_m_sec();
        if (next_moving_notify_time_ > cur_time)
            return;

        next_moving_notify_time_ = cur_time + 300;

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_character_state_packet(state.p_pos_self, state.adv_rot_self, instance_base::FUNC_MOVE, 0);
    }

    void python_player_event_handler::on_move(const state &state) {
        uint32_t cur_time = el_timer_get_m_sec();
        next_waiting_notify_time_ = cur_time + 100;
        next_moving_notify_time_ = cur_time + 300;

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_character_state_packet(state.p_pos_self, state.adv_rot_self, instance_base::FUNC_MOVE, 0);
    }

    void python_player_event_handler::on_stop(const state &state) {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_character_state_packet(state.p_pos_self, state.adv_rot_self, instance_base::FUNC_WAIT, 0);
    }

    void python_player_event_handler::on_warp(const state &state) {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_character_state_packet(state.p_pos_self, state.adv_rot_self, instance_base::FUNC_WAIT, 0);
    }

    void python_player_event_handler::on_attack(const state &state, uint16_t motion_index) {
        assert(motion_index < 255);

        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_character_state_packet(state.p_pos_self, state.adv_rot_self, instance_base::FUNC_COMBO, motion_index);
    }

    void python_player_event_handler::on_use_skill(const state &state, uint32_t mot_skill, uint32_t arg) {
        python_network_stream &net_stream = get_python_network_stream();
        net_stream.send_character_state_packet(state.p_pos_self, state.adv_rot_self, instance_base::FUNC_SKILL | mot_skill, arg);
    }

    void python_player_event_handler::on_update() {
    }

    void python_player_event_handler::on_change_shape() {
        get_python_player().new_stop();
    }

    void python_player_event_handler::on_hit(uint32_t skill, actor_instance &actor_victim, bool is_send_packet) {
        uint32_t vid_victim = actor_victim.virtual_id();

        get_python_player().set_target(vid_victim, false);

        if (is_send_packet) {
            python_network_stream &stream = get_python_network_stream();
            stream.send_attack_packet(skill, vid_victim);
        }

        if (!actor_victim.is_pushing())
            return;

        if (is_huge_race(actor_victim.race()))
            return;

        get_python_character_manager().adjust_collision_with_other_objects(&actor_victim);

        const pixel_position &p_pos_last = actor_victim.new_get_last_pixel_position_ref();

        victim victim_value;
        victim_value.vid = vid_victim;
        victim_value.pixel_x = int32_t(p_pos_last.x);
        victim_value.pixel_y = int32_t(p_pos_last.y);

        actor_victim.temp_push(victim_value.pixel_x, victim_value.pixel_y);

        vctk_victim_.push_back(victim_value);
    }

    void python_player_event_handler::flush_victim_list() {
        if (vctk_victim_.empty())
            return;

        // Fixes a crash when using Daejingak (#0000682).
        unsigned int victim_count = vctk_victim_.size();

        python_network_stream &stream = get_python_network_stream();

        packet_cg_sync_position packet_sync_pos;
        packet_sync_pos.header_value = HEADER_CG_SYNC_POSITION;
        packet_sync_pos.size = sizeof(packet_sync_pos) + sizeof(packet_cg_sync_position_element) * victim_count;

        stream.send(sizeof(packet_sync_pos), &packet_sync_pos);

        for (unsigned int i = 0; i < victim_count; ++i) {
            const victim &victim_value = vctk_victim_[i];
            stream.send_sync_position_element_packet(victim_value.vid, victim_value.pixel_x, victim_value.pixel_y);
        }

        stream.send_sequence();
        vctk_victim_.clear();
    }

    python_player_event_handler::python_player_event_handler() {
        prev_combo_index_ = 0;
        next_moving_notify_time_ = 0;
        next_waiting_notify_time_ = 0;
    }

    void python_player_event_handler::normal_bow_attack_fly_event_handler_auto_clear::on_set_fly_target() {
        state s;
        inst_main_->new_get_pixel_position(&s.p_pos_self);
        s.adv_rot_self = inst_main_->graphic_thing_instance_ptr()->target_rotation();

        python_network_stream &rpns = get_python_network_stream();
        rpns.send_fly_targeting_packet(inst_target_->virtual_id(), inst_target_->graphic_thing_instance_ptr()->on_get_fly_target_position());
    }
    void python_player_event_handler::normal_bow_attack_fly_event_handler_auto_clear::on_shoot(uint32_t skill_index) {
        python_network_stream &rpns = get_python_network_stream();
        rpns.send_shoot_packet(skill_index);
    }

    void python_player_event_handler::normal_bow_attack_fly_event_handler_auto_clear::set(python_player_event_handler *parent, instance_base *inst_main, instance_base *inst_target) {
        parent_ = (parent);
        inst_main_ = (inst_main);
        inst_target_ = (inst_target);
    }

    void python_player_event_handler::normal_bow_attack_fly_event_handler_auto_clear::set_target(instance_base *inst_target) {
        inst_target_ = inst_target;
    }

    void python_player_event_handler::normal_bow_attack_fly_event_handler_auto_clear::on_exploding_at_another_target(uint32_t skill_index, uint32_t vid) {
        return;

        metin2::log::debug("Shot hit another target (skill {}, vid {})", skill_index, vid);

        python_network_stream &stream = get_python_network_stream();
        stream.send_attack_packet(skill_index, vid);

        python_character_manager &chr_mgr = get_python_character_manager();
        instance_base *instance = chr_mgr.instance_ptr(vid);
        if (instance) {
            instance->graphic_thing_instance_ref().on_shoot_damage();
        }
    }
    void python_player_event_handler::normal_bow_attack_fly_event_handler_auto_clear::on_exploding_at_target(uint32_t skill_index) {
    }
}
