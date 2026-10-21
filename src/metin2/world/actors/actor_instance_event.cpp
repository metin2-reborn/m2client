#include "actor_instance.h"

namespace metin2::inline world::inline actors {

    void actor_instance::on_syncing_impl() {
        event_handler &event_handler_value = event_handler_ref_impl();

        event_handler::state state;
        state.p_pos_self = new_get_cur_pixel_position_ref();
        state.adv_rot_self = advancing_rotation();
        event_handler_value.on_syncing(state);
    }

    void actor_instance::on_waiting_impl() {
        assert(!is_pushing());

        event_handler &event_handler_value = event_handler_ref_impl();

        event_handler::state state;
        state.p_pos_self = new_get_cur_pixel_position_ref();
        state.adv_rot_self = advancing_rotation();
        event_handler_value.on_waiting(state);
    }

    void actor_instance::on_moving_impl() {
        assert(!is_pushing());

        event_handler &event_handler_value = event_handler_ref_impl();

        const pixel_position &p_pos_cur = new_get_cur_pixel_position_ref();
        const pixel_position &p_pos_dst = new_get_dst_pixel_position_ref();

        pixel_position p_pos_dir = p_pos_dst - p_pos_cur;
        float distance = sqrt(p_pos_dir.x * p_pos_dir.x + p_pos_dir.y * p_pos_dir.y);

        event_handler::state state;

        if (distance > 1000.0f) {
            p_pos_dir = math::normalize(p_pos_dir);
            p_pos_dir = p_pos_dir * 1000.0f;
            state.p_pos_self = p_pos_dir + p_pos_cur;
        } else {
            state.p_pos_self = p_pos_dst;
        }
        state.adv_rot_self = advancing_rotation();
        event_handler_value.on_moving(state);
    }

    void actor_instance::on_move_impl() {
        event_handler &event_handler_value = event_handler_ref_impl();

        event_handler::state state;
        state.p_pos_self = new_get_cur_pixel_position_ref();
        state.adv_rot_self = advancing_rotation();
        event_handler_value.on_move(state);
    }

    void actor_instance::on_stop_impl() {
        event_handler &event_handler_value = event_handler_ref_impl();

        event_handler::state state;
        state.p_pos_self = new_get_cur_pixel_position_ref();
        state.adv_rot_self = advancing_rotation();
        event_handler_value.on_stop(state);
    }

    void actor_instance::on_warp_impl() {
        event_handler &event_handler_value = event_handler_ref_impl();

        event_handler::state state;
        state.p_pos_self = new_get_cur_pixel_position_ref();
        state.adv_rot_self = advancing_rotation();
        event_handler_value.on_warp(state);
    }

    void actor_instance::on_attack_impl(uint16_t motion_index) {
        event_handler &event_handler_value = event_handler_ref_impl();

        event_handler::state state;
        state.p_pos_self = new_get_cur_pixel_position_ref();
        state.adv_rot_self = target_rotation();
        event_handler_value.on_attack(state, motion_index);
    }

    void actor_instance::on_use_skill_impl(uint32_t mot_skill, uint32_t loop_count, bool is_moving_skill) {
        event_handler &event_handler_value = event_handler_ref_impl();

        event_handler::state state;
        state.p_pos_self = new_get_cur_pixel_position_ref();
        state.adv_rot_self = advancing_rotation();

        uint32_t arg = loop_count;
        if (is_moving_skill)
            arg |= 1 << 4;

        event_handler_value.on_use_skill(state, mot_skill, arg);
    }

    void actor_instance::on_hit_impl(uint32_t skill, actor_instance &actor_victm, bool is_send_packet) {
        event_handler &event_handler_value = event_handler_ref_impl();
        event_handler_value.on_hit(skill, actor_victm, is_send_packet);
    }

    void actor_instance::on_clear_affects_impl() {
        event_handler &event_handler_value = event_handler_ref_impl();
        event_handler_value.on_clear_affects();
    }

    void actor_instance::on_set_affect_impl(uint32_t affect) {
        event_handler &event_handler_value = event_handler_ref_impl();
        event_handler_value.on_set_affect(affect);
    }

    void actor_instance::on_reset_affect_impl(uint32_t affect) {
        event_handler &event_handler_value = event_handler_ref_impl();
        event_handler_value.on_reset_affect(affect);
    }

    auto actor_instance::event_handler_ref_impl() -> actor_instance::event_handler & {
        assert(event_handler_value_ != nullptr && "CActorInstance::GetEventHandlerRef");
        return *event_handler_value_;
    }

    void actor_instance::set_event_handler(event_handler *event_handler_value) {
        event_handler_value_ = event_handler_value;
    }

    auto actor_instance::event_handler::empty_ptr() -> actor_instance::event_handler * {
        static class empty_event_handler : public event_handler {
          public:
            empty_event_handler() = default;
            ~empty_event_handler() override = default;

            void on_syncing(const state &state) override {}
            void on_waiting(const state &state) override {}
            void on_moving(const state &state) override {}
            void on_move(const state &state) override {}
            void on_stop(const state &state) override {}
            void on_warp(const state &state) override {}

            void on_clear_affects() override {}
            void on_set_affect(uint32_t affect) override {}
            void on_reset_affect(uint32_t affect) override {}

            void on_attack(const state &state, uint16_t motion_index) override {}
            void on_use_skill(const state &state, uint32_t mot_skill, uint32_t mot_loop_count) override {}

            void on_hit(uint32_t mot_attack, actor_instance &actor_victim, bool is_send_packet) override {}

            void on_change_shape() override {}

        } empty_event_handler;

        return &empty_event_handler;
    }
}
