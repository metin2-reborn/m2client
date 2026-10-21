#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>
#include <string>
#include "crc32.h"
#include "parse.h"

#include "pool.h"
#include "text_file_loader.h"

namespace metin2::inline core {

    std::map<uint32_t, std::unique_ptr<text_file_loader>> text_file_loader::map_name_key_text_file_loader_;
    bool text_file_loader::is_cache_mode_ = false;

    dynamic_pool<text_file_loader::group_node> text_file_loader::group_node::pool_value;

    auto text_file_loader::group_node::get_token_vector(const std::string &group_name_value) -> token_vector * {
        uint32_t group_name_key_value = gen_name_key(group_name_value.c_str(), group_name_value.length());

        auto f = map_key_vct_token.find(group_name_key_value);
        if (map_key_vct_token.end() == f)
            return nullptr;

        return &f->second;
    }

    void text_file_loader::group_node::insert_token_vector(const std::string &group_name_value, const token_vector &vct_token) {
        uint32_t group_name_key_value = gen_name_key(group_name_value.c_str(), group_name_value.length());

        map_key_vct_token.insert(std::map<uint32_t, token_vector>::value_type(group_name_key_value, vct_token));
    }

    auto text_file_loader::group_node::gen_name_key(const char *group_name_value, uint32_t group_name_len) -> uint32_t {
        return get_crc32(group_name_value, group_name_len);
    }

    auto text_file_loader::group_node::get_group_name() -> const std::string & {
        return group_name;
    }

    auto text_file_loader::group_node::is_group_name_key(uint32_t group_name_key_value) -> bool {
        if (group_name_key_value == group_name_key)
            return true;

        return false;
    }

    void text_file_loader::group_node::set_group_name(const std::string &group_name_value) {
        group_name = group_name_value;
        stl_lowers(group_name);

        group_name_key = gen_name_key(group_name.c_str(), group_name.length());
    }

    auto text_file_loader::group_node::new_value() -> text_file_loader::group_node * {
        return pool_value.alloc();
    }

    void text_file_loader::group_node::delete_value(group_node *node) {
        node->map_key_vct_token.clear();
        node->child_node_vector.clear();
        node->group_name = "";
        node->group_name_key = 0;
        pool_value.free(node);
    }

    void text_file_loader::group_node::destroy_system() {
        pool_value.destroy();
    }

    auto text_file_loader::cache(const char *file_name) -> text_file_loader * {
        uint32_t name_key = get_crc32(file_name, strlen(file_name));
        auto f = map_name_key_text_file_loader_.find(name_key);
        if (map_name_key_text_file_loader_.end() != f) {
            if (!is_cache_mode_) {
                f->second = std::make_unique<text_file_loader>();
                f->second->load(file_name);
            }
            f->second->set_top();
            return f->second.get();
        }

        auto loader = std::make_unique<text_file_loader>();
        loader->load(file_name);
        return map_name_key_text_file_loader_.emplace(name_key, std::move(loader)).first->second.get();
    }

    void text_file_loader::set_cache_mode() {
        is_cache_mode_ = true;
    }

    void text_file_loader::destroy_system() {
        map_name_key_text_file_loader_.clear();

        group_node::destroy_system();
    }

    void text_file_loader::destroy() {
        destroy_group_node_vector();
    }

    text_file_loader::text_file_loader() {
        set_top();

        buf_size_ = 0;

        global_node_.group_name = "global";
        global_node_.parent_node = nullptr;

        vct_node_.reserve(128);
    }

    text_file_loader::~text_file_loader() {
        destroy();
    }

    void text_file_loader::destroy_group_node_vector() {
        std::vector<group_node *>::iterator i;
        for (i = vct_node_.begin(); i != vct_node_.end(); ++i)
            group_node::delete_value(*i);
        vct_node_.clear();
    }

    auto text_file_loader::file_name() -> const char * {
        return file_name_.c_str();
    }

    auto text_file_loader::is_empty() -> bool {
        return file_name_.empty();
    }

    auto text_file_loader::load(const char *file_name) -> bool {
        file_name_ = "";

        const void *data;
        mapped_file file;
        if (!get_file_manager().get(file, file_name, &data))
            return false;

        buf_size_ = file.size();
        buf_data_.assign(static_cast<const char *>(data), static_cast<const char *>(data) + buf_size_);

        file_name_ = file_name;
        dwcur_line_index_ = 0;

        text_file_loader_value_.bind(buf_size_, buf_data_.data());
        return load_group(&global_node_);
    }

