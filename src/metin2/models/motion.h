#pragma once

#include <granny.h>

namespace metin2::inline models {

    class motion {
      public:
        motion();
        virtual ~motion();

        auto is_empty() -> bool;

        void destroy();
        auto bind_granny_animation(granny_animation *pgrn_ani) -> bool;

        auto granny_animation_pointer() const -> granny_animation *;

        auto get_duration() const -> float;

      protected:
        void initialize();

      protected:
        granny_animation *pgrn_ani_;
    };
}
