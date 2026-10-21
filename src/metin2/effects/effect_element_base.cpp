#include "effect_element_base.h"
#include <cmath>
#include <metin2/core/parse.h>

namespace metin2::inline effects {

    void effect_element_base::get_position(float time_value, math::vector3 &position_value) {
        if (time_event_table_position_value_.empty()) {
            position_value = math::vector3(0.0f, 0.0f, 0.0f);
            return;
        }
        if (time_event_table_position_value_.size() == 1) {
            position_value = time_event_table_position_value_[0].position_value;
            return;
        }
        if (time_event_table_position_value_.front().time > time_value) {
            position_value = time_event_table_position_value_.front().position_value;
            return;
        }
        if (time_event_table_position_value_.back().time < time_value) {
            position_value = time_event_table_position_value_.back().position_value;
            return;
        }

        auto result = std::lower_bound(time_event_table_position_value_.begin(), time_event_table_position_value_.end(), time_value);

        effect_position &effect_position_value = *result;
        auto prev_value = result;
        if (time_event_table_position_value_.begin() != result) {
            prev_value = result - 1;
        } else {
            position_value = result->position_value;
            return;
        }
        effect_position &prev_effect_position = *prev_value;
        int moving_type_value = prev_effect_position.moving_type_value;

        if (MOVING_TYPE_DIRECT == moving_type_value) {
            float head = fabs(effect_position_value.time - time_value) / fabs(effect_position_value.time - prev_effect_position.time);
            float tail = 1.0f - fabs(effect_position_value.time - time_value) / fabs(effect_position_value.time - prev_effect_position.time);
            position_value = (prev_effect_position.position_value * head) + (effect_position_value.position_value * tail);
        } else if (MOVING_TYPE_BEZIER_CURVE == moving_type_value) {
            float ft = (time_value - prev_effect_position.time) / (effect_position_value.time - prev_effect_position.time);

            position_value = prev_effect_position.position_value * (1.0f - ft) * (1.0f - ft) +
                             (prev_effect_position.position_value + prev_effect_position.control_point) * (1.0f - ft) * ft * 2 +
                             effect_position_value.position_value * ft * ft;
        }
    }

    auto effect_element_base::is_data() -> bool {
        return on_is_data();
    }

    void effect_element_base::clear() {
        start_time_ = 0.0f;

        on_clear();
    }

    auto effect_element_base::load_script(text_file_loader &text_file_loader) -> bool {
        token_vector *token_vector;
        if (!text_file_loader.token_float("starttime", &start_time_)) {
            start_time_ = 0.0f;
        }
        if (text_file_loader.get_token_vector("timeeventposition", &token_vector)) {
            time_event_table_position_value_.clear();

            for (uint32_t i = 0; i < token_vector->size();) {
                effect_position effect_position;
                effect_position.time = metin2::to_number<double>(token_vector->at(i++).c_str());
                if (token_vector->at(i) == "MOVING_TYPE_BEZIER_CURVE") {
                    i++;

                    effect_position.moving_type_value = MOVING_TYPE_BEZIER_CURVE;

                    effect_position.position_value.x = metin2::to_number<double>(token_vector->at(i++).c_str());
                    effect_position.position_value.y = metin2::to_number<double>(token_vector->at(i++).c_str());
                    effect_position.position_value.z = metin2::to_number<double>(token_vector->at(i++).c_str());

                    effect_position.control_point.x = metin2::to_number<double>(token_vector->at(i++).c_str());
                    effect_position.control_point.y = metin2::to_number<double>(token_vector->at(i++).c_str());
                    effect_position.control_point.z = metin2::to_number<double>(token_vector->at(i++).c_str());
                } else if (token_vector->at(i) == "MOVING_TYPE_DIRECT") {
                    i++;

                    effect_position.moving_type_value = MOVING_TYPE_DIRECT;

                    effect_position.position_value.x = metin2::to_number<double>(token_vector->at(i++).c_str());
                    effect_position.position_value.y = metin2::to_number<double>(token_vector->at(i++).c_str());
                    effect_position.position_value.z = metin2::to_number<double>(token_vector->at(i++).c_str());

                    effect_position.control_point = math::vector3(0.0f, 0.0f, 0.0f);
                } else {
                    return false;
                }

                time_event_table_position_value_.push_back(effect_position);
            }
        }

        return on_load_script(text_file_loader);
    }

    auto effect_element_base::start_time() -> float {
        return start_time_;
    }

    effect_element_base::effect_element_base() {
        start_time_ = 0.0f;
    }
    effect_element_base::~effect_element_base() = default;
}