    auto text_file_loader::load_group(group_node *group_node_value) -> bool {
        token_vector token_vector_value;
        int local_group_depth = 0;

        for (; dwcur_line_index_ < text_file_loader_value_.line_count(); ++dwcur_line_index_) {
            int ret;

            if ((ret = text_file_loader_value_.split_line2(dwcur_line_index_, &token_vector_value)) != 0) {
                if (ret == -2)
                    metin2::log::error("File {} line {}: missing closing quote", file_name_.c_str(), dwcur_line_index_);
                continue;
            }

            stl_lowers(token_vector_value[0]);

            if ('{' == token_vector_value[0][0]) {
                local_group_depth++;
                continue;
            }

            if ('}' == token_vector_value[0][0]) {
                local_group_depth--;
                break;
            }

            if (0 == token_vector_value[0].compare("group")) {
                if (2 != token_vector_value.size()) {
                    assert(!"There is no group name!");
                    continue;
                }

                group_node *new_node = group_node::new_value();
                vct_node_.push_back(new_node);

                new_node->parent_node = group_node_value;
                new_node->set_group_name(token_vector_value[1]);
                group_node_value->child_node_vector.push_back(new_node);

                ++dwcur_line_index_;

                if (false == load_group(new_node))
                    return false;
            } else if (0 == token_vector_value[0].compare("list")) {
                if (2 != token_vector_value.size()) {
                    assert(!"There is no list name!");
                    continue;
                }

                token_vector sub_token_vector;

                stl_lowers(token_vector_value[1]);
                std::string key = token_vector_value[1];

                token_vector_value.clear();

                ++dwcur_line_index_;
                for (; dwcur_line_index_ < text_file_loader_value_.line_count(); ++dwcur_line_index_) {
                    if (!text_file_loader_value_.split_line(dwcur_line_index_, &sub_token_vector))
                        continue;

                    if ('{' == sub_token_vector[0][0])
                        continue;

                    if ('}' == sub_token_vector[0][0])
                        break;

                    for (uint32_t j = 0; j < sub_token_vector.size(); ++j) {
                        token_vector_value.push_back(sub_token_vector[j]);
                    }
                }

                group_node_value->insert_token_vector(key, token_vector_value);
            } else {
                std::string key = token_vector_value[0];

                if (1 == token_vector_value.size()) {
                    metin2::log::error("File {} line {}: key {} has no value", file_name_.c_str(), dwcur_line_index_, key.c_str());
                    break;
                }

                token_vector_value.erase(token_vector_value.begin());
                group_node_value->insert_token_vector(key, token_vector_value);
            }
        }

        return (local_group_depth == 0);
    }

    void text_file_loader::set_top() {
        pcur_node_ = &global_node_;
    }

    auto text_file_loader::child_node_count() -> uint32_t {
        if (!pcur_node_) {
            assert(!"Node to access has not set!");
            return 0;
        }

        return pcur_node_->child_node_vector.size();
    }

    auto text_file_loader::set_child_node(const char *c_sz_key) -> bool {
        if (!pcur_node_) {
            assert(!"Node to access has not set!");
            return false;
        }

        uint32_t dw_key = group_node::gen_name_key(c_sz_key, strlen(c_sz_key));

        for (uint32_t i = 0; i < pcur_node_->child_node_vector.size(); ++i) {
            group_node *group_node_value = pcur_node_->child_node_vector[i];
            if (group_node_value->is_group_name_key(dw_key)) {
                pcur_node_ = group_node_value;
                return true;
            }
        }

        return false;
    }
    auto text_file_loader::set_child_node(const std::string &key_head, uint32_t index) -> bool {
        char key[32 + 1];
        snprintf(key, sizeof(key), "%s%02u", key_head.c_str(), index);

        return set_child_node(key);
    }
    auto text_file_loader::set_child_node(uint32_t index) -> bool {
        if (!pcur_node_) {
            assert(!"Node to access has not set!");
            return false;
        }

        if (index >= pcur_node_->child_node_vector.size()) {
            assert(!"Node index to set is too large to access!");
            return false;
        }

        pcur_node_ = pcur_node_->child_node_vector[index];

        return true;
    }

    auto text_file_loader::set_parent_node() -> bool {
        if (!pcur_node_) {
            assert(!"Node to access has not set!");
            return false;
        }

        if (nullptr == pcur_node_->parent_node) {
            assert(!"Current group node is already top!");
            return false;
        }

        pcur_node_ = pcur_node_->parent_node;

        return true;
    }

    auto text_file_loader::current_node_name(std::string *pstr_name) -> bool {
        if (!pcur_node_)
            return false;
        if (nullptr == pcur_node_->parent_node)
            return false;

        *pstr_name = pcur_node_->get_group_name();

        return true;
    }

