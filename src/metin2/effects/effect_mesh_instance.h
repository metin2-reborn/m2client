#pragma once

#include <metin2/graphics/grp_image_instance.h>
#include <metin2/graphics/grp_screen.h>
#include "effect_element_base_instance.h"
#include "effect_mesh.h"
#include "frame_controller.h"

namespace metin2::inline effects {

    class effect_mesh_instance : public effect_element_base_instance {
      public:
        // Per-mesh instance of the texture data.
        struct texture_instance {
            frame_controller texture_frame_controller;
            std::vector<graphic_image_instance *> texture_instance_vector;
        };

      public:
        effect_mesh_instance();
        ~effect_mesh_instance() override;

      public:
        static void destroy_system();

        static auto new_value() -> effect_mesh_instance *;
        static void delete_value(effect_mesh_instance *mesh_instance);

        static dynamic_pool<effect_mesh_instance> pool_value;

      protected:
        void on_set_data_pointer(effect_element_base *element) override;

        void on_initialize() override;
        void on_destroy() override;

        auto on_update(float elapsed_time) -> bool override;
        void on_render() override;

        auto is_active() -> bool;

      protected:
        effect_mesh_script *mesh_script_;
        effect_mesh *effect_mesh_value_;

        frame_controller mesh_frame_controller_;
        std::vector<texture_instance> texture_instance_vector_;

        effect_mesh::ref ro_mesh_;
    };
}
