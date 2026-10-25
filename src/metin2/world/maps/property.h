#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "map_type.h"

namespace metin2::inline world::inline maps {

    // A placeable map object (TOML, [asset] type "Property"). The maps place objects by the id, not
    // by file name. Exactly one of the kind tables ([tree], [building], [effect], [ambience],
    // [dungeon_block]) is set; type() says which.
    class property {
      public:
        explicit property(std::string_view file_name);

        auto load(std::string_view text) -> bool;

        auto file_name() const -> const std::string & { return file_name_; }
        auto crc() const -> uint32_t { return crc_; }
        auto name() const -> const std::string & { return name_; }
        auto type() const -> property_types::property_type { return type_; }

        auto tree() const -> const property_types::property_tree & { return tree_; }
        auto building() const -> const property_types::property_building & { return building_; }
        auto effect() const -> const property_types::property_effect & { return effect_; }
        auto ambience() const -> const property_types::property_ambience & { return ambience_; }
        auto dungeon_block() const -> const property_types::property_dungeon_block & { return dungeon_block_; }

      private:
        std::string file_name_;
        uint32_t crc_ = 0;
        std::string name_;
        property_types::property_type type_ = property_types::PROPERTY_TYPE_NONE;

        property_types::property_tree tree_;
        property_types::property_building building_;
        property_types::property_effect effect_;
        property_types::property_ambience ambience_;
        property_types::property_dungeon_block dungeon_block_;
    };
}
