#include <algorithm>
#include <metin2/core/timer.h>

#include <metin2/gpu/types.h>
#include "grp_light_manager.h"
#include "state_manager.h"

namespace metin2::inline graphics {

    float light_base::cur_time_ = 0.0f;

    light_manager::light_manager() {
        center_position_ = math::vector3(0.0f, 0.0f, 0.0f);
        limit_light_count_ = LIGHT_LIMIT_DEFAULT;
    }

    light_manager::~light_manager() = default;

    void light_manager::destroy() {
        light_pool_.destroy();
    }

    void light_manager::initialize() {
        set_skip_index(1);

        non_using_light_id_deque_.clear();

        light_map_value_.clear();
        light_pool_.free_all();
    }

    void light_manager::register_light(light_type /*LightType*/, light_id *pout_light_id, gpu::light &light_data) {
        light *light = light_pool_.alloc();
        light_id id = new_light_id();
        light->set_parameter(id, light_data);
        light_map_value_[id] = light;
        *pout_light_id = id;
    }

    void light_manager::delete_light(light_id light_id) {
        auto itor = light_map_value_.find(light_id);

        if (light_map_value_.end() == itor) {
            assert(!"CLightManager::DeleteLight - Failed to find light ID!");
            return;
        }

        light *light = itor->second;

        light->clear();
        light_pool_.free(light);

        light_map_value_.erase(itor);

        release_light_id(light_id);
    }

    auto light_manager::get_light(light_id light_id) -> light * {
        auto itor = light_map_value_.find(light_id);

        if (light_map_value_.end() == itor) {
            assert(!"CLightManager::SetLightData - Failed to find light ID!");
            return nullptr;
        }

        return itor->second;
    }

    void light_manager::set_center_position(const math::vector3 &position) {
        center_position_ = position;
    }

    void light_manager::set_skip_index(uint32_t skip_index) {
        skip_index_ = skip_index;
    }

    struct light_comp {
        auto operator()(const light *l, const light *r) const -> bool {
            return l->distance() < r->distance();
        }
    };

    auto light_manager::new_light_id() -> light_id {
        if (!non_using_light_id_deque_.empty()) {
            light_id id = non_using_light_id_deque_.back();
            non_using_light_id_deque_.pop_back();
            return (id);
        }

        return skip_index_ + light_map_value_.size();
    }

    void light_manager::release_light_id(light_id light_id) {
        non_using_light_id_deque_.push_back(light_id);
    }

    void light_manager::update() {
        cur_time_ = get_timer().current_second();
    }

    light::light() {
        initialize();
    }

    light::~light() {
        clear();
    }

    void light::initialize() {
        light_id_value_ = 0;
        is_edited_ = true;
        distance_ = 0.0f;

        light_data_ = {};

        light_data_.type = gpu::light_type::POINT;
        light_data_.attenuation0 = 0.0f;
        light_data_.attenuation1 = 1.0f;
        light_data_.attenuation2 = 0.0f;
    }

    void light::clear() {
        if (light_id_value_)
            set_device_light(false);
        initialize();
    }

    void light::set_device_light(bool active) {
        if (active && is_edited_) {
            if (device_)
                STATEMANAGER.set_light(light_id_value_, &light_data_);
        }
        if (device_) {
            STATEMANAGER.enable_light(light_id_value_, active);
        }
    }

    void light::set_parameter(light_id id, const gpu::light &light) {
        light_id_value_ = id;
        light_data_ = light;
    }

    void light::set_diffuse_color(float fr, float fg, float fb, float fa) {
        if (light_data_.diffuse.r == fr && light_data_.diffuse.g == fg && light_data_.diffuse.b == fb && light_data_.diffuse.a == fa)
            return;
        light_data_.diffuse.r = fr;
        light_data_.diffuse.g = fg;
        light_data_.diffuse.b = fb;
        light_data_.diffuse.a = fa;
        is_edited_ = true;
    }

    void light::set_ambient_color(float fr, float fg, float fb, float fa) {
        if (light_data_.ambient.r == fr && light_data_.ambient.g == fg && light_data_.ambient.b == fb && light_data_.ambient.a == fa)
            return;
        light_data_.ambient.r = fr;
        light_data_.ambient.g = fg;
        light_data_.ambient.b = fb;
        light_data_.ambient.a = fa;
        is_edited_ = true;
    }

    void light::set_range(float range) {
        if (light_data_.range == range)
            return;

        light_data_.range = range;
        is_edited_ = true;
    }

    auto light::get_position() const -> const math::vector3 & {
        return light_data_.position;
    }

    void light::set_position(float fx, float fy, float fz) {
        if (light_data_.position.x == fx && light_data_.position.y == fy && light_data_.position.z == fz)
            return;

        light_data_.position.x = fx;
        light_data_.position.y = fy;
        light_data_.position.z = fz;
        is_edited_ = true;
    }

    void light::set_distance(float distance) {
        distance_ = distance;
    }

    void light::update() {
        if (ambient_color_transitor_.is_active_time(cur_time_)) {
            if (!ambient_color_transitor_.is_active()) {
                ambient_color_transitor_.set_active();
                ambient_color_transitor_.set_source_value(light_data_.ambient);
            } else {
                math::color color;

                ambient_color_transitor_.value(cur_time_, &color);
                set_ambient_color(color.r, color.g, color.b, color.a);
            }
        }

        if (diffuse_color_transitor_.is_active_time(cur_time_)) {
            if (!diffuse_color_transitor_.is_active()) {
                diffuse_color_transitor_.set_active();
                diffuse_color_transitor_.set_source_value(light_data_.diffuse);
            } else {
                math::color color;
                diffuse_color_transitor_.value(cur_time_, &color);
                set_diffuse_color(color.r, color.g, color.b, color.a);
            }
        }

        if (range_transitor_.is_active_time(cur_time_)) {
            if (!range_transitor_.is_active()) {
                range_transitor_.set_active();
                range_transitor_.set_source_value(light_data_.range);
            } else {
                float range = light_data_.range;
                range_transitor_.value(cur_time_, &range);
                set_range(range);
            }
        }
    }
}
