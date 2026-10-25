#include <metin2/core/mapped_file.h>
#include <metin2/core/stl.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>

#include "property_manager.h"

namespace metin2::inline world::inline maps {

    property_manager::property_manager() = default;

    property_manager::~property_manager() {
        clear();
    }

    auto property_manager::initialize() -> bool {
        get_file_manager().for_each("property", "", [this](const std::string &file_name) -> void {
            if (file_name.ends_with(".toml"))
                register_value(("property:" + file_name).c_str());
        });
        return true;
    }

    auto property_manager::register_value(const char *file_name) -> bool {
        mapped_file file;
        const void *data;

        if (!get_file_manager().get(file, file_name, &data))
            return false;

        auto *p_property = new property(file_name);

        if (!p_property->load(std::string_view(static_cast<const char *>(data), file.size()))) {
            delete p_property;
            return false;
        }

        uint32_t crc = p_property->crc();

        auto itor = property_by_crc_map_.find(crc);

        if (property_by_crc_map_.end() != itor) {
            metin2::log::debug("Property already registered, {} replaced by {}", itor->second->file_name(), file_name);

            delete itor->second;
            itor->second = p_property;
        } else
            property_by_crc_map_.insert(property_crc_map::value_type(crc, p_property));

        return true;
    }

    auto property_manager::get(uint32_t crc, property **property) -> bool {
        auto itor = property_by_crc_map_.find(crc);

        if (property_by_crc_map_.end() == itor)
            return false;

        *property = itor->second;
        return true;
    }

    void property_manager::clear() {
        stl_wipe_second(property_by_crc_map_);
    }
}
