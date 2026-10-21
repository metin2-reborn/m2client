#include "python_non_player.h"
#include <metin2/core/lzo.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/log/log.h>
#include "instance_base.h"
#include "python_character_manager.h"

#include <metin2/core/utils.h>
#include <vector>

namespace metin2::inline client::inline characters {

    auto python_non_player::load_non_player_data(const char *file_name) -> bool {
        static uint32_t mob_proto_key[4] =
            {
                4813894,
                18955,
                552631,
                6822045};

        mapped_file file;
        const void *pv_data;

        metin2::log::debug("Loading mob proto {} (mob table size {})", file_name, sizeof(mob_table));

        if (!get_file_manager().get(file, file_name, &pv_data))
            return false;

        uint32_t four_cc, elements, data_size;

        file.read(&four_cc, sizeof(uint32_t));

        // format v2 ("MMP2"): UTF-8 names in 64-byte fields (convert older files with proto_convert)
        if (four_cc != MAKEFOURCC('M', 'M', 'P', '2')) {
            metin2::log::error("Mob proto {} is not version 2 (convert it with proto_convert)", file_name);
            return false;
        }

        file.read(&elements, sizeof(uint32_t));
        file.read(&data_size, sizeof(uint32_t));

        std::vector<uint8_t> data(data_size);
        file.read(data.data(), data_size);

        lz_object z_obj;
        if (!get_clzo().decompress(z_obj, data.data(), mob_proto_key))
            return false;

        if ((z_obj.size() % sizeof(mob_table)) != 0 || elements > z_obj.size() / sizeof(mob_table)) {
            metin2::log::error("Mob proto: {} bytes do not hold {} mob tables", z_obj.size(), elements);
            return false;
        }

        const uint8_t *source = z_obj.buffer();
        for (uint32_t i = 0; i < elements; ++i) {
            mob_table table;
            memcpy(&table, source + i * sizeof(mob_table), sizeof(mob_table));
            non_player_data_map_value_.insert_or_assign(table.vnum, table);
        }
        return true;
    }

    auto python_non_player::get_name(uint32_t vnum, const char **name) -> bool {
        const mob_table *p = table(vnum);

        if (!p)
            return false;

        *name = p->locale_name;

        return true;
    }

    auto python_non_player::instance_type(uint32_t vnum, uint8_t *type) -> bool {
        const mob_table *p = table(vnum);

        // Unknown vnums return false, which callers treat as a player character.
        if (!p)
            return false;

        *type = p->type_value;

        return true;
    }

    auto python_non_player::table(uint32_t vnum) -> const python_non_player::mob_table * {
        auto itor = non_player_data_map_value_.find(vnum);

        if (itor == non_player_data_map_value_.end())
            return nullptr;

        return &itor->second;
    }

    auto python_non_player::get_event_type(uint32_t vnum) -> uint8_t {
        const mob_table *p = table(vnum);

        if (!p) {
            return ON_CLICK_EVENT_NONE;
        }

        return p->on_click_type;
    }

    auto python_non_player::monster_name(uint32_t vnum) -> const char * {
        const python_non_player::mob_table *table_value = table(vnum);
        if (!table_value) {
            static const char *empty = "";
            return empty;
        }

        return table_value->locale_name;
    }

    auto python_non_player::monster_color(uint32_t vnum) -> uint32_t {
        const python_non_player::mob_table *table_value = table(vnum);
        if (!table_value)
            return 0;

        return table_value->monster_color;
    }

    void python_non_player::clear() {
    }

    void python_non_player::destroy() {
        non_player_data_map_value_.clear();
    }

    python_non_player::python_non_player() {
        clear();
    }

    python_non_player::~python_non_player(void) {
        destroy();
    }
}
