#include "actor_instance.h"
#include "race_data.h"

namespace metin2::inline world::inline actors {

    void actor_instance::push_impl(int x, int y) {
        if (is_resist_fallen())
            return;

        const math::vector3 &src = get_position();
        const math::vector3 dst = math::vector3(x, -y, src.z);
        const math::vector3 delta = dst - src;

        const int loop_value = 100;
        const math::vector3 inc = delta / loop_value;

        math::vector3 movement(0.0f, 0.0f, 0.0f);

        physics_world *world = physics_world::get_physics_world();

        if (!world) {
            return;
        }

        for (int i = 0; i < loop_value; ++i) {
            if (world->is_physical_collision(src + movement)) {
                reset_blending_position();
                return;
            }
            movement += inc;
        }

        set_blending_position(dst);

        if (!is_using_skill()) {
            int len = sqrt(delta.x * delta.x + delta.y * delta.y);
            if (len > 150.0f) {
                intercept_once_motion(race_motion_data::NAME_DAMAGE_FLYING);
                push_once_motion(race_motion_data::NAME_STAND_UP);
            }
        }
    }

    void actor_instance::temp_push(int x, int y) {
        push_impl(x, y);
    }

    auto actor_instance::is_syncing() -> bool {
        if (is_dead())
            return true;

        if (is_stun())
            return true;

        if (is_pushing())
            return true;

        return false;
    }

    auto actor_instance::is_pushing() -> bool {
        return physics_object_value_.is_blending();
    }
}
