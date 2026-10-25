#include <metin2/core/file_loader.h>
#include <metin2/core/lzo.h>
#include <metin2/core/parse.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/graphics/resource_manager.h>
#include <metin2/log/log.h>

#include "item_manager.h"

#include <metin2/core/utils.h>
#include <vector>

namespace metin2::inline world::inline actors {
    extern bool use_vietnam_convert_weapon_vnum;
}
namespace metin2::inline world::inline actors {
    extern auto vietnam_convert_weapon_vnum(uint32_t vnum) -> uint32_t;
}
namespace metin2::inline world::inline items {

    static uint32_t item_proto_key[4] =
        {
            173217,
            72619434,
            408587239,
            27973291};

    auto item_manager::select_item_data(uint32_t index) -> bool {
        auto f = item_map_value_.find(index);

        if (item_map_value_.end() == f) {
            int n = vec_item_range_.size();
            for (int i = 0; i < n; i++) {
                item_data *p = vec_item_range_[i];
                const item_data::item_table *table = p->table();
                if ((table->vnum < index) &&
                    index < (table->vnum + table->vnum_range)) {
                    selected_item_data_ = p;
                    return true;
                }
            }
            metin2::log::debug("Item {} not found", index);
            return false;
        }

        selected_item_data_ = f->second;

        return true;
    }

    auto item_manager::selected_item_data_pointer() -> item_data * {
        return selected_item_data_;
    }

    auto item_manager::item_data_pointer(uint32_t item_id, item_data **item_data_value) -> bool {
        if (0 == item_id)
            return false;

        auto f = item_map_value_.find(item_id);

        if (item_map_value_.end() == f) {
            int n = vec_item_range_.size();
            for (int i = 0; i < n; i++) {
                item_data *p = vec_item_range_[i];
                const item_data::item_table *table_value = p->table();
                if ((table_value->vnum < item_id) &&
                    item_id < (table_value->vnum + table_value->vnum_range)) {
                    *item_data_value = p;
                    return true;
                }
            }
            metin2::log::debug("Item {} not found", item_id);
            return false;
        }

        *item_data_value = f->second;

        return true;
    }

    auto item_manager::make_item_data(uint32_t index) -> item_data * {
        auto f = item_map_value_.find(index);

        if (item_map_value_.end() == f) {
            item_data *item_data = item_data::new_value();

            item_map_value_.insert(item_map::value_type(index, item_data));

            return item_data;
        }

        return f->second;
    }

    auto item_manager::load_item_list(const char *file_name) -> bool {
        mapped_file file;
        const void *data;

        if (!get_file_manager().get(file, file_name, &data))
            return false;

        memory_text_file_loader text_file_loader;
        text_file_loader.bind(file.size(), data);

        token_vector token_vector;
        for (uint32_t i = 0; i < text_file_loader.line_count(); ++i) {
            if (!text_file_loader.split_line(i, &token_vector, "\t"))
                continue;

            if (!(token_vector.size() == 3 || token_vector.size() == 4)) {
                metin2::log::warn("Item list {}: line {} is malformed and skipped", file_name, i);
                continue;
            }

            const std::string &id = token_vector[0];
            const std::string &icon = token_vector[2];

            uint32_t item_v_num = metin2::to_number<int>(id.c_str());

            item_data *item_data = make_item_data(item_v_num);

            if (use_vietnam_convert_weapon_vnum) {
                uint32_t mild_item_vnum = vietnam_convert_weapon_vnum(item_v_num);
                if (mild_item_vnum == item_v_num) {
                    if (4 == token_vector.size()) {
                        const std::string &model_file_name = token_vector[3];
                        item_data->set_default_item_data(icon.c_str(), model_file_name.c_str());
                    } else {
                        item_data->set_default_item_data(icon.c_str());
                    }
                } else {
                    uint32_t mild_base_vnum = mild_item_vnum / 10 * 10;
                    char mild_icon_path[256];
                    snprintf(mild_icon_path, sizeof(mild_icon_path), "textures:icon/item/%.5d.png", mild_base_vnum);
                    if (4 == token_vector.size()) {
                        char mild_model_path[256];
                        snprintf(mild_model_path, sizeof(mild_model_path), "item:weapon/%.5d.gr2", mild_base_vnum);
                        item_data->set_default_item_data(mild_icon_path, mild_model_path);
                    } else {
                        item_data->set_default_item_data(mild_icon_path);
                    }
                }
            } else {
                if (4 == token_vector.size()) {
                    const std::string &model_file_name = token_vector[3];
                    item_data->set_default_item_data(icon.c_str(), model_file_name.c_str());
                } else {
                    item_data->set_default_item_data(icon.c_str());
                }
            }
        }

        return true;
    }

    auto snap_string(const std::string &src, std::string &temp) -> const std::string & {
        uint32_t src_len = src.length();
        if (src_len < 2)
            return src;

        if (src[0] != '"')
            return src;

        uint32_t left_cut = 1;

        uint32_t right_cut = src_len;
        if (src[src_len - 1] == '"')
            right_cut = src_len - 1;

        temp = src.substr(left_cut, right_cut - left_cut);
        return temp;
    }

