#pragma once

#include <cstdint>

namespace metin2::inline core {

    // Frame clock. Either follows the real monotonic clock or, after use_custom_time(), advances by
    // a fixed ~16.5 ms per advance() call.
    class timer {
      public:
        timer();
        ~timer();

        void advance();
        void adjust(int time_gap);
        void set_base_time();

        auto current_second() -> float;
        auto current_millisecond() -> uint32_t;

        auto elapsed_second() -> float;
        auto elapsed_milliecond() -> uint32_t;

        void use_custom_time();

      protected:
        bool use_real_time_;
        uint32_t base_time_;
        uint32_t current_time_;
        float current_time_seconds_;
        uint32_t elapsed_time_;
        int index_value_;
    };

    inline auto get_timer() -> timer & {
        static timer instance;
        return instance;
    }

    // Milliseconds since the timer was first used (32-bit, wraps like the original timeGetTime delta).
    auto el_timer_get_m_sec() -> uint32_t;

    void el_timer_set_server_m_sec(uint32_t server_time_value);
    auto el_timer_get_server_m_sec() -> uint32_t;

    void el_timer_set_frame_m_sec();
    auto el_timer_get_frame_m_sec() -> uint32_t;
    auto el_timer_get_server_frame_m_sec() -> uint32_t;
}
