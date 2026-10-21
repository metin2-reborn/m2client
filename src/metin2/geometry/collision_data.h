#pragma once
#include <cstdint>
#include <metin2/gpu/types.h>
#include <metin2/math/math.h>
#include <vector>

namespace metin2::inline geometry {

    struct sphere_data {
        math::vector3 position_value;
        float radius;
    };

    struct plane_data {
        math::vector3 position_value;
        math::vector3 normal_value;

        math::vector3 quad_position[4];
        math::vector3 inside_vector[4];
    };

    struct aabb_data {
        math::vector3 min;
        math::vector3 max;
    };

    struct obb_data {
        math::vector3 min;
        math::vector3 max;
        math::matrix rot;
    };

    struct cylinder_data {
        math::vector3 position_value;
        float radius;
        float height;
    };

    enum collision_type {
        COLLISION_TYPE_PLANE,
        COLLISION_TYPE_BOX,
        COLLISION_TYPE_SPHERE,
        COLLISION_TYPE_CYLINDER,
        COLLISION_TYPE_AABB,
        COLLISION_TYPE_OBB,
    };

    struct dynamic_sphere_instance {
        math::vector3 position_value;
        math::vector3 last_position;

        float radius;
    };

    class static_collision_data {
      public:
        uint32_t type_value;
        char name_value[32 + 1];

        math::vector3 position_value;
        float dimensions[3];
        math::quaternion quat_rotation;
    };

    void destroy_collision_instance_system();

    using static_collision_data_vector = std::vector<static_collision_data>;

    class base_collision_instance {
      public:
        virtual ~base_collision_instance() = default;
        virtual void render(uint32_t fill_mode = gpu::fill::SOLID) = 0;

        auto movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {
            return on_movement_collision_dynamic_sphere(s);
        }
        auto collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool {
            return on_collision_dynamic_sphere(s);
        }

        auto collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 {
            return on_get_collision_movement_adjust(s);
        }

        void destroy();

        static auto build_collision_instance(const static_collision_data *collision_data, const math::matrix *mat) -> base_collision_instance *;

      protected:
        virtual auto on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 = 0;
        virtual auto on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool = 0;
        virtual auto on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool = 0;
        virtual void on_destroy() = 0;
    };

    class sphere_collision_instance : public base_collision_instance {
      public:
        auto attribute() -> sphere_data &;
        auto attribute() const -> const sphere_data &;
        void render(uint32_t fill_mode = gpu::fill::SOLID) override;

      protected:
        void on_destroy() override;
        auto on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool override;
        auto on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool override;
        auto on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 override;

      protected:
        sphere_data attribute_;
    };

    class plane_collision_instance : public base_collision_instance {
      public:
        auto attribute() -> plane_data &;
        auto attribute() const -> const plane_data &;
        void render(uint32_t fill_mode = gpu::fill::SOLID) override;

      protected:
        void on_destroy() override;
        auto on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool override;
        auto on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool override;
        auto on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 override;

      protected:
        plane_data attribute_;
    };

    class aabb_collision_instance : public base_collision_instance {
      public:
        auto attribute() -> aabb_data &;
        auto attribute() const -> const aabb_data &;
        void render(uint32_t fill_mode = gpu::fill::SOLID) override;

      protected:
        void on_destroy() override;
        auto on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool override;
        auto on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool override;
        auto on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 override;

      protected:
        aabb_data attribute_;
    };

    class obb_collision_instance : public base_collision_instance {
      public:
        auto attribute() -> obb_data &;
        auto attribute() const -> const obb_data &;
        void render(uint32_t fill_mode = gpu::fill::SOLID) override;

      protected:
        void on_destroy() override;
        auto on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool override;
        auto on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool override;
        auto on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 override;

      protected:
        obb_data attribute_;
    };

    class cylinder_collision_instance : public base_collision_instance {
      public:
        auto attribute() -> cylinder_data &;
        auto attribute() const -> const cylinder_data &;
        void render(uint32_t fill_mode = gpu::fill::SOLID) override;

      protected:
        void on_destroy() override;
        auto on_movement_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool override;
        auto on_collision_dynamic_sphere(const dynamic_sphere_instance &s) const -> bool override;
        auto on_get_collision_movement_adjust(const dynamic_sphere_instance &s) const -> math::vector3 override;

        auto collide_cylinder_vs_dynamic_sphere(const cylinder_data &c_rattribute, const dynamic_sphere_instance &s) const -> bool;

      protected:
        cylinder_data attribute_;
    };

    using sphere_collision_instance_vector = std::vector<sphere_collision_instance>;
    using dynamic_sphere_instance_vector = std::vector<dynamic_sphere_instance>;
    using collision_instance_vector = std::vector<base_collision_instance *>;
}
