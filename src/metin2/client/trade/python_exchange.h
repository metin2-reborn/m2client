#pragma once

#include <metin2/client/protocol/packet.h>

namespace metin2::inline client::inline trade {

    class python_exchange {
      public:
        enum {
            EXCHANGE_ITEM_MAX_NUM = 12,
        };

        struct trade {
            char name_value[CHARACTER_NAME_MAX_LEN + 1];

            uint32_t item_vnum[EXCHANGE_ITEM_MAX_NUM];
            uint8_t item_count[EXCHANGE_ITEM_MAX_NUM];
            uint32_t item_metin[EXCHANGE_ITEM_MAX_NUM][ITEM_SOCKET_SLOT_MAX_NUM];
            player_item_attribute item_attr[EXCHANGE_ITEM_MAX_NUM][ITEM_ATTRIBUTE_SLOT_MAX_NUM];

            uint8_t accept;
            uint32_t elk;
        };
        using exchange_data = trade;

      public:
        python_exchange();
        ~python_exchange();

        void clear();

        void start();
        void end();
        auto is_trading() -> bool;

        void set_self_name(const char *name);
        void set_target_name(const char *name);

        auto name_from_target() -> char *;

        void set_elk_to_target(uint32_t elk);
        void set_elk_to_self(uint32_t elk);

        auto elk_from_target() -> uint32_t;
        auto elk_from_self() -> uint32_t;

        void set_item_to_target(uint32_t pos, uint32_t vnum, uint8_t count);
        void set_item_to_self(uint32_t pos, uint32_t vnum, uint8_t count);

        void set_item_metin_socket_to_target(int pos, int imetinpos, uint32_t vnum);
        void set_item_metin_socket_to_self(int pos, int imetinpos, uint32_t vnum);

        void set_item_attribute_to_target(int pos, int iattrpos, uint8_t type, short value);
        void set_item_attribute_to_self(int pos, int iattrpos, uint8_t type, short value);

        void del_item_of_target(uint8_t pos);
        void del_item_of_self(uint8_t pos);

        auto item_vnum_from_target(uint8_t pos) -> uint32_t;
        auto item_vnum_from_self(uint8_t pos) -> uint32_t;

        auto item_count_from_target(uint8_t pos) -> uint8_t;
        auto item_count_from_self(uint8_t pos) -> uint8_t;

        auto item_metin_socket_from_target(uint8_t pos, int metin_socket_pos) -> uint32_t;
        auto item_metin_socket_from_self(uint8_t pos, int metin_socket_pos) -> uint32_t;

        void item_attribute_from_target(uint8_t pos, int attr_pos, uint8_t *type, short *value);
        void item_attribute_from_self(uint8_t pos, int attr_pos, uint8_t *type, short *value);

        void set_accept_to_target(uint8_t accept);
        void set_accept_to_self(uint8_t accept);

        auto accept_from_target() -> bool;
        auto accept_from_self() -> bool;

      protected:
        bool is_trading_;

        bool elk_mode_; // trading by clicking the elk
        exchange_data self_;
        exchange_data victim_value_;
    };

    inline auto get_python_exchange() -> python_exchange & {
        static python_exchange instance;
        return instance;
    }
}
