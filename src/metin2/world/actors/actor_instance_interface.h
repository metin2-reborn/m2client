#pragma once

#include <metin2/models/thing_instance.h>

namespace metin2::inline world::inline actors {

    class actor_instance_interface : public graphic_thing_instance {
      public:
        enum {
            ID = ACTOR_OBJECT
        };
        auto get_type() const -> uint32_t override { return ID; }

        actor_instance_interface() = default;
        ~actor_instance_interface() override = default;
        virtual auto test_collision_with_dynamic_sphere(const dynamic_sphere_instance &dsi) -> bool = 0;
        virtual auto virtual_id() -> uint32_t = 0;
    };
}
