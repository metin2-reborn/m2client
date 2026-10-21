#include "frame_controller.h"

namespace metin2::inline effects {

    void frame_controller::update(float elapsed_time) {
        last_frame_time_ -= elapsed_time;

        for (int i = 0; i < 20; ++i) {
            if (last_frame_time_ < 0.0f) {
                last_frame_time_ += frame_time_;
                ++dwcur_frame_;

                if (dwcur_frame_ >= max_frame_) {
                    if (is_loop_ && --loop_count_ != 0) {
                        if (loop_count_ < 0)
                            loop_count_ = 0;
                        dwcur_frame_ = 0;
                    } else {
                        loop_count_ = 1;
                        dwcur_frame_ = 0;
                        is_active_ = false;
                        return;
                    }
                }
            } else {
                break;
            }
        }
    }

    auto frame_controller::current_frame() -> uint32_t {
        return dwcur_frame_;
    }

    void frame_controller::set_max_frame(uint32_t max_frame) {
        max_frame_ = max_frame;
    }
    void frame_controller::set_frame_time(float time) {
        frame_time_ = time;
        last_frame_time_ = time;
    }
    void frame_controller::set_start_frame(uint32_t start_frame) {
        start_frame_ = start_frame;
    }

    void frame_controller::set_loop_flag(bool flag) {
        is_loop_ = flag;
    }

    void frame_controller::set_loop_count(int loop_count) {
        loop_count_ = loop_count;
    }

    void frame_controller::set_active(bool flag) {
        is_active_ = flag;
    }

    auto frame_controller::is_active(uint32_t main_frame) -> bool {
        if (main_frame < start_frame_)
            return false;

        return is_active_;
    }

    void frame_controller::clear() {
        is_active_ = true;
        dwcur_frame_ = 0;
        last_frame_time_ = 0.0f;
        loop_count_ = 0;
    }

    frame_controller::frame_controller() {
        is_active_ = true;
        dwcur_frame_ = 0;
        last_frame_time_ = 0.0f;
        loop_count_ = 0;

        is_loop_ = false;
        max_frame_ = 0;
        frame_time_ = 0.0f;
        start_frame_ = 0;
    }
    frame_controller::~frame_controller() = default;
}
