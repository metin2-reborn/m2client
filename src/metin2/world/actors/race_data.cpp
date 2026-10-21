#include "race_data.h"
#include <metin2/core/file_name.h>
#include <metin2/core/utils.h>
#include <metin2/graphics/attribute_instance.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/log/log.h>
#include "race_motion_data.h"

namespace metin2::inline world::inline actors {

    dynamic_pool<race_data> race_data::pool_value_;
    dynamic_pool<race_data::motion_mode_data> race_data::motion_mode_data_pool_;

    auto race_data::smoke_bone() -> const std::string & {
        return smoke_bone_name_;
    }

    auto race_data::smoke_effect_id(uint32_t smoke) -> uint32_t {
        if (smoke >= SMOKE_NUM) {
            metin2::log::error("Smoke {} out of range", smoke);
            return 0;
        }

        return smoke_effect_id_[smoke];
    }

    auto race_data::find_hair(uint32_t hair_value) -> race_data::hair * {
        auto f = map_hair_key_hair_.find(hair_value);
        if (map_hair_key_hair_.end() == f) {
            if (hair_value != 0)
                metin2::log::error("Hair {} does not exist", hair_value);
            return nullptr;
        }

        return &f->second;
    }

    void race_data::set_hair_skin(uint32_t hair_value, uint32_t part, const char *model_file_name, const char *src_file_name, const char *dst_file_name) {
        skin skin_value;
        skin_value.part_value = part;
        skin_value.src_file_name = src_file_name;
        skin_value.dst_file_name = dst_file_name;

        file_name_helper::change_dos_path(skin_value.src_file_name);
        map_hair_key_hair_[hair_value].vct_skin.push_back(skin_value);
        map_hair_key_hair_[hair_value].model_file_name = model_file_name;
    }

    auto race_data::find_shape(uint32_t shape_value) -> race_data::shape * {
        auto f = map_shape_key_shape_.find(shape_value);
        if (map_shape_key_shape_.end() == f)
            return nullptr;

        return &f->second;
    }

    void race_data::set_shape_model(uint32_t shape_value, const char *model_file_name) {
        map_shape_key_shape_[shape_value].model_file_name = model_file_name;
    }

    void race_data::append_shape_skin(uint32_t shape_value, uint32_t part, const char *src_file_name, const char *dst_file_name) {
        skin skin_value;
        skin_value.part_value = part;
        skin_value.src_file_name = src_file_name;
        skin_value.dst_file_name = dst_file_name;

        file_name_helper::change_dos_path(skin_value.src_file_name);
        map_shape_key_shape_[shape_value].vct_skin.push_back(skin_value);
    }

    auto race_data::new_value() -> race_data * {
        return pool_value_.alloc();
    }

    void race_data::delete_value(race_data *race_data) {
        race_data->destroy();
        pool_value_.free(race_data);
    }

    void race_data::create_system(uint32_t capacity, uint32_t mot_mode_capacity) {
        motion_mode_data_pool_.create(mot_mode_capacity);
        pool_value_.create(capacity);
    }

    void race_data::destroy_system() {
        pool_value_.destroy();
        motion_mode_data_pool_.destroy();
    }

    auto race_data::create_motion_mode_iterator(motion_mode_data_iterator &itor) -> bool {
        if (motion_mode_data_map_value_.empty())
            return false;

        itor = motion_mode_data_map_value_.begin();

        return true;
    }
    auto race_data::next_motion_mode_iterator(motion_mode_data_iterator &itor) -> bool {
        ++itor;

        return motion_mode_data_map_value_.end() != itor;
    }

