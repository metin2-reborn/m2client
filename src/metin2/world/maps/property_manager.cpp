#include <metin2/core/parse.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>

#include <metin2/core/file_loader.h>
#include "property.h"
#include "property_manager.h"

#include <strings.h>

namespace metin2::inline world::inline maps {

    property_manager::property_manager() = default;

    property_manager::~property_manager() {
        clear();
    }

    auto property_manager::initialize() -> bool {
        get_file_manager().for_each("system", "property/", [this](const std::string &file_name) -> void {
            if (!strcasecmp("property/reserve", file_name.c_str()))
                load_reserved_crc(file_name.c_str());
            else
                register_value(file_name.c_str());
        });
        return true;
    }

    auto property_manager::load_reserved_crc(const char *file_name) -> bool {
        mapped_file file;
        const void *data;

        if (!get_file_manager().get(file, file_name, &data))
            return false;

        memory_text_file_loader text_file_loader;
        text_file_loader.bind(file.size(), data);

        for (uint32_t i = 0; i < text_file_loader.line_count(); ++i) {
            const char *line = text_file_loader.line_string(i).c_str();

            if (!line || !*line)
                continue;

            reserve_crc(metin2::to_number<int>(line));
        }

        return true;
    }

    void property_manager::reserve_crc(uint32_t crc) {
        reserved_crc_set_.insert(crc);
    }

    auto property_manager::register_value(const char *file_name, property **pp_property) -> bool {
        mapped_file file;
        const void *data;

        if (!get_file_manager().get(file, file_name, &data))
            return false;

        auto *p_property = new property(file_name);

        if (!p_property->read_from_memory(data, file.size(), file_name)) {
            delete p_property;
            return false;
        }

        uint32_t crc = p_property->get_crc();

        auto itor = property_by_crc_map_.find(crc);

        if (property_by_crc_map_.end() != itor) {
            metin2::log::debug("Property already registered, {} replaced by {}", itor->second->file_name(), file_name);

            delete itor->second;
            itor->second = p_property;
        } else
            property_by_crc_map_.insert(property_crc_map::value_type(crc, p_property));

        if (pp_property)
            *pp_property = p_property;

        return true;
    }

    auto property_manager::get(const char *file_name, property **property) -> bool {
        return register_value(file_name, property);
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
