#pragma once

#include <metin2/core/pool.h>
#include <metin2/graphics/grp_object_instance.h>
#include <metin2/sound/type.h>

#include "effect_data.h"
#include "effect_element_base_instance.h"
#include "effect_mesh_instance.h"
#include "particle_system_instance.h"
#include "simple_light_instance.h"

namespace metin2::inline effects {

    class effect_instance : public graphic_object_instance {
      public:
        enum {
            ID = EFFECT_OBJECT
        };
        auto get_type() const -> uint32_t override {
            return effect_instance::ID;
        }

        auto bounding_sphere(math::vector3 &center, float &radius) -> bool override;

        static void destroy_system();

        static auto new_value() -> effect_instance *;
        static void delete_value(effect_instance *eft_inst);

      public:
        effect_instance();
        ~effect_instance() override;

        auto less_render_order(effect_instance *eft_inst) -> bool;

        void set_effect_data_pointer(effect_data *effect_data);

        void clear();
        auto is_alive() -> bool;
        void set_active();
        void set_deactive();
        void set_global_matrix(const math::matrix &global);
        void update_sound();
        void on_update() override;
        void on_render() override;
        void on_blend_render() override {}
        void on_render_to_shadow_map() override {}
        void on_render_shadow() override {}
        void on_render_pc_blocker() override {}

      protected:
        void initialize_impl();

        void set_particle_data(particle_system_data *data);
        void set_mesh_data(effect_mesh_script *mesh);
        void set_light_data(light_data *data);

        void on_update_collision_data(const static_collision_data_vector *pscd_vector) override {}
        void on_update_heigh_instance(attribute_instance *attribute_instance) override {}
        auto on_get_object_height(float x, float y, float *height) -> bool override { return false; }

      protected:
        bool is_alive_;
        float sound_time_;          // seconds the effect has played, for its sound cues
        uint32_t next_sound_frame_; // the first cue frame not played yet
        math::matrix global_;

        effect_data *eft_data_;

        std::vector<particle_system_instance *> particle_instance_vector_;
        std::vector<effect_mesh_instance *> mesh_instance_vector_;
        std::vector<light_instance *> light_instance_vector_;

        sound_types::sound_instance_vector *sound_instance_vector_value_;

        float bounding_sphere_radius_;
        math::vector3 bounding_sphere_position_;

        float last_time_;

      public:
        static dynamic_pool<effect_instance> pool_value;
    };
}
