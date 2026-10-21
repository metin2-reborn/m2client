#include "color_transition_helper.h"
#include <metin2/core/timer.h>

namespace metin2::inline graphics {

    void color_transition_helper::clear(const float &red,
        const float &green,
        const float &blue,
        const float &alpha) {

        src_red_ = red;
        src_green_ = green;
        src_blue_ = blue;
        src_alpha_ = alpha;

        dst_red_ = red;
        dst_green_ = green;
        dst_blue_ = blue;
        dst_alpha_ = alpha;

        cur_color_ = 0x00000000;

        start_time_ = duration_ = 0;
    }

    void color_transition_helper::set_src_color(const float &red,
        const float &green,
        const float &blue,
        const float &alpha) {
        src_red_ = red;
        src_green_ = green;
        src_blue_ = blue;
        src_alpha_ = alpha;
    }

    void color_transition_helper::set_transition(const float &red,
        const float &green,
        const float &blue,
        const float &alpha,
        const uint32_t &duration) {
        dst_red_ = red;
        dst_green_ = green;
        dst_blue_ = blue;
        dst_alpha_ = alpha;

        duration_ = duration;
    }

    void color_transition_helper::start_transition() {
        transition_started_ = true;
        start_time_ = el_timer_get_m_sec();
    }

    auto color_transition_helper::update() -> bool {

        uint32_t cur_time = el_timer_get_m_sec();
        uint32_t elapsed_time = cur_time - start_time_;

        float fpercent = (float)(elapsed_time) / (float)(duration_);
        if (fpercent <= 0.0f)
            fpercent = 0.0f;
        if (fpercent >= 1.0f)
            fpercent = 1.0f;
        float cur_red, cur_green, cur_blue, cur_alpha;
        cur_red = src_red_ + (dst_red_ - src_red_) * fpercent;
        cur_green = src_green_ + (dst_green_ - src_green_) * fpercent;
        cur_blue = src_blue_ + (dst_blue_ - src_blue_) * fpercent;
        cur_alpha = src_alpha_ + (dst_alpha_ - src_alpha_) * fpercent;

        cur_color_ = (((uint32_t)(cur_alpha * 255.0f) & 0xff) << 24) |
                     (((uint32_t)(cur_red * 255.0f) & 0xff) << 16) |
                     (((uint32_t)(cur_green * 255.0f) & 0xff) << 8) |
                     ((uint32_t)(cur_blue * 255.0f) & 0xff);

        if ((1.0f == fpercent) && (cur_alpha == dst_alpha_) && (cur_red == dst_red_) && (cur_green == dst_green_) && (cur_blue == dst_blue_)) {
            transition_started_ = false;
            return false;
        }

        return true;
    }

    auto color_transition_helper::cur_color() -> const uint32_t & {
        return cur_color_;
    }

    color_transition_helper::color_transition_helper() : transition_started_(false) {
        clear(0.0f, 0.0f, 0.0f, 0.0f);
    }

    color_transition_helper::~color_transition_helper() {
        clear(0.0f, 0.0f, 0.0f, 0.0f);
    }
}
