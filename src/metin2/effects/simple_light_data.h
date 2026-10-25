#pragma once

#include <metin2/math/math.h>

#include <metin2/gpu/types.h>
#include "effect_element_base.h"
#include "type.h"

namespace metin2::inline effects {

    class light_data : public effect_element_base {
        friend class light_instance;

      public:
        light_data();
        ~light_data() override;

        void range(float time, float &range);
        auto get_duration() -> float;
        auto is_loop() -> bool {
            return loop_flag_;
        }
        auto get_loop_count() -> int {
            return loop_count_;
        }
        void initialize_light(gpu::light &light);

      protected:
        void on_clear() override;
        auto on_is_data() -> bool override;

        auto on_load_script(asset_table &table) -> bool override;

      protected:
        float max_range_;
        float duration_;
        time_event_table_float time_event_table_range_;

        math::color ambient_value_;
        math::color diffuse_value_;

        bool loop_flag_;
        int loop_count_;

        float attenuation0_;
        float attenuation1_;
        float attenuation2_;

      public:
        static void destroy_system();

        static auto new_value() -> light_data *;
        static void delete_value(light_data *data);

        static dynamic_pool<light_data> pool_value;
    };
}
