#include <metin2/log/log.h>
#include <metin2/world/items/item_manager.h>
#include "actor_instance.h"
#include "race_data.h"
#include "race_manager.h"

#include <metin2/core/file_name.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/models/material.h>

namespace metin2::inline world::inline actors {

    auto actor_instance::virtual_id() -> uint32_t {
        return self_vid_;
    }

    void actor_instance::set_virtual_id(uint32_t vid) {
        self_vid_ = vid;
    }

    void actor_instance::update_attribute() {
        if (!attribute_instance_value_)
            return;

        if (!need_update_collision_)
            return;

        need_update_collision_ = false;

        const static_collision_data_vector &vec_colli_data = attribute_instance_value_->object_pointer()->get_collision_data_vector();
        update_collision_data(&vec_colli_data);

        attribute_instance_value_->refresh_object(get_transform());
        update_height_instance(attribute_instance_value_);
    }

    void actor_instance::create_attribute_instance(attribute_data *data) {
        attribute_instance_value_ = attribute_instance::new_value();
        attribute_instance_value_->clear();
        attribute_instance_value_->set_object_pointer(data);
        if (data->is_empty()) {
            attribute_instance_value_->clear();
            attribute_instance::delete_value(attribute_instance_value_);
        }
    }

    auto actor_instance::race() -> uint32_t {
        return race_;
    }

    auto actor_instance::set_race(uint32_t race) -> bool {
        race_data *race_data;
        if (!get_race_manager().race_data_pointer(race, &race_data)) {
            race_ = 0;
            cur_race_data_ = nullptr;
            return false;
        }

        race_ = race;
        cur_race_data_ = race_data;

        attribute_data *attribute_data = race_data->attribute_data_ptr();
        if (attribute_data) {
            create_attribute_instance(attribute_data);
        }

        memset(part_item_id_, 0, sizeof(part_item_id_));

        clear_attaching_effect_impl();

        graphic_thing_instance::clear();

        // Only PCs create every part separately.
        if (is_pc()) {
            graphic_thing_instance::reserve_model_thing(race_data::PART_MAX_NUM);
            graphic_thing_instance::reserve_model_instance(race_data::PART_MAX_NUM);
        } else {
            graphic_thing_instance::reserve_model_thing(1);
            graphic_thing_instance::reserve_model_instance(1);
        }

        race_data::motion_mode_data_iterator itor;

        if (race_data->create_motion_mode_iterator(itor)) {
            do {
                uint16_t motion_mode = itor->first;
                race_data::motion_mode_data *motion_mode_data = itor->second;

                auto itor_motion = motion_mode_data->motion_vector_map_value.begin();
                for (; itor_motion != motion_mode_data->motion_vector_map_value.end(); ++itor_motion) {
                    uint16_t motion_index = itor_motion->first;
                    const race_data::motion_vector &motion_vector = itor_motion->second;
                    race_data::motion_vector::const_iterator it;
                    uint32_t i;
                    for (i = 0, it = motion_vector.begin(); it != motion_vector.end(); ++i, ++it) {
                        uint32_t motion_key = MAKE_RANDOM_MOTION_KEY(motion_mode, motion_index, i);
                        graphic_thing_instance::register_motion_thing(motion_key, it->motion_value);
                    }
                }
            } while (race_data->next_motion_mode_iterator(itor));
        }

        return true;
    }

    void actor_instance::set_hair(uint32_t e_hair) {
        hair_value_ = e_hair;

        race_data *race_data;

        if (!get_race_manager().race_data_pointer(race_, &race_data))
            return;

        race_data::hair *pk_hair = race_data->find_hair(e_hair);
        if (pk_hair) {
            if (!pk_hair->model_file_name.empty()) {
                auto *hair_thing = (graphic_thing *)get_resource_manager().resource_pointer(pk_hair->model_file_name.c_str());
                register_model_thing(race_data::PART_HAIR, hair_thing);
                set_model_instance(race_data::PART_HAIR, race_data::PART_HAIR, 0, race_data::PART_MAIN);
            }

            const std::vector<race_data::skin> &vct_skin = pk_hair->vct_skin;
            std::vector<race_data::skin>::const_iterator i;
            for (i = vct_skin.begin(); i != vct_skin.end(); ++i) {
                const race_data::skin &skin_item = *i;

                resource *res = get_resource_manager().resource_pointer(skin_item.dst_file_name.c_str());

                if (res)
                    set_material_image_pointer(race_data::PART_HAIR, skin_item.src_file_name.c_str(), static_cast<graphic_image *>(res));
            }
        }
    }

