#include <metin2/graphics/resource_manager.h>
#include <metin2/log/log.h>

#include "item_data.h"

namespace metin2::inline world::inline items {

    dynamic_pool<item_data> item_data::pool_value;

    auto item_data::new_value() -> item_data * {
        return pool_value.alloc();
    }

    void item_data::delete_value(item_data *item_data) {
        item_data->clear();
        pool_value.free(item_data);
    }

    void item_data::destroy_system() {
        pool_value.destroy();
    }

    auto item_data::model_thing() -> graphic_thing * {
        return model_thing_;
    }

    auto item_data::sub_model_thing() -> graphic_thing * {
        if (sub_model_thing_)
            return sub_model_thing_;
        else
            return model_thing_;
    }

    auto item_data::drop_model_thing() -> graphic_thing * {
        return drop_model_thing_;
    }

    auto item_data::icon_image() -> graphic_sub_image * {
        if (icon_image_ == nullptr && icon_file_name_.empty() == false)
            set_icon_image(icon_file_name_.c_str());
        return icon_image_;
    }

    auto item_data::lod_model_thing_count() -> uint32_t {
        return lod_model_thing_vector_.size();
    }

    auto item_data::lod_model_thing_pointer(uint32_t index, graphic_thing **model_thing) -> bool {
        if (index >= lod_model_thing_vector_.size())
            return false;

        *model_thing = lod_model_thing_vector_[index];

        return true;
    }

    auto item_data::attaching_data_count() -> uint32_t {
        return attaching_data_vector_value_.size();
    }

    auto item_data::collision_data_pointer(uint32_t index, const race_types::attaching_data **attaching_data) -> bool {
        if (index >= attaching_data_count())
            return false;

        if (race_types::ATTACHING_DATA_TYPE_COLLISION_DATA != attaching_data_vector_value_[index].type_value)
            return false;

        *attaching_data = &attaching_data_vector_value_[index];
        return true;
    }

    auto item_data::attaching_data_pointer(uint32_t index, const race_types::attaching_data **attaching_data) -> bool {
        if (index >= attaching_data_count())
            return false;

        *attaching_data = &attaching_data_vector_value_[index];
        return true;
    }

    void item_data::set_summary(const std::string &summ) {
        summary_ = summ;
    }

    void item_data::set_description(const std::string &desc) {
        description_ = desc;
    }
    void item_data::set_default_item_data(const char *icon_file_name, const char *model_file_name) {
        if (model_file_name) {
            model_file_name_ = model_file_name;
            drop_model_file_name_ = model_file_name;
        } else {
            model_file_name_ = "";
            drop_model_file_name_ = "item:etc/item_bag.gr2";
        }
        icon_file_name_ = icon_file_name;

        sub_model_file_name_ = "";
        description_ = "";
        summary_ = "";
        memset(item_table_value_.sockets, 0, sizeof(item_table_value_.sockets));

        load_files();
    }

    void item_data::load_files() {
        if (!model_file_name_.empty())
            model_thing_ = (graphic_thing *)get_resource_manager().resource_pointer(model_file_name_.c_str());

        if (!sub_model_file_name_.empty())
            sub_model_thing_ = (graphic_thing *)get_resource_manager().resource_pointer(sub_model_file_name_.c_str());

        if (!drop_model_file_name_.empty())
            drop_model_thing_ = (graphic_thing *)get_resource_manager().resource_pointer(drop_model_file_name_.c_str());

        if (!lod_model_file_name_vector_.empty()) {
            lod_model_thing_vector_.clear();
            lod_model_thing_vector_.resize(lod_model_file_name_vector_.size());

            for (uint32_t i = 0; i < lod_model_file_name_vector_.size(); ++i) {
                const std::string &lod_model_file_name = lod_model_file_name_vector_[i];
                lod_model_thing_vector_[i] = (graphic_thing *)get_resource_manager().resource_pointer(lod_model_file_name.c_str());
            }
        }
    }

    void item_data::set_icon_image(const char *file_name) {
        if (!get_resource_manager().is_file_exist(file_name)) {
            metin2::log::error("Item icon {} does not exist", file_name);
            icon_image_ = nullptr;
        } else if (icon_image_ == nullptr)
            icon_image_ = (graphic_sub_image *)get_resource_manager().resource_pointer(file_name);
    }

    void item_data::set_item_table_data(item_table *item_table_value) {
        memcpy(&item_table_value_, item_table_value, sizeof(item_table));
    }

    auto item_data::table() const -> const item_data::item_table * {
        return &item_table_value_;
    }

    auto item_data::get_index() const -> uint32_t {
        return item_table_value_.vnum;
    }

    auto item_data::get_name() const -> const char * {
        return item_table_value_.locale_name;
    }

    auto item_data::description() const -> const char * {
        return description_.c_str();
    }

    auto item_data::summary() const -> const char * {
        return summary_.c_str();
    }

