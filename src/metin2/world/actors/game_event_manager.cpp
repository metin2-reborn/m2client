#include "game_event_manager.h"

namespace metin2::inline world::inline actors {

    void game_event_manager::set_center_position(float fx, float fy, float fz) {
        center_position_.x = +fx;
        center_position_.y = -fy;
        center_position_.z = +fz;
    }

    void game_event_manager::update() {
    }

    void game_event_manager::process_event_screen_waving(actor_instance *actor_instance, const race_motion_data::screen_waving_event_data *data) {
        pixel_position pixel_position;
        actor_instance->get_pixel_position(&pixel_position);

        float fdx = pixel_position.x - center_position_.x;
        float fdy = pixel_position.y - center_position_.y;
        float fdz = pixel_position.z - center_position_.z;

        if (fdx * fdx + fdy * fdy + fdz * fdz > data->affecting_range * data->affecting_range)
            return;

        set_screen_effect_waving(data->duration_time, data->power);
    }

    game_event_manager::game_event_manager() : center_position_(0.0f, 0.0f, 0.0f) {
    }

    game_event_manager::~game_event_manager() = default;
}
