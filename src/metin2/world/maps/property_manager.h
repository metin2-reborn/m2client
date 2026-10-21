#pragma once

#include <map>
#include <set>
#include "property.h"

namespace metin2::inline world::inline maps {

    class property_manager {
      public:
        property_manager();
        ~property_manager();

        void clear();

        auto load_reserved_crc(const char *file_name) -> bool;
        void reserve_crc(uint32_t crc);

        // Registers every property/ file of the System archive.
        auto initialize() -> bool;
        auto register_value(const char *file_name, property **property = nullptr) -> bool;

        auto get(uint32_t crc, property **property) -> bool;
        auto get(const char *file_name, property **property) -> bool;

      protected:
        using property_crc_map = std::map<uint32_t, property *>;
        using crc_set = std::set<uint32_t>;

        property_crc_map property_by_crc_map_;
        crc_set reserved_crc_set_;
    };

    inline auto get_property_manager() -> property_manager & {
        static property_manager instance;
        return instance;
    }
}
