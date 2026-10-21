#pragma once

#include <metin2/world/actors/actor_instance.h>
#include <metin2/world/flying/fly_handler.h>

#include <metin2/client/characters/instance_base.h>
#include <metin2/client/protocol/python_network_stream.h>

namespace metin2::inline client::inline player {

    class python_player_event_handler : public actor_instance::event_handler {
      public:
        ~python_player_event_handler() override;

        void on_syncing(const state &state) override;
        void on_waiting(const state &state) override;
        void on_moving(const state &state) override;
        void on_move(const state &state) override;
        void on_stop(const state &state) override;
        void on_warp(const state &state) override;
        void on_clear_affects() override;
        void on_set_affect(uint32_t affect) override;
        void on_reset_affect(uint32_t affect) override;
        void on_attack(const state &state, uint16_t motion_index) override;
        void on_use_skill(const state &state, uint32_t mot_skill, uint32_t arg) override;
        virtual void on_update();
        void on_change_shape() override;
        void on_hit(uint32_t skill, actor_instance &actor_victim, bool is_send_packet) override;

        void flush_victim_list();

      public:
        python_player_event_handler();

      protected:
      protected:
        struct victim {
            uint32_t vid;
            int32_t pixel_x;
            int32_t pixel_y;
        };

      protected:
        std::vector<victim> vctk_victim_;

        uint32_t prev_combo_index_;
        uint32_t next_waiting_notify_time_;
        uint32_t next_moving_notify_time_;
        pixel_position p_pos_prev_waiting_;

      private:
        class normal_bow_attack_fly_event_handler_auto_clear : public fly_event_handler {
          public:
            normal_bow_attack_fly_event_handler_auto_clear() = default;
            ~normal_bow_attack_fly_event_handler_auto_clear() override = default;

            void set(python_player_event_handler *parent, instance_base *inst_main, instance_base *inst_target);
            void set_target(instance_base *inst_target);

            void on_set_fly_target() override;
            void on_shoot(uint32_t skill_index) override;

            void on_exploding_out_of_range() override {}
            void on_exploding_at_background() override {}
            void on_exploding_at_another_target(uint32_t skill_index, uint32_t vid) override;
            void on_exploding_at_target(uint32_t skill_index) override;

          protected:
            python_player_event_handler *parent_;
            instance_base *inst_main_;
            instance_base *inst_target_;
        } normal_bow_attack_fly_event_handler_auto_clear_value_;

      public:
        auto normal_bow_attack_fly_event_handler(instance_base *inst_main, instance_base *inst_target) -> fly_event_handler * {
            normal_bow_attack_fly_event_handler_auto_clear_value_.set(this, inst_main, inst_target);
            return &normal_bow_attack_fly_event_handler_auto_clear_value_;
        }
    };

    auto get_python_player_event_handler() -> python_player_event_handler &;
}
