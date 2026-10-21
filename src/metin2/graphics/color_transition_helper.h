#pragma once
#include <cstdint>
#include <metin2/gpu/types.h>

namespace metin2::inline graphics {

    class color_transition_helper {
      public:
        color_transition_helper();
        ~color_transition_helper();

        void clear(const float &red,
            const float &green,
            const float &blue,
            const float &alpha);
        void set_src_color(const float &red,
            const float &green,
            const float &blue,
            const float &alpha);
        void set_transition(const float &red,
            const float &green,
            const float &blue,
            const float &alpha,
            const uint32_t &duration);
        auto cur_color() -> const uint32_t &;

        void start_transition();
        auto update() -> bool;

        auto is_transition_started() -> bool { return transition_started_; }

      private:
        uint32_t cur_color_;

        uint32_t start_time_; // time the transition started
        uint32_t duration_;   // how long the transition takes

        bool transition_started_;

        float src_red_, src_green_, src_blue_, src_alpha_;
        float dst_red_, dst_green_, dst_blue_, dst_alpha_;
    };
}
