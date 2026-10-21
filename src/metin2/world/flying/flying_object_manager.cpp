
#include "flying_object_manager.h"
#include <metin2/core/random.h>
#include <metin2/log/log.h>
#include "flying_data.h"
#include "flying_instance.h"

#include <metin2/core/utils.h>
#include <metin2/world/actors/actor_instance.h>

namespace metin2::inline world::inline flying {

    flying_manager::flying_manager() {
        map_manager_value_ = nullptr;
    }

    flying_manager::~flying_manager() {
        destroy();
    }

    void flying_manager::destroy_flying_instance_list() {
        flying_instance_list::iterator i;
        for (i = lst_fly_inst_.begin(); i != lst_fly_inst_.end(); ++i) {
            flying_instance *fly_inst = *i;
            flying_instance::delete_value(fly_inst);
        }
        lst_fly_inst_.clear();
    }

    void flying_manager::destroy_flying_data_map() {
        flying_data_map::iterator i;
        for (i = map_fly_data_.begin(); i != map_fly_data_.end(); ++i) {
            flying_data *fly_data = i->second;
            flying_data::delete_value(fly_data);
        }
        map_fly_data_.clear();
    }

    void flying_manager::delete_all_instances() {
        destroy_flying_instance_list();
    }

    void flying_manager::destroy() {
        destroy_flying_instance_list();
        destroy_flying_data_map();

        map_manager_value_ = nullptr;
    }

    auto flying_manager::register_flying_data(const char *filename) -> bool {
        std::string s;
        string_path(filename, s);
        uint32_t ret_crc = get_case_crc32(s.c_str(), s.size());

        if (map_fly_data_.find(ret_crc) != map_fly_data_.end()) {
            return false;
        }

        flying_data *flying_data = flying_data::new_value();
        if (!flying_data->load_script_file(filename)) {
            metin2::log::debug("Cannot load flying data {}", filename);
            flying_data::delete_value(flying_data);
            return false;
        }

        map_fly_data_.insert(std::make_pair(ret_crc, flying_data));
        return true;
    }

    auto flying_manager::register_flying_data(const char *filename, uint32_t &r_ret_crc) -> bool {
        std::string s;
        string_path(filename, s);
        r_ret_crc = get_case_crc32(s.c_str(), s.size());

        if (map_fly_data_.find(r_ret_crc) != map_fly_data_.end()) {
            metin2::log::error("Flying data {} already registered", filename);
            return false;
        }

        flying_data *flying_data = flying_data::new_value();
        if (!flying_data->load_script_file(filename)) {
            metin2::log::error("Cannot load flying data {}", filename);
            flying_data::delete_value(flying_data);
            return false;
        }

        map_fly_data_.insert(std::make_pair(r_ret_crc, flying_data));
        return true;
    }

    auto flying_manager::create_flying_instance_fly_target(const uint32_t id,
        const math::vector3 &start_position,
        const fly_target &cr_fly_target,
        bool can_attack) -> flying_instance * {
        if (map_fly_data_.find(id) == map_fly_data_.end()) {
            return nullptr;
        }

        flying_instance *flying_instance = flying_instance::new_value();
        flying_instance->create(map_fly_data_[id], start_position, cr_fly_target, can_attack);

        lst_fly_inst_.push_back(flying_instance);

        flying_instance->id = id_counter_++;
        return flying_instance;
    }

    void flying_manager::update() {
        auto i = lst_fly_inst_.begin();

        while (i != lst_fly_inst_.end()) {
            flying_instance *fly_inst = *i;
            if (!fly_inst->update()) {
                flying_instance::delete_value(fly_inst);
                i = lst_fly_inst_.erase(i);
            } else {
                ++i;
            }
        }
    }

    void flying_manager::render() {
        std::for_each(lst_fly_inst_.begin(), lst_fly_inst_.end(), std::mem_fn(&flying_instance::render));
    }

    auto flying_manager::register_indexed_fly_data(uint32_t index, uint8_t type, const char *file_name) -> bool {
        uint32_t crc;
        if (!register_flying_data(file_name, crc)) {
            metin2::log::error("Cannot register flying data {} from {}: file cannot be loaded", index, file_name);
            return false;
        }

        index_fly_data index_fly_data_value;
        index_fly_data_value.type_value = type;
        index_fly_data_value.crc = crc;
        map_index_fly_data_.insert(std::make_pair(index, index_fly_data_value));

        return true;
    }

    void flying_manager::create_indexed_fly(uint32_t index, actor_instance *start_actor, actor_instance *end_actor) {
        if (map_index_fly_data_.end() == map_index_fly_data_.find(index)) {
            metin2::log::error("Flying data index {} not registered", index);
            return;
        }

        pixel_position pos_start;
        start_actor->get_pixel_position(&pos_start);

        index_fly_data &index_fly_data_value = map_index_fly_data_[index];
        switch (index_fly_data_value.type_value) {
        case INDEX_FLY_TYPE_NORMAL:
            {
                create_flying_instance_fly_target(index_fly_data_value.crc,
                    math::vector3(pos_start.x, pos_start.y, pos_start.z),
                    end_actor,
                    false);
                break;
            }
        case INDEX_FLY_TYPE_FIRE_CRACKER:
            {
                float rot = fmod(start_actor->rotation() - 90.0f + 360.0f, 360.0f) + metin2::random::fnumber(-30.0f, 30.0f);

                float distance = metin2::random::fnumber(2000.0f, 5000.0f);
                float fx_rand = distance * cosf(math::to_radian(rot));
                float fy_rand = distance * sinf(math::to_radian(rot));
                float fz_rand = metin2::random::fnumber(1000.0f, 2500.0f);

                create_flying_instance_fly_target(index_fly_data_value.crc,
                    math::vector3(pos_start.x, pos_start.y, pos_start.z + 200),
                    math::vector3(pos_start.x + fx_rand, pos_start.y + fy_rand, pos_start.z + fz_rand),
                    false);
                break;
            }
        case INDEX_FLY_TYPE_AUTO_FIRE:
            {
                create_flying_instance_fly_target(index_fly_data_value.crc,
                    math::vector3(pos_start.x, pos_start.y, pos_start.z + 100.0f),
                    end_actor,
                    false);
                break;
            }
        }
    }
}
