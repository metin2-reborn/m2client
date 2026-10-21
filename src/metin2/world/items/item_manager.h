#pragma once

#include "item_data.h"

namespace metin2::inline world::inline items {

    class item_manager {
      public:
        enum item_desc_col {
            ITEMDESC_COL_VNUM,
            ITEMDESC_COL_NAME,
            ITEMDESC_COL_DESC,
            ITEMDESC_COL_SUMM,
            ITEMDESC_COL_NUM,
        };

      public:
        using item_map = std::map<uint32_t, item_data *>;
        using item_name_map = std::map<std::string, item_data *>;

      public:
        item_manager();
        ~item_manager();

        void destroy();

        auto select_item_data(uint32_t index) -> bool;
        auto selected_item_data_pointer() -> item_data *;

        auto item_data_pointer(uint32_t item_id, item_data **item_data) -> bool;

        auto load_item_desc(const char *file_name) -> bool;
        auto load_item_list(const char *file_name) -> bool;
        auto load_item_table(const char *file_name) -> bool;
        auto make_item_data(uint32_t index) -> item_data *;

      protected:
        item_map item_map_value_;
        std::vector<item_data *> vec_item_range_;
        item_data *selected_item_data_;
    };

    inline auto get_item_manager() -> item_manager & {
        static item_manager instance;
        return instance;
    }
}
