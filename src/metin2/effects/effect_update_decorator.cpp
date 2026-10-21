#include "effect_update_decorator.h"
#include "particle_instance.h"

namespace metin2::inline effects {
    namespace update_decorators {

        auto air_resistance_decorator::clone_impl(particle_instance *pfi, particle_instance *pi) -> base_decorator * {
            pi->air_resistance_ = pfi->air_resistance_;
            return new air_resistance_decorator;
        }

        void air_resistance_decorator::excute_impl(const decorator_data &d) {
            d.instance->velocity_ *= 1.0f - d.instance->air_resistance_;
        }
        auto gravity_decorator::clone_impl(particle_instance *pfi, particle_instance *pi) -> base_decorator * {
            pi->gravity_ = pfi->gravity_;
            return new gravity_decorator;
        }

        void gravity_decorator::excute_impl(const decorator_data &d) {
            d.instance->velocity_.z -= d.instance->gravity_ * d.elapsed_time;
        }

        auto rotation_decorator::clone_impl(particle_instance *pfi, particle_instance *pi) -> base_decorator * {
            pi->rotation_speed_ = pfi->rotation_speed_;
            return new rotation_decorator;
        }

        void rotation_decorator::excute_impl(const decorator_data &d) {
            d.instance->rotation_ += d.instance->rotation_speed_ * d.elapsed_time;
        }

    }
}
