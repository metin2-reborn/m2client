#include "map_config.h"

#include <metin2/core/mapped_file.h>
#include <metin2/files/asset_toml.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>

namespace metin2::inline world::inline maps {

    namespace {
        constexpr std::string_view map_type_names[] = {"Outdoor", "Indoor"};

        // Two whole numbers ([x, y]) of at least min.
        auto read_pair(asset_table &table, std::string_view key, int64_t min, int64_t &x, int64_t &y) -> bool {
            const toml::array *pair = table.array(key);
            const auto first = pair && pair->size() == 2 ? (*pair)[0].value_exact<int64_t>() : std::nullopt;
            const auto second = pair && pair->size() == 2 ? (*pair)[1].value_exact<int64_t>() : std::nullopt;
            if (!first || !second || *first < min || *second < min) {
                table.error(key, "must be two whole numbers");
                return false;
            }
            x = *first;
            y = *second;
            return true;
        }
    }

    auto load_map_config(std::string_view text, std::string_view name, map_config &config) -> bool {
        const auto table = parse_asset_toml(text, name, "Map");
        if (!table) {
            return false;
        }

        asset_table root(*table, name);
        root.table("asset");
        bool ok = true;

        if (const toml::table *map_table = root.table("map")) {
            asset_table values(*map_table, name);
            int type = 0;
            ok &= values.required_enum("type", map_type_names, type);
            config.indoor = type == 1;
            ok &= values.optional("parent", config.parent);
            ok &= values.finish();
        } else {
            root.error("map", "is missing");
            ok = false;
        }

        if (const toml::table *terrain_table = root.table("terrain")) {
            asset_table values(*terrain_table, name);
            config.has_terrain = true;
            int64_t x = 0;
            int64_t y = 0;
            if (read_pair(values, "size", 1, x, y)) {
                config.size_x = static_cast<int32_t>(x);
                config.size_y = static_cast<int32_t>(y);
            } else {
                ok = false;
            }
            if (values.array("base_position")) {
                if (read_pair(values, "base_position", 0, x, y)) {
                    config.has_base_position = true;
                    config.base_x = static_cast<uint32_t>(x);
                    config.base_y = static_cast<uint32_t>(y);
                } else {
                    ok = false;
                }
            }
            ok &= values.required("view_radius", config.view_radius);
            if (config.view_radius <= 0) {
                values.error("view_radius", "must be greater than 0");
                ok = false;
            }
            ok &= values.required("height_scale", config.height_scale);
            ok &= values.required("texture_set", config.texture_set);
            ok &= values.optional("environment", config.environment);
            ok &= values.optional("visible", config.terrain_visible);
            ok &= values.finish();
        }

        return root.finish() && ok;
    }

    auto read_map_config(const std::string &file_name, map_config &config) -> bool {
        mapped_file file;
        const void *data = nullptr;
        if (!get_file_manager().get(file, file_name.c_str(), &data)) {
            metin2::log::error("Cannot read map config {}", file_name);
            return false;
        }
        return load_map_config(std::string_view(static_cast<const char *>(data), file.size()), file_name, config);
    }
}