    auto item_manager::load_item_desc(const char *file_name) -> bool {
        const void *data;
        mapped_file file;
        if (!get_file_manager().get(file, file_name, &data)) {
            metin2::log::debug("Cannot load item descriptions {}", file_name);
            return false;
        }

        memory_text_file_loader text_file_loader;
        text_file_loader.bind(file.size(), data);

        std::string temp;

        token_vector token_vector;
        for (uint32_t i = 0; i < text_file_loader.line_count(); ++i) {
            if (!text_file_loader.split_line_by_tab(i, &token_vector))
                continue;

            while (token_vector.size() < ITEMDESC_COL_NUM)
                token_vector.push_back("");

            uint32_t vnum = metin2::to_number<int>(token_vector[ITEMDESC_COL_VNUM].c_str());
            const std::string &desc = token_vector[ITEMDESC_COL_DESC];
            const std::string &summ = token_vector[ITEMDESC_COL_SUMM];
            auto f = item_map_value_.find(vnum);
            if (item_map_value_.end() == f)
                continue;

            item_data *item_data_find = f->second;

            item_data_find->set_description(snap_string(desc, temp));
            item_data_find->set_summary(snap_string(summ, temp));
        }
        return true;
    }

    auto get_hash_code(const char *string) -> uint32_t {
        size_t i, len;
        uint32_t ch;
        uint32_t result;

        len = strlen(string);
        result = 5381;
        for (i = 0; i < len; i++) {
            ch = (uint32_t)string[i];
            result = ((result << 5) + result) + ch; // hash * 33 + ch
        }

        return result;
    }

    auto item_manager::load_item_table(const char *file_name) -> bool {
        mapped_file file;
        const void *pv_data;

        if (!get_file_manager().get(file, file_name, &pv_data))
            return false;

        uint32_t four_cc, elements, data_size;
        uint32_t version = 0;
        uint32_t stride = 0;

        // format v2: UTF-8 names in 64-byte fields (convert older files with proto_convert)
        file.read(&four_cc, sizeof(uint32_t));
        if (four_cc != MAKEFOURCC('M', 'I', 'P', 'X')) {
            metin2::log::error("Item proto {}: invalid file type", file_name);
            return false;
        }

        file.read(&version, sizeof(uint32_t));
        file.read(&stride, sizeof(uint32_t));

        if (version != 2) {
            metin2::log::error("Item proto {} has version {}, expected 2 (convert it with proto_convert)", file_name, version);
            return false;
        }
        if (stride != sizeof(item_data::item_table)) {
            metin2::log::error("Item proto {}: record size {} does not match the item table size {}", file_name, stride, sizeof(item_data::item_table));
            return false;
        }

        file.read(&elements, sizeof(uint32_t));
        file.read(&data_size, sizeof(uint32_t));

        std::vector<uint8_t> data(data_size);
        file.read(data.data(), data_size);

        lz_object z_obj;
        if (!get_clzo().decompress(z_obj, data.data(), item_proto_key))
            return false;

        if (elements > z_obj.size() / sizeof(item_data::item_table)) {
            metin2::log::error("Item proto: {} bytes do not hold {} item tables", z_obj.size(), elements);
            return false;
        }

        char name[64 + 1];

        auto *table = (item_data::item_table *)z_obj.buffer();
        std::map<uint32_t, uint32_t> item_name_map_value;

        for (uint32_t i = 0; i < elements; ++i, ++table) {
            item_data *item_data;
            uint32_t vnum = table->vnum;

            auto f = item_map_value_.find(vnum);
            if (item_map_value_.end() == f) {
                snprintf(name, sizeof(name), "textures:icon/item/%05d.png", vnum);

                if (get_resource_manager().is_file_exist(name) == false) {
                    auto it_vnum = item_name_map_value.find(get_hash_code(table->name_value));

                    if (it_vnum != item_name_map_value.end())
                        snprintf(name, sizeof(name), "textures:icon/item/%05d.png", it_vnum->second);
                    else
                        snprintf(name, sizeof(name), "textures:icon/item/%05d.png", vnum - vnum % 10);

                    if (get_resource_manager().is_file_exist(name) == false) {
                        const uint32_t empty_bowl = 27995;
                        snprintf(name, sizeof(name), "textures:icon/item/%05d.png", empty_bowl);
                    }
                }

                item_data = item_data::new_value();

                item_data->set_default_item_data(name);
                item_map_value_.insert(item_map::value_type(vnum, item_data));
            } else {
                item_data = f->second;
            }
            if (item_name_map_value.find(get_hash_code(table->name_value)) == item_name_map_value.end())
                item_name_map_value.insert(std::map<uint32_t, uint32_t>::value_type(get_hash_code(table->name_value), table->vnum));
            item_data->set_item_table_data(table);
            if (0 != table->vnum_range) {
                vec_item_range_.push_back(item_data);
            }
        }

        return true;
    }

    void item_manager::destroy() {
        item_map::iterator i;
        for (i = item_map_value_.begin(); i != item_map_value_.end(); ++i)
            item_data::delete_value(i->second);

        item_map_value_.clear();
    }

    item_manager::item_manager() : selected_item_data_(nullptr) {
    }
    item_manager::~item_manager() {
        destroy();
    }
}
