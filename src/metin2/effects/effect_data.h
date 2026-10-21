#pragma once

#include <metin2/sound/type.h>

#include "effect_mesh.h"
#include "particle_system_data.h"
#include "simple_light_data.h"

namespace metin2::inline effects {

    class effect_data {
      public:
        using particle_vector = std::vector<particle_system_data *>;
        using mesh_vector = std::vector<effect_mesh_script *>;
        using light_vector = std::vector<light_data *>;

      public:
        effect_data();
        virtual ~effect_data();

        void clear();
        auto load_script(const char *file_name) -> bool;
        auto load_sound_script_data(const char *file_name) -> bool;

        auto particle_count() -> uint32_t;
        auto particle_pointer(uint32_t position) -> particle_system_data *;

        auto mesh_count() -> uint32_t;
        auto mesh_pointer(uint32_t position) -> effect_mesh_script *;

        auto light_count() -> uint32_t;
        auto light_pointer(uint32_t position) -> light_data *;

        auto get_sound_instance_vector() -> sound_types::sound_instance_vector *;

        auto bounding_sphere_radius() -> float;
        auto bounding_sphere_position() -> math::vector3;

        auto file_name() const -> const char *;

      protected:
        void clear_particle_data_vector();
        void clear_light_data_vector();
        void clear_mesh_data_vector();

        virtual auto alloc_particle() -> particle_system_data *;
        virtual auto alloc_mesh() -> effect_mesh_script *;
        virtual auto alloc_light() -> light_data *;

      protected:
        particle_vector particle_vector_value_;
        mesh_vector mesh_vector_value_;
        light_vector light_vector_value_;
        sound_types::sound_instance_vector sound_instance_vector_value_;

        float bounding_sphere_radius_;
        math::vector3 bounding_sphere_position_;

        std::string file_name_;

      public:
        static void destroy_system();

        static auto new_value() -> effect_data *;
        static void delete_value(effect_data *data);

        static dynamic_pool<effect_data> pool_value;
    };
}
