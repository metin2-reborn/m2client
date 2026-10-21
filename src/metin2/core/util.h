#pragma once

#include "file_loader.h"

#include <map>
#include <vector>

#include <metin2/math/math.h>

namespace metin2::inline graphics {
    extern auto get_max_texture_height() -> uint32_t;
}
namespace metin2::inline graphics {
    extern auto get_max_texture_width() -> uint32_t;
}
namespace metin2::inline core {

    template <typename T>
    class transitor {
      public:
        transitor() = default;
        ~transitor() = default;

        void set_active(bool active = true) {
            activated_ = active;
        }

        auto is_active() -> bool {
            return activated_;
        }

        auto is_active_time(float fcur_time) -> bool {
            if (fcur_time >= end_time_)
                return false;

            return true;
        }

        void set_source_value(const T &source_value) {
            source_value_ = source_value;
        }

        void set_transition(const T &source_value, const T &target_value, float start_time, float blend_time) {
            source_value_ = source_value;
            target_value_ = target_value;
            start_time_ = start_time;
            end_time_ = start_time + blend_time;
        }

        auto value(float fcur_time, T *value) -> bool {
            if (fcur_time <= start_time_)
                return false;

            float percentage = (fcur_time - start_time_) / (end_time_ - start_time_);
            *value = source_value_ + (target_value_ - source_value_) * percentage;
            return true;
        }

      protected:
        uint32_t id_;

        bool activated_;
        float start_time_;
        float end_time_;

        T source_value_;
        T target_value_;
    };

    using transitor_float = transitor<float>;
    using transitor_color = transitor<math::color>;

    void printf_tabs(FILE *file, int tab_count, const char *string, ...);

    extern auto load_multiple_text_data(const char *file_name, token_vector_map &token_vector_map_value) -> bool;

}
