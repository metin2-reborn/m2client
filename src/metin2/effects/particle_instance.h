#pragma once

#include <metin2/core/pool.h>
#include <metin2/graphics/grp_base.h>
#include "effect_update_decorator.h"

namespace metin2::inline effects {
    class particle_property;
    class emitter_property;

    class particle_instance {
        friend class particle_system_data;
        friend class particle_system_instance;

        friend class update_decorators::base_decorator;
        friend class update_decorators::air_resistance_decorator;
        friend class update_decorators::gravity_decorator;
        friend class update_decorators::rotation_decorator;

      public:
        particle_instance();
        ~particle_instance();

        auto radius_approximation() -> float;

        auto update(float elapsed_time, float angle) -> bool;

      protected:
        math::vector3 start_position_;

        math::vector3 position_value_;
        math::vector3 last_position_;
        math::vector3 velocity_;

        math::vector2 half_size_;
        math::vector2 scale_;

        float rotation_;
        dwordcolor dc_color_;

        uint8_t texture_animation_type_;
        uint8_t frame_index_;

        float life_time_;
        float last_life_time_;

        particle_property *particle_property_value_;
        emitter_property *emitter_property_value_;

        float air_resistance_;
        float rotation_speed_;
        float gravity_;

        update_decorators::base_decorator *decorator_;

      public:
        static auto new_value() -> particle_instance *;
        static void destroy_system();

        void transform(const math::matrix *local = nullptr);
        void transform(const math::matrix *local, const float z_rotation);

        auto particle_mesh_pointer() -> pt_vertex *;

        void delete_this();

        void destroy();

      protected:
        void initialize_impl();
        pt_vertex particle_mesh_[4];

      public:
        static dynamic_pool<particle_instance> pool_value;
    };
}