    auto race_data::motion_key(uint16_t motion_mode_index, uint16_t motion_index, MOTION_KEY *motion_key) -> bool {
        motion_mode_data *motion_mode_data_value;
        if (!motion_mode_data_pointer(motion_mode_index, &motion_mode_data_value))
            return false;

        if (motion_mode_data_value->motion_vector_map_value.end() == motion_mode_data_value->motion_vector_map_value.find(motion_index)) {
            uint16_t general_mode = race_motion_data::MODE_GENERAL;

            switch (motion_mode_index) {
            case race_motion_data::MODE_HORSE_ONEHAND_SWORD:
            case race_motion_data::MODE_HORSE_TWOHAND_SWORD:
            case race_motion_data::MODE_HORSE_DUALHAND_SWORD:
            case race_motion_data::MODE_HORSE_FAN:
            case race_motion_data::MODE_HORSE_BELL:
            case race_motion_data::MODE_HORSE_BOW:
                general_mode = race_motion_data::MODE_HORSE;
                break;

            default:
                general_mode = race_motion_data::MODE_GENERAL;
                break;
            }

            motion_mode_data *motion_mode_general_data;
            if (!motion_mode_data_pointer(general_mode, &motion_mode_general_data))
                return false;

            if (motion_mode_general_data->motion_vector_map_value.end() == motion_mode_general_data->motion_vector_map_value.find(motion_index))
                return false;

            *motion_key = MAKE_MOTION_KEY(general_mode, motion_index);
        } else {
            *motion_key = MAKE_MOTION_KEY(motion_mode_index, motion_index);
        }

        return true;
    }

    auto race_data::motion_mode_data_pointer(uint16_t motion_mode, motion_mode_data **motion_mode_data_value) -> bool {
        auto itor = motion_mode_data_map_value_.find(motion_mode);
        if (itor == motion_mode_data_map_value_.end())
            return false;

        *motion_mode_data_value = itor->second;

        return true;
    }

    auto race_data::motion_vector_pointer(uint16_t motion_mode, uint16_t motion_index, motion_vector **motion_vector_value) -> bool {
        motion_mode_data *motion_mode_data_value;
        if (!motion_mode_data_pointer(motion_mode, &motion_mode_data_value))
            return false;

        auto itor = motion_mode_data_value->motion_vector_map_value.find(motion_index);
        if (motion_mode_data_value->motion_vector_map_value.end() == itor)
            return false;

        *motion_vector_value = &itor->second;

        return true;
    }

    auto race_data::motion_data_pointer(uint16_t motion_mode, uint16_t motion_index, uint16_t motion_sub_index, race_motion_data **motion_data) -> bool {
        const motion_vector *motion_vector_value;
        if (!motion_vector_pointer(motion_mode, motion_index, &motion_vector_value))
            return false;

        if (motion_sub_index >= motion_vector_value->size())
            return false;

        const motion &motion_value = motion_vector_value->at(motion_sub_index);

        if (!motion_value.motion_data)
            return false;

        *motion_data = motion_value.motion_data;

        return true;
    }

    auto race_data::motion_data_pointer(uint32_t motion_key, race_motion_data **motion_data) -> bool {
        return motion_data_pointer(GET_MOTION_MODE(motion_key), GET_MOTION_INDEX(motion_key), GET_MOTION_SUB_INDEX(motion_key), motion_data);
    }

    auto race_data::motion_vector_pointer(uint16_t motion_mode, uint16_t motion_index, const motion_vector **c_pp_motion_vector) -> bool {
        motion_vector *p_motion_vector;
        if (!motion_vector_pointer(motion_mode, motion_index, &p_motion_vector))
            return false;

        *c_pp_motion_vector = p_motion_vector;

        return true;
    }

    auto race_data::attaching_data_count() -> uint32_t {
        return attaching_data_vector_value_.size();
    }

    auto race_data::attaching_data_pointer(uint32_t index, const race_types::attaching_data **attaching_data) -> bool {
        if (index >= attaching_data_count())
            return false;

        *attaching_data = &attaching_data_vector_value_[index];

        return true;
    }

    auto race_data::collision_data_pointer(uint32_t index, const race_types::attaching_data **attaching_data) -> bool {
        if (index >= attaching_data_count())
            return false;

        if (race_types::ATTACHING_DATA_TYPE_COLLISION_DATA != attaching_data_vector_value_[index].type_value)
            return false;

        *attaching_data = &attaching_data_vector_value_[index];

        return true;
    }

    void race_data::set_race(uint32_t race_index) {
        race_index_ = race_index;
    }

    void race_data::register_attaching_bone_name(uint32_t part_index, const char *bone_name) {
        attaching_bone_name_map_value_.insert(attaching_bone_name_map::value_type(part_index, bone_name));
    }

