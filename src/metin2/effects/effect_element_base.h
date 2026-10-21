#pragma once

#include "type.h"

namespace metin2::inline effects {

    class effect_element_base {
      public:
        effect_element_base();
        virtual ~effect_element_base();

        void clear();
        auto is_data() -> bool;

        auto load_script(text_file_loader &text_file_loader) -> bool;

        void get_position(float time, math::vector3 &position);
        auto start_time() -> float;

      protected:
        virtual void on_clear() = 0;
        virtual auto on_is_data() -> bool = 0;
        virtual auto on_load_script(text_file_loader &text_file_loader) -> bool = 0;

      protected:
        float start_time_;
        time_event_table_position time_event_table_position_value_;
    };
}
