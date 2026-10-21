#pragma once
#include <cstdint>
#include <deque>
#include <metin2/graphics/grp_screen.h>
#include <metin2/math/math.h>
#include <string>
#include "flying_data.h"

namespace metin2::inline world::inline flying {

    class fly_trace : public screen {
      public:
        fly_trace();
        ~fly_trace() override;

        void destroy();

        void create(const flying_data::flying_attach_data &flying_attach_data);

        void update_new_position(const math::vector3 &position);

        void update();
        void render();

      protected:
        void initialize_impl();

      protected:
        using time_position = std::pair<float, math::vector3>;
        using time_position_deque = std::deque<time_position>;

        bool rect_shape_;
        uint32_t color_value_;
        float size_;
        float tail_length_;

        time_position_deque time_position_deque_value_;

      public:
        static void destroy_system();

        static auto new_value() -> fly_trace *;
        static void delete_value(fly_trace *inst);

        static dynamic_pool<fly_trace> pool_value;
    };
}
