#pragma once
#include <cstdint>

namespace metin2::inline effects {

    class frame_controller {
      public:
        frame_controller();
        virtual ~frame_controller();

        void clear();

        void update(float elapsed_time);

        auto current_frame() -> uint32_t;

        void set_max_frame(uint32_t max_frame);
        void set_frame_time(float time);
        void set_start_frame(uint32_t start_frame);
        void set_loop_flag(bool flag);
        void set_loop_count(int loop_count);

        void set_active(bool flag);
        auto is_active(uint32_t main_frame = 0) -> bool;

      protected:
        // Dynamic
        bool is_active_;
        uint32_t dwcur_frame_;
        float last_frame_time_;

        int loop_count_;

        // Static
        bool is_loop_;
        uint32_t max_frame_;
        float frame_time_;
        uint32_t start_frame_;
    };
}
