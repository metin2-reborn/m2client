#include "affect_flag_container.h"
#include <cstring>
#include <metin2/log/log.h>

namespace metin2::inline client::inline player {

    affect_flag_container::affect_flag_container() {
        clear();
    }

    affect_flag_container::~affect_flag_container() = default;

    void affect_flag_container::clear() {
        memset(element_value_, 0, sizeof(element_value_));
    }

    void affect_flag_container::copy_instance(const affect_flag_container &affect_container) {
        memcpy(element_value_, affect_container.element_value_, sizeof(element_value_));
    }

    void affect_flag_container::copy_data(uint32_t pos, uint32_t byte_size, const void *c_pv_data) {
        const auto *c_pb_data = (const uint8_t *)c_pv_data;
        element mask = 0x01;

        uint32_t bit_end = pos + byte_size * 8;
        for (uint32_t i = pos; i < bit_end; ++i) {
            set(i, (*c_pb_data & mask) ? true : false);
            mask <<= 1;

            if (mask == 0) {
                ++c_pb_data;
                mask = 0x01;
            }
        }
    }

    void affect_flag_container::convert_to_position(unsigned *ret_x, unsigned *ret_y) const {
        auto *pos = (uint32_t *)element_value_;
        *ret_x = pos[0];
        *ret_y = pos[1];
    }

    void affect_flag_container::set(uint32_t pos, bool is_set) {
        if (pos / 8 >= BYTE_SIZE) {
            metin2::log::error("Cannot set affect flag {}: out of range (limit {}, value {})", pos, BYTE_SIZE * 8, is_set);
            return;
        }

        uint8_t &element_value = element_value_[pos / 8];

        auto mask = uint8_t(1 << (pos & 7));
        if (is_set)
            element_value |= mask;
        else
            element_value &= ~mask;
    }

    auto affect_flag_container::is_set(uint32_t pos) const -> bool {
        if (pos / 8 >= BYTE_SIZE) {
            metin2::log::error("Cannot read affect flag {}: out of range (limit {})", pos, BYTE_SIZE * 8);
            return false;
        }

        const uint8_t &element_value = element_value_[pos / 8];

        auto mask = uint8_t(1 << (pos & 7));
        if (element_value & mask)
            return true;

        return false;
    }
}