    auto text_file_loader::get_token_vector(const std::string &key, token_vector **token_vector_value) -> bool {
        if (!pcur_node_) {
            assert(!"Node to access has not set!");
            return false;
        }

        token_vector *ret_token_vector = pcur_node_->get_token_vector(key);
        if (!ret_token_vector)
            return false;

        *token_vector_value = ret_token_vector;

        return true;
    }

    auto text_file_loader::token_boolean(const std::string &key, bool *data) -> bool {
        token_vector *token_vector;
        if (!get_token_vector(key, &token_vector))
            return false;

        if (token_vector->empty()) {
            return false;
        }

        *data = bool(metin2::to_number<int>(token_vector->at(0).c_str()));

        return true;
    }

    auto text_file_loader::token_byte(const std::string &key, uint8_t *data) -> bool {
        token_vector *token_vector;
        if (!get_token_vector(key, &token_vector))
            return false;

        if (token_vector->empty()) {
            return false;
        }

        *data = uint8_t(metin2::to_number<int>(token_vector->at(0).c_str()));

        return true;
    }

    auto text_file_loader::token_word(const std::string &key, uint16_t *data) -> bool {
        token_vector *token_vector;
        if (!get_token_vector(key, &token_vector))
            return false;

        if (token_vector->empty()) {
            return false;
        }

        *data = uint16_t(metin2::to_number<int>(token_vector->at(0).c_str()));

        return true;
    }

    auto text_file_loader::token_integer(const std::string &key, int *data) -> bool {
        token_vector *token_vector;
        if (!get_token_vector(key, &token_vector))
            return false;

        if (token_vector->empty()) {
            return false;
        }

        *data = metin2::to_number<int>(token_vector->at(0).c_str());

        return true;
    }

    auto text_file_loader::token_double_word(const std::string &key, uint32_t *data) -> bool {
        return token_integer(key, *(int **)(&data));
    }

    auto text_file_loader::token_float(const std::string &key, float *data) -> bool {
        token_vector *token_vector;
        if (!get_token_vector(key, &token_vector))
            return false;

        if (token_vector->empty()) {
            return false;
        }

        *data = metin2::to_number<double>(token_vector->at(0).c_str());

        return true;
    }

    auto text_file_loader::token_vector2(const std::string &key, math::vector2 *vector2) -> bool {
        token_vector *token_vector;
        if (!get_token_vector(key, &token_vector))
            return false;

        if (token_vector->size() != 2) {
            return false;
        }

        vector2->x = metin2::to_number<double>(token_vector->at(0).c_str());
        vector2->y = metin2::to_number<double>(token_vector->at(1).c_str());

        return true;
    }

    auto text_file_loader::token_vector3(const std::string &key, math::vector3 *vector3) -> bool {
        token_vector *token_vector;
        if (!get_token_vector(key, &token_vector))
            return false;

        if (token_vector->size() != 3) {
            return false;
        }

        vector3->x = metin2::to_number<double>(token_vector->at(0).c_str());
        vector3->y = metin2::to_number<double>(token_vector->at(1).c_str());
        vector3->z = metin2::to_number<double>(token_vector->at(2).c_str());

        return true;
    }

    auto text_file_loader::token_position(const std::string &key, math::vector3 *vector) -> bool {
        return token_vector3(key, vector);
    }

    auto text_file_loader::token_direction(const std::string &key, math::vector3 *vector) -> bool {
        token_vector *token_vector;
        if (!get_token_vector(key, &token_vector))
            return false;

        if (token_vector->size() != 3) {
            return false;
        }

        vector->x = metin2::to_number<double>(token_vector->at(0).c_str());
        vector->y = metin2::to_number<double>(token_vector->at(1).c_str());
        vector->z = metin2::to_number<double>(token_vector->at(2).c_str());

        return true;
    }

    auto text_file_loader::token_color(const std::string &key, math::color *color) -> bool {
        token_vector *token_vector;
        if (!get_token_vector(key, &token_vector))
            return false;

        if (token_vector->size() != 4) {
            return false;
        }

        color->r = metin2::to_number<double>(token_vector->at(0).c_str());
        color->g = metin2::to_number<double>(token_vector->at(1).c_str());
        color->b = metin2::to_number<double>(token_vector->at(2).c_str());
        color->a = metin2::to_number<double>(token_vector->at(3).c_str());

        return true;
    }

    auto text_file_loader::token_string(const std::string &key, std::string *string) -> bool {
        token_vector *token_vector;
        if (!get_token_vector(key, &token_vector))
            return false;

        if (token_vector->empty()) {
            return false;
        }

        *string = token_vector->at(0);

        return true;
    }
}
