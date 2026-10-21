#pragma once

#include <metin2/graphics/grp_screen.h>

#include "effect_element_base_instance.h"
#include "simple_light_data.h"

namespace metin2::inline effects {

    class light_instance : public effect_element_base_instance {
      public:
        friend class light_data;

        light_instance();
        ~light_instance() override;

      protected:
        void on_set_data_pointer(effect_element_base *element) override;

        void on_initialize() override;
        void on_destroy() override;

        auto on_update(float elapsed_time) -> bool override;
        void on_render() override;

        uint32_t light_id_value_;
        light_data *data_value_;

        int loop_count_;

      public:
        static void destroy_system();

        static auto new_value() -> light_instance *;
        static void delete_value(light_instance *data);

        static dynamic_pool<light_instance> pool_value;
    };
}
