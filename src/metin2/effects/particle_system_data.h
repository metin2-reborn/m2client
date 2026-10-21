#pragma once

#include <metin2/core/text_file_loader.h>

#include "effect_element_base.h"
#include "emitter_property.h"
#include "particle_property.h"

namespace metin2::inline effects {

    class particle_instance;

    class particle_system_data : public effect_element_base {
      public:
        ~particle_system_data() override;
        particle_system_data();

        auto emitter_property_pointer() -> emitter_property *;
        auto particle_property_pointer() -> particle_property *;

        void change_texture(const char *file_name);

        void build_decorator(particle_instance *instance);

      protected:
        auto on_load_script(text_file_loader &text_file_loader) -> bool override;

        void on_clear() override;
        auto on_is_data() -> bool override;

        emitter_property emitter_property_value_;
        particle_property particle_property_value_;

      public:
        static void destroy_system();

        static auto new_value() -> particle_system_data *;
        static void delete_value(particle_system_data *data);

        static dynamic_pool<particle_system_data> pool_value;
    };
}
