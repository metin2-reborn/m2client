#pragma once

#include <cstdint>

namespace metin2::inline core {

    // Frame clock on the monotonic clock. advance() is called once per frame; the elapsed time it
    // reports is capped at MAX_FRAME_STEP_MS, so a stall (a map load, a paused window) is simulated
    // as one long frame instead of a jump.
    class timer {
      public:
        static constexpr uint32_t MAX_FRAME_STEP_MS = 100;

        timer();
        ~timer();

        void advance();
        void set_base_time();

        auto current_second() -> float;
        auto current_millisecond() -> uint32_t;

        auto elapsed_second() -> float;
        auto elapsed_milliecond() -> uint32_t;

      protected:
        uint32_t current_time_;
        uint32_t elapsed_time_;
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
