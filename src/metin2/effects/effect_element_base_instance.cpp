#include "effect_element_base_instance.h"

namespace metin2::inline effects {

    auto effect_element_base_instance::update(float elapsed_time) -> bool {
        if (start_) {
            local_time_ += elapsed_time;

            return on_update(elapsed_time);
        } else {
            remaining_time_ -= elapsed_time;
            if (remaining_time_ <= 0.0f)
                start_ = true;
            return true;
        }
    }

    void effect_element_base_instance::render() {
        if (!start_)
            return;

        assert(mc_pmat_local_);

        on_render();
    }

    void effect_element_base_instance::set_local_matrix_pointer(const math::matrix *matrix) {
        mc_pmat_local_ = matrix;
    }

    void effect_element_base_instance::set_data_pointer(effect_element_base *element) {
        remaining_time_ = element->start_time();
        if (remaining_time_ <= 0.0f)
            start_ = true;
        else
            start_ = false;

        on_set_data_pointer(element);
    }

    auto effect_element_base_instance::is_active() -> bool {
        return is_active_;
    }
    void effect_element_base_instance::set_active() {
        is_active_ = true;
    }
    void effect_element_base_instance::set_deactive() {
        is_active_ = false;
    }

    void effect_element_base_instance::initialize() {
        mc_pmat_local_ = nullptr;

        is_active_ = true;

        local_time_ = 0.0f;

        start_ = false;
        remaining_time_ = 0.0f;

        on_initialize();
    }

    void effect_element_base_instance::destroy() {
        on_destroy();
        initialize();
    }

    effect_element_base_instance::effect_element_base_instance() = default;
    effect_element_base_instance::~effect_element_base_instance() = default;
}
