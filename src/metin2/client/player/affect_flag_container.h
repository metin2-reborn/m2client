#pragma once
#include <cstdint>

namespace metin2::inline client::inline player {

    class affect_flag_container {
      public:
        enum {
            BIT_SIZE = 64,
            BYTE_SIZE = BIT_SIZE / 8 + (1 * ((BIT_SIZE & 7) ? 1 : 0)),
        };

      public:
        affect_flag_container();
        ~affect_flag_container();

        void clear();
        void copy_instance(const affect_flag_container &affect_container);
        void set(uint32_t pos, bool is_set);
        auto is_set(uint32_t pos) const -> bool;

        void copy_data(uint32_t pos, uint32_t byte_size, const void *data);

        void convert_to_position(unsigned *ret_x, unsigned *ret_y) const;

      private:
        using element = unsigned char;

        element element_value_[BYTE_SIZE];
    };
}