    void race_data::register_motion_mode(uint16_t motion_mode_index) {
        motion_mode_data *motion_mode_data_value = motion_mode_data_pool_.alloc();
        motion_mode_data_value->motion_mode_index = motion_mode_index;
        motion_mode_data_value->motion_vector_map_value.clear();
        motion_mode_data_map_value_.insert(motion_mode_data_map::value_type(motion_mode_index, motion_mode_data_value));
    }

    auto race_data::new_register_motion(race_motion_data *motion_data, uint16_t motion_mode_index, uint16_t motion_index, const char *file_name, uint8_t percentage) -> graphic_thing * {
        auto *motion_thing = (graphic_thing *)get_resource_manager().resource_pointer(file_name);

        motion_mode_data *motion_mode_data_value;
        if (!motion_mode_data_pointer(motion_mode_index, &motion_mode_data_value)) {
            AssertLog("Failed getting motion mode data!");
            return nullptr;
        }

        motion motion_value;
        motion_value.percentage = percentage;
        motion_value.motion_value = motion_thing;
        motion_value.motion_data = motion_data;
        motion_mode_data_value->motion_vector_map_value[motion_index].push_back(motion_value);

        return motion_thing;
    }

    auto race_data::register_motion_data(uint16_t motion_mode, uint16_t motion_index, const char *file_name, uint8_t percentage) -> graphic_thing * {
        race_motion_data *race_motion_data = race_motion_data::new_value();
        if (!race_motion_data->load_motion_data(file_name)) {
            metin2::log::error("Cannot load motion data {}", file_name);
            race_motion_data::delete_value(race_motion_data);
            race_motion_data = nullptr;
            return nullptr;
        }

        // The motion should really carry its index.
        race_motion_data->set_name(motion_index);

        // FIXME: if the GR2 is already loaded, the same one is added a second time.

        return new_register_motion(race_motion_data, motion_mode, motion_index, race_motion_data->motion_file_name(), percentage);
    }

    auto race_data::set_motion_random_weight(uint16_t motion_mode_index, uint16_t motion_index, uint16_t motion_sub_index, uint8_t percentage) -> bool {
        motion_mode_data *motion_mode_data_value;

        if (!motion_mode_data_pointer(motion_mode_index, &motion_mode_data_value)) {
            return false;
        }

        auto itor = motion_mode_data_value->motion_vector_map_value.find(motion_index);

        if (motion_mode_data_value->motion_vector_map_value.end() != itor) {
            motion_vector &motion_vector_value = itor->second;
            if (motion_sub_index < motion_vector_value.size()) {
                motion_vector_value[motion_sub_index].percentage = percentage;
            } else {
                return false;
            }
        } else {
            metin2::log::error("Cannot set random weight (mode {}, motion {}, sub index {}, percentage {}): motion not found", motion_mode_index, motion_index, motion_sub_index, percentage);

            return false;
        }

        return true;
    }

    void race_data::register_normal_attack(uint16_t motion_mode_index, uint16_t motion_index) {
        normal_attack_index_map_value_.insert(normal_attack_index_map::value_type(motion_mode_index, motion_index));
    }
    auto race_data::normal_attack_index(uint16_t motion_mode_index, uint16_t *motion_index) -> bool {
        auto itor = normal_attack_index_map_value_.find(motion_mode_index);

        if (normal_attack_index_map_value_.end() == itor)
            return false;

        *motion_index = itor->second;

        return true;
    }

    void race_data::reserve_combo_attack(uint16_t motion_mode_index, uint16_t combo_type, uint32_t combo_count) {
        combo_data combo_data_value;
        combo_data_value.combo_index_vector_value.clear();
        combo_data_value.combo_index_vector_value.resize(combo_count);
        combo_attack_data_map_value_.insert(combo_attack_data_map::value_type(MAKE_COMBO_KEY(motion_mode_index, combo_type), combo_data_value));
    }

