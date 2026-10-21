#pragma once

#include "effect_element_base_instance.h"
#include "particle_instance.h"
#include "particle_property.h"

#include <list>
#include <metin2/graphics/grp_image_instance.h>
#include <metin2/graphics/grp_screen.h>
#include <metin2/graphics/state_manager.h>
#include "emitter_property.h"
#include "particle_system_data.h"

namespace metin2::inline effects {

    class particle_system_instance : public effect_element_base_instance {
      public:
        static void destroy_system();

        static auto new_value() -> particle_system_instance *;
        static void delete_value(particle_system_instance *data);

        static dynamic_pool<particle_system_instance> pool_value;

      public:
        template <typename T>
        inline void for_each_particle_rendering(T &fun_obj) {
            uint32_t frame_index;
            for (frame_index = 0; frame_index < vct_img_inst_.size(); frame_index++) {
                STATEMANAGER.set_texture(0, vct_img_inst_[frame_index]->texture_reference().gpu_texture());
                auto itor = particle_instance_list_vector_value_[frame_index].begin();
                for (; itor != particle_instance_list_vector_value_[frame_index].end(); ++itor) {
                    if (!in_frustum(*itor))
                        continue;
                    fun_obj(*itor);
                }
            }
        }

        particle_system_instance();
        ~particle_system_instance() override;

        void on_set_data_pointer(effect_element_base *element) override;

        void create_particles(float elapsed_time);

        inline auto in_frustum(particle_instance *instance) -> bool {
            if (particle_property_value_->attach_flag)
                return screen::get_frustum().view_volume_test(vector3d(
                                                                  instance->position_value_.x + mc_pmat_local_->_41,
                                                                  instance->position_value_.y + mc_pmat_local_->_42,
                                                                  instance->position_value_.z + mc_pmat_local_->_43),
                           instance->radius_approximation()) != VS_OUTSIDE;
            else
                return screen::get_frustum().view_volume_test(vector3d(instance->position_value_.x, instance->position_value_.y, instance->position_value_.z), instance->radius_approximation()) != VS_OUTSIDE;
        }

        auto emission_count() -> uint32_t;

      protected:
        void on_initialize() override;
        void on_destroy() override;

        auto on_update(float elapsed_time) -> bool override;
        void on_render() override;

      protected:
        float emission_residue_;

        uint32_t current_emission_count_;
        int loop_count_;

        using particle_instance_list = std::list<particle_instance *>;
        using particle_instance_list_vector = std::vector<particle_instance_list>;
        particle_instance_list_vector particle_instance_list_vector_value_;

        using image_instance_vector = std::vector<graphic_image_instance *>;
        image_instance_vector vct_img_inst_;

        particle_system_data *data_value_;

        particle_property *particle_property_value_;
        emitter_property *emitter_property_value_;
    };
}
