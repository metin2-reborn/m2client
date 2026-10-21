#pragma once
#include <cstdint>

namespace metin2::inline world::inline flying {

    class fly_event_handler {
      public:
        fly_event_handler() = default;
        virtual ~fly_event_handler() = default;

        // Called by actor_instance.
        virtual void on_set_fly_target() {}
        virtual void on_shoot(uint32_t skill_index) {}

        // Called by flying_instance.
        virtual void on_exploding_out_of_range() {}
        virtual void on_exploding_at_background() {}
        virtual void on_exploding_at_another_target(uint32_t skill_index, uint32_t vid) {}
        virtual void on_exploding_at_target(uint32_t skill_index) {}
    };
}
