#include "property.h"

#include <metin2/core/file_name.h>
#include <metin2/files/asset_toml.h>

#include <array>

namespace metin2::inline world::inline maps {

    namespace {
        constexpr std::array<std::string_view, property_types::PROPERTY_TYPE_MAX_NUM> kKindTables = {
            "", "tree", "building", "effect", "ambience", "dungeon_block"};
        constexpr std::array<std::string_view, property_types::AMBIENCE_PLAY_TYPE_MAX_NUM> kPlayTypes = {"Once", "Step", "Loop"};

        auto attribute_file_name(std::string model_file_name) -> std::string {
            return file_name_helper::no_extension(model_file_name) + ".mdatr";
        }
    }

    property::property(std::string_view file_name) : file_name_(file_name) {}

    auto property::load(std::string_view text) -> bool {
        const auto table = parse_asset_toml(text, file_name_, "Property");
        if (!table) {
            return false;
        }

        asset_table root(*table, file_name_);
        root.table("asset");
        if (const toml::table *header = root.table("property")) {
            asset_table values(*header, file_name_);
            values.required("id", crc_);
            values.required("name", name_);
            if (!values.finish()) {
                return false;
            }
        } else {
            root.error("property", "is missing");
        }

        const toml::table *kind = nullptr;
        for (size_t i = 1; i < kKindTables.size(); ++i) {
            const toml::table *found = root.table(kKindTables[i]);
            if (!found) {
                continue;
            }
            if (kind) {
                root.error(kKindTables[i], "is a second kind table; a property has one");
                return false;
            }
            kind = found;
            type_ = static_cast<property_types::property_type>(i);
        }
        if (!kind) {
            root.error("", "has no kind table ([tree], [building], [effect], [ambience] or [dungeon_block])");
            return false;
        }

        asset_table values(*kind, file_name_);
        switch (type_) {
        case property_types::PROPERTY_TYPE_TREE:
            values.required("file", tree_.file_name);
            break;

        case property_types::PROPERTY_TYPE_BUILDING:
            values.required("file", building_.file_name);
            values.optional("shadow", building_.is_shadow_flag);
            building_.attribute_data_file_name = attribute_file_name(building_.file_name);
            break;

        case property_types::PROPERTY_TYPE_EFFECT:
            values.required("file", effect_.file_name);
            break;

        case property_types::PROPERTY_TYPE_AMBIENCE:
            {
                int play_type = 0;
                values.required_enum("play_type", kPlayTypes, play_type);
                ambience_.play_type = static_cast<property_types::ambience_play_type>(play_type);
                values.required("interval", ambience_.play_interval);
                values.required("interval_variation", ambience_.play_interval_variation);
                values.optional("max_volume_area_percentage", ambience_.max_volume_area_percentage);
                if (const toml::array *sounds = values.array("sounds")) {
                    for (const toml::node &node : *sounds) {
                        const auto sound = node.value_exact<std::string>();
                        if (!sound) {
                            values.error("sounds", "must be an array of strings");
                            break;
                        }
                        ambience_.ambience_sound_vector.push_back(*sound);
                    }
                } else {
                    values.error("sounds", "is missing");
                }
                break;
            }

        case property_types::PROPERTY_TYPE_DUNGEON_BLOCK:
            values.required("file", dungeon_block_.file_name);
            dungeon_block_.attribute_data_file_name = attribute_file_name(dungeon_block_.file_name);
            break;

        default:
            break;
        }

        const bool kind_ok = values.finish();
        return root.finish() && kind_ok;
    }
}
