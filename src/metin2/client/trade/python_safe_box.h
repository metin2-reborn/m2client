#pragma once

#include <metin2/client/app/game_type.h>

#include <cstdint>

namespace metin2::inline client::inline trade {
    class python_safe_box {
      public:
        enum {
            SAFEBOX_SLOT_X_COUNT = 5,
            SAFEBOX_SLOT_Y_COUNT = 9,
            SAFEBOX_PAGE_SIZE = SAFEBOX_SLOT_X_COUNT * SAFEBOX_SLOT_Y_COUNT,
        };

        using item_instance_vector = std::vector<packet_item>;

      public:
        python_safe_box();

        ~python_safe_box();

        void open_safe_box(int size);
        void set_item_data(uint32_t slot_index, const packet_item &item_data);
        void del_item_data(uint32_t slot_index);

        auto money() -> uint32_t;

        auto slot_item_id(uint32_t slot_index, uint32_t *item_id) -> bool;

        auto item_data_ptr(uint32_t slot_index, packet_item **instance) -> bool;

        void open_mall(int size);
        void set_mall_item_data(uint32_t slot_index, const packet_item &item_data);
        void del_mall_item_data(uint32_t slot_index);
        auto mall_item_data_ptr(uint32_t slot_index, packet_item **instance) -> bool;
        auto slot_mall_item_id(uint32_t slot_index, uint32_t *item_id) -> bool;
        auto mall_size() -> uint32_t;

      protected:
        item_instance_vector item_instance_vector_value_;
        item_instance_vector mall_item_instance_vector_;
        uint32_t money_;
    };

    inline auto get_python_safe_box() -> python_safe_box & {
        static python_safe_box instance;

        return instance;
    }
}