    void race_data::register_combo_attack(uint16_t motion_mode_index, uint16_t combo_type, uint32_t combo_index, uint16_t motion_index) {
        auto itor = combo_attack_data_map_value_.find(MAKE_COMBO_KEY(motion_mode_index, combo_type));
        if (combo_attack_data_map_value_.end() == itor)
            return;

        combo_index_vector &combo_index_vector_value = itor->second.combo_index_vector_value;
        if (combo_index >= combo_index_vector_value.size()) {
            AssertLog("CRaceData::RegisterCombo - Strange combo index!");
            return;
        }

        combo_index_vector_value[combo_index] = motion_index;
    }

    auto race_data::combo_data_pointer(uint16_t motion_mode_index, uint16_t combo_type, combo_data **combo_data_value) -> bool {
        auto itor = combo_attack_data_map_value_.find(MAKE_COMBO_KEY(motion_mode_index, combo_type));

        if (combo_attack_data_map_value_.end() == itor)
            return false;

        *combo_data_value = &itor->second;
        return true;
    }

    auto race_data::motion_list_file_name() const -> const char * {
        return motion_list_file_name_.c_str();
    }

    auto race_data::base_model_thing() -> graphic_thing * {
        if (!base_model_thing_) {
            base_model_thing_ = (graphic_thing *)get_resource_manager().resource_pointer(base_model_file_name_.c_str());
        }

        return base_model_thing_;
    }

    auto race_data::lod_model_thing() -> graphic_thing * {
        if (!lod_model_thing_) {
            std::string lod_file_name = file_name_helper::no_extension(base_model_file_name_) + "_lod_01.gr2";
            if (get_resource_manager().is_file_exist(lod_file_name.c_str())) {
                lod_model_thing_ = (graphic_thing *)get_resource_manager().resource_pointer(lod_file_name.c_str());
            }
        }

        return lod_model_thing_;
    }

    auto race_data::attribute_data_ptr() -> attribute_data * {
        if (attribute_file_name_.empty())
            return nullptr;

        if (!get_resource_manager().is_file_exist(attribute_file_name_.c_str()))
            return nullptr;

        return (attribute_data *)get_resource_manager().resource_pointer(attribute_file_name_.c_str());
    }

    auto race_data::attaching_bone_name(uint32_t part_index, const char **c_psz_bone_name) -> bool {
        auto itor = attaching_bone_name_map_value_.find(part_index);
        if (itor == attaching_bone_name_map_value_.end())
            return false;

        const std::string &c_rstr_bone_name = itor->second;

        *c_psz_bone_name = c_rstr_bone_name.c_str();

        return true;
    }

    auto race_data::is_tree() -> bool {
        return !tree_file_name_.empty();
    }

    auto race_data::tree_file_name() -> const char * {
        return tree_file_name_.c_str();
    }

    void race_data::destroy() {
        map_hair_key_hair_.clear();
        map_shape_key_shape_.clear();

        base_model_file_name_ = "";
        tree_file_name_ = "";
        attribute_file_name_ = "";
        motion_list_file_name_ = "motlist.txt";

        attaching_bone_name_map_value_.clear();
        model_data_map_value_.clear();

        normal_attack_index_map_value_.clear();
        combo_attack_data_map_value_.clear();

        auto itor_mode = motion_mode_data_map_value_.begin();
        for (; itor_mode != motion_mode_data_map_value_.end(); ++itor_mode) {
            motion_mode_data *motion_mode_data_value = itor_mode->second;

            auto itor_motion = motion_mode_data_value->motion_vector_map_value.begin();
            for (; itor_motion != motion_mode_data_value->motion_vector_map_value.end(); ++itor_motion) {
                motion_vector &motion_vector_value = itor_motion->second;
                for (uint32_t i = 0; i < motion_vector_value.size(); ++i) {
                    race_motion_data::delete_value(motion_vector_value[i].motion_data);
                }
            }

            motion_mode_data_pool_.free(motion_mode_data_value);
        }
        motion_mode_data_map_value_.clear();

        initialize_impl();
    }

    void race_data::initialize_impl() {
        motion_list_file_name_ = "motlist.txt";

        base_model_thing_ = nullptr;
        lod_model_thing_ = nullptr;

        race_index_ = 0;

        memset(smoke_effect_id_, 0, sizeof(smoke_effect_id_));
    }

    race_data::race_data() {
        initialize_impl();
    }
    race_data::~race_data() {
        destroy();
    }
}
