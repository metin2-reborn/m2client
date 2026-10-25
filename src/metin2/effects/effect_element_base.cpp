#include "effect_element_base.h"
#include <cmath>

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

    auto effect_element_base::load_script(asset_table &table) -> bool {
        constexpr std::string_view moving_type_names[] = {"Direct", "Bezier"};

        start_time_ = 0.0f;
        bool ok = table.optional("start_time", start_time_);

        time_event_table_position_value_.clear();
        if (const toml::array *path = table.array("path")) {
            for (const toml::node &node : *path) {
                const toml::table *entry = node.as_table();
                if (!entry) {
                    table.error("path", "must hold tables { time, mode, position, control }");
                    return false;
                }
                asset_table entry_table(*entry, table.file_name());
                effect_position position{};
                position.moving_type_value = MOVING_TYPE_DIRECT;
                ok &= entry_table.required("time", position.time);
                ok &= entry_table.required_enum("mode", moving_type_names, position.moving_type_value);
                ok &= entry_table.required("position", position.position_value);
                if (position.moving_type_value == MOVING_TYPE_BEZIER_CURVE) {
                    ok &= entry_table.required("control", position.control_point);
                }
                ok &= entry_table.finish();
                time_event_table_position_value_.push_back(position);
            }
        }

        return on_load_script(table) && ok;
    }

    auto effect_element_base::start_time() -> float {
        return start_time_;
    }

    effect_element_base::effect_element_base() {
        start_time_ = 0.0f;
    }
    effect_element_base::~effect_element_base() = default;
}
