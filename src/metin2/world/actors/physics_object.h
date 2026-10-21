#pragma once

#include <metin2/world/maps/map_type.h>
#include <metin2/world/maps/map_util.h>

namespace metin2::inline world::inline actors {

    class physics_world {
      public:
        physics_world() {
            assert(world_ == nullptr);
            world_ = this;
        }

        virtual ~physics_world() {
            if (this == world_)
                world_ = nullptr;
        }

        static auto get_physics_world() -> physics_world * {
            return world_;
        }

        virtual auto is_physical_collision(const math::vector3 &check_position) -> bool = 0;

      private:
        static physics_world *world_;
    };

    class actor_instance;
    // Not nice, but this avoids a dependency inversion and keeps the old interface. A proper design
    // would manage the world and the live objects in a lower-level module.
    class object_manager {
      public:
        object_manager() {
            assert(obj_manager_ == nullptr);
            obj_manager_ = this;
        }

        virtual ~object_manager() {
            if (this == obj_manager_)
                obj_manager_ = nullptr;
        }

        static auto get_object_manager() -> object_manager * {
            return obj_manager_;
        }

        virtual void adjust_collision_with_other_objects(actor_instance *inst) = 0;

      private:
        static object_manager *obj_manager_;
    };

    class physics_object {
      public:
        physics_object();
        virtual ~physics_object();

        void initialize();

        void update(float elapsed_time);

        auto is_blending() -> bool;

        void set_direction(const math::vector3 &direction);
        void increase_external_force(const math::vector3 &base_position, float force);
        void set_last_position(const pixel_position &position, float blending_time);
        void last_position(pixel_position *position);

        auto x_movement() -> float;
        auto y_movement() -> float;

        void set_actor_instance(actor_instance *inst) { actor_instance_value_ = inst; }

      protected:
        void accumulate(math::vector3 *position);

      protected:
        float mass_;
        float friction_;
        math::vector3 direction_value_;
        math::vector3 acceleration_;
        math::vector3 velocity_;

        math::vector3 last_position_;
        ease_out_interpolation x_pushing_position_;
        ease_out_interpolation y_pushing_position_;

        actor_instance *actor_instance_value_;
    };
}