    void actor_instance::set_shape(uint32_t e_shape, float specular) {
        shape_value_ = e_shape;

        race_data *race_data;
        if (!get_race_manager().race_data_pointer(race_, &race_data))
            return;

        race_data::shape *pk_shape = race_data->find_shape(e_shape);
        if (pk_shape) {
            resource_manager &res_mgr = get_resource_manager();

            if (pk_shape->model_file_name.empty()) {
                graphic_thing *model_thing = race_data->base_model_thing();
                register_model_thing(0, model_thing);
            } else {
                auto *model_thing = (graphic_thing *)res_mgr.resource_pointer(pk_shape->model_file_name.c_str());
                register_model_thing(0, model_thing);
            }

            {
                std::string lod_model_file_name;

                char lod_model_file_name_end[256];
                for (uint32_t lod_index = 1; lod_index <= 3; ++lod_index) {
                    snprintf(lod_model_file_name_end, sizeof(lod_model_file_name_end), "_lod_%.2d.gr2", lod_index);
                    lod_model_file_name = file_name_helper::no_extension(pk_shape->model_file_name) + lod_model_file_name_end;
                    if (!res_mgr.is_file_exist(lod_model_file_name.c_str()))
                        break;

                    auto *lod_model_thing = (graphic_thing *)res_mgr.resource_pointer(lod_model_file_name.c_str());
                    if (!lod_model_thing)
                        break;

                    register_lod_thing(0, lod_model_thing);
                }
            }

            set_model_instance(0, 0, 0);

            const std::vector<race_data::skin> &vct_skin = pk_shape->vct_skin;
            std::vector<race_data::skin>::const_iterator i;
            for (i = vct_skin.begin(); i != vct_skin.end(); ++i) {
                const race_data::skin &skin_item = *i;

                resource *res = get_resource_manager().resource_pointer(skin_item.dst_file_name.c_str());

                if (res) {
                    if (specular > 0.0f) {
                        material_data material_data;
                        material_data.image = static_cast<graphic_image *>(res);
                        material_data.is_specular_enable = true;
                        material_data.specular_power = specular;
                        material_data.sphere_map_index = 0;
                        set_material_data(skin_item.part_value, skin_item.src_file_name.c_str(), material_data);
                    } else {
                        set_material_image_pointer(skin_item.part_value, skin_item.src_file_name.c_str(), static_cast<graphic_image *>(res));
                    }
                }
            }
        } else {
            if (race_data->is_tree()) {
                create_tree(race_data->tree_file_name());
            } else {
                graphic_thing *model_thing = race_data->base_model_thing();
                register_model_thing(0, model_thing);

                graphic_thing *lod_model_thing = race_data->lod_model_thing();
                register_lod_thing(0, lod_model_thing);

                set_model_instance(0, 0, 0);
            }
        }

        for (uint32_t i = 0; i < race_data->attaching_data_count(); ++i) {
            const race_types::attaching_data *attaching_data;
            if (!race_data->attaching_data_pointer(i, &attaching_data))
                continue;

            switch (attaching_data->type_value) {
            case race_types::ATTACHING_DATA_TYPE_EFFECT:
                if (attaching_data->is_attaching) {
                    attach_effect_by_name(0, attaching_data->attaching_bone_name.c_str(), attaching_data->effect_data_value->file_name.c_str());
                } else {
                    attach_effect_by_name(0, nullptr, attaching_data->effect_data_value->file_name.c_str());
                }
                break;
            }
        }
    }

    void actor_instance::change_material(const char *file_name) {
        race_data *race_data;
        if (!get_race_manager().race_data_pointer(race_, &race_data))
            return;

        race_data::shape *shape = race_data->find_shape(shape_value_);
        if (!shape)
            return;

        const std::vector<race_data::skin> &vct_skin = shape->vct_skin;
        if (vct_skin.empty())
            return;

        auto i = vct_skin.begin();
        const race_data::skin &skin_item = *i;

        std::string dst_file_name = "character:npc/guild_symbol/guild_symbol.png";
        dst_file_name = file_name;

        resource *res = get_resource_manager().resource_pointer(dst_file_name.c_str());
        if (!res)
            return;

        set_material_image_pointer(skin_item.part_value, skin_item.src_file_name.c_str(), static_cast<graphic_image *>(res));
    }

    auto actor_instance::part_item_id(uint32_t part_index) -> uint32_t {
        if (part_index >= race_data::PART_MAX_NUM) {
            metin2::log::error("Part {} out of range (limit {})", part_index, race_data::PART_MAX_NUM);
            return 0;
        }

        return part_item_id_[part_index];
    }

    void actor_instance::set_specular_info(bool enable, int part, float alpha) {
        race_data *race_data;
        if (!get_race_manager().race_data_pointer(race_, &race_data))
            return;

        race_data::shape *shape = race_data->find_shape(shape_value_);
        if (shape->vct_skin.empty())
            return;

        std::string filename = shape->vct_skin[0].src_file_name.c_str();
        file_name_helper::change_dos_path(filename);

        graphic_thing_instance::set_specular_info(part, filename.c_str(), enable, alpha);
    }
}
