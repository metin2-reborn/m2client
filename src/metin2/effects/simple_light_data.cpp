#include "simple_light_data.h"
#include <metin2/gpu/types.h>

namespace metin2::inline effects {

    dynamic_pool<light_data> light_data::pool_value;

    void light_data::destroy_system() {
        pool_value.destroy();
    }

    auto light_data::new_value() -> light_data * {
        return pool_value.alloc();
    }

    void light_data::delete_value(light_data *data) {
        data->clear();
        pool_value.free(data);
    }

    void light_data::on_clear() {
        max_range_ = 300.0f;

        time_event_table_range_.clear();

        ambient_value_.r = 0.5f;
        ambient_value_.g = 0.5f;
        ambient_value_.b = 0.5f;
        ambient_value_.a = 1.0f;
        diffuse_value_.r = 0.0f;
        diffuse_value_.g = 0.0f;
        diffuse_value_.b = 0.0f;
        diffuse_value_.a = 1.0f;

        duration_ = 1.0f;

        attenuation0_ = 0.0f;
        attenuation1_ = 0.1f;
        attenuation2_ = 0.0f;

        loop_flag_ = false;
        loop_count_ = 0;
    }
    void light_data::range(float time, float &range) {
        if (time_event_table_range_.empty()) {
            range = 1.0f * max_range_;
            if (range < 0.0f)
                range = 0.0f;
            return;
        }

        get_time_event_blend_value(time, time_event_table_range_, &range);
        range *= max_range_;
        if (range < 0.0f)
            range = 0.0f;
    }

    auto light_data::on_is_data() -> bool {
        return true;
    }

    auto light_data::on_load_script(asset_table &table) -> bool {
        duration_ = 1.0f;
        loop_flag_ = false;
        loop_count_ = 0;
        time_event_table_range_.clear();
        bool ok = table.optional("duration", duration_);
        ok &= table.optional("loop", loop_flag_);
        ok &= table.optional("loop_count", loop_count_);
        ok &= table.required("ambient", ambient_value_);
        ok &= table.required("diffuse", diffuse_value_);
        ok &= table.required("max_range", max_range_);
        math::vector3 attenuation;
        ok &= table.required("attenuation", attenuation);
        attenuation0_ = attenuation.x;
        attenuation1_ = attenuation.y;
        attenuation2_ = attenuation.z;
        ok &= read_time_events(table, "range_events", time_event_table_range_);
        return ok;
    }

    light_data::light_data() {
        clear();
    }

    light_data::~light_data() = default;

    auto light_data::get_duration() -> float {
        return duration_;
    }
    void light_data::initialize_light(gpu::light &light) {
        light.type = gpu::light_type::POINT;

        light.ambient = ambient_value_;
        light.diffuse = diffuse_value_;
        light.attenuation0 = attenuation0_;
        light.attenuation1 = attenuation1_;
        light.attenuation2 = attenuation2_;

        math::vector3 position;
        get_position(0.0f, position);
        light.position = position;

        range(0.0f, light.range);
    }
}
