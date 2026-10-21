#include "actor_instance.h"
#include "weapon_trace.h"

namespace metin2::inline world::inline actors {

    void actor_instance::trace_process() {
        if (!weapon_trace_vector_.empty()) {
            std::vector<weapon_trace *>::iterator it;
            for (it = weapon_trace_vector_.begin(); it != weapon_trace_vector_.end(); ++it) {
                weapon_trace *weapon_trace = (*it);
                weapon_trace->set_position(x_, y_, z_);
                weapon_trace->set_rotation(fcur_rotation_);
                weapon_trace->update(reach_scale());
            }
        }
    }

    void actor_instance::render_trace() {
        for_each(weapon_trace_vector_.begin(), weapon_trace_vector_.end(), std::mem_fn(&weapon_trace::render));
    }

    void actor_instance::destroy_weapon_trace() {
        std::for_each(weapon_trace_vector_.begin(), weapon_trace_vector_.end(), weapon_trace::delete_value);
        weapon_trace_vector_.clear();
    }

    void actor_instance::show_weapon_trace() {
        for_each(weapon_trace_vector_.begin(), weapon_trace_vector_.end(), std::mem_fn(&weapon_trace::turn_on));
    }

    void actor_instance::hide_weapon_trace() {
        for_each(weapon_trace_vector_.begin(), weapon_trace_vector_.end(), std::mem_fn(&weapon_trace::turn_off));
    }
}
