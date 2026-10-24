#include "timer.h"

#include <algorithm>
#include <chrono>

namespace metin2::inline core {

    namespace {
        uint32_t server_time = 0;
        uint32_t client_time = 0;
        uint32_t frame_time = 0;

        // Monotonic milliseconds, truncated to 32 bits like timeGetTime().
        auto monotonic_m_sec() -> uint32_t {
            using namespace std::chrono;
            return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
        }

        auto base_time() -> uint32_t {
            static const uint32_t s_base = monotonic_m_sec();
            return s_base;
        }
    } // namespace

    auto el_timer_get_m_sec() -> uint32_t {
        return monotonic_m_sec() - base_time();
    }

    void el_timer_set_server_m_sec(uint32_t server_time_value) {
        server_time = server_time_value;
        client_time = get_timer().current_millisecond();
    }

    auto el_timer_get_server_m_sec() -> uint32_t {
        return get_timer().current_millisecond() - client_time + server_time;
    }

    auto el_timer_get_frame_m_sec() -> uint32_t {
        return frame_time;
    }

    auto el_timer_get_server_frame_m_sec() -> uint32_t {
        return el_timer_get_frame_m_sec() - client_time + server_time;
    }

    void el_timer_set_frame_m_sec() {
        frame_time = el_timer_get_m_sec();
    }

    timer::timer() {
        base_time();

        current_time_ = 0;
        elapsed_time_ = 0;
    }

    timer::~timer() = default;

    void timer::set_base_time() {
        current_time_ = 0;
    }

    void timer::advance() {
        const uint32_t current_time = el_timer_get_m_sec();

        if (current_time_ == 0)
            current_time_ = current_time;

        elapsed_time_ = std::min(current_time - current_time_, MAX_FRAME_STEP_MS);
        current_time_ = current_time;
    }

    auto timer::current_second() -> float {
        return el_timer_get_m_sec() / 1000.0f;
    }

    auto timer::current_millisecond() -> uint32_t {
        return el_timer_get_m_sec();
    }

    auto timer::elapsed_second() -> float {
        return elapsed_milliecond() / 1000.0f;
    }

    auto timer::elapsed_milliecond() -> uint32_t {
        return elapsed_time_;
    }
}
