#pragma once

#include <metin2/client/protocol/packet.h>

namespace metin2::inline client::inline trade {
    enum shop_coin_type {
        SHOP_COIN_TYPE_GOLD, // default
        SHOP_COIN_TYPE_SECONDARY_COIN,
    };

    class python_shop {
      public:
        python_shop();

        ~python_shop();

        void clear();

        void set_item_data(uint32_t index, const shop_item_data &shop_item_data);
        auto get_item_data(uint32_t index, const shop_item_data **item_data) -> bool;

        void set_item_data(uint8_t tab_idx, uint32_t slot_pos, const shop_item_data &shop_item_data);
        auto get_item_data(uint8_t tab_idx, uint32_t slot_pos, const shop_item_data **item_data) -> bool;

        void set_tab_count(uint8_t tab_count) { tab_count_ = tab_count; }
        auto tab_count() -> uint8_t { return tab_count_; }

        void set_tab_coin_type(uint8_t tab_idx, uint8_t coin_type);
        auto tab_coin_type(uint8_t tab_idx) -> uint8_t;

        void set_tab_name(uint8_t tab_idx, const char *name);
        auto tab_name(uint8_t tab_idx) -> const char *;

        void open(bool is_private_shop, bool is_main_private_shop);
        void close();
        auto is_open() -> bool;
        auto is_private_shop() -> bool;
        auto is_main_player_private_shop() -> bool;

        void clear_private_shop_stock();
        void add_private_shop_item_stock(item_pos item_pos, uint8_t display_pos, uint32_t price);
        void del_private_shop_item_stock(item_pos item_pos);
        auto private_shop_item_price(item_pos item_pos) -> int;
        void build_private_shop(const char *name);

      protected:
      protected:
        bool is_shoping_;
        bool is_private_shop_;
        bool is_main_player_private_shop_;

        struct shop_tab {
            shop_tab() {
                coin_type = SHOP_COIN_TYPE_GOLD;
            }
            uint8_t coin_type;
            std::string name_value;
            shop_item_data items[SHOP_HOST_ITEM_MAX_NUM];
        };

        uint8_t tab_count_;
        shop_tab shoptabs_[SHOP_TAB_COUNT_MAX];

        using private_shop_item_stock = std::map<item_pos, shop_item_table>;

        private_shop_item_stock private_shop_item_stock_value_;
    };

    inline auto get_python_shop() -> python_shop & {
        static python_shop instance;

        return instance;
    }
}
