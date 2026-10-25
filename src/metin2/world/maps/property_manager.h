#pragma once

#include <map>
#include "property.h"

namespace metin2::inline world::inline maps {

    class property_manager {
      public:
        property_manager();
        ~property_manager();

        void clear();

        // Registers every property script (*.toml) of the property archive.
        auto initialize() -> bool;
        auto register_value(const char *file_name) -> bool;

        auto get(uint32_t crc, property **property) -> bool;

      protected:
        using property_crc_map = std::map<uint32_t, property *>;

        property_crc_map property_by_crc_map_;
    };

    inline auto get_property_manager() -> property_manager & {
        static property_manager instance;
        return instance;
    }
}
