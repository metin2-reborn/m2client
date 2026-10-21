#include <metin2/graphics/grp_light_manager.h>

#include <metin2/gpu/types.h>
#include "simple_light_instance.h"

namespace metin2::inline effects {

    dynamic_pool<light_instance> light_instance::pool_value;

    void light_instance::destroy_system() {
        pool_value.destroy();
    }

    auto light_instance::new_value() -> light_instance * {
        return pool_value.alloc();
    }

    void light_instance::delete_value(light_instance *data) {
        data->destroy();
        pool_value.free(data);
    }

    void light_instance::on_set_data_pointer(effect_element_base *element) {
        // Release a previous light without resetting the start time set_data_pointer just set.
        on_destroy();
        on_initialize();

        data_value_ = ((light_data *)element);

        loop_count_ = data_value_->get_loop_count();

        gpu::light light;
        data_value_->initialize_light(light);
        get_light_manager().register_light(LIGHT_TYPE_DYNAMIC, &light_id_value_, light);
    }

    auto light_instance::on_update(float elapsed_time) -> bool {
        if (!is_active()) {
            destroy();
            return false;
        }

        if (local_time_ >= data_value_->get_duration()) {
            if (data_value_->is_loop() && --loop_count_ != 0) {
                if (loop_count_ < 0)
                    loop_count_ = 0;
                local_time_ -= data_value_->get_duration();
            } else {
                destroy();
                loop_count_ = 1;
                return false;
            }
        }

        light *light = get_light_manager().get_light(light_id_value_);

        if (light) {
            light->set_ambient_color(data_value_->ambient_value_.r, data_value_->ambient_value_.g, data_value_->ambient_value_.b, data_value_->ambient_value_.a);
            light->set_diffuse_color(data_value_->diffuse_value_.r, data_value_->diffuse_value_.g, data_value_->diffuse_value_.b, data_value_->diffuse_value_.a);

            float range;
            data_value_->range(local_time_, range);
            light->set_range(range);

            math::vector3 pos;
            data_value_->get_position(local_time_, pos);
            pos = math::transform_coord(pos, *mc_pmat_local_);
            light->set_position(pos.x, pos.y, pos.z);
        }

        return true;
    }

    void light_instance::on_render() {
    }

    void light_instance::on_initialize() {
        light_id_value_ = 0;
    }

    void light_instance::on_destroy() {
        if (light_id_value_) {
            get_light_manager().delete_light(light_id_value_);
        }
    }

    light_instance::light_instance() {
        initialize();
    }

    light_instance::~light_instance() {
        destroy();
    }
}
