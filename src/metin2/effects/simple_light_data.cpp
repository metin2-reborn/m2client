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

    auto light_data::on_load_script(text_file_loader &text_file_loader) -> bool {
        if (!text_file_loader.token_float("duration", &duration_))
            duration_ = 1.0f;

        if (!text_file_loader.token_boolean("loopflag", &loop_flag_))
            loop_flag_ = false;

        if (!text_file_loader.token_integer("loopcount", &loop_count_))
            loop_count_ = 0;

        if (!text_file_loader.token_color("ambientcolor", &ambient_value_))
            return false;

        if (!text_file_loader.token_color("diffusecolor", &diffuse_value_))
            return false;

        if (!text_file_loader.token_float("maxrange", &max_range_))
            return false;

        if (!text_file_loader.token_float("attenuation0", &attenuation0_))
            return false;

        if (!text_file_loader.token_float("attenuation1", &attenuation1_))
            return false;

        if (!text_file_loader.token_float("attenuation2", &attenuation2_))
            return false;

        if (!get_token_time_event_float(text_file_loader, "timeeventrange", &time_event_table_range_)) {
            time_event_table_range_.clear();
        }

        return true;
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
