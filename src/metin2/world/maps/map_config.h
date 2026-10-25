#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace metin2::inline world::inline maps {

    // A map's map.toml ([asset] type "Map"). A map with a parent reads all of its data, this file's
    // [terrain] included, from the parent map's directory.
    struct map_config {
        bool indoor = false;
        std::string parent;

        bool has_terrain = false;
        int32_t size_x = 0;
        int32_t size_y = 0;
        bool has_base_position = false;
        uint32_t base_x = 0;
        uint32_t base_y = 0;
        int32_t view_radius = 0;
        float height_scale = 0.0f;
        std::string texture_set;
        std::string environment;
        bool terrain_visible = true;
    };

    auto load_map_config(std::string_view text, std::string_view name, map_config &config) -> bool;
    // Reads the file through the file manager first.
    auto read_map_config(const std::string &file_name, map_config &config) -> bool;
}
