#include "python_exchange.h"
#include <metin2/core/utf8.h>

namespace metin2::inline client::inline trade {

    void python_exchange::set_self_name(const char *name) {
        utf8::copy_field(self_.name_value, name);
    }

    void python_exchange::set_target_name(const char *name) {
        utf8::copy_field(victim_value_.name_value, name);
    }

    auto python_exchange::name_from_target() -> char * {
        return victim_value_.name_value;
    }

    void python_exchange::set_elk_to_target(uint32_t elk) {
        victim_value_.elk = elk;
    }

    void python_exchange::set_elk_to_self(uint32_t elk) {
        self_.elk = elk;
    }

    auto python_exchange::elk_from_target() -> uint32_t {
        return victim_value_.elk;
    }

    auto python_exchange::elk_from_self() -> uint32_t {
        return self_.elk;
    }

    void python_exchange::set_item_to_target(uint32_t pos, uint32_t vnum, uint8_t count) {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return;

        victim_value_.item_vnum[pos] = vnum;
        victim_value_.item_count[pos] = count;
    }

    void python_exchange::set_item_to_self(uint32_t pos, uint32_t vnum, uint8_t count) {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return;

        self_.item_vnum[pos] = vnum;
        self_.item_count[pos] = count;
    }

    void python_exchange::set_item_metin_socket_to_target(int pos, int imetinpos, uint32_t vnum) {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return;

        victim_value_.item_metin[pos][imetinpos] = vnum;
    }

    void python_exchange::set_item_metin_socket_to_self(int pos, int imetinpos, uint32_t vnum) {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return;

        self_.item_metin[pos][imetinpos] = vnum;
    }

    void python_exchange::set_item_attribute_to_target(int pos, int iattrpos, uint8_t type, short value) {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return;

        victim_value_.item_attr[pos][iattrpos].type_value = type;
        victim_value_.item_attr[pos][iattrpos].value = value;
    }

    void python_exchange::set_item_attribute_to_self(int pos, int iattrpos, uint8_t type, short value) {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return;

        self_.item_attr[pos][iattrpos].type_value = type;
        self_.item_attr[pos][iattrpos].value = value;
    }

    void python_exchange::del_item_of_target(uint8_t pos) {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return;

        victim_value_.item_vnum[pos] = 0;
        victim_value_.item_count[pos] = 0;
    }

    void python_exchange::del_item_of_self(uint8_t pos) {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return;

        self_.item_vnum[pos] = 0;
        self_.item_count[pos] = 0;
    }

    auto python_exchange::item_vnum_from_target(uint8_t pos) -> uint32_t {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return 0;

        return victim_value_.item_vnum[pos];
    }

    auto python_exchange::item_vnum_from_self(uint8_t pos) -> uint32_t {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return 0;

        return self_.item_vnum[pos];
    }

    auto python_exchange::item_count_from_target(uint8_t pos) -> uint8_t {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return 0;

        return victim_value_.item_count[pos];
    }

    auto python_exchange::item_count_from_self(uint8_t pos) -> uint8_t {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return 0;

        return self_.item_count[pos];
    }

    auto python_exchange::item_metin_socket_from_target(uint8_t pos, int metin_socket_pos) -> uint32_t {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return 0;

        return victim_value_.item_metin[pos][metin_socket_pos];
    }

    auto python_exchange::item_metin_socket_from_self(uint8_t pos, int metin_socket_pos) -> uint32_t {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return 0;

        return self_.item_metin[pos][metin_socket_pos];
    }

    void python_exchange::item_attribute_from_target(uint8_t pos, int attr_pos, uint8_t *type, short *value) {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return;

        *type = victim_value_.item_attr[pos][attr_pos].type_value;
        *value = victim_value_.item_attr[pos][attr_pos].value;
    }

    void python_exchange::item_attribute_from_self(uint8_t pos, int attr_pos, uint8_t *type, short *value) {
        if (pos >= EXCHANGE_ITEM_MAX_NUM)
            return;

        *type = self_.item_attr[pos][attr_pos].type_value;
        *value = self_.item_attr[pos][attr_pos].value;
    }

    void python_exchange::set_accept_to_target(uint8_t accept) {
        victim_value_.accept = accept ? true : false;
    }

    void python_exchange::set_accept_to_self(uint8_t accept) {
        self_.accept = accept ? true : false;
    }

    auto python_exchange::accept_from_target() -> bool {
        return victim_value_.accept ? true : false;
    }

    auto python_exchange::accept_from_self() -> bool {
        return self_.accept ? true : false;
    }

    void python_exchange::start() {
        is_trading_ = true;
    }

    void python_exchange::end() {
        is_trading_ = false;
    }

    auto python_exchange::is_trading() -> bool {
        return is_trading_;
    }

    void python_exchange::clear() {
        memset(&self_, 0, sizeof(self_));
        memset(&victim_value_, 0, sizeof(victim_value_));
    }

    python_exchange::python_exchange() {
        clear();
        is_trading_ = false;
        elk_mode_ = false; // not in clear(): clear() runs when the trade start packet arrives
    }
    python_exchange::~python_exchange() = default;
}
