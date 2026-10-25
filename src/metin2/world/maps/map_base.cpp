#include "map_base.h"
#include "map_config.h"

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
        map_config config;
        if (!read_map_config(map_file_name(get_name(), "map.toml"), config)) {
            return false;
        }

        // A map with a parent reads all of its data from the parent map (keeps downloads small).
        // Partial overrides are not supported.
        parent_map_name_ = config.parent;
        set_type(config.indoor ? MAPTYPE_INDOOR : MAPTYPE_OUTDOOR);
        return true;
    }
}