    auto item_data::get_type() const -> uint8_t {
        return item_table_value_.type_value;
    }

    auto item_data::sub_type() const -> uint8_t {
        return item_table_value_.sub_type;
    }

#define DEF_STR(x) #x

    auto item_data::use_type_string() const -> const char * {
        if (get_type() != item_data::ITEM_TYPE_USE)
            return "NOT_USE_TYPE";

        switch (sub_type()) {
        case USE_TUNING:
            return DEF_STR(USE_TUNING);
        case USE_DETACHMENT:
            return DEF_STR(USE_DETACHMENT);
        case USE_CLEAN_SOCKET:
            return DEF_STR(USE_CLEAN_SOCKET);
        case USE_CHANGE_ATTRIBUTE:
            return DEF_STR(USE_CHANGE_ATTRIBUTE);
        case USE_ADD_ATTRIBUTE:
            return DEF_STR(USE_ADD_ATTRIBUTE);
        case USE_ADD_ATTRIBUTE2:
            return DEF_STR(USE_ADD_ATTRIBUTE2);
        case USE_ADD_ACCESSORY_SOCKET:
            return DEF_STR(USE_ADD_ACCESSORY_SOCKET);
        case USE_PUT_INTO_ACCESSORY_SOCKET:
            return DEF_STR(USE_PUT_INTO_ACCESSORY_SOCKET);
        case USE_PUT_INTO_BELT_SOCKET:
            return DEF_STR(USE_PUT_INTO_BELT_SOCKET);
        case USE_PUT_INTO_RING_SOCKET:
            return DEF_STR(USE_PUT_INTO_RING_SOCKET);
        }
        return "USE_UNKNOWN_TYPE";
    }

    auto item_data::weapon_type() const -> uint32_t {
        return item_table_value_.sub_type;
    }

    auto item_data::size() const -> uint8_t {
        return item_table_value_.size;
    }

    auto item_data::is_anti_flag(uint32_t flag) const -> bool {
        return (flag & item_table_value_.anti_flags) != 0;
    }

    auto item_data::is_flag(uint32_t flag) const -> bool {
        return (flag & item_table_value_.flags_value) != 0;
    }

    auto item_data::is_wearable_flag(uint32_t flag) const -> bool {
        return (flag & item_table_value_.wear_flags) != 0;
    }

    auto item_data::has_next_grade() const -> bool {
        return 0 != item_table_value_.refined_vnum;
    }

    auto item_data::wear_flags() const -> uint32_t {
        return item_table_value_.wear_flags;
    }

    auto item_data::i_sell_item_price() const -> uint32_t {
        return item_table_value_.i_sell_item_price;
    }

    auto item_data::limit(uint8_t index, item_limit *item_limit_value) const -> bool {
        if (index >= ITEM_LIMIT_MAX_NUM) {
            assert(index < ITEM_LIMIT_MAX_NUM);
            return false;
        }

        *item_limit_value = item_table_value_.limits[index];

        return true;
    }

    auto item_data::apply(uint8_t index, item_apply *item_apply_value) const -> bool {
        if (index >= ITEM_APPLY_MAX_NUM) {
            assert(index < ITEM_APPLY_MAX_NUM);
            return false;
        }

        *item_apply_value = item_table_value_.applies[index];
        return true;
    }

    auto item_data::value(uint8_t index) const -> int32_t {
        if (index >= ITEM_VALUES_MAX_NUM) {
            assert(index < ITEM_VALUES_MAX_NUM);
            return 0;
        }

        return item_table_value_.values[index];
    }

    // Must stay in sync with the server's version. A socket count of 1, 2 or 3 means a beginner, intermediate
    // or advanced weapon.
    auto item_data::socket_count() const -> int {
        return item_table_value_.gain_socket_pct;
    }

    auto item_data::specular_poweru() const -> uint32_t {
        return item_table_value_.specular_value;
    }

    auto item_data::specular_powerf() const -> float {
        uint32_t specular_power = specular_poweru();

        return float(specular_power) / 100.0f;
    }

    // The refine level is the last digit of the item vnum.
    auto item_data::refine() const -> uint32_t {
        return get_index() % 10;
    }

    auto item_data::is_equipment() const -> bool {
        switch (get_type()) {
        case ITEM_TYPE_WEAPON:
        case ITEM_TYPE_ARMOR:
            return true;
            break;
        }

        return false;
    }

    void item_data::clear() {
        summary_ = "";
        model_file_name_ = "";
        sub_model_file_name_ = "";
        drop_model_file_name_ = "";
        icon_file_name_ = "";
        lod_model_file_name_vector_.clear();

        model_thing_ = nullptr;
        sub_model_thing_ = nullptr;
        drop_model_thing_ = nullptr;
        icon_image_ = nullptr;
        lod_model_thing_vector_.clear();

        memset(&item_table_value_, 0, sizeof(item_table_value_));
    }

    item_data::item_data() {
        clear();
    }

    item_data::~item_data() = default;
}
