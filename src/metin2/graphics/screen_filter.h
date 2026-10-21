#pragma once

#include "grp_screen.h"

namespace metin2::inline graphics {

    class screen_filter : public screen {
      public:
        screen_filter();
        ~screen_filter() override;

        void set_enable(bool flag);
        void set_blend_type(uint8_t src_type, uint8_t dest_type);
        void set_color(const math::color &color);

        void render();

      protected:
        bool enable_;
        uint8_t src_type_;
        uint8_t dest_type_;
        math::color color_value_;
    };
}
