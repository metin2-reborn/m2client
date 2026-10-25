#include "map_base.h"
#include <metin2/core/stl.h>
#include <metin2/core/util.h>
#include <metin2/log/log.h>

namespace metin2::inline world::inline maps {

    map_base::map_base() {
        clear();
    }

    map_base::~map_base() {
        clear();
    }

    void map_base::clear() {
        name_value_ = "";
        type_value_ = MAPTYPE_INVALID;
        mc_environment_data_ = nullptr;
        leave();
    }

    auto map_base::enter() -> bool {
        ready_ = true;
        return true;
    }

    auto map_base::leave() -> bool {
        ready_ = false;
        return true;
    }

    void map_base::set_environment_data_ptr(const environment_data *environment_data) {
        mc_environment_data_ = environment_data;
        on_set_environment_data_ptr();
    }

    void map_base::reset_environment_data_ptr(const environment_data *environment_data) {
        mc_environment_data_ = environment_data;
        on_reset_environment_data_ptr();
    }

    void map_base::render() {
        if (is_ready())
            on_render();
    }

    auto map_base::load_property() -> bool {
        std::string file_name = map_file_name(get_name(), "MapProperty.txt");

        token_vector_map token_vector_map;

        if (!load_multiple_text_data(file_name.c_str(), token_vector_map)) {
            metin2::log::error("Cannot load map property {}: the file is probably missing", file_name.c_str());
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("scripttype")) {
            metin2::log::error("Map property {}: no scripttype key", file_name.c_str());
            return false;
        }

        if (token_vector_map.end() == token_vector_map.find("maptype")) {
            metin2::log::error("Map property {}: no maptype key", file_name.c_str());
            return false;
        }

        // A map with ParentMapName in MapProperty.txt reads all of its data from the parent map (keeps downloads small).
        // Partial overrides are not supported.
        if (token_vector_map.end() != token_vector_map.find("parentmapname")) {
            parent_map_name_ = token_vector_map["parentmapname"][0];
        }

        const std::string &type = token_vector_map["scripttype"][0];
        const std::string &map_type = token_vector_map["maptype"][0];

        if (0 != type.compare("MapProperty")) {
            metin2::log::error("Map property {}: unknown map type", file_name.c_str());
            return false;
        }

        if (0 == map_type.compare("Indoor"))
            set_type(MAPTYPE_INDOOR);
        else if (0 == map_type.compare("Outdoor"))
            set_type(MAPTYPE_OUTDOOR);
        else if (0 == map_type.compare("Invalid"))
            set_type(MAPTYPE_OUTDOOR);

        return true;
    }
}
