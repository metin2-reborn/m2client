#pragma once
#include "map_type.h"

#include <string_view>

namespace metin2::inline world::inline maps {

    void environment_init(environment_data &env_data);
    // Reads an environment script (TOML, [asset] type "Environment") over the values already in
    // env_data (environment_init gives the defaults).
    auto environment_load(environment_data &env_data, const char *env_file_name) -> bool;
    auto environment_parse(environment_data &env_data, std::string_view text, std::string_view name) -> bool;

    auto get_linear_interpolation(float begin, float end, float cur_rate) -> float;

    class ease_out_interpolation {
      public:
        ease_out_interpolation();
        virtual ~ease_out_interpolation();

        void initialize();

        auto setup(float start, float end, float time) -> bool;
        void interpolate(float elapsed_time);
        auto is_playing() -> bool;

        auto value() -> float;
        auto changing_value() -> float;

      protected:
        float remaining_time_;
        float value_;
        float speed_;
        float acceleration_;

        float start_value_;
        float last_value_;
    };
}
