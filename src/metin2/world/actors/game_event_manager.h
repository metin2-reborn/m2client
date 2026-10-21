#pragma once

#include "actor_instance.h"

namespace metin2::inline world::inline actors {

    // Manages in-game events such as explosions, screen shakes and the flight and hits of ranged
    // weapons, and controls in-game cinematic events.

    class game_event_manager : public screen {
      public:
        game_event_manager();
        ~game_event_manager() override;

        void set_center_position(float fx, float fy, float fz);
        void update();

        void process_event_screen_waving(actor_instance *actor_instance, const race_motion_data::screen_waving_event_data *data);

      protected:
        pixel_position center_position_;
    };

    inline auto get_game_event_manager() -> game_event_manager & {
        static game_event_manager instance;
        return instance;
    }
}
