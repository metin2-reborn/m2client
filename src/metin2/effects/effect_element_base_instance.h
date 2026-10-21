#pragma once

#include "effect_element_base.h"

namespace metin2::inline effects {

    class effect_element_base_instance {
      public:
        effect_element_base_instance();
        virtual ~effect_element_base_instance();

        void set_data_pointer(effect_element_base *element);

        void initialize();
        void destroy();

        void set_local_matrix_pointer(const math::matrix *matrix);
        auto update(float elapsed_time) -> bool;
        void render();

        auto is_active() -> bool;
        void set_active();
        void set_deactive();

      protected:
        virtual void on_set_data_pointer(effect_element_base *element) = 0;

        virtual void on_initialize() = 0;
        virtual void on_destroy() = 0;

        virtual auto on_update(float elapsed_time) -> bool = 0;
        virtual void on_render() = 0;

      protected:
        const math::matrix *mc_pmat_local_;

        bool is_active_;

        float local_time_;
        float remaining_time_;
        bool start_;
    };
}
