#pragma once

#include <metin2/world/actors/actor_instance_interface.h>
#include <set>
#include "fly_target.h"

namespace metin2::inline world::inline actors {
    class actor_instance;
}

namespace metin2::inline world::inline flying {

    class flying_data;
    class fly_trace;
    class fly_event_handler;

    class flying_instance {
      public:
        void clear();
        void set_data_pointer(flying_data *data, const math::vector3 &start_position);
        void set_fly_target(const fly_target &cr_target);

      public:
        friend class CSceneFly;

        struct attach_effect_instance {
            int attach_index;
            uint32_t effect_instance_index;

            fly_trace *fly_trace_value;
        };

        flying_instance();
        virtual ~flying_instance();

        void destroy();
        void create(flying_data *data, const math::vector3 &start_pos, const fly_target &target, bool can_attack);

        auto update() -> bool;
        void render();

        auto get_position() -> const math::vector3 & { return position_value_; }

        void adjust_direction_for_homing(const math::vector3 &target_position);

        using attach_effect_instance_vector = std::vector<attach_effect_instance>;

        void build_attach_instance();
        void update_attach_instance();
        void render_attach_instance();
        void clear_attach_instance();

        void set_event_handler(fly_event_handler *handler) { handler_ = handler; }

        void set_owner(actor_instance_interface *owner) { owner_ = owner; }
        void set_skill_index(uint32_t index) { skill_index_ = index; }

        void explode(bool bomb = true);
        void bomb();

        uint32_t id;

      protected:
        void initialize_impl();

        void set_data_pointer_impl(flying_data *data, const math::vector3 &start_position);
        void set_target_direction(const fly_target &target);
        void set_target_normalized_direction(const math::vector3 &normalized_direction);

      protected:
        flying_data *data_value_;
        math::quaternion rot_;

        float start_time_;

        bool alive_;
        bool can_attack_;

        int pierce_count_;
        uint32_t skill_index_;

        math::vector3 position_value_;
        math::vector3 velocity_;
        math::vector3 local_velocity_;
        math::vector3 accel_;

        float remain_range_;

        fly_target fly_target_value_;

        math::quaternion attach_rotation_;
        attach_effect_instance_vector attach_effect_instance_value_;

        fly_event_handler *handler_;

        actor_instance_interface *owner_;

        bool target_hitted_;
        std::set<actor_instance_interface *> hitted_object_set_;

      public:
        static auto new_value() -> flying_instance * { return pool_value.alloc(); }
        static void delete_value(flying_instance *instance) {
            instance->destroy();
            pool_value.free(instance);
        }

        static void destroy_system() { pool_value.destroy(); }

        static dynamic_pool<flying_instance> pool_value;
    };
}
