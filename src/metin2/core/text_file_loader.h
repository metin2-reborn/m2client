#pragma once

#include "file_loader.h"
#include "mapped_file.h"
#include "pool.h"
#include "util.h"

#include <memory>
#include <vector>

namespace metin2::inline core {

    class text_file_loader {
      public:
        struct group_node {
            static auto gen_name_key(const char *group_name_value, uint32_t group_name_len) -> uint32_t;

            void set_group_name(const std::string &group_name_value);
            auto is_group_name_key(uint32_t group_name_key_value) -> bool;

            auto get_group_name() -> const std::string &;

            auto get_token_vector(const std::string &group_name_value) -> token_vector *;
            void insert_token_vector(const std::string &group_name_value, const token_vector &vct_token);

            uint32_t group_name_key;
            std::string group_name;

            std::map<uint32_t, token_vector> map_key_vct_token;

            group_node *parent_node;
            std::vector<group_node *> child_node_vector;

            static auto new_value() -> group_node *;
            static void delete_value(group_node *node);

            static void destroy_system();
            static dynamic_pool<group_node> pool_value;
        };

        using group_node_vector = std::vector<group_node *>;

        class goto_child {
          public:
            goto_child(text_file_loader *owner_value, const char *key) : owner(owner_value) {
                owner->set_child_node(key);
            }
            goto_child(text_file_loader *owner_value, uint32_t index) : owner(owner_value) {
                owner->set_child_node(index);
            }
            ~goto_child() {
                owner->set_parent_node();
            }

            text_file_loader *owner;
        };

      public:
        static void destroy_system();

        static void set_cache_mode();

        static auto cache(const char *file_name) -> text_file_loader *;

      public:
        text_file_loader();
        virtual ~text_file_loader();

        void destroy();

        auto load(const char *file_name) -> bool;
        auto file_name() -> const char *;

        auto is_empty() -> bool;

        void set_top();
        auto child_node_count() -> uint32_t;
        auto set_child_node(const char *key) -> bool;
        auto set_child_node(const std::string &key_head, uint32_t index) -> bool;
        auto set_child_node(uint32_t index) -> bool;
        auto set_parent_node() -> bool;
        auto current_node_name(std::string *pstr_name) -> bool;

        auto get_token_vector(const std::string &key, token_vector **token_vector) -> bool;
        auto token_boolean(const std::string &key, bool *data) -> bool;
        // for callers that still keep the value in a 32-bit BOOL
        auto token_byte(const std::string &key, uint8_t *data) -> bool;
        auto token_word(const std::string &key, uint16_t *data) -> bool;
        auto token_integer(const std::string &key, int *data) -> bool;
        auto token_double_word(const std::string &key, uint32_t *data) -> bool;
        auto token_float(const std::string &key, float *data) -> bool;
        auto token_vector2(const std::string &key, math::vector2 *vector2) -> bool;
        auto token_vector3(const std::string &key, math::vector3 *vector3) -> bool;

        auto token_position(const std::string &key, math::vector3 *vector) -> bool;
        auto token_direction(const std::string &key, math::vector3 *vector) -> bool;
        auto token_color(const std::string &key, math::color *color) -> bool;
        auto token_string(const std::string &key, std::string *string) -> bool;

      protected:
        void destroy_group_node_vector();

        auto load_group(group_node *group_node_value) -> bool;

      protected:
        std::string file_name_;

        std::vector<char> buf_data_;
        uint32_t buf_size_;

        uint32_t dwcur_line_index_;

        memory_text_file_loader text_file_loader_value_;

        group_node global_node_;
        group_node *pcur_node_;

        std::vector<group_node *> vct_node_;

      protected:
        static std::map<uint32_t, std::unique_ptr<text_file_loader>> map_name_key_text_file_loader_;
        static bool is_cache_mode_;
    };
}
